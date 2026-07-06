#ifndef PLATFORM_UTILS_H

#include <string>
#include <vector>

// Platform detection
#ifdef _WIN32
    #define PLATFORM_WINDOWS
    #include <psapi.h>
    #include <tlhelp32.h>
    #include <iphlpapi.h>
    #include <winsock2.h>
    #include <windows.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "psapi.lib")
    #pragma comment(lib, "iphlpapi.lib")
    #pragma comment(lib, "ws2_32.lib")
#elif __APPLE__
    #ifndef PLATFORM_MACOS
    #define PLATFORM_MACOS
    #endif
    #include <sys/types.h>
    #include <sys/sysctl.h>
    #include <sys/proc_info.h>
    #include <libproc.h>
    #include <mach/mach.h>
    #include <mach/processor_info.h>
    #include <mach/mach_host.h>
    #include <ifaddrs.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#else
    #define PLATFORM_LINUX
    #include <unistd.h>
    #include <dirent.h>
    #include <sys/stat.h>
#endif

namespace PlatformUtils {
    std::string getCurrentTimestamp();
    void sleepMs(int milliseconds);
    bool createDirectory(const std::string& path);
    std::string getExecutablePath();
    
    // Platform-specific system info
    double getCpuUsage();
    long getMemoryUsage();
    double getLoadAverage();
}

#endif