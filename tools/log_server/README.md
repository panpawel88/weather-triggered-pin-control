# ESP32 Remote Logging Server (Python)

Simple HTTP server that receives and stores logs from ESP32 devices.

## Installation

1. Install Python 3.7+ (if not already installed): https://www.python.org/
2. Install dependencies:
   ```bash
   cd server_examples/python
   pip install -r requirements.txt
   ```

## Usage

### Start the server:
```bash
python log_server.py
```

Or specify a custom port:
```bash
python log_server.py 8080
```

### Verbose mode (prints all logs to console):

**Linux/Mac:**
```bash
VERBOSE=1 python log_server.py
```

**Windows:**
```cmd
set VERBOSE=1
python log_server.py
```

## How It Works

- **POST /api/logs** - Receives log data from ESP32 devices
- **GET /api/logs** - Lists all stored log files
- **GET /api/logs/:filename** - Retrieves a specific log file (as text)
- **GET /health** - Health check endpoint

Logs are saved to `device_logs/` directory with format:
- `{device_name}_{YYYYMMDD}.log`

Each file contains human-readable text logs organized by batch with timestamps.

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
==================================================
ESP32 Remote Logging Server (Python)
==================================================
Server listening on port 3000
Log directory: /path/to/device_logs

Endpoints:
  POST http://localhost:3000/api/logs - Receive logs from ESP32
  GET  http://localhost:3000/api/logs - List all log files
  GET  http://localhost:3000/health   - Health check

Set VERBOSE=1 to print all log messages to console
==================================================
```

When logs are received:
```
[2025-10-22T14:30:15.123456] Received 25 logs from weather-esp32 (dropped: 0)
```

## Log File Format

```
================================================================================
Received at: 2025-10-22T14:30:15.123456
Device: weather-esp32
Log count: 25
Dropped messages: 0
================================================================================
[2025-10-22 14:30:00] INFO    WEATHER_CONTROL Weather Triggered Pin Control starting
[2025-10-22 14:30:01] INFO    WEATHER_CONTROL Timezone initialization successful
...
```
