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

### Remote Logging Endpoints

- **POST /api/logs** - Receives log data from ESP32 devices
- **GET /api/logs** - Lists all stored log files
- **GET /api/logs/:filename** - Retrieves a specific log file (as text)

Logs are saved to `device_logs/` directory with format:
- `{device_name}_{YYYYMMDD}.log`

Each file contains human-readable text logs organized by batch with timestamps.

### Weather Diagnostics Endpoints

- **POST /api/diagnostics** - Receives weather diagnostic data from ESP32
- **GET /api/diagnostics** - Lists diagnostic files (automatic 30-day cleanup)
- **GET /api/diagnostics/:filename** - Retrieves a specific diagnostic file (as JSON)
- **GET /diagnostics** - Web interface to view diagnostic data with tables and charts
- **GET /** - Same as /diagnostics (root path)

Diagnostics are saved to `diagnostics/` directory with format:
- `{device_name}_{YYYYMMDD}.json`

Each file contains weather forecast data including:
- Tomorrow's date, sunrise, and sunset times
- Hourly cloudcover percentages (daytime hours only)
- Calculated average cloudcover
- Pin off hour and LED count based on forecast

**Note:** Diagnostic files older than 30 days are automatically deleted when accessing the `/api/diagnostics` endpoint.

### Health Check

- **GET /health** - Health check endpoint

## Configuration

Update your ESP32's `config.h` with the server URLs:

```c
// Remote logging endpoint
#define REMOTE_LOG_SERVER_URL "http://YOUR_SERVER_IP:3000/api/logs"

// Weather diagnostics endpoint
#define REMOTE_DIAGNOSTICS_URL "http://YOUR_SERVER_IP:3000/api/diagnostics"
```

Replace `YOUR_SERVER_IP` with:
- Your computer's local IP address (e.g., `192.168.1.100`)
- Or use hostname if mDNS is configured (e.g., `myserver.local`)

To enable/disable these features, edit `hardware_config.h`:
```c
#define HW_REMOTE_LOGGING_ENABLED true      // Enable/disable remote logging
#define HW_WEATHER_DIAGNOSTICS_ENABLED true // Enable/disable weather diagnostics
```

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

## Diagnostic Data Format

Example diagnostic JSON file (`weather-esp32_20251101.json`):

```json
{
  "device": "weather-esp32",
  "timestamp": "2025-11-01 16:00:00",
  "date": "2025-11-02",
  "sunrise": "07:35",
  "sunset": "17:15",
  "avg_cloudcover": 45.3,
  "pin_off_hour": 18,
  "led_count": 3,
  "hourly": [
    {"hour": 8, "cloudcover": 23.0},
    {"hour": 9, "cloudcover": 45.0},
    {"hour": 10, "cloudcover": 67.0},
    ...
  ]
}
```

## Viewing Diagnostics

Open your web browser and navigate to:
```
http://YOUR_SERVER_IP:3000/diagnostics
```

The web interface displays:
- **Summary section**: Device name, forecast date, sunrise/sunset times, average cloudcover, pin off hour, LED count
- **Hourly table**: Hour-by-hour cloudcover percentages with color-coded cells
- **Date selector**: Dropdown to view historical diagnostic data (last 30 days)

The hourly cloudcover table uses color coding:
- 🟢 Green (0-9%): Very Clear
- 🔵 Blue (10-19%): Clear
- 🟡 Yellow (20-39%): Mostly Clear / Partly Cloudy
- ⚪ Gray (40-100%): Cloudy / Very Cloudy
