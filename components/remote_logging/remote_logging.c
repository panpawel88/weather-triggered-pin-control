#include "remote_logging.h"
#include "hardware_config.h"
#include "rtc_helper.h"
#include "timezone_helper.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// Check if config.h exists and include it
#ifndef __has_include
    #error "Compiler does not support __has_include"
#endif

#if !__has_include("config.h")
    #error "config.h not found! Please copy components/weather_common/include/config.h.example to components/weather_common/include/config.h"
#endif
#include "config.h"

static const char *TAG = "REMOTE_LOG";

// Log entry structure
typedef struct {
    char timestamp[20];     // "YYYY-MM-DD HH:MM:SS"
    char level[8];          // "ERROR", "WARN", "INFO", "DEBUG", "VERBOSE"
    char tag[16];           // Component tag (truncated if needed)
    char message[128];      // Log message (truncated if needed)
} log_entry_t;

// Circular buffer structure
typedef struct {
    log_entry_t *entries;
    int capacity;
    int count;              // Current number of entries
    int write_index;        // Next position to write
    int dropped;            // Number of dropped messages since last flush
    SemaphoreHandle_t mutex;
    bool initialized;
} log_buffer_t;

static log_buffer_t g_log_buffer = {0};
static vprintf_like_t g_original_vprintf = NULL;

// Convert ESP log level to string
static const char* log_level_to_string(esp_log_level_t level) {
    switch (level) {
        case ESP_LOG_ERROR:   return "ERROR";
        case ESP_LOG_WARN:    return "WARN";
        case ESP_LOG_INFO:    return "INFO";
        case ESP_LOG_DEBUG:   return "DEBUG";
        case ESP_LOG_VERBOSE: return "VERBOSE";
        default:              return "NONE";
    }
}

// Parse log level from formatted log string
static void parse_log_entry(const char *log_str, char *level, char *tag, char *message) {
    // ESP-IDF log format: "X (12345) TAG: message"
    // where X is E/W/I/D/V

    if (log_str == NULL || strlen(log_str) < 3) {
        strcpy(level, "INFO");
        strcpy(tag, "UNKNOWN");
        strncpy(message, log_str ? log_str : "", 127);
        message[127] = '\0';
        return;
    }

    // Parse level from first character
    switch (log_str[0]) {
        case 'E': strcpy(level, "ERROR"); break;
        case 'W': strcpy(level, "WARN"); break;
        case 'I': strcpy(level, "INFO"); break;
        case 'D': strcpy(level, "DEBUG"); break;
        case 'V': strcpy(level, "VERBOSE"); break;
        default:  strcpy(level, "INFO"); break;
    }

    // Find TAG between ") " and ": "
    const char *tag_start = strchr(log_str, ')');
    if (tag_start) {
        tag_start += 2; // Skip ") "
        const char *tag_end = strstr(tag_start, ": ");
        if (tag_end) {
            int tag_len = tag_end - tag_start;
            if (tag_len > 15) tag_len = 15;
            strncpy(tag, tag_start, tag_len);
            tag[tag_len] = '\0';

            // Copy message
            strncpy(message, tag_end + 2, 127);
            message[127] = '\0';
            return;
        }
    }

    // Fallback: couldn't parse, use whole string as message
    strcpy(tag, "UNKNOWN");
    strncpy(message, log_str, 127);
    message[127] = '\0';
}

// Get current timestamp from RTC (in local time)
static void get_timestamp(char *timestamp_str) {
    datetime_t utc_time, local_time;

    // Read UTC time from RTC
    if (rtc_read_time(&utc_time) != ESP_OK) {
        strcpy(timestamp_str, "0000-00-00 00:00:00");
        return;
    }

    // Convert to local time (CET/CEST with automatic DST)
    if (utc_to_local(&utc_time, &local_time) != ESP_OK) {
        // Fallback to UTC if conversion fails
        snprintf(timestamp_str, 20, "%04d-%02d-%02d %02d:%02d:%02d",
                 utc_time.year, utc_time.month, utc_time.day,
                 utc_time.hour, utc_time.minute, utc_time.second);
        return;
    }

    // Use local time for timestamp
    snprintf(timestamp_str, 20, "%04d-%02d-%02d %02d:%02d:%02d",
             local_time.year, local_time.month, local_time.day,
             local_time.hour, local_time.minute, local_time.second);
}

// Check if a tag is allowed for remote logging
static bool is_tag_allowed(const char *tag) {
#ifdef HW_REMOTE_LOG_TAG_COUNT
    #if HW_REMOTE_LOG_TAG_COUNT == 0
        // Filter disabled, allow all tags
        return true;
    #else
        // Check whitelist
        static const char *allowed_tags[] = HW_REMOTE_LOG_TAGS;
        for (int i = 0; i < HW_REMOTE_LOG_TAG_COUNT; i++) {
            if (strcmp(tag, allowed_tags[i]) == 0) {
                return true;
            }
        }
        return false;
    #endif
#else
    // No filter configured, allow all tags
    return true;
#endif
}

