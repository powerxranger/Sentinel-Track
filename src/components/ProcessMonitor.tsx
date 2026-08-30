import React, { useState, useEffect } from 'react';
import { Activity, Search, RefreshCw, AlertCircle } from 'lucide-react';

interface ProcessInfo {
  id: number;
  pid: number;
  name: string;
  cpu_usage: number;
  memory_usage: number;
  timestamp: string;
}

const ProcessMonitor: React.FC = () => {
  const [processes, setProcesses] = useState<ProcessInfo[]>([]);
  const [loading, setLoading] = useState(true);
  const [searchTerm, setSearchTerm] = useState('');
  const [sortBy, setSortBy] = useState<'cpu_usage' | 'memory_usage' | 'name'>('cpu_usage');
  const [sortOrder, setSortOrder] = useState<'asc' | 'desc'>('desc');

  useEffect(() => {
    const fetchProcesses = async () => {
      try {
        const response = await fetch('http://localhost:3001/api/processes?limit=1000');
        if (response.ok) {
          const data = await response.json();
          setProcesses(data);
        }
        setLoading(false);
      } catch (error) {
        console.error('Error fetching processes:', error);
        setLoading(false);
      }
    };

    fetchProcesses();
    const interval = setInterval(fetchProcesses, 3000); // Update every 3 seconds

    return () => clearInterval(interval);
  }, []);

  const filteredAndSortedProcesses = React.useMemo(() => {
    let filtered = processes.filter(process =>
      process.name.toLowerCase().includes(searchTerm.toLowerCase()) ||
      process.pid.toString().includes(searchTerm)
    );

    // Group by PID and get the latest entry for each
    const latestProcesses = new Map<number, ProcessInfo>();
    filtered.forEach(process => {
      const existing = latestProcesses.get(process.pid);
      if (!existing || new Date(process.timestamp) > new Date(existing.timestamp)) {
        latestProcesses.set(process.pid, process);
      }
    });

    filtered = Array.from(latestProcesses.values());

    return filtered.sort((a, b) => {
      let aValue = a[sortBy];
      let bValue = b[sortBy];
      
      if (typeof aValue === 'string') {
        aValue = aValue.toLowerCase();
        bValue = (bValue as string).toLowerCase();
      }
      
      if (sortOrder === 'asc') {
        return aValue < bValue ? -1 : aValue > bValue ? 1 : 0;
      } else {
        return aValue > bValue ? -1 : aValue < bValue ? 1 : 0;
      }
    });
  }, [processes, searchTerm, sortBy, sortOrder]);

  const handleSort = (column: 'cpu_usage' | 'memory_usage' | 'name') => {
    if (sortBy === column) {
      setSortOrder(sortOrder === 'asc' ? 'desc' : 'asc');
    } else {
      setSortBy(column);
      setSortOrder('desc');
    }
  };

  const formatMemory = (kb: number) => {
    if (kb > 1024 * 1024) {
      return `${(kb / (1024 * 1024)).toFixed(1)} GB`;
    } else if (kb > 1024) {
      return `${(kb / 1024).toFixed(1)} MB`;
    } else {
      return `${kb} KB`;
    }
  };

  const getProcessStatusColor = (cpu: number, memory: number) => {
    if (cpu > 80 || memory > 2 * 1024 * 1024) { // > 80% CPU or > 2 GB RAM
      return 'text-red-400';
    } else if (cpu > 20 || memory > 1024 * 1024) { // > 20% CPU or > 1 GB RAM
      return 'text-yellow-400';
    } else {
      return 'text-green-400';
    }
  };

  if (loading) {
    return (
      <div className="flex items-center justify-center h-64">
        <div className="animate-spin rounded-full h-12 w-12 border-b-2 border-blue-500"></div>
      </div>
    );
  }

  return (
    <div className="space-y-6">
      {/* Header */}
      <div className="flex items-center justify-between">
        <div className="flex items-center space-x-3">
          <Activity className="h-6 w-6 text-blue-400" />
          <h2 className="text-2xl font-bold text-white">Process Monitor</h2>
        </div>
        <div className="flex items-center space-x-4">
          <div className="relative">
            <Search className="absolute left-3 top-1/2 transform -translate-y-1/2 h-4 w-4 text-slate-400" />
            <input
              type="text"
              placeholder="Search processes..."
              value={searchTerm}
              onChange={(e) => setSearchTerm(e.target.value)}
              className="pl-10 pr-4 py-2 bg-slate-800 border border-slate-700 rounded-lg text-white placeholder-slate-400 focus:ring-2 focus:ring-blue-500 focus:border-transparent"
            />
          </div>
          <button 
            onClick={() => window.location.reload()}
            className="p-2 bg-blue-600 hover:bg-blue-700 rounded-lg transition-colors"
          >
            <RefreshCw className="h-4 w-4 text-white" />
          </button>
        </div>
      </div>

      {/* Stats Overview */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
        <div className="bg-slate-800/50 backdrop-blur-sm rounded-lg p-4 border border-slate-700">
          <div className="flex items-center space-x-3">
            <Activity className="h-8 w-8 text-blue-400" />
            <div>
              <p className="text-sm text-slate-400">Total Processes</p>
              <p className="text-xl font-bold text-white">{filteredAndSortedProcesses.length}</p>
            </div>
          </div>
        </div>
        
        <div className="bg-slate-800/50 backdrop-blur-sm rounded-lg p-4 border border-slate-700">
          <div className="flex items-center space-x-3">
            <AlertCircle className="h-8 w-8 text-yellow-400" />
            <div>
              <p className="text-sm text-slate-400">High CPU Usage</p>
              <p className="text-xl font-bold text-white">
                {filteredAndSortedProcesses.filter(p => p.cpu_usage > 20).length}
              </p>
              <span className="inline-block mt-1 px-2 py-0.5 rounded-full text-xs font-medium bg-yellow-900/30 text-yellow-400 border border-yellow-700/40">
                threshold: &gt;20% CPU
              </span>
            </div>
          </div>
        </div>

        <div className="bg-slate-800/50 backdrop-blur-sm rounded-lg p-4 border border-slate-700">
          <div className="flex items-center space-x-3">
            <AlertCircle className="h-8 w-8 text-red-400" />
            <div>
              <p className="text-sm text-slate-400">High Memory Usage</p>
              <p className="text-xl font-bold text-white">
                {filteredAndSortedProcesses.filter(p => p.memory_usage > 1024 * 1024).length}
              </p>
              <span className="inline-block mt-1 px-2 py-0.5 rounded-full text-xs font-medium bg-red-900/30 text-red-400 border border-red-700/40">
                threshold: &gt;1 GB RAM
              </span>
            </div>
          </div>
        </div>
      </div>

      {/* Process Table */}
      <div className="bg-slate-800/50 backdrop-blur-sm rounded-xl border border-slate-700 overflow-hidden">
        <div className="overflow-x-auto">
          <table className="w-full">
            <thead className="bg-slate-700/50">
              <tr>
                <th className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider">
                  PID
                </th>
                <th 
                  className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider cursor-pointer hover:text-white transition-colors"
                  onClick={() => handleSort('name')}
                >
                  Process Name
                  {sortBy === 'name' && (
                    <span className="ml-1">{sortOrder === 'asc' ? '↑' : '↓'}</span>
                  )}
                </th>
                <th 
                  className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider cursor-pointer hover:text-white transition-colors"
                  onClick={() => handleSort('cpu_usage')}
                >
                  CPU Usage
                  {sortBy === 'cpu_usage' && (
                    <span className="ml-1">{sortOrder === 'asc' ? '↑' : '↓'}</span>
                  )}
                </th>
                <th 
                  className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider cursor-pointer hover:text-white transition-colors"
                  onClick={() => handleSort('memory_usage')}
                >
                  Memory Usage
                  {sortBy === 'memory_usage' && (
                    <span className="ml-1">{sortOrder === 'asc' ? '↑' : '↓'}</span>
                  )}
                </th>
                <th className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider">
                  Status
                </th>
                <th className="px-6 py-3 text-left text-xs font-medium text-slate-300 uppercase tracking-wider">
                  Last Seen
                </th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-700">
              {filteredAndSortedProcesses.map((process) => (
                <tr key={process.id} className="hover:bg-slate-700/30 transition-colors">
                  <td className="px-6 py-4 whitespace-nowrap text-sm font-mono text-slate-300">
                    {process.pid}
                  </td>
                  <td className="px-6 py-4 whitespace-nowrap text-sm text-white font-medium">
                    {process.name}
                  </td>
                  <td className="px-6 py-4 whitespace-nowrap text-sm text-slate-300">
                    <div className="flex items-center space-x-2">
                      <span>{process.cpu_usage.toFixed(1)}%</span>
                      <div className="w-16 bg-slate-700 rounded-full h-2">
                        <div 
                          className={`h-2 rounded-full transition-all ${
                            process.cpu_usage > 80 ? 'bg-red-500' : 
                            process.cpu_usage > 50 ? 'bg-yellow-500' : 'bg-green-500'
                          }`}
                          style={{ width: `${Math.min(process.cpu_usage, 100)}%` }}
                        ></div>
                      </div>
                    </div>
                  </td>
                  <td className="px-6 py-4 whitespace-nowrap text-sm text-slate-300">
                    {formatMemory(process.memory_usage)}
                  </td>
                  <td className="px-6 py-4 whitespace-nowrap text-sm">
                    <span className={`inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-medium ${getProcessStatusColor(process.cpu_usage, process.memory_usage)}`}>
                      {process.cpu_usage > 80 || process.memory_usage > 2 * 1024 * 1024
                        ? 'High'
                        : process.cpu_usage > 20 || process.memory_usage > 1024 * 1024
                        ? 'Moderate'
                        : 'Normal'}
                    </span>
                  </td>
                  <td className="px-6 py-4 whitespace-nowrap text-sm text-slate-400">
                    {new Date(process.timestamp + 'Z').toLocaleTimeString()}
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
        
        {filteredAndSortedProcesses.length === 0 && (
          <div className="text-center py-12">
            <Activity className="mx-auto h-12 w-12 text-slate-600" />
            <h3 className="mt-2 text-sm font-medium text-slate-400">No processes found</h3>
            <p className="mt-1 text-sm text-slate-500">
              {searchTerm ? 'Try adjusting your search criteria.' : 'No process data available.'}
            </p>
          </div>
        )}
      </div>
    </div>
  );
};

export default ProcessMonitor;