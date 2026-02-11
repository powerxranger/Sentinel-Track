#include "../include/ProcessMonitor.h"
#include "../include/PlatformUtils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <set>

#ifdef PLATFORM_WINDOWS
    #include <windows.h>
    #include <tlhelp32.h>
    #include <psapi.h>
#elif defined(PLATFORM_MACOS)
    #include <sys/sysctl.h>
    #include <libproc.h>
    #include <sys/proc_info.h>
#else
    #include <dirent.h>
    #include <unistd.h>
    #include <sys/stat.h>
#endif

ProcessMonitor::ProcessMonitor() : previous_total_cpu_time(0) {
    updateProcessList();
}

ProcessMonitor::~ProcessMonitor() {
    // Cleanup if needed
}

std::vector<int> ProcessMonitor::getAllPids() {
    std::vector<int> pids;
    
#ifdef PLATFORM_WINDOWS
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return pids;
    }
    
    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    if (Process32First(hProcessSnap, &pe32)) {
        do {
            pids.push_back(pe32.th32ProcessID);
        } while (Process32Next(hProcessSnap, &pe32));
    }
    
    CloseHandle(hProcessSnap);
    
#elif defined(PLATFORM_MACOS)
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
    size_t size;
    
    if (sysctl(mib, 4, NULL, &size, NULL, 0) < 0) {
        return pids;
    }
    
    struct kinfo_proc* procs = (struct kinfo_proc*)malloc(size);
    if (sysctl(mib, 4, procs, &size, NULL, 0) < 0) {
        free(procs);
        return pids;
    }
    
    int nprocs = size / sizeof(struct kinfo_proc);
    for (int i = 0; i < nprocs; i++) {
        pids.push_back(procs[i].kp_proc.p_pid);
    }
    
    free(procs);
    
#else
    // Linux implementation (existing code)
    DIR* proc_dir = opendir("/proc");
    if (proc_dir == nullptr) {
        return pids;
    }
    
    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        if (entry->d_type == DT_DIR) {
            int pid = std::atoi(entry->d_name);
            if (pid > 0) {
                pids.push_back(pid);
            }
        }
    }
    
    closedir(proc_dir);
#endif
    
    return pids;
}

ProcessInfo ProcessMonitor::parseProcessInfo(int pid) {
    ProcessInfo info;
    info.pid = pid;
    info.cpu_usage = 0.0;
    info.memory_usage = 0;
    info.state = "Unknown";
    info.parent_pid = 0;
    info.name = "Unknown";
    info.command = "Unknown";
    
#ifdef PLATFORM_WINDOWS
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess) {
        // Get process name
        char processName[MAX_PATH];
        if (GetModuleBaseNameA(hProcess, NULL, processName, sizeof(processName))) {
            info.name = std::string(processName);
        }
        
        // Get memory usage
        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
            info.memory_usage = pmc.WorkingSetSize / 1024; // Convert to KB
        }
        
        // Get CPU usage (simplified)
        FILETIME creationTime, exitTime, kernelTime, userTime;
        if (GetProcessTimes(hProcess, &creationTime, &exitTime, &kernelTime, &userTime)) {
            ULARGE_INTEGER kt, ut;
            kt.LowPart = kernelTime.dwLowDateTime;
            kt.HighPart = kernelTime.dwHighDateTime;
            ut.LowPart = userTime.dwLowDateTime;
            ut.HighPart = userTime.dwHighDateTime;
            
            unsigned long long totalTime = kt.QuadPart + ut.QuadPart;
            info.cpu_usage = calculateCpuUsage(pid, totalTime);
        }
        
        info.state = "Running";
        CloseHandle(hProcess);
    }
    
#elif defined(PLATFORM_MACOS)
    struct proc_taskallinfo taskInfo;
    int size = proc_pidinfo(pid, PROC_PIDTASKALLINFO, 0, &taskInfo, sizeof(taskInfo));
    
    if (size == sizeof(taskInfo)) {
        info.name = std::string(taskInfo.pbsd.pbi_comm);
        info.memory_usage = taskInfo.ptinfo.pti_resident_size / 1024; // Convert to KB
        info.parent_pid = taskInfo.pbsd.pbi_ppid;
        info.state = "Running";
        
        // Get command line
        char pathbuf[PROC_PIDPATHINFO_MAXSIZE];
        if (proc_pidpath(pid, pathbuf, sizeof(pathbuf)) > 0) {
            info.command = std::string(pathbuf);
        }
        
        // CPU usage calculation (simplified)
        unsigned long long current_cpu_time = taskInfo.ptinfo.pti_total_user + taskInfo.ptinfo.pti_total_system;
        info.cpu_usage = calculateCpuUsage(pid, current_cpu_time);
    }
    
