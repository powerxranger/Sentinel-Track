#include "../include/NetworkMonitor.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

#ifdef PLATFORM_WINDOWS
    #define _WIN32_WINNT 0x0600
    #define WINVER 0x0600

    #include <winsock2.h>
    #include <windows.h>
    #include <ws2tcpip.h>
    #include <psapi.h>
    #include <iphlpapi.h>
#elif defined(PLATFORM_MACOS)
    #include <libproc.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/sysctl.h>
#endif

NetworkMonitor::NetworkMonitor() {
#ifdef PLATFORM_WINDOWS
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    updateConnectionList();
}

NetworkMonitor::~NetworkMonitor() {
#ifdef PLATFORM_WINDOWS
    WSACleanup();
#endif
}

std::string NetworkMonitor::ipToString(unsigned long ip) {
    return std::to_string((ip >> 0) & 0xFF) + "." +
           std::to_string((ip >> 8) & 0xFF) + "." +
           std::to_string((ip >> 16) & 0xFF) + "." +
           std::to_string((ip >> 24) & 0xFF);
}

std::vector<NetworkConnection> NetworkMonitor::parseTcpConnections() {
    std::vector<NetworkConnection> connections;
    
#ifdef PLATFORM_WINDOWS
    PMIB_TCPTABLE_OWNER_PID pTcpTable;
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;
    
    // Get the size needed
    dwRetVal = GetExtendedTcpTable(NULL, &dwSize, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
    if (dwRetVal == ERROR_INSUFFICIENT_BUFFER) {
        pTcpTable = (MIB_TCPTABLE_OWNER_PID*)malloc(dwSize);
        if (pTcpTable == NULL) return connections;
        
        dwRetVal = GetExtendedTcpTable(pTcpTable, &dwSize, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
        if (dwRetVal == NO_ERROR) {
            for (DWORD i = 0; i < pTcpTable->dwNumEntries; i++) {
                NetworkConnection conn;
                conn.protocol = "TCP";
                conn.local_ip = ipToString(pTcpTable->table[i].dwLocalAddr);
                conn.local_port = ntohs((u_short)pTcpTable->table[i].dwLocalPort);
                conn.remote_ip = ipToString(pTcpTable->table[i].dwRemoteAddr);
                conn.remote_port = ntohs((u_short)pTcpTable->table[i].dwRemotePort);
                conn.pid = pTcpTable->table[i].dwOwningPid;
                conn.process_name = getProcessNameByPid(conn.pid);
                
                // Convert state
                switch (pTcpTable->table[i].dwState) {
                    case MIB_TCP_STATE_CLOSED: conn.state = "CLOSED"; break;
                    case MIB_TCP_STATE_LISTEN: conn.state = "LISTEN"; break;
                    case MIB_TCP_STATE_SYN_SENT: conn.state = "SYN_SENT"; break;
                    case MIB_TCP_STATE_SYN_RCVD: conn.state = "SYN_RECV"; break;
                    case MIB_TCP_STATE_ESTAB: conn.state = "ESTABLISHED"; break;
                    case MIB_TCP_STATE_FIN_WAIT1: conn.state = "FIN_WAIT1"; break;
                    case MIB_TCP_STATE_FIN_WAIT2: conn.state = "FIN_WAIT2"; break;
                    case MIB_TCP_STATE_CLOSE_WAIT: conn.state = "CLOSE_WAIT"; break;
                    case MIB_TCP_STATE_CLOSING: conn.state = "CLOSING"; break;
                    case MIB_TCP_STATE_LAST_ACK: conn.state = "LAST_ACK"; break;
                    case MIB_TCP_STATE_TIME_WAIT: conn.state = "TIME_WAIT"; break;
                    default: conn.state = "UNKNOWN"; break;
                }
                
                connections.push_back(conn);
            }
        }
        free(pTcpTable);
    }
    
#elif defined(PLATFORM_MACOS)
    FILE* fp = popen("netstat -an -p tcp 2>/dev/null", "r");
    if (!fp) return connections;

    auto parseNetstatAddr = [](const std::string& addr, std::string& ip, int& port) {
        size_t last_dot = addr.rfind('.');
        if (last_dot == std::string::npos) return;
        ip = addr.substr(0, last_dot);
        if (ip == "*") ip = "0.0.0.0";
        try { port = std::stoi(addr.substr(last_dot + 1)); }
        catch (...) { port = 0; }
    };

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "tcp", 3) != 0) continue;
        char proto[16], local_addr[64], remote_addr[64], state[32] = "";
        int recv_q, send_q;
        int n = sscanf(line, "%15s %d %d %63s %63s %31s",
                       proto, &recv_q, &send_q, local_addr, remote_addr, state);
        if (n < 5) continue;

        NetworkConnection conn;
        conn.protocol = "TCP";
        conn.pid = 0;
        conn.process_name = "Unknown";
        parseNetstatAddr(std::string(local_addr), conn.local_ip, conn.local_port);
        parseNetstatAddr(std::string(remote_addr), conn.remote_ip, conn.remote_port);
        conn.state = (state[0] != '\0') ? std::string(state) : "UNKNOWN";
        connections.push_back(conn);
    }
    pclose(fp);
    
