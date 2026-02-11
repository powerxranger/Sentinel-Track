#ifndef NETWORK_MONITOR_H
#define NETWORK_MONITOR_H

#include <vector>
#include <string>
#include <unordered_set>

struct NetworkConnection {
    std::string local_ip;
    int local_port;
    std::string remote_ip;
    int remote_port;
    std::string protocol;
    std::string state;
    int pid;
    std::string process_name;
};

class NetworkMonitor {
private:
    std::unordered_set<std::string> previous_connections;
    
    std::vector<NetworkConnection> parseTcpConnections();
    std::vector<NetworkConnection> parseUdpConnections();
    std::string getConnectionKey(const NetworkConnection& conn);
    std::string getProcessNameByPid(int pid);

public:
    NetworkMonitor();
    ~NetworkMonitor();
    
    std::vector<NetworkConnection> getCurrentConnections();
    std::vector<NetworkConnection> getNewConnections();
    std::vector<NetworkConnection> getListeningPorts();
    void updateConnectionList();
    
    // Utility functions
    static std::string ipToString(unsigned long ip);
    std::vector<int> getOpenPorts();
};

#endif