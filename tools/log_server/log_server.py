#!/usr/bin/env python3
"""
ESP32 Remote Logging Server (Python)

Simple HTTP server that receives and stores logs from ESP32 devices.
Logs are saved to text files organized by device name and date.

Installation:
    pip install flask

Usage:
    python log_server.py [port]
    Default port: 3000

Example:
    python log_server.py 3000
"""

import sys
import os
import json
from datetime import datetime, timedelta
from pathlib import Path
from flask import Flask, request, jsonify, render_template_string

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 3000
LOG_DIR = Path(__file__).parent / 'device_logs'
DIAGNOSTICS_DIR = Path(__file__).parent / 'diagnostics'

# Create directories if they don't exist
LOG_DIR.mkdir(exist_ok=True)
DIAGNOSTICS_DIR.mkdir(exist_ok=True)

app = Flask(__name__)

# Verbose mode (print logs to console)
VERBOSE = os.environ.get('VERBOSE', '0') == '1'


@app.route('/health', methods=['GET'])
def health():
    """Health check endpoint"""
    return jsonify({
        'status': 'ok',
        'timestamp': datetime.now().isoformat()
    })


@app.route('/api/logs', methods=['POST'])
def receive_logs():
    """Log ingestion endpoint"""
    try:
        data = request.get_json()

        if not data or 'device' not in data or 'logs' not in data:
            return jsonify({'error': 'Invalid payload format'}), 400

        device = data['device']
        dropped = data.get('dropped', 0)
        logs = data['logs']

        if not isinstance(logs, list):
            return jsonify({'error': 'Logs must be an array'}), 400

        # Generate filename: device_YYYYMMDD.log
        date_str = datetime.now().strftime('%Y%m%d')
        filename = f"{device}_{date_str}.log"
        filepath = LOG_DIR / filename

        # Prepare log entry header
        timestamp = datetime.now().isoformat()
        header = f"\n{'='*80}\n"
        header += f"Received at: {timestamp}\n"
        header += f"Device: {device}\n"
        header += f"Log count: {len(logs)}\n"
        header += f"Dropped messages: {dropped}\n"
        header += f"{'='*80}\n"

        # Append to file
        with open(filepath, 'a', encoding='utf-8') as f:
            f.write(header)
            for log in logs:
                timestamp = log.get('timestamp', 'N/A')
                level = log.get('level', 'INFO').ljust(7)
                tag = log.get('tag', 'UNKNOWN').ljust(15)
                message = log.get('message', '')
                f.write(f"[{timestamp}] {level} {tag} {message}\n")
            f.write("\n")

        # Console output
        print(f"[{datetime.now().isoformat()}] Received {len(logs)} logs from {device} (dropped: {dropped})")

        # Optional: Print log messages to console
        if VERBOSE:
            for log in logs:
                timestamp = log.get('timestamp', 'N/A')
                level = log.get('level', 'INFO').ljust(7)
                tag = log.get('tag', 'UNKNOWN').ljust(15)
                message = log.get('message', '')
                print(f"  [{timestamp}] {level} {tag} {message}")

        return jsonify({
            'success': True,
            'received': len(logs),
            'saved_to': filename
        })

    except Exception as e:
        print(f"Error processing logs: {e}")
        return jsonify({'error': 'Internal server error'}), 500


@app.route('/api/logs', methods=['GET'])
def list_logs():
    """List all log files"""
    try:
        files = []
        for filepath in sorted(LOG_DIR.glob('*.log'), key=os.path.getmtime, reverse=True):
            stat = filepath.stat()
            files.append({
                'filename': filepath.name,
                'size': stat.st_size,
                'modified': datetime.fromtimestamp(stat.st_mtime).isoformat()
            })

        return jsonify({'files': files})

    except Exception as e:
        print(f"Error listing logs: {e}")
        return jsonify({'error': 'Internal server error'}), 500


@app.route('/api/logs/<filename>', methods=['GET'])
def get_log_file(filename):
    """Get specific log file contents"""
    try:
        filepath = LOG_DIR / filename

        if not filepath.exists():
            return jsonify({'error': 'Log file not found'}), 404

        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()

        return content, 200, {'Content-Type': 'text/plain; charset=utf-8'}

    except Exception as e:
        print(f"Error reading log file: {e}")
        return jsonify({'error': 'Internal server error'}), 500


