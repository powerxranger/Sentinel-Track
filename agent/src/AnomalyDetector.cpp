#include "../include/AnomalyDetector.h"
#include <algorithm>
#include <numeric>
#include <ctime>
#include <fstream>
#include <sstream>

AnomalyDetector::AnomalyDetector() 
    : high_cpu_threshold(80.0), high_memory_threshold(1024 * 1024), // 1GB in KB
      max_new_processes_per_minute(10), max_new_connections_per_minute(50),
      new_processes_count(0), new_connections_count(0), last_reset_time(time(nullptr)) {
    
    // Initialize with reasonable defaults
    cpu_history.reserve(100);
    memory_history.reserve(100);
}

AnomalyDetector::~AnomalyDetector() {
    // Cleanup if needed
}

bool AnomalyDetector::isUnknownProcess(const ProcessInfo& process) {
    // Check if this is a known process based on common system processes
    std::vector<std::string> common_processes = {
        "init", "kthreadd", "ksoftirqd", "systemd", "bash", "sh", "ssh", "sshd",
        "dbus", "networkd", "resolved", "cron", "rsyslog", "kernel", "migration"
    };
    
    // Check if process name contains any known patterns
    for (const auto& known : common_processes) {
        if (process.name.find(known) != std::string::npos) {
            return false;
        }
    }
    
    // Check against our learned known processes
    return known_processes.find(process.name) == known_processes.end();
}

bool AnomalyDetector::isSuspiciousPort(int port) {
    // Check for unusual or suspicious ports
    std::vector<int> suspicious_ports = {
        4444, 5555, 6666, 7777, 8888, 9999, // Common backdoor ports
        1234, 12345, 54321, // Simple sequential ports often used by malware
        31337, 1337, // Leet speak ports
        6667, 6668, 6669, // IRC ports sometimes used by botnets
        8080, 8888, 9000, 9001 // Alternative HTTP ports that might be suspicious
    };
    
    return std::find(suspicious_ports.begin(), suspicious_ports.end(), port) != suspicious_ports.end();
}

bool AnomalyDetector::isRapidMemoryIncrease(long current_memory) {
    if (memory_history.size() < 2) {
        memory_history.push_back(current_memory);
        return false;
    }
    
    // Calculate average memory usage over last few measurements
    double avg_memory = std::accumulate(memory_history.begin(), memory_history.end(), 0.0) / memory_history.size();
    
    // Check if current memory is significantly higher than average
    bool is_rapid_increase = current_memory > (avg_memory * 1.5); // 50% increase
    
    // Update history
    memory_history.push_back(current_memory);
    if (memory_history.size() > 10) {
        memory_history.erase(memory_history.begin());
    }
    
    return is_rapid_increase;
}

bool AnomalyDetector::isRapidCpuSpike(double current_cpu) {
    if (cpu_history.size() < 2) {
        cpu_history.push_back(current_cpu);
        return false;
    }
    
    // Calculate average CPU usage over last few measurements
    double avg_cpu = std::accumulate(cpu_history.begin(), cpu_history.end(), 0.0) / cpu_history.size();
    
    // Check if current CPU is significantly higher than average
    bool is_rapid_spike = current_cpu > (avg_cpu + 30.0) && current_cpu > 70.0; // 30% above average and above 70%
    
    // Update history
    cpu_history.push_back(current_cpu);
    if (cpu_history.size() > 10) {
        cpu_history.erase(cpu_history.begin());
    }
    
    return is_rapid_spike;
}

void AnomalyDetector::updateBaselines(const std::vector<ProcessInfo>& processes) {
    // Update known processes
    for (const auto& process : processes) {
        known_processes[process.name]++;
    }
}

void AnomalyDetector::resetCounters() {
    time_t current_time = time(nullptr);
    if (current_time - last_reset_time >= 60) { // Reset every minute
        new_processes_count = 0;
        new_connections_count = 0;
        last_reset_time = current_time;
    }
}

std::vector<AnomalyAlert> AnomalyDetector::checkProcessAnomalies(const std::vector<ProcessInfo>& processes) {
    std::vector<AnomalyAlert> alerts;
    resetCounters();
    
    for (const auto& process : processes) {
        // Check for high CPU usage
        if (process.cpu_usage > high_cpu_threshold) {
            AnomalyAlert alert;
            alert.type = "HIGH_CPU";
            alert.severity = "WARNING";
            alert.message = "Process " + process.name + " using excessive CPU";
            alert.details = "PID: " + std::to_string(process.pid) + ", CPU: " + std::to_string(process.cpu_usage) + "%";
            alert.timestamp = ""; // Will be set by logger
            alerts.push_back(alert);
        }
        
        // Check for high memory usage
        if (process.memory_usage > high_memory_threshold) {
            AnomalyAlert alert;
            alert.type = "HIGH_MEMORY";
            alert.severity = "WARNING";
            alert.message = "Process " + process.name + " using excessive memory";
            alert.details = "PID: " + std::to_string(process.pid) + ", Memory: " + std::to_string(process.memory_usage) + " KB";
            alert.timestamp = "";
            alerts.push_back(alert);
        }
        
        // Check for unknown processes
        if (isUnknownProcess(process)) {
            AnomalyAlert alert;
            alert.type = "UNKNOWN_PROCESS";
            alert.severity = "INFO";
            alert.message = "Unknown process detected: " + process.name;
            alert.details = "PID: " + std::to_string(process.pid) + ", Command: " + process.command;
            alert.timestamp = "";
            alerts.push_back(alert);
        }
    }
    
    // Update baselines for learning
    updateBaselines(processes);
    
    return alerts;
}

