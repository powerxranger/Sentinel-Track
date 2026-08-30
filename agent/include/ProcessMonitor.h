#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H

#include <vector>
#include <string>
#include <unordered_map>
#include <set>

struct ProcessInfo {
    int pid;
    std::string name;
    std::string command;
    double cpu_usage;
    long memory_usage; // in KB
    std::string state;
    int parent_pid;
    std::string start_time;
};

class ProcessMonitor {
private:
    std::unordered_map<int, ProcessInfo> previous_processes;
    std::unordered_map<int, unsigned long long> previous_cpu_times;
    std::set<int> previous_pid_set;
    unsigned long long previous_total_cpu_time;
    unsigned long long snapshot_total_cpu_time;

    ProcessInfo parseProcessInfo(int pid);
    unsigned long long getTotalCpuTime();
    unsigned long long getProcessCpuTime(int pid);
    double calculateCpuUsage(int pid, unsigned long long current_cpu_time);

public:
    ProcessMonitor();
    ~ProcessMonitor();
    
    std::vector<ProcessInfo> getCurrentProcesses();
    std::vector<ProcessInfo> getNewProcesses();
    std::vector<int> getTerminatedProcesses();
    void updateProcessList();
    
    // Utility functions
    static std::vector<int> getAllPids();
    static long getSystemMemoryTotal();
    static long getSystemMemoryUsed();
};

#endif