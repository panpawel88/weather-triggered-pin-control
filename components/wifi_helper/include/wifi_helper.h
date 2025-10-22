#ifndef WIFI_HELPER_H
#define WIFI_HELPER_H

#include "esp_err.h"

/**
 * @file wifi_helper.h
 * @brief Generic WiFi helper functions for ESP32
 *
 * Provides simple WiFi station mode initialization, connection waiting,
 * and shutdown functions. Decoupled from any specific application logic.
 */

/**
 * @brief Scan for available WiFi networks
 *
 * Initializes WiFi in scan mode, scans for networks, prints results, and cleans up.
 * This is a standalone function that doesn't require wifi_init() to be called first.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_scan_networks(void);

/**
 * @brief Initialize WiFi station mode
 *
 * This function initializes NVS, network interface, and WiFi.
 * It uses WIFI_SSID and WIFI_PASSWORD from config.h.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_init(void);

/**
 * @brief Wait for WiFi connection with retry logic
 *
 * Polls for IP address assignment to verify connection.
 *
 * @param max_retries Maximum number of retry attempts
 * @param retry_delay_ms Delay between retries in milliseconds
 * @return ESP_OK if connected, ESP_ERR_TIMEOUT if not connected after max retries
 */
esp_err_t wifi_wait_connected(int max_retries, int retry_delay_ms);

/**
 * @brief Shutdown WiFi to save power
 *
 * Stops WiFi and cleans up resources. Safe to call before deep sleep.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t wifi_shutdown(void);

#endif // WIFI_HELPER_H
