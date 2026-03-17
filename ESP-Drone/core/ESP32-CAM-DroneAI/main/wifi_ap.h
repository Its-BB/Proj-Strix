#ifndef WIFI_AP_H
#define WIFI_AP_H

#include "esp_wifi.h"
#include "esp_err.h"

/**
 * @brief Initialize and start WiFi Access Point
 * @return ESP_OK on success
 */
esp_err_t wifi_ap_init(void);

/**
 * @brief Get AP IP address as string
 * @param ip_str Buffer to store IP string (minimum 16 bytes)
 * @return ESP_OK on success
 */
esp_err_t wifi_ap_get_ip_string(char* ip_str);

/**
 * @brief Check if AP is running
 * @return true if AP is active
 */
bool wifi_ap_is_running(void);

#endif // WIFI_AP_H