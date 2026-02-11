import React, { useState, useEffect } from 'react';
import Dashboard from './components/Dashboard';
import ProcessMonitor from './components/ProcessMonitor';
import NetworkMonitor from './components/NetworkMonitor';
import AlertsView from './components/AlertsView';
import { Activity, Network, AlertTriangle, BarChart3 } from 'lucide-react';

function App() {
  const [activeTab, setActiveTab] = useState('dashboard');
  const [isConnected, setIsConnected] = useState(false);

  useEffect(() => {
    // Check if API server is running
    const checkConnection = async () => {
      try {
        const response = await fetch('http://localhost:3001/api/dashboard');
        setIsConnected(response.ok);
      } catch (error) {
        setIsConnected(false);
      }
    };

    checkConnection();
    const interval = setInterval(checkConnection, 10000); // Check every 10 seconds

    return () => clearInterval(interval);
  }, []);

  const tabs = [
    { id: 'dashboard', name: 'Dashboard', icon: BarChart3 },
    { id: 'processes', name: 'Processes', icon: Activity },
    { id: 'network', name: 'Network', icon: Network },
    { id: 'alerts', name: 'Alerts', icon: AlertTriangle },
  ];

  const renderContent = () => {
    switch (activeTab) {
      case 'dashboard':
        return <Dashboard />;
      case 'processes':
        return <ProcessMonitor />;
      case 'network':
        return <NetworkMonitor />;
      case 'alerts':
        return <AlertsView />;
      default:
        return <Dashboard />;
    }
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-slate-900 via-slate-800 to-slate-900">
      {/* Header */}
      <header className="bg-slate-800/80 backdrop-blur-sm border-b border-slate-700 sticky top-0 z-50">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="flex items-center justify-between h-16">
            <div className="flex items-center space-x-3">
              <div className="p-2 bg-blue-600 rounded-lg">
                <Activity className="h-6 w-6 text-white" />
              </div>
              <div>
                <h1 className="text-xl font-bold text-white">SentinelTrack</h1>
                <p className="text-sm text-slate-400">System Monitoring Dashboard</p>
              </div>
            </div>
            
            <div className="flex items-center space-x-4">
              <div className="flex items-center space-x-2">
                <div className={`w-2 h-2 rounded-full ${isConnected ? 'bg-green-400' : 'bg-red-400'}`}></div>
                <span className="text-sm text-slate-300">
                  {isConnected ? 'Connected' : 'Disconnected'}
                </span>
              </div>
            </div>
          </div>
        </div>
      </header>

      {/* Navigation */}
      <nav className="bg-slate-800/60 backdrop-blur-sm border-b border-slate-700">
        <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
          <div className="flex space-x-8">
            {tabs.map((tab) => {
              const Icon = tab.icon;
              return (
                <button
                  key={tab.id}
                  onClick={() => setActiveTab(tab.id)}
                  className={`flex items-center space-x-2 px-3 py-4 text-sm font-medium border-b-2 transition-colors ${
                    activeTab === tab.id
                      ? 'border-blue-500 text-blue-400'
                      : 'border-transparent text-slate-400 hover:text-slate-300 hover:border-slate-300'
                  }`}
                >
                  <Icon className="h-4 w-4" />
                  <span>{tab.name}</span>
                </button>
              );
            })}
          </div>
        </div>
      </nav>

      {/* Main Content */}
      <main className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
        {!isConnected && (
          <div className="mb-6 bg-red-900/20 border border-red-800 rounded-lg p-4">
            <div className="flex items-center space-x-3">
              <AlertTriangle className="h-5 w-5 text-red-400" />
              <div>
                <h3 className="text-sm font-medium text-red-300">API Server Disconnected</h3>
                <p className="text-xs text-red-400 mt-1">
                  Make sure the Node.js server is running on port 3001
                </p>
              </div>
            </div>
          </div>
        )}
        
        {renderContent()}
      </main>
    </div>
  );
}

export default App;