#else
    // Linux implementation (existing code)
    std::ifstream tcp_file("/proc/net/tcp");
    
    if (!tcp_file.is_open()) {
        return connections;
    }
    
    std::string line;
    std::getline(tcp_file, line); // Skip header
    
    while (std::getline(tcp_file, line)) {
        std::istringstream iss(line);
        std::string sl, local_address, rem_address, st, tx_queue, rx_queue, tr, tm_when, retrnsmt, uid, timeout, inode;
        
        iss >> sl >> local_address >> rem_address >> st >> tx_queue >> rx_queue >> tr >> tm_when >> retrnsmt >> uid >> timeout >> inode;
        
        NetworkConnection conn;
        conn.protocol = "TCP";
        
        // Parse local address (IP:Port in hex)
        size_t colon_pos = local_address.find(':');
        if (colon_pos != std::string::npos) {
            std::string local_ip_hex = local_address.substr(0, colon_pos);
            std::string local_port_hex = local_address.substr(colon_pos + 1);
            
            unsigned long local_ip_num = std::stoul(local_ip_hex, nullptr, 16);
            conn.local_ip = ipToString(local_ip_num);
            conn.local_port = std::stoi(local_port_hex, nullptr, 16);
        }
        
        // Parse remote address
        colon_pos = rem_address.find(':');
        if (colon_pos != std::string::npos) {
            std::string rem_ip_hex = rem_address.substr(0, colon_pos);
            std::string rem_port_hex = rem_address.substr(colon_pos + 1);
            
            unsigned long rem_ip_num = std::stoul(rem_ip_hex, nullptr, 16);
            conn.remote_ip = ipToString(rem_ip_num);
            conn.remote_port = std::stoi(rem_port_hex, nullptr, 16);
        }
        
        // Parse state
        int state_num = std::stoi(st, nullptr, 16);
        switch (state_num) {
            case 1: conn.state = "ESTABLISHED"; break;
            case 2: conn.state = "SYN_SENT"; break;
            case 3: conn.state = "SYN_RECV"; break;
            case 4: conn.state = "FIN_WAIT1"; break;
            case 5: conn.state = "FIN_WAIT2"; break;
            case 6: conn.state = "TIME_WAIT"; break;
            case 7: conn.state = "CLOSE"; break;
            case 8: conn.state = "CLOSE_WAIT"; break;
            case 9: conn.state = "LAST_ACK"; break;
            case 10: conn.state = "LISTEN"; break;
            case 11: conn.state = "CLOSING"; break;
            default: conn.state = "UNKNOWN"; break;
        }
        
        conn.pid = 0;
        conn.process_name = "Unknown";
        
        connections.push_back(conn);
    }
#endif
    
    return connections;
}

