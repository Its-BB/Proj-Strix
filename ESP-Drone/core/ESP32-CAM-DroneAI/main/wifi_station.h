/**
 * WiFi Station Mode Implementation
 * 
 * Connects ESP32-CAM to an existing WiFi network
 */

#ifndef WIFI_STATION_H
#define WIFI_STATION_H

#include <stdbool.h>
#include "esp_err.h"

/**
 * Initialize WiFi in station mode and connect to network
 * Uses credentials from wifi_config.h
 */
esp_err_t wifi_station_init(void);

/**
 * Check if WiFi is connected
 */
bool wifi_station_is_connected(void);

/**
 * Get the IP address of the ESP32
 */
esp_err_t wifi_station_get_ip_string(char* ip_str);

#endif // WIFI_STATION_H