// Custom vprintf that buffers logs
static int remote_vprintf(const char *fmt, va_list args) {
    // Call original vprintf for serial output
    int ret = 0;
    if (g_original_vprintf) {
        ret = g_original_vprintf(fmt, args);
    }

    // Only buffer if remote logging is enabled
#ifndef HW_REMOTE_LOGGING_ENABLED
    return ret;
#elif !HW_REMOTE_LOGGING_ENABLED
    return ret;
#endif

    if (!g_log_buffer.initialized || !g_log_buffer.mutex) {
        return ret;
    }

    // Format the log message
    char log_buffer[256];
    vsnprintf(log_buffer, sizeof(log_buffer), fmt, args);

    // Remove trailing newline if present
    size_t len = strlen(log_buffer);
    if (len > 0 && log_buffer[len - 1] == '\n') {
        log_buffer[len - 1] = '\0';
    }

    // Parse log entry
    char level[8], tag[16], message[128];
    parse_log_entry(log_buffer, level, tag, message);

    // Check if tag is allowed for remote logging
    if (!is_tag_allowed(tag)) {
        return ret; // Tag not in whitelist, skip buffering
    }

    // Add to circular buffer (thread-safe)
    if (xSemaphoreTake(g_log_buffer.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        log_entry_t *entry = &g_log_buffer.entries[g_log_buffer.write_index];

        // Get timestamp
        get_timestamp(entry->timestamp);

        // Copy parsed data
        strncpy(entry->level, level, sizeof(entry->level) - 1);
        entry->level[sizeof(entry->level) - 1] = '\0';

        strncpy(entry->tag, tag, sizeof(entry->tag) - 1);
        entry->tag[sizeof(entry->tag) - 1] = '\0';

        strncpy(entry->message, message, sizeof(entry->message) - 1);
        entry->message[sizeof(entry->message) - 1] = '\0';

        // Move write index (circular)
        g_log_buffer.write_index = (g_log_buffer.write_index + 1) % g_log_buffer.capacity;

        // Update count (if full, we're overwriting oldest)
        if (g_log_buffer.count < g_log_buffer.capacity) {
            g_log_buffer.count++;
        } else {
            // Buffer full, dropping oldest
            g_log_buffer.dropped++;
        }

        xSemaphoreGive(g_log_buffer.mutex);
    }

    return ret;
}

esp_err_t remote_logging_init(void) {
    if (g_log_buffer.initialized) {
        ESP_LOGW(TAG, "Remote logging already initialized");
        return ESP_FAIL;
    }

#ifndef HW_REMOTE_LOGGING_ENABLED
    ESP_LOGI(TAG, "Remote logging disabled in hardware_config.h");
    return ESP_OK;
#elif !HW_REMOTE_LOGGING_ENABLED
    ESP_LOGI(TAG, "Remote logging disabled in hardware_config.h");
    return ESP_OK;
#endif

#ifndef HW_LOG_BUFFER_SIZE
    #error "HW_LOG_BUFFER_SIZE not defined in hardware_config.h"
#endif

    // Allocate circular buffer
    g_log_buffer.capacity = HW_LOG_BUFFER_SIZE;
    g_log_buffer.entries = malloc(sizeof(log_entry_t) * g_log_buffer.capacity);
    if (!g_log_buffer.entries) {
        ESP_LOGE(TAG, "Failed to allocate log buffer");
        return ESP_FAIL;
    }

    // Create mutex
    g_log_buffer.mutex = xSemaphoreCreateMutex();
    if (!g_log_buffer.mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
        free(g_log_buffer.entries);
        return ESP_FAIL;
    }

    g_log_buffer.count = 0;
    g_log_buffer.write_index = 0;
    g_log_buffer.dropped = 0;
    g_log_buffer.initialized = true;

    // Hook into logging system
    g_original_vprintf = esp_log_set_vprintf(remote_vprintf);

    ESP_LOGI(TAG, "Remote logging initialized (buffer size: %d messages, device: %s)",
             g_log_buffer.capacity, HW_LOG_DEVICE_NAME);

    return ESP_OK;
}

esp_err_t remote_logging_flush(void) {
    if (!g_log_buffer.initialized) {
        return ESP_FAIL;
    }

#ifndef HW_REMOTE_LOGGING_ENABLED
    return ESP_OK;
#elif !HW_REMOTE_LOGGING_ENABLED
    return ESP_OK;
#endif

    // Temporarily unhook logging to prevent recursive calls to remote_vprintf
    // while we're inside this function (which could cause deadlock on mutex)
    vprintf_like_t saved_vprintf = esp_log_set_vprintf(g_original_vprintf);

#ifndef REMOTE_LOG_SERVER_URL
    ESP_LOGW(TAG, "REMOTE_LOG_SERVER_URL not defined in config.h, skipping flush");
    esp_log_set_vprintf(saved_vprintf);
    return ESP_FAIL;
#endif

    if (xSemaphoreTake(g_log_buffer.mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to acquire mutex for flush");
        esp_log_set_vprintf(saved_vprintf);
        return ESP_FAIL;
    }

    if (g_log_buffer.count == 0 && g_log_buffer.dropped == 0) {
        xSemaphoreGive(g_log_buffer.mutex);
        esp_log_set_vprintf(saved_vprintf);
        return ESP_OK; // Nothing to send
    }

    // Build JSON payload
    char *json_payload = malloc(8192); // Adjust size as needed
    if (!json_payload) {
        ESP_LOGE(TAG, "Failed to allocate JSON buffer");
        xSemaphoreGive(g_log_buffer.mutex);
        esp_log_set_vprintf(saved_vprintf);
        return ESP_FAIL;
    }

    int offset = 0;
    offset += snprintf(json_payload + offset, 8192 - offset,
                      "{\"device\":\"%s\",\"dropped\":%d,\"logs\":[",
                      HW_LOG_DEVICE_NAME, g_log_buffer.dropped);

    // Calculate read index (oldest message in circular buffer)
    int read_index;
    if (g_log_buffer.count < g_log_buffer.capacity) {
        read_index = 0; // Buffer not full yet, start from beginning
    } else {
        read_index = g_log_buffer.write_index; // Buffer full, oldest is at write position
    }

    // Add log entries
    for (int i = 0; i < g_log_buffer.count && offset < 8000; i++) {
        log_entry_t *entry = &g_log_buffer.entries[read_index];

        // Escape quotes in message
        char escaped_msg[256];
        int esc_idx = 0;
        for (int j = 0; entry->message[j] && esc_idx < 254; j++) {
            if (entry->message[j] == '"' || entry->message[j] == '\\') {
                escaped_msg[esc_idx++] = '\\';
            }
            escaped_msg[esc_idx++] = entry->message[j];
        }
        escaped_msg[esc_idx] = '\0';

        offset += snprintf(json_payload + offset, 8192 - offset,
                          "%s{\"timestamp\":\"%s\",\"level\":\"%s\",\"tag\":\"%s\",\"message\":\"%s\"}",
                          i > 0 ? "," : "", entry->timestamp, entry->level, entry->tag, escaped_msg);

        read_index = (read_index + 1) % g_log_buffer.capacity;
    }

    offset += snprintf(json_payload + offset, 8192 - offset, "]}");

    // Send HTTP POST
    esp_http_client_config_t config = {
        .url = REMOTE_LOG_SERVER_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        free(json_payload);
        xSemaphoreGive(g_log_buffer.mutex);
        esp_log_set_vprintf(saved_vprintf);
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_payload, strlen(json_payload));

    esp_err_t err = esp_http_client_perform(client);
    int status_code = esp_http_client_get_status_code(client);

    esp_http_client_cleanup(client);
    free(json_payload);

    if (err == ESP_OK && status_code >= 200 && status_code < 300) {
        ESP_LOGI(TAG, "Flushed %d logs to server (dropped: %d)", g_log_buffer.count, g_log_buffer.dropped);

        // Clear buffer after successful send
        g_log_buffer.count = 0;
        g_log_buffer.write_index = 0;
        g_log_buffer.dropped = 0;

        xSemaphoreGive(g_log_buffer.mutex);
        esp_log_set_vprintf(saved_vprintf);
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Failed to send logs: HTTP %d, err=%d", status_code, err);
        xSemaphoreGive(g_log_buffer.mutex);
        esp_log_set_vprintf(saved_vprintf);
        return ESP_FAIL;
    }
}

int remote_logging_get_buffered_count(void) {
    if (!g_log_buffer.initialized || !g_log_buffer.mutex) {
        return 0;
    }

    int count = 0;
    if (xSemaphoreTake(g_log_buffer.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        count = g_log_buffer.count;
        xSemaphoreGive(g_log_buffer.mutex);
    }
    return count;
}

int remote_logging_get_dropped_count(void) {
    if (!g_log_buffer.initialized || !g_log_buffer.mutex) {
        return 0;
    }

    int dropped = 0;
    if (xSemaphoreTake(g_log_buffer.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        dropped = g_log_buffer.dropped;
        xSemaphoreGive(g_log_buffer.mutex);
    }
    return dropped;
}

esp_err_t remote_logging_deinit(void) {
    if (!g_log_buffer.initialized) {
        return ESP_OK;
    }

    // Restore original vprintf
    if (g_original_vprintf) {
        esp_log_set_vprintf(g_original_vprintf);
    }

    // Free resources
    if (g_log_buffer.mutex) {
        vSemaphoreDelete(g_log_buffer.mutex);
    }
    if (g_log_buffer.entries) {
        free(g_log_buffer.entries);
    }

    memset(&g_log_buffer, 0, sizeof(log_buffer_t));
    return ESP_OK;
}
