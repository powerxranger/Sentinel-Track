# SentinelTrack - Cross-Platform System Monitoring Tool

A comprehensive system monitoring solution that works on **Linux**, **macOS (including M1)**, and **Windows**. Consists of a C++ agent for real-time system monitoring and a React-based web dashboard for visualization and analysis.

## Supported Platforms

- ✅ **Linux** (Ubuntu, Debian, CentOS, RHEL, etc.)
- ✅ **macOS** (Intel and Apple Silicon M1/M2)
- ✅ **Windows** (Windows 10/11 with MinGW or Visual Studio)

## Architecture Overview

### Components

1. **C++ Agent** (`/agent/`)
   - Cross-platform system monitoring agent
   - Process monitoring with CPU/memory tracking
   - Network connection monitoring
   - Anomaly detection engine
   - SQLite database logging
   - JSON file logging

2. **Node.js API Server** (`/server/`)
   - Express.js REST API
   - SQLite database interface
   - WebSocket support for real-time updates
   

3. **React Dashboard** (`/src/`)
   - Modern web-based interface
   - Real-time system statistics
   - Process monitoring with search/filter
   - Network connection analysis
   - Security alerts management
   - Responsive design with dark theme
   

## Installation & Setup

### Prerequisites

#### macOS (M1/Intel)
```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install sqlite3 node npm
# For building from source, also install:
brew install gcc make
```

#### Windows
```bash
# Install using Chocolatey (recommended)
choco install sqlite nodejs npm mingw

# Or install manually:
# 1. Download and install Node.js from https://nodejs.org/
# 2. Download SQLite from https://www.sqlite.org/download.html
# 3. Install MinGW-w64 or Visual Studio Build Tools
```

#### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install build-essential libsqlite3-dev sqlite3 nodejs npm
```

#### Linux (CentOS/RHEL)
```bash
sudo yum install gcc-c++ sqlite-devel sqlite nodejs npm
# or for newer versions:
sudo dnf install gcc-c++ sqlite-devel sqlite nodejs npm
```

### Build and Run

#### 1. Install Node.js Dependencies
```bash
# Install frontend dependencies
npm install

# Install server dependencies
cd server && npm install && cd ..
```

#### 2. Build the C++ Agent
```bash
cd agent

# The Makefile automatically detects your platform
make

# On Windows with Visual Studio, you can also use:
# cl /EHsc /I include src/*.cpp /Fe:sentineltrack.exe sqlite3.lib psapi.lib iphlpapi.lib ws2_32.lib

cd ..
```

#### 3. Create Data Directory
```bash
mkdir -p data
```

## Running SentinelTrack

### Method 1: Run All Components Separately

#### Terminal 1 - Start the C++ Agent
```bash
cd agent

# macOS/Linux
./sentineltrack

# Windows
sentineltrack.exe

# Or with elevated privileges (recommended for full system access)
# macOS/Linux: sudo ./sentineltrack
# Windows: Run as Administrator
```

#### Terminal 2 - Start the API Server
```bash
npm run server
```

#### Terminal 3 - Start the Dashboard
```bash
npm run dev
```

### Method 2: Run API + Dashboard Together
```bash
# Start both API server and dashboard
npm run dev-full

# Then in another terminal, start the agent
cd agent && ./sentineltrack
```

### Access the Dashboard
Open your browser and navigate to: **http://localhost:5173**

## Platform-Specific Features

### macOS (M1/Intel)
- Native Apple Silicon (ARM64) support
- Uses `libproc` and `sysctl` for system information
- Mach kernel APIs for CPU/memory statistics
- Network monitoring via BSD socket APIs

### Windows
- Windows API integration (`psapi.lib`, `iphlpapi.lib`)
- Process monitoring via `CreateToolhelp32Snapshot`
- Network monitoring via `GetExtendedTcpTable`/`GetExtendedUdpTable`
- Windows service compatibility

### Linux
- `/proc` filesystem monitoring
- Raw socket network monitoring
- Signal handling for graceful shutdown
- Systemd service compatibility

## Testing the System

### Generate Test Activity

#### High CPU Usage
```bash
# macOS/Linux
stress --cpu 2 --timeout 60s
# or
yes > /dev/null &  # Kill with: killall yes

