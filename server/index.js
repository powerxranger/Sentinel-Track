import express from 'express';
import cors from 'cors';
import sqlite3 from 'sqlite3';
import { fileURLToPath } from 'url';
import { dirname, join } from 'path';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

const app = express();
const PORT = 3001;

// Middleware
app.use(cors());
app.use(express.json());

// Initialize SQLite database
const dbPath = join(__dirname, '../data/sentineltrack.db');
const db = new sqlite3.Database(dbPath);

// Initialize database tables
db.serialize(() => {
  // Processes table
  db.run(`CREATE TABLE IF NOT EXISTS processes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    pid INTEGER,
    name TEXT,
    cpu_usage REAL,
    memory_usage INTEGER,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
  )`);

  // Network connections table
  db.run(`CREATE TABLE IF NOT EXISTS network_connections (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    local_ip TEXT,
    local_port INTEGER,
    remote_ip TEXT,
    remote_port INTEGER,
    protocol TEXT,
    state TEXT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
  )`);

  // Alerts table
  db.run(`CREATE TABLE IF NOT EXISTS alerts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    type TEXT,
    severity TEXT,
    message TEXT,
    details TEXT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
  )`);

  // System stats table
  db.run(`CREATE TABLE IF NOT EXISTS system_stats (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    cpu_usage REAL,
    memory_usage REAL,
    disk_usage REAL,
    load_average REAL,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
  )`);
});

// Periodic cleanup to prevent unbounded DB growth
setInterval(() => {
  db.run(`DELETE FROM processes WHERE timestamp < datetime('now', '-1 hour')`);
  db.run(`DELETE FROM network_connections WHERE timestamp < datetime('now', '-1 hour')`);
  db.run(`DELETE FROM system_stats WHERE timestamp < datetime('now', '-24 hours')`);
  db.run(`DELETE FROM alerts WHERE timestamp < datetime('now', '-24 hours')`);
}, 5 * 60 * 1000);

// API Routes

// Get recent processes
app.get('/api/processes', (req, res) => {
  const limit = req.query.limit || 100;
  db.all(
    `SELECT p.* FROM processes p
    INNER JOIN (
      SELECT pid, MAX(timestamp) as max_ts FROM processes
      WHERE timestamp > datetime('now', '-60 seconds')
      GROUP BY pid
    ) latest ON p.pid = latest.pid AND p.timestamp = latest.max_ts
    ORDER BY p.cpu_usage DESC
    LIMIT ?`,
    [limit],
    (err, rows) => {
      if (err) {
        res.status(500).json({ error: err.message });
        return;
      }
      res.json(rows);
    }
  );
});

// Get network connections
app.get('/api/network', (req, res) => {
  const limit = req.query.limit || 100;
  db.all(
    `SELECT * FROM network_connections WHERE timestamp > datetime('now', '-60 seconds') ORDER BY timestamp DESC LIMIT ?`,
    [limit],
    (err, rows) => {
      if (err) {
        res.status(500).json({ error: err.message });
        return;
      }
      res.json(rows);
    }
  );
});

// Get alerts
app.get('/api/alerts', (req, res) => {
  const limit = req.query.limit || 50;
  db.all(
    `SELECT * FROM alerts ORDER BY timestamp DESC LIMIT ?`,
    [limit],
    (err, rows) => {
      if (err) {
        res.status(500).json({ error: err.message });
        return;
      }
      res.json(rows);
    }
  );
});

// Get system statistics
app.get('/api/system-stats', (req, res) => {
  const limit = req.query.limit || 24;
  db.all(
    `SELECT * FROM system_stats ORDER BY timestamp DESC LIMIT ?`,
    [limit],
    (err, rows) => {
      if (err) {
        res.status(500).json({ error: err.message });
        return;
      }
      res.json(rows.reverse()); // Return in chronological order for charts
    }
  );
});

// Get dashboard summary
app.get('/api/dashboard', (req, res) => {
  const summary = {};
  
  // Get latest system stats
  db.get(
    `SELECT * FROM system_stats ORDER BY timestamp DESC LIMIT 1`,
    (err, row) => {
      if (err) {
        res.status(500).json({ error: err.message });
        return;
      }
      summary.systemStats = row;
      
      // Get process count
      db.get(
        `SELECT COUNT(DISTINCT pid) as process_count FROM processes WHERE timestamp > datetime('now', '-60 seconds')`,
        (err, row) => {
          if (err) {
            res.status(500).json({ error: err.message });
            return;
          }
          summary.processCount = row.process_count;
          
          // Get network connection count
          db.get(
            `SELECT COUNT(DISTINCT local_ip || ':' || local_port || '-' || remote_ip || ':' || remote_port) as connection_count FROM network_connections WHERE timestamp > datetime('now', '-60 seconds')`,
            (err, row) => {
              if (err) {
                res.status(500).json({ error: err.message });
                return;
              }
              summary.connectionCount = row.connection_count;
              
              // Get alert count
              db.get(
                `SELECT COUNT(*) as alert_count FROM alerts WHERE timestamp > datetime('now', '-24 hours')`,
                (err, row) => {
                  if (err) {
                    res.status(500).json({ error: err.message });
                    return;
                  }
                  summary.alertCount = row.alert_count;
                  res.json(summary);
                }
              );
            }
          );
        }
      );
    }
  );
});

// Start server
app.listen(PORT, () => {
  console.log(`SentinelTrack API Server running on port ${PORT}`);
});