#else
    // Linux implementation (existing code)
    std::ifstream stat_file("/proc/" + std::to_string(pid) + "/stat");
    if (stat_file.is_open()) {
        std::string line;
        std::getline(stat_file, line);
        std::istringstream iss(line);
        
        std::string token;
        int field_num = 0;
        while (iss >> token && field_num < 25) {
            switch (field_num) {
                case 1:
                    info.name = token.substr(1, token.length() - 2);
                    break;
                case 2:
                    info.state = token;
                    break;
                case 3:
                    info.parent_pid = std::stoi(token);
                    break;
                case 22:
                    info.memory_usage = std::stol(token) / 1024;
                    break;
            }
            field_num++;
        }
        stat_file.close();
    }
    
    std::ifstream cmdline_file("/proc/" + std::to_string(pid) + "/cmdline");
    if (cmdline_file.is_open()) {
        std::getline(cmdline_file, info.command);
        std::replace(info.command.begin(), info.command.end(), '\0', ' ');
        cmdline_file.close();
    }
    
    unsigned long long current_cpu_time = getProcessCpuTime(pid);
    info.cpu_usage = calculateCpuUsage(pid, current_cpu_time);
#endif
    
    return info;
}

unsigned long long ProcessMonitor::getTotalCpuTime() {
#ifdef PLATFORM_WINDOWS
    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        ULARGE_INTEGER kt, ut;
        kt.LowPart = kernelTime.dwLowDateTime;
        kt.HighPart = kernelTime.dwHighDateTime;
        ut.LowPart = userTime.dwLowDateTime;
        ut.HighPart = userTime.dwHighDateTime;
        return kt.QuadPart + ut.QuadPart;
    }
    return 0;
    
#elif defined(PLATFORM_MACOS)
    host_cpu_load_info_data_t cpuinfo;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    
    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, 
                       (host_info_t)&cpuinfo, &count) == KERN_SUCCESS) {
        return cpuinfo.cpu_ticks[CPU_STATE_USER] + 
               cpuinfo.cpu_ticks[CPU_STATE_SYSTEM] + 
               cpuinfo.cpu_ticks[CPU_STATE_IDLE];
    }
    return 0;
    
#else
    // Linux implementation (existing)
    std::ifstream stat_file("/proc/stat");
    if (!stat_file.is_open()) {
        return 0;
    }
    
    std::string line;
    std::getline(stat_file, line);
    
    std::istringstream iss(line);
    std::string cpu_label;
    iss >> cpu_label;
    
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    iss >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
    
    return user + nice + system + idle + iowait + irq + softirq + steal;
#endif
}

unsigned long long ProcessMonitor::getProcessCpuTime(int pid) {
#ifdef PLATFORM_WINDOWS
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (hProcess) {
        FILETIME creationTime, exitTime, kernelTime, userTime;
        if (GetProcessTimes(hProcess, &creationTime, &exitTime, &kernelTime, &userTime)) {
            ULARGE_INTEGER kt, ut;
            kt.LowPart = kernelTime.dwLowDateTime;
            kt.HighPart = kernelTime.dwHighDateTime;
            ut.LowPart = userTime.dwLowDateTime;
            ut.HighPart = userTime.dwHighDateTime;
            
            CloseHandle(hProcess);
            return kt.QuadPart + ut.QuadPart;
        }
        CloseHandle(hProcess);
    }
    return 0;
    
#elif defined(PLATFORM_MACOS)
    struct proc_taskallinfo taskInfo;
    int size = proc_pidinfo(pid, PROC_PIDTASKALLINFO, 0, &taskInfo, sizeof(taskInfo));
    
    if (size == sizeof(taskInfo)) {
        return taskInfo.ptinfo.pti_total_user + taskInfo.ptinfo.pti_total_system;
    }
    return 0;
    
#else
    // Linux implementation (existing)
    std::ifstream stat_file("/proc/" + std::to_string(pid) + "/stat");
    if (!stat_file.is_open()) {
        return 0;
    }
    
    std::string line;
    std::getline(stat_file, line);
    std::istringstream iss(line);
    
    std::string token;
    for (int i = 0; i < 13; i++) {
        iss >> token;
    }
    
    unsigned long long utime, stime;
    iss >> utime >> stime;
    
    return utime + stime;
#endif
}