std::vector<NetworkConnection> NetworkMonitor::parseUdpConnections() {
    std::vector<NetworkConnection> connections;
    
#ifdef PLATFORM_WINDOWS
    PMIB_UDPTABLE_OWNER_PID pUdpTable;
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;
    
    dwRetVal = GetExtendedUdpTable(NULL, &dwSize, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0);
    if (dwRetVal == ERROR_INSUFFICIENT_BUFFER) {
        pUdpTable = (MIB_UDPTABLE_OWNER_PID*)malloc(dwSize);
        if (pUdpTable == NULL) return connections;
        
        dwRetVal = GetExtendedUdpTable(pUdpTable, &dwSize, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0);
        if (dwRetVal == NO_ERROR) {
            for (DWORD i = 0; i < pUdpTable->dwNumEntries; i++) {
                NetworkConnection conn;
                conn.protocol = "UDP";
                conn.local_ip = ipToString(pUdpTable->table[i].dwLocalAddr);
                conn.local_port = ntohs((u_short)pUdpTable->table[i].dwLocalPort);
                conn.remote_ip = "0.0.0.0";
                conn.remote_port = 0;
                conn.state = "ESTABLISHED";
                conn.pid = pUdpTable->table[i].dwOwningPid;
                conn.process_name = getProcessNameByPid(conn.pid);
                
                connections.push_back(conn);
            }
        }
        free(pUdpTable);
    }
    
#elif defined(PLATFORM_MACOS)
    FILE* fp = popen("netstat -an -p udp 2>/dev/null", "r");
    if (!fp) return connections;

    auto parseNetstatAddr = [](const std::string& addr, std::string& ip, int& port) {
        size_t last_dot = addr.rfind('.');
        if (last_dot == std::string::npos) return;
        ip = addr.substr(0, last_dot);
        if (ip == "*") ip = "0.0.0.0";
        try { port = std::stoi(addr.substr(last_dot + 1)); }
        catch (...) { port = 0; }
    };

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "udp", 3) != 0) continue;
        char proto[16], local_addr[64], remote_addr[64];
        int recv_q, send_q;
        int n = sscanf(line, "%15s %d %d %63s %63s",
                       proto, &recv_q, &send_q, local_addr, remote_addr);
        if (n < 4) continue;

        NetworkConnection conn;
        conn.protocol = "UDP";
        conn.state = "ESTABLISHED";
        conn.pid = 0;
        conn.process_name = "Unknown";
        parseNetstatAddr(std::string(local_addr), conn.local_ip, conn.local_port);
        if (n >= 5) parseNetstatAddr(std::string(remote_addr), conn.remote_ip, conn.remote_port);
        connections.push_back(conn);
    }
    pclose(fp);

#else
    // Linux implementation (existing code)
    std::ifstream udp_file("/proc/net/udp");
    
    if (!udp_file.is_open()) {
        return connections;
    }
    
    std::string line;
    std::getline(udp_file, line); // Skip header
    
    while (std::getline(udp_file, line)) {
        std::istringstream iss(line);
        std::string sl, local_address, rem_address, st, tx_queue, rx_queue, tr, tm_when, retrnsmt, uid, timeout, inode;
        
        iss >> sl >> local_address >> rem_address >> st >> tx_queue >> rx_queue >> tr >> tm_when >> retrnsmt >> uid >> timeout >> inode;
        
        NetworkConnection conn;
        conn.protocol = "UDP";
        conn.state = "ESTABLISHED";
        
        // Parse local address
        size_t colon_pos = local_address.find(':');
        if (colon_pos != std::string::npos) {
            std::string local_ip_hex = local_address.substr(0, colon_pos);
            std::string local_port_hex = local_address.substr(colon_pos + 1);
            
            unsigned long local_ip_num = std::stoul(local_ip_hex, nullptr, 16);
            conn.local_ip = ipToString(local_ip_num);
            conn.local_port = std::stoi(local_port_hex, nullptr, 16);
        }
        
        // Parse remote address
        colon_pos = rem_address.find(':');
        if (colon_pos != std::string::npos) {
            std::string rem_ip_hex = rem_address.substr(0, colon_pos);
            std::string rem_port_hex = rem_address.substr(colon_pos + 1);
            
            unsigned long rem_ip_num = std::stoul(rem_ip_hex, nullptr, 16);
            conn.remote_ip = ipToString(rem_ip_num);
            conn.remote_port = std::stoi(rem_port_hex, nullptr, 16);
        }
        
        conn.pid = 0;
        conn.process_name = "Unknown";
        
        connections.push_back(conn);
    }