std::vector<AnomalyAlert> AnomalyDetector::checkNetworkAnomalies(const std::vector<NetworkConnection>& connections) {
    std::vector<AnomalyAlert> alerts;
    
    for (const auto& connection : connections) {
        // Check for suspicious ports
        if (isSuspiciousPort(connection.local_port)) {
            AnomalyAlert alert;
            alert.type = "SUSPICIOUS_PORT";
            alert.severity = "WARNING";
            alert.message = "Suspicious port detected: " + std::to_string(connection.local_port);
            alert.details = "Protocol: " + connection.protocol + ", State: " + connection.state;
            alert.timestamp = "";
            alerts.push_back(alert);
        }
        
        if (isSuspiciousPort(connection.remote_port)) {
            AnomalyAlert alert;
            alert.type = "SUSPICIOUS_PORT";
            alert.severity = "WARNING";
            alert.message = "Connection to suspicious port: " + std::to_string(connection.remote_port);
            alert.details = "Remote IP: " + connection.remote_ip + ", Protocol: " + connection.protocol;
            alert.timestamp = "";
            alerts.push_back(alert);
        }
        
        // Check for connections to private networks from public IPs (potential lateral movement)
        if (connection.remote_ip.substr(0, 3) != "127" && 
            connection.remote_ip.substr(0, 3) != "192" && 
            connection.remote_ip.substr(0, 2) != "10" &&
            connection.local_ip.substr(0, 3) == "192") {
            
            AnomalyAlert alert;
            alert.type = "EXTERNAL_CONNECTION";
            alert.severity = "INFO";
            alert.message = "External connection detected";
            alert.details = "Local: " + connection.local_ip + ":" + std::to_string(connection.local_port) + 
                           " -> Remote: " + connection.remote_ip + ":" + std::to_string(connection.remote_port);
            alert.timestamp = "";
            alerts.push_back(alert);
        }
    }
    
    new_connections_count += connections.size();
    
    // Check for too many new connections
    if (new_connections_count > max_new_connections_per_minute) {
        AnomalyAlert alert;
        alert.type = "RAPID_CONNECTIONS";
        alert.severity = "WARNING";
        alert.message = "Rapid network connections detected";
        alert.details = "Count: " + std::to_string(new_connections_count) + " connections in the last minute";
        alert.timestamp = "";
        alerts.push_back(alert);
    }
    
    return alerts;
}

std::vector<AnomalyAlert> AnomalyDetector::checkSystemAnomalies(double cpu_usage, long memory_usage) {
    std::vector<AnomalyAlert> alerts;
    
    // Check for rapid CPU spikes
    if (isRapidCpuSpike(cpu_usage)) {
        AnomalyAlert alert;
        alert.type = "CPU_SPIKE";
        alert.severity = "WARNING";
        alert.message = "Rapid CPU usage increase detected";
        alert.details = "Current CPU: " + std::to_string(cpu_usage) + "%";
        alert.timestamp = "";
        alerts.push_back(alert);
    }
    
    // Check for rapid memory increases
    if (isRapidMemoryIncrease(memory_usage)) {
        AnomalyAlert alert;
        alert.type = "MEMORY_SPIKE";
        alert.severity = "WARNING";
        alert.message = "Rapid memory usage increase detected";
        alert.details = "Current Memory: " + std::to_string(memory_usage) + " KB";
        alert.timestamp = "";
        alerts.push_back(alert);
    }
    
    // Check for overall high system resource usage
    if (cpu_usage > 90.0) {
        AnomalyAlert alert;
        alert.type = "SYSTEM_OVERLOAD";
        alert.severity = "CRITICAL";
        alert.message = "System CPU overload detected";
        alert.details = "CPU Usage: " + std::to_string(cpu_usage) + "%";
        alert.timestamp = "";
        alerts.push_back(alert);
    }
    
    return alerts;
}

void AnomalyDetector::updateConfiguration(double cpu_thresh, long mem_thresh, int proc_rate, int conn_rate) {
    high_cpu_threshold = cpu_thresh;
    high_memory_threshold = mem_thresh;
    max_new_processes_per_minute = proc_rate;
    max_new_connections_per_minute = conn_rate;
}

void AnomalyDetector::loadKnownProcesses(const std::string& whitelist_file) {
    std::ifstream file(whitelist_file);
    if (!file.is_open()) {
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line[0] != '#') { // Skip comments
            known_processes[line] = 1;
        }
    }
}