#ifndef ANOMALY_DETECTOR_H
#define ANOMALY_DETECTOR_H

#include <vector>
#include <unordered_map>
#include <string>
#include "ProcessMonitor.h"
#include "NetworkMonitor.h"

struct AnomalyAlert {
    std::string type;
    std::string severity;
    std::string message;
    std::string details;
    std::string timestamp;
};

class AnomalyDetector {
private:
    // Thresholds for anomaly detection
    double high_cpu_threshold;
    long high_memory_threshold;
    int max_new_processes_per_minute;
    int max_new_connections_per_minute;
    
    // Historical data for baseline comparison
    std::vector<double> cpu_history;
    std::vector<long> memory_history;
    std::unordered_map<std::string, int> known_processes;
    std::unordered_map<int, int> port_usage_history;
    
    // Counters for rate-based detection
    int new_processes_count;
    int new_connections_count;
    time_t last_reset_time;
    
    bool isUnknownProcess(const ProcessInfo& process);
    bool isSuspiciousPort(int port);
    bool isRapidMemoryIncrease(long current_memory);
    bool isRapidCpuSpike(double current_cpu);
    void updateBaselines(const std::vector<ProcessInfo>& processes);
    void resetCounters();

public:
    AnomalyDetector();
    ~AnomalyDetector();
    
    std::vector<AnomalyAlert> checkProcessAnomalies(const std::vector<ProcessInfo>& processes);
    std::vector<AnomalyAlert> checkNetworkAnomalies(const std::vector<NetworkConnection>& connections);
    std::vector<AnomalyAlert> checkSystemAnomalies(double cpu_usage, long memory_usage);
    
    void updateConfiguration(double cpu_thresh, long mem_thresh, int proc_rate, int conn_rate);
    void loadKnownProcesses(const std::string& whitelist_file);
};

#endif