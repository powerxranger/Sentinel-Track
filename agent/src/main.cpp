#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>
#include "../include/ProcessMonitor.h"
#include "../include/NetworkMonitor.h"
#include "../include/EventLogger.h"
#include "../include/AnomalyDetector.h"
#include "../include/PlatformUtils.h"

#ifdef PLATFORM_WINDOWS
    #include <windows.h>
    #include <conio.h>
#else
    #include <unistd.h>
    #include <sys/stat.h>
#endif

// Global flag for graceful shutdown
volatile sig_atomic_t running = 1;

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ". Shutting down gracefully..." << std::endl;
    running = 0;
}

#ifdef PLATFORM_WINDOWS
static BOOL WINAPI consoleCtrlHandler(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_CLOSE_EVENT) {
        running = 0;
        return TRUE;
    }
    return FALSE;
}
#endif

void printBanner() {
    std::cout << "================================================" << std::endl;
    std::cout << "        SentinelTrack System Monitor" << std::endl;
    std::cout << "     Real-time Process & Network Monitoring" << std::endl;
    std::cout << "================================================" << std::endl;
}

void createDataDirectory() {
    PlatformUtils::createDirectory("../data");
}

int main(int argc, char* argv[]) {
    printBanner();
    
    // Set up signal handlers for graceful shutdown
#ifdef PLATFORM_WINDOWS
    SetConsoleCtrlHandler(consoleCtrlHandler, TRUE);
#else
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
#endif
    
    // Create data directory if it doesn't exist
    createDataDirectory();
    
    // Initialize components
    ProcessMonitor processMonitor;
    NetworkMonitor networkMonitor;
    EventLogger logger("../data/sentineltrack.db", "../data/sentineltrack.log");
    AnomalyDetector anomalyDetector;
    
    if (!logger.isInitialized()) {
        std::cerr << "Failed to initialize EventLogger. Exiting." << std::endl;
        return 1;
    }
    
    std::cout << "SentinelTrack agent started. Monitoring system..." << std::endl;
    std::cout << "Press Ctrl+C to stop monitoring." << std::endl;
    
    int cycle_count = 0;
    auto last_stats_time = std::chrono::steady_clock::now();
    auto last_process_log_time = std::chrono::steady_clock::now();
    
    while (running) {
        try {
            auto start_time = std::chrono::steady_clock::now();
            
            // Monitor processes
            processMonitor.updateProcessList();
            auto current_processes = processMonitor.getCurrentProcesses();
            auto new_processes = processMonitor.getNewProcesses();
            auto terminated_processes = processMonitor.getTerminatedProcesses();
            
            // Log new processes
            for (const auto& process : new_processes) {
                std::cout << "[PROCESS] New: " << process.name << " (PID: " << process.pid << ")" << std::endl;
            }
            
            // Log terminated processes
            for (int pid : terminated_processes) {
                std::cout << "[PROCESS] Terminated: PID " << pid << std::endl;
            }
            
            // Monitor network connections
            networkMonitor.updateConnectionList();
            auto current_connections = networkMonitor.getCurrentConnections();
            auto new_connections = networkMonitor.getNewConnections();
            
            // Log new network connections
            for (const auto& connection : new_connections) {
                logger.logNetworkConnection(connection);
                std::cout << "[NETWORK] New connection: " << connection.local_ip << ":" 
                         << connection.local_port << " -> " << connection.remote_ip << ":" 
                         << connection.remote_port << " (" << connection.protocol << ")" << std::endl;
            }
            
            // Check for anomalies
            auto process_anomalies = anomalyDetector.checkProcessAnomalies(current_processes);
            auto network_anomalies = anomalyDetector.checkNetworkAnomalies(current_connections);
            
            // Get system stats and check for system anomalies
            auto system_stats = logger.getSystemStats();
            auto system_anomalies = anomalyDetector.checkSystemAnomalies(
                system_stats.cpu_usage, 
                static_cast<long>(system_stats.memory_usage * 1024 * 1024) // Convert to KB
            );
            
            // Log all anomalies
            for (const auto& anomaly : process_anomalies) {
                logger.logAlert(anomaly.type, anomaly.severity, anomaly.message, anomaly.details);
                std::cout << "[ALERT] " << anomaly.severity << ": " << anomaly.message << std::endl;
            }
            
            for (const auto& anomaly : network_anomalies) {
                logger.logAlert(anomaly.type, anomaly.severity, anomaly.message, anomaly.details);
                std::cout << "[ALERT] " << anomaly.severity << ": " << anomaly.message << std::endl;
            }
            
            for (const auto& anomaly : system_anomalies) {
                logger.logAlert(anomaly.type, anomaly.severity, anomaly.message, anomaly.details);
                std::cout << "[ALERT] " << anomaly.severity << ": " << anomaly.message << std::endl;
            }
            
            // Log system statistics every 10 cycles (approximately every 10 seconds)
            auto current_time = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(current_time - last_stats_time).count() >= 3) {
                logger.logSystemStats(system_stats);
                last_stats_time = current_time;

                for (const auto& process: current_processes) {
                    logger.logProcess(process);
                }
                last_process_log_time = current_time;

                // Log all current connections so the dashboard count stays accurate
                for (const auto& connection : current_connections) {
                    logger.logNetworkConnection(connection);
                }
                
                std::cout << "[STATS] CPU: " << system_stats.cpu_usage << "%, "
                         << "Memory: " << system_stats.memory_usage << "%, "
                         << "Load: " << system_stats.load_average << std::endl;
            }
            
            // Display monitoring summary every 100 cycles
            if (++cycle_count % 100 == 0) {
                std::cout << "\n--- Monitoring Summary ---" << std::endl;
                std::cout << "Active processes: " << current_processes.size() << std::endl;
                std::cout << "Active connections: " << current_connections.size() << std::endl;
                std::cout << "System CPU: " << system_stats.cpu_usage << "%" << std::endl;
                std::cout << "System Memory: " << system_stats.memory_usage << "%" << std::endl;
                std::cout << "------------------------\n" << std::endl;
            }
            
            // Sleep for monitoring interval (1 second)
            auto end_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            auto sleep_time = std::chrono::milliseconds(1000) - elapsed;
            
            if (sleep_time > std::chrono::milliseconds(0)) {
                PlatformUtils::sleepMs(sleep_time.count());
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error in monitoring loop: " << e.what() << std::endl;
            PlatformUtils::sleepMs(1000);
        }
    }
    
    // Cleanup
    logger.flushLogs();
    std::cout << "SentinelTrack agent stopped." << std::endl;
    
    return 0;
}