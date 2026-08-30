#include "../include/EventLogger.h"
#include "../include/PlatformUtils.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>

EventLogger::EventLogger(const std::string& db_file, const std::string& json_file) 
    : db(nullptr), db_path(db_file), json_path(json_file) {
    
    if (!initializeDatabase()) {
        std::cerr << "Failed to initialize database: " << db_path << std::endl;
        return;
    }
    
    json_log.open(json_path, std::ios::app);
    if (!json_log.is_open()) {
        std::cerr << "Failed to open JSON log file: " << json_path << std::endl;
    }
}

EventLogger::~EventLogger() {
    if (db) {
        sqlite3_close(db);
    }
    if (json_log.is_open()) {
        json_log.close();
    }
}

bool EventLogger::initializeDatabase() {
    int rc = sqlite3_open(db_path.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    
    // Create tables (same as before)
    const char* create_processes_table = R"(
        CREATE TABLE IF NOT EXISTS processes (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            pid INTEGER,
            name TEXT,
            cpu_usage REAL,
            memory_usage INTEGER,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    const char* create_network_table = R"(
        CREATE TABLE IF NOT EXISTS network_connections (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            local_ip TEXT,
            local_port INTEGER,
            remote_ip TEXT,
            remote_port INTEGER,
            protocol TEXT,
            state TEXT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    const char* create_alerts_table = R"(
        CREATE TABLE IF NOT EXISTS alerts (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            type TEXT,
            severity TEXT,
            message TEXT,
            details TEXT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    const char* create_system_stats_table = R"(
        CREATE TABLE IF NOT EXISTS system_stats (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            cpu_usage REAL,
            memory_usage REAL,
            disk_usage REAL,
            load_average REAL,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    char* err_msg = nullptr;
    
    if (sqlite3_exec(db, create_processes_table, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::cerr << "SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
        return false;
    }
    
    if (sqlite3_exec(db, create_network_table, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::cerr << "SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
        return false;
    }
    
    if (sqlite3_exec(db, create_alerts_table, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::cerr << "SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
        return false;
    }
    
    if (sqlite3_exec(db, create_system_stats_table, nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::cerr << "SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
        return false;
    }
    
    return true;
}

std::string EventLogger::getCurrentTimestamp() {
    return PlatformUtils::getCurrentTimestamp();
}

void EventLogger::logToJson(const std::string& event_type, const std::string& data) {
    if (!json_log.is_open()) return;
    
    std::string timestamp = getCurrentTimestamp();
    json_log << "{\"timestamp\":\"" << timestamp << "\",\"type\":\"" << event_type 
             << "\",\"data\":" << data << "}" << std::endl;
    json_log.flush();
}

void EventLogger::logProcess(const ProcessInfo& process) {
    if (!db) return;
    
    const char* sql = "INSERT INTO processes (pid, name, cpu_usage, memory_usage) VALUES (?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, process.pid);
        sqlite3_bind_text(stmt, 2, process.name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 3, process.cpu_usage);
        sqlite3_bind_int64(stmt, 4, process.memory_usage);
        
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    
    // Log to JSON
    std::stringstream json_data;
    json_data << "{\"pid\":" << process.pid << ",\"name\":\"" << process.name 
              << "\",\"cpu_usage\":" << process.cpu_usage 
              << ",\"memory_usage\":" << process.memory_usage << "}";
    logToJson("process", json_data.str());
}

void EventLogger::logNetworkConnection(const NetworkConnection& connection) {
    if (!db) return;
    
    const char* sql = "INSERT INTO network_connections (local_ip, local_port, remote_ip, remote_port, protocol, state) VALUES (?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, connection.local_ip.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, connection.local_port);
        sqlite3_bind_text(stmt, 3, connection.remote_ip.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 4, connection.remote_port);
        sqlite3_bind_text(stmt, 5, connection.protocol.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 6, connection.state.c_str(), -1, SQLITE_STATIC);
        
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    
    // Log to JSON
    std::stringstream json_data;
    json_data << "{\"local_ip\":\"" << connection.local_ip << "\",\"local_port\":" << connection.local_port
              << ",\"remote_ip\":\"" << connection.remote_ip << "\",\"remote_port\":" << connection.remote_port
              << ",\"protocol\":\"" << connection.protocol << "\",\"state\":\"" << connection.state << "\"}";
    logToJson("network", json_data.str());
}

void EventLogger::logAlert(const std::string& type, const std::string& severity, 
                          const std::string& message, const std::string& details) {
    if (!db) return;
    
    const char* sql = "INSERT INTO alerts (type, severity, message, details) VALUES (?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, type.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, severity.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, message.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, details.c_str(), -1, SQLITE_STATIC);
        
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    
    // Log to JSON
    std::stringstream json_data;
    json_data << "{\"type\":\"" << type << "\",\"severity\":\"" << severity
              << "\",\"message\":\"" << message << "\",\"details\":\"" << details << "\"}";
    logToJson("alert", json_data.str());
}

void EventLogger::logSystemStats(const SystemStats& stats) {
    if (!db) return;
    
    const char* sql = "INSERT INTO system_stats (cpu_usage, memory_usage, disk_usage, load_average) VALUES (?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_double(stmt, 1, stats.cpu_usage);
        sqlite3_bind_double(stmt, 2, stats.memory_usage);
        sqlite3_bind_double(stmt, 3, stats.disk_usage);
        sqlite3_bind_double(stmt, 4, stats.load_average);
        
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }
    
    // Log to JSON
    std::stringstream json_data;
    json_data << "{\"cpu_usage\":" << stats.cpu_usage << ",\"memory_usage\":" << stats.memory_usage
              << ",\"disk_usage\":" << stats.disk_usage << ",\"load_average\":" << stats.load_average << "}";
    logToJson("system_stats", json_data.str());
}

SystemStats EventLogger::getSystemStats() {
    SystemStats stats;
    stats.timestamp = getCurrentTimestamp();
    stats.cpu_usage = PlatformUtils::getCpuUsage();
    long total_mem = PlatformUtils::getTotalMemory();
    stats.memory_usage = total_mem > 0 ? (double)PlatformUtils::getMemoryUsage() / total_mem * 100.0 : 0.0;
    stats.load_average = PlatformUtils::getLoadAverage();
    stats.disk_usage = 50.0; // Placeholder - would need platform-specific disk usage code
    
    return stats;
}

bool EventLogger::isInitialized() const {
    return db != nullptr;
}

void EventLogger::flushLogs() {
    if (json_log.is_open()) {
        json_log.flush();
    }
}