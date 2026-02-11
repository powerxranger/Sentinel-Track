import React, { useState, useEffect } from 'react';
import { AlertTriangle, Search, RefreshCw, XCircle, CheckCircle, Clock } from 'lucide-react';

interface Alert {
  id: number;
  type: string;
  severity: string;
  message: string;
  details: string;
  timestamp: string;
}

const AlertsView: React.FC = () => {
  const [alerts, setAlerts] = useState<Alert[]>([]);
  const [loading, setLoading] = useState(true);
  const [searchTerm, setSearchTerm] = useState('');
  const [filterSeverity, setFilterSeverity] = useState<'all' | 'INFO' | 'WARNING' | 'ERROR' | 'CRITICAL'>('all');

  useEffect(() => {
    const fetchAlerts = async () => {
      try {
        const response = await fetch('http://localhost:3001/api/alerts?limit=200');
        if (response.ok) {
          const data = await response.json();
          setAlerts(data);
        }
        setLoading(false);
      } catch (error) {
        console.error('Error fetching alerts:', error);
        setLoading(false);
      }
    };

    fetchAlerts();
    const interval = setInterval(fetchAlerts, 10000); // Update every 10 seconds

    return () => clearInterval(interval);
  }, []);

  const filteredAlerts = React.useMemo(() => {
    return alerts.filter(alert => {
      const matchesSearch = 
        alert.message.toLowerCase().includes(searchTerm.toLowerCase()) ||
        alert.type.toLowerCase().includes(searchTerm.toLowerCase()) ||
        alert.details.toLowerCase().includes(searchTerm.toLowerCase());

      const matchesSeverity = filterSeverity === 'all' || alert.severity === filterSeverity;

      return matchesSearch && matchesSeverity;
    });
  }, [alerts, searchTerm, filterSeverity]);

  const getSeverityIcon = (severity: string) => {
    switch (severity) {
      case 'CRITICAL':
        return <XCircle className="h-5 w-5 text-red-500" />;
      case 'ERROR':
        return <XCircle className="h-5 w-5 text-red-400" />;
      case 'WARNING':
        return <AlertTriangle className="h-5 w-5 text-yellow-400" />;
      case 'INFO':
        return <CheckCircle className="h-5 w-5 text-blue-400" />;
      default:
        return <Clock className="h-5 w-5 text-slate-400" />;
    }
  };

  const getSeverityColor = (severity: string) => {
    switch (severity) {
      case 'CRITICAL':
        return 'bg-red-900/20 text-red-300 border border-red-800';
      case 'ERROR':
        return 'bg-red-900/20 text-red-400 border border-red-700';
      case 'WARNING':
        return 'bg-yellow-900/20 text-yellow-300 border border-yellow-700';
      case 'INFO':
        return 'bg-blue-900/20 text-blue-300 border border-blue-700';
      default:
        return 'bg-slate-900/20 text-slate-400 border border-slate-600';
    }
  };

  const getTypeColor = (type: string) => {
    switch (type) {
      case 'HIGH_CPU':
        return 'bg-orange-900/20 text-orange-400';
      case 'HIGH_MEMORY':
        return 'bg-red-900/20 text-red-400';
      case 'UNKNOWN_PROCESS':
        return 'bg-purple-900/20 text-purple-400';
      case 'SUSPICIOUS_PORT':
        return 'bg-yellow-900/20 text-yellow-400';
      case 'EXTERNAL_CONNECTION':
        return 'bg-blue-900/20 text-blue-400';
      case 'CPU_SPIKE':
        return 'bg-orange-900/20 text-orange-400';
      case 'MEMORY_SPIKE':
        return 'bg-red-900/20 text-red-400';
      case 'SYSTEM_OVERLOAD':
        return 'bg-red-900/20 text-red-300';
      default:
        return 'bg-slate-900/20 text-slate-400';
    }
  };

  const formatTimestamp = (timestamp: string) => {
    const date = new Date(timestamp);
    const now = new Date();
    const diffInMinutes = Math.floor((now.getTime() - date.getTime()) / (1000 * 60));
    
    if (diffInMinutes < 1) {
      return 'Just now';
    } else if (diffInMinutes < 60) {
      return `${diffInMinutes}m ago`;
    } else if (diffInMinutes < 1440) {
      return `${Math.floor(diffInMinutes / 60)}h ago`;
    } else {
      return date.toLocaleDateString();
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
    total: filteredAlerts.length,
    critical: filteredAlerts.filter(a => a.severity === 'CRITICAL').length,
    warning: filteredAlerts.filter(a => a.severity === 'WARNING').length,
    info: filteredAlerts.filter(a => a.severity === 'INFO').length,
  };

  return (
    <div className="space-y-6">
      {/* Header */}
      <div className="flex items-center justify-between">
        <div className="flex items-center space-x-3">
          <AlertTriangle className="h-6 w-6 text-red-400" />
          <h2 className="text-2xl font-bold text-white">Security Alerts</h2>
        </div>
        <div className="flex items-center space-x-4">
          <div className="relative">
            <Search className="absolute left-3 top-1/2 transform -translate-y-1/2 h-4 w-4 text-slate-400" />
            <input
              type="text"
              placeholder="Search alerts..."
              value={searchTerm}
              onChange={(e) => setSearchTerm(e.target.value)}
              className="pl-10 pr-4 py-2 bg-slate-800 border border-slate-700 rounded-lg text-white placeholder-slate-400 focus:ring-2 focus:ring-blue-500 focus:border-transparent"
            />
          </div>
          <select
            value={filterSeverity}
            onChange={(e) => setFilterSeverity(e.target.value as 'all' | 'INFO' | 'WARNING' | 'ERROR' | 'CRITICAL')}
            className="px-3 py-2 bg-slate-800 border border-slate-700 rounded-lg text-white focus:ring-2 focus:ring-blue-500 focus:border-transparent"
          >
            <option value="all">All Severities</option>
            <option value="CRITICAL">Critical</option>
            <option value="ERROR">Error</option>
            <option value="WARNING">Warning</option>
            <option value="INFO">Info</option>
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
            <AlertTriangle className="h-8 w-8 text-blue-400" />
            <div>
              <p className="text-sm text-slate-400">Total Alerts</p>
              <p className="text-xl font-bold text-white">{stats.total}</p>
            </div>
          </div>
        </div>
        
        <div className="bg-slate-800/50 backdrop-blur-sm rounded-lg p-4 border border-slate-700">
          <div className="flex items-center space-x-3">
            <XCircle className="h-8 w-8 text-red-400" />
            <div>
              <p className="text-sm text-slate-400">Critical</p>
              <p className="text-xl font-bold text-white">{stats.critical}</p>
            </div>
          </div>
        </div>
        
        <div className="bg-slate-800/50 backdrop-blur-sm rounded-lg p-4 border border-slate-700">
          <div className="flex items-center space-x-3">
            <AlertTriangle className="h-8 w-8 text-yellow-400" />
            <div>
              <p className="text-sm text-slate-400">Warnings</p>
              <p className="text-xl font-bold text-white">{stats.warning}</p>
            </div>
          </div>
        </div>
        
        <div className="bg-slate-800/50 backdrop-blur-sm rounded-lg p-4 border border-slate-700">
          <div className="flex items-center space-x-3">
            <CheckCircle className="h-8 w-8 text-blue-400" />
            <div>
              <p className="text-sm text-slate-400">Info</p>
              <p className="text-xl font-bold text-white">{stats.info}</p>
            </div>
          </div>
        </div>
      </div>

      {/* Alerts List */}
      <div className="space-y-4">
        {filteredAlerts.map((alert) => (
          <div
            key={alert.id}
            className={`rounded-xl p-6 transition-all hover:shadow-lg ${getSeverityColor(alert.severity)}`}
          >
            <div className="flex items-start space-x-4">
              <div className="flex-shrink-0 mt-1">
                {getSeverityIcon(alert.severity)}
              </div>
              
              <div className="flex-1 min-w-0">
                <div className="flex items-center justify-between mb-2">
                  <div className="flex items-center space-x-3">
                    <h3 className="text-lg font-semibold text-white">{alert.message}</h3>
                    <span className={`inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-medium ${getTypeColor(alert.type)}`}>
                      {alert.type.replace(/_/g, ' ')}
                    </span>
                  </div>
                  <div className="flex items-center space-x-2 text-sm text-slate-400">
                    <Clock className="h-4 w-4" />
                    <span>{formatTimestamp(alert.timestamp)}</span>
                  </div>
                </div>
                
                <p className="text-slate-300 mb-3">{alert.details}</p>
                
                <div className="flex items-center justify-between">
                  <span className={`inline-flex items-center px-3 py-1 rounded-full text-sm font-medium ${
                    alert.severity === 'CRITICAL' ? 'bg-red-600 text-white' :
                    alert.severity === 'ERROR' ? 'bg-red-500 text-white' :
                    alert.severity === 'WARNING' ? 'bg-yellow-500 text-black' :
                    'bg-blue-500 text-white'
                  }`}>
                    {alert.severity}
                  </span>
                  
                  <div className="text-xs text-slate-500">
                    Alert ID: {alert.id}
                  </div>
                </div>
              </div>
            </div>
          </div>
        ))}
        
        {filteredAlerts.length === 0 && (
          <div className="text-center py-12 bg-slate-800/50 backdrop-blur-sm rounded-xl border border-slate-700">
            <CheckCircle className="mx-auto h-12 w-12 text-green-400" />
            <h3 className="mt-2 text-lg font-medium text-white">No alerts found</h3>
            <p className="mt-1 text-sm text-slate-400">
              {searchTerm || filterSeverity !== 'all' 
                ? 'Try adjusting your search criteria.' 
                : 'Your system is running smoothly with no alerts.'}
            </p>
          </div>
        )}
      </div>
    </div>
  );
};

export default AlertsView;