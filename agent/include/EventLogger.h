#ifndef EVENT_LOGGER_H
#define EVENT_LOGGER_H

#include <string>
#include <sqlite3.h>
#include "ProcessMonitor.h"
#include "NetworkMonitor.h"

enum class LogLevel {
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

enum class EventType {
    PROCESS_STARTED,
    PROCESS_TERMINATED,
    NETWORK_CONNECTION,
    ANOMALY_DETECTED,
    SYSTEM_STATS
};

struct SystemStats {
    double cpu_usage;
    double memory_usage;
    double disk_usage;
    double load_average;
    std::string timestamp;
};

class EventLogger {
private:
    sqlite3* db;
    std::string db_path;

    bool initializeDatabase();
    std::string getCurrentTimestamp();

public:
    EventLogger(const std::string& db_file);
    ~EventLogger();
    
    void logProcess(const ProcessInfo& process);
    void logNetworkConnection(const NetworkConnection& connection);
    void logAlert(const std::string& type, const std::string& severity, 
                  const std::string& message, const std::string& details);
    void logSystemStats(const SystemStats& stats);
    
    // Utility functions
    SystemStats getSystemStats();
    bool isInitialized() const;
    void flushLogs();
};

#endif