@app.route('/api/diagnostics', methods=['POST'])
def receive_diagnostics():
    """Diagnostics ingestion endpoint"""
    try:
        data = request.get_json()

        # Validate payload
        required_fields = ['device', 'timestamp', 'date', 'sunrise', 'sunset',
                          'avg_cloudcover', 'pin_off_hour', 'led_count', 'hourly']
        if not data or not all(field in data for field in required_fields):
            return jsonify({'error': 'Invalid payload format'}), 400

        device = data['device']
        date_str = data['date'].replace('-', '')  # "2025-11-02" -> "20251102"

        # Generate filename: device_YYYYMMDD.json
        filename = f"{device}_{date_str}.json"
        filepath = DIAGNOSTICS_DIR / filename

        # Save diagnostic data as JSON
        with open(filepath, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2)

        print(f"[{datetime.now().isoformat()}] Received diagnostics from {device} for {data['date']}")

        if VERBOSE:
            print(f"  Sunrise: {data['sunrise']}, Sunset: {data['sunset']}")
            print(f"  Avg cloudcover: {data['avg_cloudcover']}%")
            print(f"  Pin off hour: {data['pin_off_hour']}, LED count: {data['led_count']}")
            print(f"  Hourly data points: {len(data['hourly'])}")

        return jsonify({
            'success': True,
            'saved_to': filename
        })

    except Exception as e:
        print(f"Error processing diagnostics: {e}")
        return jsonify({'error': 'Internal server error'}), 500


@app.route('/api/diagnostics', methods=['GET'])
def list_diagnostics():
    """List diagnostic files with 30-day cleanup"""
    try:
        # Cleanup files older than 30 days
        cutoff_date = datetime.now() - timedelta(days=30)
        for filepath in DIAGNOSTICS_DIR.glob('*.json'):
            stat = filepath.stat()
            if datetime.fromtimestamp(stat.st_mtime) < cutoff_date:
                filepath.unlink()
                print(f"Deleted old diagnostic file: {filepath.name}")

        # List remaining files
        files = []
        for filepath in sorted(DIAGNOSTICS_DIR.glob('*.json'), key=os.path.getmtime, reverse=True):
            stat = filepath.stat()

            # Extract date from filename (device_YYYYMMDD.json)
            filename_parts = filepath.stem.split('_')
            if len(filename_parts) >= 2:
                date_str = filename_parts[-1]  # Get last part (YYYYMMDD)
                try:
                    # Format as YYYY-MM-DD
                    formatted_date = f"{date_str[:4]}-{date_str[4:6]}-{date_str[6:8]}"
                    device_name = '_'.join(filename_parts[:-1])
                except:
                    formatted_date = date_str
                    device_name = filepath.stem
            else:
                formatted_date = 'unknown'
                device_name = filepath.stem

            files.append({
                'filename': filepath.name,
                'date': formatted_date,
                'device': device_name,
                'size': stat.st_size,
                'modified': datetime.fromtimestamp(stat.st_mtime).isoformat()
            })

        return jsonify({'files': files})

    except Exception as e:
        print(f"Error listing diagnostics: {e}")
        return jsonify({'error': 'Internal server error'}), 500


@app.route('/api/diagnostics/<filename>', methods=['GET'])
def get_diagnostic_file(filename):
    """Get specific diagnostic file contents"""
    try:
        filepath = DIAGNOSTICS_DIR / filename

        if not filepath.exists():
            return jsonify({'error': 'Diagnostic file not found'}), 404

        with open(filepath, 'r', encoding='utf-8') as f:
            data = json.load(f)

        return jsonify(data)

    except Exception as e:
        print(f"Error reading diagnostic file: {e}")
        return jsonify({'error': 'Internal server error'}), 500


def get_cloudcover_color(cloudcover):
    """Get CSS color class based on cloudcover percentage"""
    if cloudcover < 10:
        return 'very-clear'
    elif cloudcover < 20:
        return 'clear'
    elif cloudcover < 30:
        return 'mostly-clear'
    elif cloudcover < 40:
        return 'partly-cloudy'
    elif cloudcover < 50:
        return 'cloudy'
    else:
        return 'very-cloudy'


