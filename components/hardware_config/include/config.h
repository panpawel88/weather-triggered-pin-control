/**
 * Weather Control Configuration File
 *
 * SETUP INSTRUCTIONS:
 * 1. Copy this file to config.h in the same directory:
 *    cp components/weather_common/include/config.h.example components/weather_common/include/config.h
 *
 * 2. Edit components/weather_common/include/config.h with your WiFi credentials
 *
 * 3. The config.h file is gitignored and will not be committed to the repository
 *
 * NOTE: Hardware pins and behavior are configured in hardware_config.h (same directory)
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// WiFi Configuration (REQUIRED)
// ============================================================================

/**
 * Your WiFi network name (SSID)
 * Example: "MyHomeNetwork"
 */
#define WIFI_SSID "YOUR_NETWORK"

/**
 * Your WiFi password
 * Example: "MySecurePassword123"
 */
#define WIFI_PASSWORD "YOUR_PASSWORD"

// ============================================================================
// Location Override (OPTIONAL)
// ============================================================================

/**
 * Uncomment and set these to override the default location from hardware_config.h
 *
 * Default location is defined in hardware_config.h:
 *   HW_DEFAULT_LATITUDE and HW_DEFAULT_LONGITUDE
 *
 * To find your coordinates:
 * - Visit https://www.openstreetmap.org/
 * - Right-click on your location and select "Show address"
 * - Use the latitude and longitude values
 *
 * Examples:
 * - Warsaw, Poland: 52.23, 21.01
 * - New York, USA: 40.71, -74.01
 * - Tokyo, Japan: 35.68, 139.65
 * - Sydney, Australia: -33.87, 151.21
 */
// #define LATITUDE 52.23f
// #define LONGITUDE 21.01f

// ============================================================================
// Remote Logging Configuration (OPTIONAL)
// ============================================================================

/**
 * Remote logging server URL for HTTP POST endpoint
 *
 * If HW_REMOTE_LOGGING_ENABLED is true in hardware_config.h, logs will be
 * sent to this HTTP server endpoint during WiFi connection.
 *
 * The ESP32 will POST JSON data to this URL with format:
 * {
 *   "device": "weather-esp32",
 *   "dropped": 0,
 *   "logs": [
 *     {"timestamp": "2025-10-22 14:30:15", "level": "INFO", "tag": "WEATHER", "message": "..."},
 *     ...
 *   ]
 * }
 *
 * Examples:
 * - Local network: "http://192.168.1.100:3000/api/logs"
 * - With hostname: "http://myserver.local:3000/api/logs"
 *
 * IMPORTANT: Must use HTTP (not HTTPS) for simple setup, or configure HTTPS certificates.
 *
 * See server_examples/ directory for ready-to-use Node.js and Python server implementations.
 */
#define REMOTE_LOG_SERVER_URL "http://192.168.1.100:3000/api/logs"

#endif // CONFIG_H
