#include "../include/PlatformUtils.h"
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>
#ifdef PLATFORM_MACOS
#include <mach-o/dyld.h>
#endif


#ifdef PLATFORM_WINDOWS
    #include <direct.h>
    #include <io.h>
#else
    #include <sys/stat.h>
    #include <unistd.h>
#endif

namespace PlatformUtils {

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void sleepMs(int milliseconds) {
    #ifdef PLATFORM_WINDOWS
        Sleep(milliseconds);
    #else
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    #endif
}

bool createDirectory(const std::string& path) {
#ifdef PLATFORM_WINDOWS
    return _mkdir(path.c_str()) == 0 || errno == EEXIST;
#else
    return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
#endif
}

std::string getExecutablePath() {
#ifdef PLATFORM_WINDOWS
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return std::string(buffer);
#elif defined(PLATFORM_MACOS)
    char buffer[1024];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0) {
        return std::string(buffer);
    }
    return "";
#else
    char buffer[1024];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        return std::string(buffer);
    }
    return "";
#endif
}

static double s_lastCpuUsage = 0.0;

double getCpuUsage() {
#ifdef PLATFORM_WINDOWS
    FILETIME idleFt, kernelFt, userFt;
    if (!GetSystemTimes(&idleFt, &kernelFt, &userFt)) return 0.0;

    ULARGE_INTEGER idle, kernel, user;
    idle.LowPart = idleFt.dwLowDateTime;   idle.HighPart = idleFt.dwHighDateTime;
    kernel.LowPart = kernelFt.dwLowDateTime; kernel.HighPart = kernelFt.dwHighDateTime;
    user.LowPart = userFt.dwLowDateTime;   user.HighPart = userFt.dwHighDateTime;

    static ULARGE_INTEGER lastIdle = {}, lastKernel = {}, lastUser = {};

    if (lastKernel.QuadPart == 0) {
        lastIdle = idle; lastKernel = kernel; lastUser = user;
        return 0.0;
    }

    unsigned long long idleDelta   = idle.QuadPart   - lastIdle.QuadPart;
    unsigned long long kernelDelta = kernel.QuadPart - lastKernel.QuadPart;
    unsigned long long userDelta   = user.QuadPart   - lastUser.QuadPart;

    lastIdle = idle; lastKernel = kernel; lastUser = user;

    unsigned long long total = kernelDelta + userDelta;
    if (total == 0) return s_lastCpuUsage;
    // kernelDelta includes idle time on Windows
    s_lastCpuUsage = (double)(total - idleDelta) / total * 100.0;
    return s_lastCpuUsage;
    
#elif defined(PLATFORM_MACOS)
    host_cpu_load_info_data_t cpuinfo;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    
    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, 
                       (host_info_t)&cpuinfo, &count) == KERN_SUCCESS) {
        
        static unsigned int lastUser = 0, lastSystem = 0, lastIdle = 0;
        
        unsigned int user = cpuinfo.cpu_ticks[CPU_STATE_USER];
        unsigned int system = cpuinfo.cpu_ticks[CPU_STATE_SYSTEM];
        unsigned int idle = cpuinfo.cpu_ticks[CPU_STATE_IDLE];
        
        unsigned int total = (user - lastUser) + (system - lastSystem) + (idle - lastIdle);
        unsigned int used = (user - lastUser) + (system - lastSystem);
        
        lastUser = user;
        lastSystem = system;
        lastIdle = idle;
        
        if (total > 0) {
            return (double)used / total * 100.0;
        }
    }
    return 0.0;
    
#else
    // Linux implementation (existing code)
    static unsigned long long lastTotalUser = 0, lastTotalUserLow = 0, lastTotalSys = 0, lastTotalIdle = 0;
    
    FILE* file = fopen("/proc/stat", "r");
    if (!file) return 0.0;
    
    unsigned long long totalUser, totalUserLow, totalSys, totalIdle, total;
    fscanf(file, "cpu %llu %llu %llu %llu", &totalUser, &totalUserLow, &totalSys, &totalIdle);
    fclose(file);
    
    if (lastTotalUser == 0) {
        lastTotalUser = totalUser;
        lastTotalUserLow = totalUserLow;
        lastTotalSys = totalSys;
        lastTotalIdle = totalIdle;
        return 0.0;
    }
    
    total = (totalUser - lastTotalUser) + (totalUserLow - lastTotalUserLow) + 
            (totalSys - lastTotalSys);
    double percent = total;
    total += (totalIdle - lastTotalIdle);
    percent /= total;
    percent *= 100;
    
    lastTotalUser = totalUser;
    lastTotalUserLow = totalUserLow;
    lastTotalSys = totalSys;
    lastTotalIdle = totalIdle;
    
    return percent;
#endif
}

long getMemoryUsage() {
#ifdef PLATFORM_WINDOWS
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    
    DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
    DWORDLONG physMemUsed = totalPhysMem - memInfo.ullAvailPhys;
    
    return (long)(physMemUsed / 1024); // Return in KB
    
#elif defined(PLATFORM_MACOS)
    vm_size_t page_size;
    vm_statistics64_data_t vm_stat;
    mach_msg_type_number_t count = sizeof(vm_stat) / sizeof(natural_t);
    
    if (host_page_size(mach_host_self(), &page_size) == KERN_SUCCESS &&
        host_statistics64(mach_host_self(), HOST_VM_INFO, 
                         (host_info64_t)&vm_stat, &count) == KERN_SUCCESS) {
        
        long used_memory = (vm_stat.active_count + vm_stat.inactive_count + 
                           vm_stat.wire_count) * page_size;
        return used_memory / 1024; // Return in KB
    }
    return 0;
    
#else
    // Linux implementation
    FILE* file = fopen("/proc/meminfo", "r");
    if (!file) return 0;
    
    long total = 0, available = 0;
    char line[256];
    
    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "MemTotal: %ld kB", &total) == 1) continue;
        if (sscanf(line, "MemAvailable: %ld kB", &available) == 1) break;
    }
    fclose(file);
    
    return total - available;
#endif
}

long getTotalMemory() {
#ifdef PLATFORM_WINDOWS
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return (long)(memInfo.ullTotalPhys / 1024);

#elif defined(PLATFORM_MACOS)
    int mib[2] = {CTL_HW, HW_MEMSIZE};
    uint64_t memsize;
    size_t size = sizeof(memsize);
    if (sysctl(mib, 2, &memsize, &size, NULL, 0) == 0) {
        return (long)(memsize / 1024);
    }
    return 0;

#else
    FILE* file = fopen("/proc/meminfo", "r");
    if (!file) return 0;
    long total = 0;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (sscanf(line, "MemTotal: %ld kB", &total) == 1) break;
    }
    fclose(file);
    return total;
#endif
}

double getLoadAverage() {
#ifdef PLATFORM_WINDOWS
    // Windows doesn't have load average; return last CPU %
    return s_lastCpuUsage;
    
#elif defined(PLATFORM_MACOS)
    double loadavg[3];
    if (getloadavg(loadavg, 3) != -1) {
        return loadavg[0];
    }
    return 0.0;
    
#else
    // Linux implementation
    FILE* file = fopen("/proc/loadavg", "r");
    if (!file) return 0.0;
    
    double load;
    fscanf(file, "%lf", &load);
    fclose(file);
    
    return load;
#endif
}

} // namespace PlatformUtils