# Windows (PowerShell)
while($true) { $result = 1; for ($i = 0; $i -lt 2000000; $i++) { $result += $i } }
```

#### High Memory Usage
```bash
# macOS/Linux
stress --vm 1 --vm-bytes 1G --timeout 60s

# Windows (PowerShell)
$array = @(); for($i=0; $i -lt 1000000; $i++) { $array += "A" * 1000 }
```

#### Network Activity
```bash
# macOS/Linux
python3 -m http.server 8888
nc -l 4444

# Windows
python -m http.server 8888
```

## Configuration

### Anomaly Detection Thresholds
Edit `agent/src/AnomalyDetector.cpp` to modify:
- CPU usage thresholds (default: 80%)
- Memory usage limits (default: 1GB)
- Process creation rate limits
- Network connection rate limits

### Database Location
By default, data is stored in:
- Database: `./data/sentineltrack.db`
- JSON logs: `./data/sentineltrack.log`

## Troubleshooting

### Common Issues

#### macOS M1 Specific
```bash
# If you get architecture errors
export ARCHFLAGS="-arch arm64"
make clean && make

# For Homebrew path issues
export PATH="/opt/homebrew/bin:$PATH"
```

#### Windows Specific
```bash
# If SQLite is not found
# Download sqlite3.dll and sqlite3.lib from https://www.sqlite.org/download.html
# Place them in your system PATH or project directory

# For MinGW compilation issues
# Make sure MinGW-w64 is in your PATH
set PATH=C:\mingw64\bin;%PATH%
```

#### Permission Issues
```bash
# macOS/Linux - Run with elevated privileges
sudo ./sentineltrack

# Windows - Run Command Prompt as Administrator
# Right-click Command Prompt -> "Run as administrator"
```

#### Port Already in Use
```bash
# Check what's using port 3001
# macOS/Linux
lsof -i :3001
# Windows
netstat -ano | findstr :3001

# Kill the process or change the port in server/index.js
```

## Estimated Performance (Local Testing)


| Platform    | CPU Usage | Memory Usage | Build Time |
|-------------|-----------|--------------|------------|
| macOS M1    | ~0.5%     | ~8MB         | ~15s       |
| macOS Intel | ~1.0%     | ~10MB        | ~20s       |
| Windows 11  | ~1.5%     | ~12MB        | ~25s       |
| Linux x64   | ~0.8%     | ~9MB         | ~18s       |

## Development

### Building for Different Architectures

#### macOS Universal Binary
```bash
# Build for both Intel and Apple Silicon
make clean
CXXFLAGS="-arch x86_64 -arch arm64" make
```

#### Windows Cross-Compilation from Linux
```bash
# Install MinGW cross-compiler
sudo apt install mingw-w64

# Build for Windows
CXX=x86_64-w64-mingw32-g++ make
```

## Production Deployment

### macOS Service (launchd)
```bash
# Create service file
sudo cp scripts/com.sentineltrack.agent.plist /Library/LaunchDaemons/
sudo launchctl load /Library/LaunchDaemons/com.sentineltrack.agent.plist
```

### Windows Service
```bash
# Use NSSM (Non-Sucking Service Manager)
nssm install SentinelTrack C:\path\to\sentineltrack.exe
nssm start SentinelTrack
```

### Linux Systemd Service
```bash
# Create service file
sudo cp scripts/sentineltrack.service /etc/systemd/system/
sudo systemctl enable sentineltrack
sudo systemctl start sentineltrack
```

## API Endpoints

- `GET /api/dashboard` - System overview summary
- `GET /api/processes` - Process list with optional limit
- `GET /api/network` - Network connections with optional limit
- `GET /api/alerts` - Security alerts with optional limit
- `GET /api/system-stats` - Historical system statistics

## Security Considerations

- **Elevated Privileges**: The agent requires elevated privileges for full system monitoring
- **Network Access**: Dashboard runs on localhost by default
- **Data Storage**: All data is stored locally in SQLite database
- **Process Monitoring**: - Basic anomaly detection for high resource usage
- **Network Monitoring**: Tracks all network connections and suspicious ports

## Contributing

1. Fork the repository
2. Create a feature branch
3. Test on multiple platforms
4. Submit a pull request

## License

This project is provided as-is for educational and development purposes.

---

**Note**: This cross-platform version maintains full compatibility with the original Linux-only version while adding native support for macOS (including M1) and Windows platforms.