@app.route('/diagnostics', methods=['GET'])
@app.route('/', methods=['GET'])
def diagnostics_page():
    """HTML visualization page for weather diagnostics"""
    try:
        # Get list of diagnostic files
        files = []
        for filepath in sorted(DIAGNOSTICS_DIR.glob('*.json'), key=os.path.getmtime, reverse=True):
            filename_parts = filepath.stem.split('_')
            if len(filename_parts) >= 2:
                date_str = filename_parts[-1]
                formatted_date = f"{date_str[:4]}-{date_str[4:6]}-{date_str[6:8]}"
            else:
                formatted_date = 'unknown'
            files.append({'filename': filepath.name, 'date': formatted_date})

        # Load latest diagnostic data
        diagnostic_data = None
        if files:
            latest_file = DIAGNOSTICS_DIR / files[0]['filename']
            with open(latest_file, 'r', encoding='utf-8') as f:
                diagnostic_data = json.load(f)

        # Render HTML template
        html_template = '''
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Weather Diagnostics</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            max-width: 900px;
            margin: 0 auto;
            background: white;
            border-radius: 12px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.2);
            overflow: hidden;
        }
        header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px;
            text-align: center;
        }
        header h1 {
            font-size: 28px;
            margin-bottom: 10px;
        }
        header p {
            font-size: 14px;
            opacity: 0.9;
        }
        .content {
            padding: 30px;
        }
        .section {
            margin-bottom: 30px;
        }
        .section-title {
            font-size: 18px;
            font-weight: 600;
            color: #333;
            margin-bottom: 15px;
            padding-bottom: 8px;
            border-bottom: 2px solid #667eea;
        }
        .info-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
            margin-bottom: 20px;
        }
        .info-card {
            background: #f7f9fc;
            padding: 15px;
            border-radius: 8px;
            border-left: 4px solid #667eea;
        }
        .info-label {
            font-size: 12px;
            color: #666;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            margin-bottom: 5px;
        }
        .info-value {
            font-size: 20px;
            font-weight: 600;
            color: #333;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            margin-top: 10px;
        }
        thead {
            background: #f7f9fc;
        }
        th {
            padding: 12px;
            text-align: left;
            font-size: 13px;
            font-weight: 600;
            color: #555;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            border-bottom: 2px solid #e0e0e0;
        }
        td {
            padding: 12px;
            border-bottom: 1px solid #f0f0f0;
        }
        tr:hover {
            background: #f9fafb;
        }
        .cloudcover-cell {
            font-weight: 600;
            padding: 8px 12px;
            border-radius: 4px;
            text-align: center;
        }
        .very-clear {
            background: #d4edda;
            color: #155724;
        }
        .clear {
            background: #d1ecf1;
            color: #0c5460;
        }
        .mostly-clear {
            background: #fff3cd;
            color: #856404;
        }
        .partly-cloudy {
            background: #ffe5b4;
            color: #856404;
        }
        .cloudy {
            background: #e2e3e5;
            color: #383d41;
        }
        .very-cloudy {
            background: #d6d8db;
            color: #1b1e21;
        }
        .no-data {
            text-align: center;
            padding: 40px;
            color: #999;
            font-size: 16px;
        }
        select {
            padding: 10px 15px;
            border: 2px solid #e0e0e0;
            border-radius: 6px;
            font-size: 14px;
            color: #333;
            cursor: pointer;
            transition: border-color 0.2s;
        }
        select:hover {
            border-color: #667eea;
        }
        select:focus {
            outline: none;
            border-color: #667eea;
        }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>⛅ Weather Diagnostics</h1>
            <p>ESP32 Weather Forecast Data Visualization</p>
        </header>
        <div class="content">
            {% if diagnostic_data %}
            <!-- Date Selector -->
            {% if files|length > 1 %}
            <div class="section">
                <label for="date-select" style="display: block; margin-bottom: 8px; font-weight: 600; color: #555;">Select Date:</label>
                <select id="date-select" onchange="window.location.href='/diagnostics?file=' + this.value">
                    {% for file in files %}
                    <option value="{{ file.filename }}" {% if loop.first %}selected{% endif %}>{{ file.date }}</option>
                    {% endfor %}
                </select>
            </div>
            {% endif %}

            <!-- Summary Info -->
            <div class="section">
                <div class="section-title">Summary</div>
                <div class="info-grid">
                    <div class="info-card">
                        <div class="info-label">Device</div>
                        <div class="info-value">{{ diagnostic_data.device }}</div>
                    </div>
                    <div class="info-card">
                        <div class="info-label">Forecast Date</div>
                        <div class="info-value">{{ diagnostic_data.date }}</div>
                    </div>
                    <div class="info-card">
                        <div class="info-label">Sunrise</div>
                        <div class="info-value">{{ diagnostic_data.sunrise }}</div>
                    </div>
                    <div class="info-card">
                        <div class="info-label">Sunset</div>
                        <div class="info-value">{{ diagnostic_data.sunset }}</div>
                    </div>
                    <div class="info-card">
                        <div class="info-label">Avg Cloudcover</div>
                        <div class="info-value">{{ "%.1f"|format(diagnostic_data.avg_cloudcover) }}%</div>
                    </div>
                    <div class="info-card">
                        <div class="info-label">Pin Off Hour</div>
                        <div class="info-value">{{ diagnostic_data.pin_off_hour }}:00</div>
                    </div>
                    <div class="info-card">
                        <div class="info-label">LED Count</div>
                        <div class="info-value">{{ diagnostic_data.led_count }} LEDs</div>
                    </div>
                    <div class="info-card">
                        <div class="info-label">Received At</div>
                        <div class="info-value" style="font-size: 14px;">{{ diagnostic_data.timestamp }}</div>
                    </div>
                </div>
            </div>

            <!-- Hourly Data Table -->
            <div class="section">
                <div class="section-title">Hourly Cloudcover (Daytime)</div>
                <table>
                    <thead>
                        <tr>
                            <th>Hour</th>
                            <th>Cloudcover</th>
                            <th>Condition</th>
                        </tr>
                    </thead>
                    <tbody>
                        {% for entry in diagnostic_data.hourly %}
                        <tr>
                            <td style="font-weight: 600;">{{ "%02d"|format(entry.hour) }}:00</td>
                            <td>
                                <div class="cloudcover-cell {{ get_color(entry.cloudcover) }}">
                                    {{ "%.1f"|format(entry.cloudcover) }}%
                                </div>
                            </td>
                            <td>
                                {% if entry.cloudcover < 10 %}
                                ☀️ Very Clear
                                {% elif entry.cloudcover < 20 %}
                                🌤️ Clear
                                {% elif entry.cloudcover < 30 %}
                                🌥️ Mostly Clear
                                {% elif entry.cloudcover < 40 %}
                                ⛅ Partly Cloudy
                                {% elif entry.cloudcover < 50 %}
                                ☁️ Cloudy
                                {% else %}
                                🌫️ Very Cloudy
                                {% endif %}
                            </td>
                        </tr>
                        {% endfor %}
                    </tbody>
                </table>
            </div>
            {% else %}
            <div class="no-data">
                <p>📭 No diagnostic data available yet.</p>
                <p style="font-size: 14px; margin-top: 10px; color: #bbb;">
                    The ESP32 will send diagnostic data at 4 PM each day.
                </p>
            </div>
            {% endif %}
        </div>
    </div>
</body>
</html>
        '''

        return render_template_string(html_template,
                                     diagnostic_data=diagnostic_data,
                                     files=files,
                                     get_color=get_cloudcover_color)

    except Exception as e:
        print(f"Error rendering diagnostics page: {e}")
        return f"<h1>Error</h1><p>{str(e)}</p>", 500


if __name__ == '__main__':
    print('=' * 50)
    print('ESP32 Remote Logging Server (Python)')
    print('=' * 50)
    print(f'Server listening on port {PORT}')
    print(f'Log directory: {LOG_DIR.absolute()}')
    print(f'Diagnostics directory: {DIAGNOSTICS_DIR.absolute()}')
    print()
    print('Endpoints:')
    print(f'  POST http://localhost:{PORT}/api/logs         - Receive logs from ESP32')
    print(f'  GET  http://localhost:{PORT}/api/logs         - List all log files')
    print(f'  POST http://localhost:{PORT}/api/diagnostics  - Receive diagnostics from ESP32')
    print(f'  GET  http://localhost:{PORT}/api/diagnostics  - List diagnostics (30-day retention)')
    print(f'  GET  http://localhost:{PORT}/diagnostics      - View diagnostics web page')
    print(f'  GET  http://localhost:{PORT}/health           - Health check')
    print()
    print('Set VERBOSE=1 to print all log messages to console')
    print('=' * 50)
    print()

    app.run(host='0.0.0.0', port=PORT, debug=False)
