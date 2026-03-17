/**
 * WiFi Access Point Implementation
 * 
 * Creates a simple WiFi hotspot for ESP32-CAM
 */

#include "wifi_ap.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/ip4_addr.h"
#include <string.h>

static const char* TAG = "WIFI_AP";

// Access Point Configuration
#define AP_SSID         "ESP32-CAM-DRONE"
#define AP_PASSWORD     "12345678"
#define AP_CHANNEL      1
#define AP_MAX_CONN     4

static bool ap_running = false;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        ESP_LOGI(TAG, "📱 Device connected to hotspot");
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        ESP_LOGI(TAG, "📱 Device disconnected from hotspot");
    }
}

esp_err_t wifi_ap_init(void)
{
    ESP_LOGI(TAG, "🔥 Starting ESP32-CAM WiFi Hotspot...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize TCP/IP adapter
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default WiFi AP
    esp_netif_t* wifi_netif = esp_netif_create_default_wifi_ap();
    
    // Set static IP configuration
    esp_netif_ip_info_t ip_info;
    IP4_ADDR(&ip_info.ip, 192, 168, 4, 1);      // Gateway IP
    IP4_ADDR(&ip_info.gw, 192, 168, 4, 1);      // Gateway
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0); // Subnet mask
    
    ESP_ERROR_CHECK(esp_netif_dhcps_stop(wifi_netif));
    ESP_ERROR_CHECK(esp_netif_set_ip_info(wifi_netif, &ip_info));
    ESP_ERROR_CHECK(esp_netif_dhcps_start(wifi_netif));

    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handler
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    // Configure WiFi AP
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .channel = AP_CHANNEL,
            .password = AP_PASSWORD,
            .max_connection = AP_MAX_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };

    // If password is empty, use open authentication
    if (strlen(AP_PASSWORD) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ap_running = true;
    
    ESP_LOGI(TAG, "✅ WiFi Hotspot Started!");
    ESP_LOGI(TAG, "   📶 SSID: %s", AP_SSID);
    ESP_LOGI(TAG, "   🔐 Password: %s", AP_PASSWORD);
    ESP_LOGI(TAG, "   🌐 IP Address: 192.168.4.1");
    ESP_LOGI(TAG, "   📱 Max Connections: %d", AP_MAX_CONN);
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "🔗 Connect to hotspot and visit: http://192.168.4.1");

    return ESP_OK;
}

esp_err_t wifi_ap_get_ip_string(char* ip_str)
{
    if (!ip_str) {
        return ESP_ERR_INVALID_ARG;
    }
    
    strcpy(ip_str, "192.168.4.1");
    return ESP_OK;
}

bool wifi_ap_is_running(void)
{
    return ap_running;
}