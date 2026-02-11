import React, { useState, useEffect } from 'react';
import { Network, Search, RefreshCw, Shield, Globe, Server } from 'lucide-react';

interface NetworkConnection {
  id: number;
  local_ip: string;
  local_port: number;
  remote_ip: string;
  remote_port: number;
  protocol: string;
  state: string;
  timestamp: string;
}

const NetworkMonitor: React.FC = () => {
  const [connections, setConnections] = useState<NetworkConnection[]>([]);
  const [loading, setLoading] = useState(true);
  const [searchTerm, setSearchTerm] = useState('');
  const [filterProtocol, setFilterProtocol] = useState<'all' | 'TCP' | 'UDP'>('all');
  const [filterState, setFilterState] = useState<'all' | 'ESTABLISHED' | 'LISTEN'>('all');

  useEffect(() => {
    const fetchConnections = async () => {
      try {
        const response = await fetch('http://localhost:3001/api/network?limit=500');
        if (response.ok) {
          const data = await response.json();
          setConnections(data);
        }
        setLoading(false);
      } catch (error) {
        console.error('Error fetching network connections:', error);
        setLoading(false);
      }
    };

    fetchConnections();
    const interval = setInterval(fetchConnections, 5000); // Update every 5 seconds

    return () => clearInterval(interval);
  }, []);

  const filteredConnections = React.useMemo(() => {
    let filtered = connections.filter(conn => {
      const matchesSearch = 
        conn.local_ip.includes(searchTerm) ||
        conn.remote_ip.includes(searchTerm) ||
        conn.local_port.toString().includes(searchTerm) ||
        conn.remote_port.toString().includes(searchTerm) ||
        conn.protocol.toLowerCase().includes(searchTerm.toLowerCase()) ||
        conn.state.toLowerCase().includes(searchTerm.toLowerCase());

      const matchesProtocol = filterProtocol === 'all' || conn.protocol === filterProtocol;
      const matchesState = filterState === 'all' || conn.state === filterState;

      return matchesSearch && matchesProtocol && matchesState;
    });

    // Remove duplicates and keep the latest
    const uniqueConnections = new Map<string, NetworkConnection>();
    filtered.forEach(conn => {
      const key = `${conn.protocol}:${conn.local_ip}:${conn.local_port}:${conn.remote_ip}:${conn.remote_port}`;
      const existing = uniqueConnections.get(key);
      if (!existing || new Date(conn.timestamp) > new Date(existing.timestamp)) {
        uniqueConnections.set(key, conn);
      }
    });

    return Array.from(uniqueConnections.values()).sort((a, b) => 
      new Date(b.timestamp).getTime() - new Date(a.timestamp).getTime()
    );
  }, [connections, searchTerm, filterProtocol, filterState]);

  const getConnectionTypeIcon = (conn: NetworkConnection) => {
    if (conn.state === 'LISTEN') {
      return <Server className="h-4 w-4 text-blue-400" />;
    } else if (conn.remote_ip.startsWith('127.') || conn.remote_ip === '::1') {
      return <Shield className="h-4 w-4 text-green-400" />;
    } else {
      return <Globe className="h-4 w-4 text-orange-400" />;
    }
  };

  const getConnectionTypeLabel = (conn: NetworkConnection) => {
    if (conn.state === 'LISTEN') {
      return 'Listening';
    } else if (conn.remote_ip.startsWith('127.') || conn.remote_ip === '::1') {
      return 'Local';
    } else {
      return 'External';
    }
  };

  const getStateColor = (state: string) => {
    switch (state) {
      case 'ESTABLISHED':
        return 'text-green-400 bg-green-900/20';
      case 'LISTEN':
        return 'text-blue-400 bg-blue-900/20';
      case 'TIME_WAIT':
        return 'text-yellow-400 bg-yellow-900/20';
      case 'CLOSE_WAIT':
        return 'text-orange-400 bg-orange-900/20';
      default:
        return 'text-slate-400 bg-slate-900/20';
    }
  };

  const getPortRiskLevel = (port: number) => {
    const commonPorts = [80, 443, 22, 21, 25, 53, 110, 143, 993, 995];
    const suspiciousPorts = [4444, 5555, 6666, 7777, 8888, 9999, 1234, 12345, 54321, 31337];
    
    if (suspiciousPorts.includes(port)) {
      return { level: 'high', color: 'text-red-400' };
    } else if (port < 1024 && !commonPorts.includes(port)) {
      return { level: 'medium', color: 'text-yellow-400' };
    } else {
      return { level: 'low', color: 'text-green-400' };
    }
  };

  if (loading) {
    return (
      <div className="flex items-center justify-center h-64">
        <div className="animate-spin rounded-full h-12 w-12 border-b-2 border-blue-500"></div>
      </div>
    );
  }

  const stats = {
    total: filteredConnections.length,
    established: filteredConnections.filter(c => c.state === 'ESTABLISHED').length,
    listening: filteredConnections.filter(c => c.state === 'LISTEN').length,
    external: filteredConnections.filter(c => !c.remote_ip.startsWith('127.') && c.remote_ip !== '::1' && c.state === 'ESTABLISHED').length,
  };

  return (
    <div className="space-y-6">
      {/* Header */}
      <div className="flex items-center justify-between">
        <div className="flex items-center space-x-3">
          <Network className="h-6 w-6 text-blue-400" />
          <h2 className="text-2xl font-bold text-white">Network Monitor</h2>
        </div>
        <div className="flex items-center space-x-4">
          <div className="relative">
            <Search className="absolute left-3 top-1/2 transform -translate-y-1/2 h-4 w-4 text-slate-400" />
            <input
              type="text"
              placeholder="Search connections..."
              value={searchTerm}
              onChange={(e) => setSearchTerm(e.target.value)}
              className="pl-10 pr-4 py-2 bg-slate-800 border border-slate-700 rounded-lg text-white placeholder-slate-400 focus:ring-2 focus:ring-blue-500 focus:border-transparent"
            />
          </div>
          <select
            value={filterProtocol}
            onChange={(e) => setFilterProtocol(e.target.value as 'all' | 'TCP' | 'UDP')}
            className="px-3 py-2 bg-slate-800 border border-slate-700 rounded-lg text-white focus:ring-2 focus:ring-blue-500 focus:border-transparent"
          >
            <option value="all">All Protocols</option>
            <option value="TCP">TCP</option>
            <option value="UDP">UDP</option>
          </select>
          <select
            value={filterState}
            onChange={(e) => setFilterState(e.target.value as 'all' | 'ESTABLISHED' | 'LISTEN')}
            className="px-3 py-2 bg-slate-800 border border-slate-700 rounded-lg text-white focus:ring-2 focus:ring-blue-500 focus:border-transparent"
          >
            <option value="all">All States</option>
            <option value="ESTABLISHED">Established</option>
            <option value="LISTEN">Listening</option>
          </select>
          <button 
            onClick={() => window.location.reload()}
            className="p-2 bg-blue-600 hover:bg-blue-700 rounded-lg transition-colors"
          >
            <RefreshCw className="h-4 w-4 text-white" />
          </button>
        </div>
      </div>

      {/* Stats Overview */}
      <div className="grid grid-cols-1 md:grid-cols-4 gap-4">
        <div className="bg-slate-800/50 backdrop-blur-sm rounded-lg p-4 border border-slate-700">
          <div className="flex items-center space-x-3">
            <Network className="h-8 w-8 text-blue-400" />
            <div>
              <p className="text-sm text-slate-400">Total Connections</p>
              <p className="text-xl font-bold text-white">{stats.total}</p>
            </div>
          </div>
        </div>
        
        <div className="bg-slate-800/50 backdrop-blur-sm rounded-lg p-4 border border-slate-700">
          <div className="flex items-center space-x-3">
            <div className="w-8 h-8 bg-green-900/20 rounded-lg flex items-center justify-center">
              <div className="w-4 h-4 bg-green-400 rounded-full"></div>
            </div>
            <div>
              <p className="text-sm text-slate-400">Established</p>
              <p className="text-xl font-bold text-white">{stats.established}</p>
            </div>
          </div>
        </div>
        
        <div className="bg-slate-800/50 backdrop-blur-sm rounded-lg p-4 border border-slate-700">
          <div className="flex items-center space-x-3">
            <Server className="h-8 w-8 text-blue-400" />
            <div>
              <p className="text-sm text-slate-400">Listening</p>
              <p className="text-xl font-bold text-white">{stats.listening}</p>
            </div>
          </div>
        </div>
        
        <div className="bg-slate-800/50 backdrop-blur-sm rounded-lg p-4 border border-slate-700">
          <div className="flex items-center space-x-3">
            <Globe className="h-8 w-8 text-orange-400" />
            <div>
              <p className="text-sm text-slate-400">External</p>
              <p className="text-xl font-bold text-white">{stats.external}</p>
            </div>
          </div>
        </div>
      </div>

      {/* Connections Table */}
      <div className="bg-slate-800/50 backdrop-blur-sm rounded-xl border border-slate-700 overflow-hidden">
        <div className="overflow-x-auto">
          <table className="w-full">
            <thead className="bg-slate-700/50">
              <tr>
                <th className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider">
                  Type
                </th>
                <th className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider">
                  Protocol
                </th>
                <th className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider">
                  Local Address
                </th>
                <th className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider">
                  Remote Address
                </th>
                <th className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider">
                  State
                </th>
                <th className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider">
                  Risk Level
                </th>
                <th className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider">
                  Timestamp
                </th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-700">
              {filteredConnections.map((connection) => {
                const localPortRisk = getPortRiskLevel(connection.local_port);
                const remotePortRisk = getPortRiskLevel(connection.remote_port);
                const maxRisk = localPortRisk.level === 'high' || remotePortRisk.level === 'high' ? 'high' :
                               localPortRisk.level === 'medium' || remotePortRisk.level === 'medium' ? 'medium' : 'low';
                
                return (
                  <tr key={connection.id} className="hover:bg-slate-700/30 transition-colors">
                    <td className="px-6 py-4 whitespace-nowrap text-sm">
                      <div className="flex items-center space-x-2">
                        {getConnectionTypeIcon(connection)}
                        <span className="text-slate-300">{getConnectionTypeLabel(connection)}</span>
                      </div>
                    </td>
                    <td className="px-6 py-4 whitespace-nowrap text-sm text-white font-medium">
                      {connection.protocol}
                    </td>
                    <td className="px-6 py-4 whitespace-nowrap text-sm font-mono text-slate-300">
                      <div>
                        <div>{connection.local_ip}:{connection.local_port}</div>
                      </div>
                    </td>
                    <td className="px-6 py-4 whitespace-nowrap text-sm font-mono text-slate-300">
                      <div>
                        <div>{connection.remote_ip}:{connection.remote_port}</div>
                      </div>
                    </td>
                    <td className="px-6 py-4 whitespace-nowrap text-sm">
                      <span className={`inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-medium ${getStateColor(connection.state)}`}>
                        {connection.state}
                      </span>
                    </td>
                    <td className="px-6 py-4 whitespace-nowrap text-sm">
                      <span className={`inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-medium ${
                        maxRisk === 'high' ? 'text-red-400 bg-red-900/20' :
                        maxRisk === 'medium' ? 'text-yellow-400 bg-yellow-900/20' :
                        'text-green-400 bg-green-900/20'
                      }`}>
                        {maxRisk === 'high' ? 'High' : maxRisk === 'medium' ? 'Medium' : 'Low'}
                      </span>
                    </td>
                    <td className="px-6 py-4 whitespace-nowrap text-sm text-slate-400">
                      {new Date(connection.timestamp).toLocaleTimeString()}
                    </td>
                  </tr>
                );
              })}
            </tbody>
          </table>
        </div>
        
        {filteredConnections.length === 0 && (
          <div className="text-center py-12">
            <Network className="mx-auto h-12 w-12 text-slate-600" />
            <h3 className="mt-2 text-sm font-medium text-slate-400">No connections found</h3>
            <p className="mt-1 text-sm text-slate-500">
              {searchTerm ? 'Try adjusting your search criteria.' : 'No network data available.'}
            </p>
          </div>
        )}
      </div>
    </div>
  );
};

export default NetworkMonitor;