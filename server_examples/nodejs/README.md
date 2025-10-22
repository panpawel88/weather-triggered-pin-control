# ESP32 Remote Logging Server (Node.js)

Simple HTTP server that receives and stores logs from ESP32 devices.

## Installation

1. Install Node.js (if not already installed): https://nodejs.org/
2. Install dependencies:
   ```bash
   cd server_examples/nodejs
   npm install
   ```

## Usage

### Start the server:
```bash
npm start
```

Or specify a custom port:
```bash
node log_server.js 8080
```

### Verbose mode (prints all logs to console):
```bash
VERBOSE=1 npm start
```

## How It Works

- **POST /api/logs** - Receives log data from ESP32 devices
- **GET /api/logs** - Lists all stored log files
- **GET /api/logs/:filename** - Retrieves a specific log file
- **GET /health** - Health check endpoint

Logs are saved to `device_logs/` directory with format:
- `{device_name}_{YYYYMMDD}.json`

Each file contains an array of log batches with timestamps.

## Configuration

Update your ESP32's `config.h` with the server URL:
```c
#define REMOTE_LOG_SERVER_URL "http://YOUR_SERVER_IP:3000/api/logs"
```

Replace `YOUR_SERVER_IP` with:
- Your computer's local IP address (e.g., `192.168.1.100`)
- Or use hostname if mDNS is configured (e.g., `myserver.local`)

### Finding your IP address:
- **Windows**: `ipconfig` (look for IPv4 Address)
- **Mac/Linux**: `ifconfig` or `ip addr` (look for inet address)

## Example Output

```
==========================================
ESP32 Remote Logging Server
==========================================
Server listening on port 3000
Log directory: /path/to/device_logs

Endpoints:
  POST http://localhost:3000/api/logs - Receive logs from ESP32
  GET  http://localhost:3000/api/logs - List all log files
  GET  http://localhost:3000/health   - Health check

Set VERBOSE=1 to print all log messages to console
==========================================
```

When logs are received:
```
[2025-10-22T14:30:15.123Z] Received 25 logs from weather-esp32 (dropped: 0)
```
