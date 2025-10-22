#include "wifi_helper.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>

// Include config.h for WiFi credentials (if available)
#if __has_include("config.h")
    #include "config.h"
#endif

// Provide default WiFi credentials if not defined in config.h
#ifndef WIFI_SSID
    #define WIFI_SSID "YOUR_WIFI_SSID"
#endif
#ifndef WIFI_PASSWORD
    #define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#endif

static const char *TAG = "WIFI_HELPER";

// Store event handler instances for cleanup
static esp_event_handler_instance_t instance_any_id = NULL;
static esp_event_handler_instance_t instance_got_ip = NULL;

esp_err_t wifi_scan_networks(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize network interface
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Starting WiFi scan...");

    // Configure scan
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true
    };

    // Start scan (blocking)
    if (esp_wifi_scan_start(&scan_config, true) != ESP_OK) {
        ESP_LOGE(TAG, "WiFi scan failed!");
        esp_wifi_stop();
        esp_wifi_deinit();
        return ESP_FAIL;
    }

    // Get scan results
    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  WiFi Scan Results");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    if (ap_count == 0) {
        ESP_LOGW(TAG, "No WiFi networks found!");
    } else {
        ESP_LOGI(TAG, "Found %d WiFi networks:", ap_count);
        ESP_LOGI(TAG, "");

        wifi_ap_record_t *ap_records = malloc(sizeof(wifi_ap_record_t) * ap_count);
        if (ap_records) {
            if (esp_wifi_scan_get_ap_records(&ap_count, ap_records) == ESP_OK) {
                ESP_LOGI(TAG, "%-32s %-6s %-4s %s", "SSID", "RSSI", "Ch", "Auth");
                ESP_LOGI(TAG, "%-32s %-6s %-4s %s", "--------------------------------",
                         "------", "----", "----");

                bool found_configured_ssid = false;
                for (int i = 0; i < ap_count; i++) {
                    const char *auth_mode;
                    switch (ap_records[i].authmode) {
                        case WIFI_AUTH_OPEN:            auth_mode = "OPEN"; break;
                        case WIFI_AUTH_WEP:             auth_mode = "WEP"; break;
                        case WIFI_AUTH_WPA_PSK:         auth_mode = "WPA"; break;
                        case WIFI_AUTH_WPA2_PSK:        auth_mode = "WPA2"; break;
                        case WIFI_AUTH_WPA_WPA2_PSK:    auth_mode = "WPA/2"; break;
                        case WIFI_AUTH_WPA3_PSK:        auth_mode = "WPA3"; break;
                        case WIFI_AUTH_WPA2_WPA3_PSK:   auth_mode = "WPA2/3"; break;
                        default:                        auth_mode = "?"; break;
                    }

                    char marker = ' ';
                    if (strcmp((char *)ap_records[i].ssid, WIFI_SSID) == 0) {
                        marker = '*';
                        found_configured_ssid = true;
                    }

                    ESP_LOGI(TAG, "%c%-31s %-6d %-4d %s",
                             marker,
                             (char *)ap_records[i].ssid,
                             ap_records[i].rssi,
                             ap_records[i].primary,
                             auth_mode);
                }

                ESP_LOGI(TAG, "");
                ESP_LOGI(TAG, "Configured SSID: \"%s\"", WIFI_SSID);
                ESP_LOGI(TAG, "Configured SSID length: %d bytes", strlen(WIFI_SSID));

                if (found_configured_ssid) {
                    ESP_LOGI(TAG, "Status: Configured SSID found in scan (marked with *)");
                } else {
                    ESP_LOGW(TAG, "Status: Configured SSID NOT found in scan!");
                    ESP_LOGW(TAG, "This will cause connection to fail with error 201");

                    // Show hex dump of configured SSID for debugging
                    ESP_LOGW(TAG, "Configured SSID hex: ");
                    for (int i = 0; i < strlen(WIFI_SSID); i++) {
                        printf("%02X ", (unsigned char)WIFI_SSID[i]);
                    }
                    printf("\n");
                }
            }
            free(ap_records);
        }
    }

    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "");

    // Clean up WiFi
    esp_wifi_stop();
    esp_wifi_deinit();

    // Clean up netif
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif != NULL) {
        esp_netif_destroy(netif);
    }

    // Clean up event loop
    esp_event_loop_delete_default();

    // Clean up network interface system
    esp_netif_deinit();

    return ESP_OK;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi started, attempting to connect...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* disconn_evt = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGW(TAG, "WiFi disconnected, reason: %d", disconn_evt->reason);
        // Reason codes:
        // 2 = AUTH_EXPIRE (password wrong)
        // 3 = AUTH_LEAVE
        // 15 = 4WAY_HANDSHAKE_TIMEOUT (password wrong)
        // 201 = NO_AP_FOUND (SSID wrong or AP not in range)
        esp_wifi_connect();  // Retry connection
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "WiFi connected, IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

esp_err_t wifi_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize network interface (only once, reuse if already initialized)
    ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(ret);
    }

    // Create event loop (only once, reuse if already created)
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(ret);
    }

    // Create WiFi station netif (only if not already created)
    if (esp_netif_get_handle_from_ifkey("WIFI_STA_DEF") == NULL) {
        esp_netif_create_default_wifi_sta();
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers only if not already registered
    if (instance_any_id == NULL) {
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            &instance_any_id));
    }
    if (instance_got_ip == NULL) {
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            &instance_got_ip));
    }

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialized");
    return ESP_OK;
}

esp_err_t wifi_wait_connected(int max_retries, int retry_delay_ms) {
    esp_netif_ip_info_t ip_info;
    int retry_count = 0;

    ESP_LOGI(TAG, "Waiting for IP address via DHCP (timeout: %d seconds)...",
             (max_retries * retry_delay_ms) / 1000);

    while (retry_count < max_retries) {
        esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        if (netif != NULL && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
            if (ip_info.ip.addr != 0) {
                ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&ip_info.ip));
                ESP_LOGI(TAG, "Netmask: " IPSTR, IP2STR(&ip_info.netmask));
                ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&ip_info.gw));
                return ESP_OK;
            }
        }

        // Log progress every 2 seconds
        if (retry_count % 4 == 0 && retry_count > 0) {
            ESP_LOGI(TAG, "Still waiting for IP... (%d/%d)", retry_count, max_retries);
        }

        vTaskDelay(retry_delay_ms / portTICK_PERIOD_MS);
        retry_count++;
    }

    ESP_LOGE(TAG, "DHCP timeout - no IP address assigned after %d seconds",
             (max_retries * retry_delay_ms) / 1000);
    ESP_LOGE(TAG, "WiFi may be connected but DHCP failed");
    return ESP_ERR_TIMEOUT;
}

esp_err_t wifi_shutdown(void) {
    esp_err_t err;

    // Stop WiFi
    err = esp_wifi_stop();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi stop failed: %s", esp_err_to_name(err));
        return err;
    }

    // Deinit WiFi (but keep netif and event loop for reuse)
    err = esp_wifi_deinit();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi deinit failed: %s", esp_err_to_name(err));
        return err;
    }

    // Note: Event handlers are kept registered for reuse
    // netif and event loop are also preserved for reuse

    ESP_LOGI(TAG, "WiFi shutdown complete (netif and event loop preserved for reuse)");
    return ESP_OK;
}
