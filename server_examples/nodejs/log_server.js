/**
 * ESP32 Remote Logging Server (Node.js)
 *
 * Simple HTTP server that receives and stores logs from ESP32 devices.
 * Logs are saved to JSON files organized by device name and date.
 *
 * Installation:
 *   npm install express
 *
 * Usage:
 *   node log_server.js [port]
 *   Default port: 3000
 *
 * Example:
 *   node log_server.js 3000
 */

const express = require('express');
const fs = require('fs');
const path = require('path');

const PORT = process.argv[2] || 3000;
const LOG_DIR = path.join(__dirname, 'device_logs');

// Create log directory if it doesn't exist
if (!fs.existsSync(LOG_DIR)) {
    fs.mkdirSync(LOG_DIR, { recursive: true });
}

const app = express();
app.use(express.json({ limit: '10mb' }));

// Health check endpoint
app.get('/health', (req, res) => {
    res.json({ status: 'ok', timestamp: new Date().toISOString() });
});

// Log ingestion endpoint
app.post('/api/logs', (req, res) => {
    try {
        const { device, dropped, logs } = req.body;

        if (!device || !Array.isArray(logs)) {
            return res.status(400).json({ error: 'Invalid payload format' });
        }

        // Generate filename: device_YYYYMMDD.json
        const date = new Date().toISOString().split('T')[0].replace(/-/g, '');
        const filename = `${device}_${date}.json`;
        const filepath = path.join(LOG_DIR, filename);

        // Prepare log entry
        const logEntry = {
            received_at: new Date().toISOString(),
            device: device,
            dropped_messages: dropped || 0,
            log_count: logs.length,
            logs: logs
        };

        // Append to file (create if doesn't exist)
        let existingLogs = [];
        if (fs.existsSync(filepath)) {
            const fileContent = fs.readFileSync(filepath, 'utf8');
            try {
                existingLogs = JSON.parse(fileContent);
                if (!Array.isArray(existingLogs)) {
                    existingLogs = [existingLogs];
                }
            } catch (e) {
                console.error(`Error parsing existing log file ${filename}:`, e.message);
                existingLogs = [];
            }
        }

        existingLogs.push(logEntry);

        // Write to file with pretty formatting
        fs.writeFileSync(filepath, JSON.stringify(existingLogs, null, 2));

        // Console output
        console.log(`[${new Date().toISOString()}] Received ${logs.length} logs from ${device} (dropped: ${dropped || 0})`);

        // Optional: Print log messages to console
        if (process.env.VERBOSE === '1') {
            logs.forEach(log => {
                console.log(`  [${log.timestamp}] ${log.level.padEnd(7)} ${log.tag.padEnd(15)} ${log.message}`);
            });
        }

        res.json({
            success: true,
            received: logs.length,
            saved_to: filename
        });

    } catch (error) {
        console.error('Error processing logs:', error);
        res.status(500).json({ error: 'Internal server error' });
    }
});

// List all log files
app.get('/api/logs', (req, res) => {
    try {
        const files = fs.readdirSync(LOG_DIR)
            .filter(f => f.endsWith('.json'))
            .map(f => {
                const stats = fs.statSync(path.join(LOG_DIR, f));
                return {
                    filename: f,
                    size: stats.size,
                    modified: stats.mtime
                };
            })
            .sort((a, b) => b.modified - a.modified);

        res.json({ files });
    } catch (error) {
        console.error('Error listing logs:', error);
        res.status(500).json({ error: 'Internal server error' });
    }
});

// Get specific log file
app.get('/api/logs/:filename', (req, res) => {
    try {
        const filepath = path.join(LOG_DIR, req.params.filename);

        if (!fs.existsSync(filepath)) {
            return res.status(404).json({ error: 'Log file not found' });
        }

        const content = fs.readFileSync(filepath, 'utf8');
        res.json(JSON.parse(content));
    } catch (error) {
        console.error('Error reading log file:', error);
        res.status(500).json({ error: 'Internal server error' });
    }
});

app.listen(PORT, '0.0.0.0', () => {
    console.log('==========================================');
    console.log('ESP32 Remote Logging Server');
    console.log('==========================================');
    console.log(`Server listening on port ${PORT}`);
    console.log(`Log directory: ${LOG_DIR}`);
    console.log('');
    console.log('Endpoints:');
    console.log(`  POST http://localhost:${PORT}/api/logs - Receive logs from ESP32`);
    console.log(`  GET  http://localhost:${PORT}/api/logs - List all log files`);
    console.log(`  GET  http://localhost:${PORT}/health   - Health check`);
    console.log('');
    console.log('Set VERBOSE=1 to print all log messages to console');
    console.log('==========================================');
});
