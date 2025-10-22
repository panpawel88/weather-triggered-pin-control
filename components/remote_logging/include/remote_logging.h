#ifndef REMOTE_LOGGING_H
#define REMOTE_LOGGING_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * @file remote_logging.h
 * @brief Remote logging component for sending ESP32 logs to HTTP server
 *
 * This component intercepts ESP-IDF logging calls and buffers them in a circular buffer.
 * Logs are sent to a remote HTTP server when flush is called (typically during WiFi connection).
 *
 * Features:
 * - Circular buffer with FIFO overflow (drops oldest when full)
 * - Each log includes timestamp from RTC
 * - Thread-safe operations
 * - Tracks dropped messages
 * - Graceful degradation if server unavailable
 */

/**
 * @brief Initialize remote logging system
 *
 * Hooks into ESP-IDF logging via esp_log_set_vprintf() to intercept all log messages.
 * Allocates circular buffer based on HW_LOG_BUFFER_SIZE configuration.
 *
 * @return ESP_OK on success, ESP_FAIL if already initialized or allocation failed
 */
esp_err_t remote_logging_init(void);

/**
 * @brief Flush all buffered logs to remote server
 *
 * Sends all buffered log messages to the configured HTTP server endpoint.
 * Should be called when WiFi connection is available.
 *
 * If server is unreachable, logs remain in buffer and will be retried on next flush.
 * If buffer overflows between flushes, oldest messages are dropped (FIFO).
 *
 * @return ESP_OK if logs sent successfully, ESP_FAIL if server unreachable or HTTP error
 */
esp_err_t remote_logging_flush(void);

/**
 * @brief Get number of messages currently in buffer
 *
 * @return Number of buffered log messages
 */
int remote_logging_get_buffered_count(void);

/**
 * @brief Get number of messages dropped due to buffer overflow
 *
 * Counter is reset after successful flush to server.
 *
 * @return Number of dropped messages since last successful flush
 */
int remote_logging_get_dropped_count(void);

/**
 * @brief Deinitialize remote logging and free resources
 *
 * Removes logging hook and frees circular buffer.
 * Any buffered logs are discarded.
 *
 * @return ESP_OK on success
 */
esp_err_t remote_logging_deinit(void);

#endif // REMOTE_LOGGING_H
