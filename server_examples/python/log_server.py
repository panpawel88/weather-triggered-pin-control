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
from datetime import datetime
from pathlib import Path
from flask import Flask, request, jsonify

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 3000
LOG_DIR = Path(__file__).parent / 'device_logs'

# Create log directory if it doesn't exist
LOG_DIR.mkdir(exist_ok=True)

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


if __name__ == '__main__':
    print('=' * 50)
    print('ESP32 Remote Logging Server (Python)')
    print('=' * 50)
    print(f'Server listening on port {PORT}')
    print(f'Log directory: {LOG_DIR.absolute()}')
    print()
    print('Endpoints:')
    print(f'  POST http://localhost:{PORT}/api/logs - Receive logs from ESP32')
    print(f'  GET  http://localhost:{PORT}/api/logs - List all log files')
    print(f'  GET  http://localhost:{PORT}/health   - Health check')
    print()
    print('Set VERBOSE=1 to print all log messages to console')
    print('=' * 50)
    print()

    app.run(host='0.0.0.0', port=PORT, debug=False)