double ProcessMonitor::calculateCpuUsage(int pid, unsigned long long current_cpu_time) {
    unsigned long long current_total_cpu_time = getTotalCpuTime();
    
    if (previous_cpu_times.find(pid) == previous_cpu_times.end() || previous_total_cpu_time == 0) {
        previous_cpu_times[pid] = current_cpu_time;
        previous_total_cpu_time = current_total_cpu_time;
        return 0.0;
    }
    
    unsigned long long cpu_time_delta = current_cpu_time - previous_cpu_times[pid];
    unsigned long long total_cpu_time_delta = current_total_cpu_time - previous_total_cpu_time;
    
    if (total_cpu_time_delta == 0) {
        return 0.0;
    }
    
    double cpu_usage = (static_cast<double>(cpu_time_delta) / total_cpu_time_delta) * 100.0;
    
    previous_cpu_times[pid] = current_cpu_time;
    previous_total_cpu_time = current_total_cpu_time;
    
    return cpu_usage;
}

std::vector<ProcessInfo> ProcessMonitor::getCurrentProcesses() {
    std::vector<ProcessInfo> processes;
    auto pids = getAllPids();
    
    for (int pid : pids) {
        try {
            ProcessInfo info = parseProcessInfo(pid);
            if (!info.name.empty() && info.name != "Unknown") {
                processes.push_back(info);
            }
        } catch (const std::exception& e) {
            continue;
        }
    }
    
    return processes;
}

std::vector<ProcessInfo> ProcessMonitor::getNewProcesses() {
    std::vector<ProcessInfo> new_processes;
    auto current_processes = getCurrentProcesses();
    
    for (const auto& process : current_processes) {
        if (previous_processes.find(process.pid) == previous_processes.end()) {
            new_processes.push_back(process);
        }
    }
    
    return new_processes;
}

std::vector<int> ProcessMonitor::getTerminatedProcesses() {
    std::vector<int> terminated_pids;
    auto current_pids = getAllPids();
    std::set<int> current_pid_set(current_pids.begin(), current_pids.end());
    
    for (const auto& pair : previous_processes) {
        if (current_pid_set.find(pair.first) == current_pid_set.end()) {
            terminated_pids.push_back(pair.first);
        }
    }
    
    return terminated_pids;
}

void ProcessMonitor::updateProcessList() {
    previous_processes.clear();
    auto current_processes = getCurrentProcesses();
    
    for (const auto& process : current_processes) {
        previous_processes[process.pid] = process;
    }
}

long ProcessMonitor::getSystemMemoryTotal() {
#ifdef PLATFORM_WINDOWS
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return memInfo.ullTotalPhys / 1024; // Convert to KB
    
#elif defined(PLATFORM_MACOS)
    int mib[2] = {CTL_HW, HW_MEMSIZE};
    uint64_t memsize;
    size_t size = sizeof(memsize);
    
    if (sysctl(mib, 2, &memsize, &size, NULL, 0) == 0) {
        return memsize / 1024; // Convert to KB
    }
    return 0;
    
#else
    // Linux implementation (existing)
    std::ifstream meminfo("/proc/meminfo");
    if (!meminfo.is_open()) {
        return 0;
    }
    
    std::string line;
    while (std::getline(meminfo, line)) {
        if (line.substr(0, 9) == "MemTotal:") {
            std::istringstream iss(line);
            std::string label, value, unit;
            iss >> label >> value >> unit;
            return std::stol(value);
        }
    }
    
    return 0;
#endif
}

long ProcessMonitor::getSystemMemoryUsed() {
    return PlatformUtils::getMemoryUsage();
}