#endif
    
    return connections;
}

std::string NetworkMonitor::getConnectionKey(const NetworkConnection& conn) {
    return conn.protocol + ":" + conn.local_ip + ":" + std::to_string(conn.local_port) + 
           "->" + conn.remote_ip + ":" + std::to_string(conn.remote_port);
}

std::string NetworkMonitor::getProcessNameByPid(int pid) {
    if (pid == 0) return "Unknown";
    
#ifdef PLATFORM_WINDOWS
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess) {
        char processName[MAX_PATH];
        if (GetModuleBaseNameA(hProcess, NULL, processName, sizeof(processName))) {
            CloseHandle(hProcess);
            return std::string(processName);
        }
        CloseHandle(hProcess);
    }
    return "Unknown";
    
#elif defined(PLATFORM_MACOS)
    struct proc_bsdinfo proc_info;
    int size = proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &proc_info, sizeof(proc_info));
    if (size == sizeof(proc_info)) {
        return std::string(proc_info.pbi_comm);
    }
    return "Unknown";
    
#else
    // Linux implementation (existing)
    std::ifstream comm_file("/proc/" + std::to_string(pid) + "/comm");
    if (comm_file.is_open()) {
        std::string name;
        std::getline(comm_file, name);
        return name;
    }
    return "Unknown";
#endif
}

std::vector<NetworkConnection> NetworkMonitor::getCurrentConnections() {
    std::vector<NetworkConnection> all_connections;
    
    // Get TCP connections
    auto tcp_connections = parseTcpConnections();
    all_connections.insert(all_connections.end(), tcp_connections.begin(), tcp_connections.end());
    
    // Get UDP connections
    auto udp_connections = parseUdpConnections();
    all_connections.insert(all_connections.end(), udp_connections.begin(), udp_connections.end());
    
    return all_connections;
}

std::vector<NetworkConnection> NetworkMonitor::getNewConnections() {
    std::vector<NetworkConnection> new_connections;
    auto current_connections = getCurrentConnections();
    
    for (const auto& conn : current_connections) {
        std::string key = getConnectionKey(conn);
        if (previous_connections.find(key) == previous_connections.end()) {
            new_connections.push_back(conn);
        }
    }
    
    return new_connections;
}

std::vector<NetworkConnection> NetworkMonitor::getListeningPorts() {
    std::vector<NetworkConnection> listening_ports;
    auto current_connections = getCurrentConnections();
    
    for (const auto& conn : current_connections) {
        if (conn.state == "LISTEN" || (conn.protocol == "UDP" && conn.remote_ip == "0.0.0.0")) {
            listening_ports.push_back(conn);
        }
    }
    
    return listening_ports;
}

void NetworkMonitor::updateConnectionList() {
    previous_connections.clear();
    auto current_connections = getCurrentConnections();
    
    for (const auto& conn : current_connections) {
        std::string key = getConnectionKey(conn);
        previous_connections.insert(key);
    }
}

std::vector<int> NetworkMonitor::getOpenPorts() {
    std::vector<int> open_ports;
    auto listening_ports = getListeningPorts();
    
    for (const auto& conn : listening_ports) {
        open_ports.push_back(conn.local_port);
    }
    
    // Remove duplicates
    std::sort(open_ports.begin(), open_ports.end());
    open_ports.erase(std::unique(open_ports.begin(), open_ports.end()), open_ports.end());
    
    return open_ports;
}