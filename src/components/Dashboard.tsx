import React, { useState, useEffect } from 'react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer, BarChart, Bar } from 'recharts';
import { Activity, HardDrive, Cpu, Network, AlertTriangle, TrendingUp } from 'lucide-react';

interface SystemStats {
  cpu_usage: number;
  memory_usage: number;
  disk_usage: number;
  load_average: number;
  timestamp: string;
}

interface DashboardSummary {
  systemStats: SystemStats;
  processCount: number;
  connectionCount: number;
  alertCount: number;
}

const Dashboard: React.FC = () => {
  const [summary, setSummary] = useState<DashboardSummary | null>(null);
  const [systemHistory, setSystemHistory] = useState<SystemStats[]>([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    const fetchData = async () => {
      try {
        // Fetch dashboard summary
        const summaryResponse = await fetch('http://localhost:3001/api/dashboard');
        if (summaryResponse.ok) {
          const summaryData = await summaryResponse.json();
          setSummary(summaryData);
        }

        // Fetch system statistics history
        const historyResponse = await fetch('http://localhost:3001/api/system-stats?limit=20');
        if (historyResponse.ok) {
          const historyData = await historyResponse.json();
          setSystemHistory(historyData);
        }

        setLoading(false);
      } catch (error) {
        console.error('Error fetching dashboard data:', error);
        setLoading(false);
      }
    };

    fetchData();
    const interval = setInterval(fetchData, 5000); // Update every 5 seconds

    return () => clearInterval(interval);
  }, []);

  if (loading) {
    return (
      <div className="flex items-center justify-center h-64">
        <div className="animate-spin rounded-full h-12 w-12 border-b-2 border-blue-500"></div>
      </div>
    );
  }

  const formatTime = (timestamp: string) => {
    return new Date(timestamp).toLocaleTimeString('en-US', { 
      hour12: false, 
      hour: '2-digit', 
      minute: '2-digit' 
    });
  };

  const chartData = systemHistory.map(stat => ({
    time: formatTime(stat.timestamp),
    cpu: stat.cpu_usage,
    memory: stat.memory_usage,
    load: stat.load_average
  }));

  return (
    <div className="space-y-6">
      {/* Header */}
      <div className="flex items-center justify-between">
        <h2 className="text-2xl font-bold text-white">System Overview</h2>
        <div className="text-sm text-slate-400">
          Last updated: {summary?.systemStats ? new Date().toLocaleTimeString() : 'Never'}
        </div>
      </div>

      {/* Key Metrics */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
        <div className="bg-slate-800/50 backdrop-blur-sm rounded-xl p-6 border border-slate-700">
          <div className="flex items-center justify-between">
            <div>
              <p className="text-sm font-medium text-slate-400">CPU Usage</p>
              <p className="text-2xl font-bold text-white">
                {summary?.systemStats?.cpu_usage ? `${summary.systemStats.cpu_usage.toFixed(1)}%` : '0%'}
              </p>
            </div>
            <div className="p-3 bg-blue-600/20 rounded-lg">
              <Cpu className="h-6 w-6 text-blue-400" />
            </div>
          </div>
          <div className="mt-4">
            <div className="w-full bg-slate-700 rounded-full h-2">
              <div 
                className="bg-blue-500 h-2 rounded-full transition-all duration-300"
                style={{ width: `${summary?.systemStats?.cpu_usage || 0}%` }}
              ></div>
            </div>
          </div>
        </div>

        <div className="bg-slate-800/50 backdrop-blur-sm rounded-xl p-6 border border-slate-700">
          <div className="flex items-center justify-between">
            <div>
              <p className="text-sm font-medium text-slate-400">Memory Usage</p>
              <p className="text-2xl font-bold text-white">
                {summary?.systemStats?.memory_usage ? `${summary.systemStats.memory_usage.toFixed(1)}%` : '0%'}
              </p>
            </div>
            <div className="p-3 bg-green-600/20 rounded-lg">
              <HardDrive className="h-6 w-6 text-green-400" />
            </div>
          </div>
          <div className="mt-4">
            <div className="w-full bg-slate-700 rounded-full h-2">
              <div 
                className="bg-green-500 h-2 rounded-full transition-all duration-300"
                style={{ width: `${summary?.systemStats?.memory_usage || 0}%` }}
              ></div>
            </div>
          </div>
        </div>

        <div className="bg-slate-800/50 backdrop-blur-sm rounded-xl p-6 border border-slate-700">
          <div className="flex items-center justify-between">
            <div>
              <p className="text-sm font-medium text-slate-400">Active Processes</p>
              <p className="text-2xl font-bold text-white">{summary?.processCount || 0}</p>
            </div>
            <div className="p-3 bg-purple-600/20 rounded-lg">
              <Activity className="h-6 w-6 text-purple-400" />
            </div>
          </div>
          <div className="mt-4 flex items-center text-sm text-slate-400">
            <TrendingUp className="h-4 w-4 mr-1" />
            <span>Monitoring active</span>
          </div>
        </div>

        <div className="bg-slate-800/50 backdrop-blur-sm rounded-xl p-6 border border-slate-700">
          <div className="flex items-center justify-between">
            <div>
              <p className="text-sm font-medium text-slate-400">Network Connections</p>
              <p className="text-2xl font-bold text-white">{summary?.connectionCount || 0}</p>
            </div>
            <div className="p-3 bg-orange-600/20 rounded-lg">
              <Network className="h-6 w-6 text-orange-400" />
            </div>
          </div>
          <div className="mt-4 flex items-center text-sm text-slate-400">
            <div className="w-2 h-2 bg-green-400 rounded-full mr-2"></div>
            <span>Active monitoring</span>
          </div>
        </div>
      </div>

      {/* Charts */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
        {/* System Performance Chart */}
        <div className="bg-slate-800/50 backdrop-blur-sm rounded-xl p-6 border border-slate-700">
          <h3 className="text-lg font-semibold text-white mb-4">System Performance</h3>
          <div className="h-64">
            <ResponsiveContainer width="100%" height="100%">
              <LineChart data={chartData}>
                <CartesianGrid strokeDasharray="3 3" stroke="#374151" />
                <XAxis 
                  dataKey="time" 
                  stroke="#9CA3AF" 
                  fontSize={12}
                  tickFormatter={(value) => value}
                />
                <YAxis stroke="#9CA3AF" fontSize={12} />
                <Tooltip 
                  contentStyle={{ 
                    backgroundColor: '#1F2937', 
                    border: '1px solid #374151',
                    borderRadius: '8px',
                    color: '#F9FAFB'
                  }}
                />
                <Line 
                  type="monotone" 
                  dataKey="cpu" 
                  stroke="#3B82F6" 
                  strokeWidth={2}
                  name="CPU %"
                  dot={false}
                />
                <Line 
                  type="monotone" 
                  dataKey="memory" 
                  stroke="#10B981" 
                  strokeWidth={2}
                  name="Memory %"
                  dot={false}
                />
              </LineChart>
            </ResponsiveContainer>
          </div>
        </div>

        {/* Load Average Chart */}
        <div className="bg-slate-800/50 backdrop-blur-sm rounded-xl p-6 border border-slate-700">
          <h3 className="text-lg font-semibold text-white mb-4">System Load Average</h3>
          <div className="h-64">
            <ResponsiveContainer width="100%" height="100%">
              <BarChart data={chartData}>
                <CartesianGrid strokeDasharray="3 3" stroke="#374151" />
                <XAxis 
                  dataKey="time" 
                  stroke="#9CA3AF" 
                  fontSize={12}
                />
                <YAxis stroke="#9CA3AF" fontSize={12} />
                <Tooltip 
                  contentStyle={{ 
                    backgroundColor: '#1F2937', 
                    border: '1px solid #374151',
                    borderRadius: '8px',
                    color: '#F9FAFB'
                  }}
                />
                <Bar 
                  dataKey="load" 
                  fill="#8B5CF6"
                  name="Load Average"
                  radius={[4, 4, 0, 0]}
                />
              </BarChart>
            </ResponsiveContainer>
          </div>
        </div>
      </div>

      {/* Alert Summary */}
      {summary?.alertCount !== undefined && summary.alertCount > 0 && (
        <div className="bg-red-900/20 border border-red-800 rounded-xl p-6">
          <div className="flex items-center space-x-3">
            <AlertTriangle className="h-6 w-6 text-red-400" />
            <div>
              <h3 className="text-lg font-semibold text-red-300">Active Alerts</h3>
              <p className="text-red-400">
                {summary.alertCount} alert{summary.alertCount !== 1 ? 's' : ''} in the last hour
              </p>
            </div>
          </div>
        </div>
      )}

      {/* System Status */}
      <div className="bg-slate-800/50 backdrop-blur-sm rounded-xl p-6 border border-slate-700">
        <h3 className="text-lg font-semibold text-white mb-4">System Status</h3>
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
          <div className="flex items-center space-x-3">
            <div className="w-3 h-3 bg-green-400 rounded-full"></div>
            <span className="text-slate-300">Process Monitor: Active</span>
          </div>
          <div className="flex items-center space-x-3">
            <div className="w-3 h-3 bg-green-400 rounded-full"></div>
            <span className="text-slate-300">Network Monitor: Active</span>
          </div>
          <div className="flex items-center space-x-3">
            <div className="w-3 h-3 bg-green-400 rounded-full"></div>
            <span className="text-slate-300">Anomaly Detection: Active</span>
          </div>
        </div>
      </div>
    </div>
  );
};

export default Dashboard;