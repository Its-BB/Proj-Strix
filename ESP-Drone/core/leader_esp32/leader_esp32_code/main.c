/**
 * ESP32 Leader Drone Broadcast Code
 * 
 * This code runs on the leader ESP32 drone and broadcasts its position
 * to follower drones via ESP-NOW protocol.
 * 
 * Features:
 * - Broadcasts drone position via ESP-NOW
 * - Sends position updates at 10Hz
 * - Includes signal strength (RSSI) information
 * - Supports multiple follower drones
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_netif.h"

#define TAG "LEADER_DRONE"

// Leader position data structure (must match follower's structure)
typedef struct {
    float x;                         // X position (meters)
    float y;                         // Y position (meters)
    float z;                         // Z position (meters, altitude)
    float vx;                        // X velocity
    float vy;                        // Y velocity
    float vz;                        // Z velocity
    int8_t rssi;                     // Signal strength (dBm)
    uint32_t timestamp;              // Timestamp of last update
    uint8_t leader_id;               // Leader drone ID
} leader_data_t;

// Broadcast MAC address (all followers listen to this)
static uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Leader data to broadcast
static leader_data_t leader_data = {
    .x = 0.0f,
    .y = 0.0f,
    .z = 0.35f,                     // Start at 35cm altitude
    .vx = 0.0f,
    .vy = 0.0f,
    .vz = 0.0f,
    .rssi = -40,                    // Good signal strength
    .timestamp = 0,
    .leader_id = 1                  // Leader ID = 1
};

// Broadcast interval (100ms for 10Hz)
#define BROADCAST_INTERVAL_MS 100

/**
 * ESP-NOW send callback
 */
static void esp_now_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS) {
        ESP_LOGD(TAG, "ESP-NOW send success");
    } else {
        ESP_LOGW(TAG, "ESP-NOW send failed");
    }
}

/**
 * ESP-NOW receive callback (for receiving follower status if needed)
 */
static void esp_now_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int data_len)
{
    ESP_LOGD(TAG, "Received data from: " MACSTR ", len: %d", MAC2STR(recv_info->src_addr), data_len);
}

/**
 * WiFi event handler
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi started");
    }
}

/**
 * Initialize WiFi for ESP-NOW
 */
static void wifi_init_espnow(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Set WiFi power to maximum
    esp_wifi_set_max_tx_power(84);  // 84 = 20dBm (maximum)
}

/**
 * Initialize ESP-NOW
 */
static void esp_now_init(void)
{
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(esp_now_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(esp_now_recv_cb));
    
    // Add broadcast peer
    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, broadcast_mac, ESP_NOW_ETH_ALEN);
    peer.channel = 0;
    peer.ifidx = ESP_IF_WIFI_STA;
    peer.encrypt = false;
    
    ESP_ERROR_CHECK(esp_now_add_peer(&peer));
    ESP_LOGI(TAG, "ESP-NOW initialized and broadcast peer added");
}

/**
 * Broadcast leader position
 */
static void broadcast_leader_position(void)
{
    leader_data.timestamp = xTaskGetTickCount();
    
    esp_err_t result = esp_now_send(broadcast_mac, (uint8_t *)&leader_data, sizeof(leader_data));
    
    if (result == ESP_OK) {
        ESP_LOGD(TAG, "Broadcast sent: pos(%.2f, %.2f, %.2f)", 
                 leader_data.x, leader_data.y, leader_data.z);
    } else {
        ESP_LOGW(TAG, "Broadcast failed: %s", esp_err_to_name(result));
    }
}

/**
 * Update leader position (placeholder - integrate with actual drone position)
 */
static void update_leader_position(void)
{
    // TODO: Replace with actual position data from drone sensors/flight controller
    // For now, simulate a circular path
    static float angle = 0.0f;
    static uint32_t update_count = 0;
    
    update_count++;
    
    // Simple circular motion simulation
    if (update_count > 100) {  // Update every 10 seconds
        angle += 0.1f;
        if (angle > 2 * 3.14159f) {
            angle = 0.0f;
        }
        
        // Simulate circular path with 0.5m radius
        leader_data.x = 0.5f * cosf(angle);
        leader_data.y = 0.5f * sinf(angle);
        leader_data.z = 0.35f;  // Hold at 35cm
        
        // Simulate velocity
        leader_data.vx = -0.5f * sinf(angle) * 0.1f;
        leader_data.vy = 0.5f * cosf(angle) * 0.1f;
        leader_data.vz = 0.0f;
        
        update_count = 0;
    }
}

/**
 * Main broadcast task
 */
static void broadcast_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Broadcast task started");
    
    while (1) {
        // Update position
        update_leader_position();
        
        // Broadcast position
        broadcast_leader_position();
        
        // Wait for next broadcast interval
        vTaskDelay(pdMS_TO_TICKS(BROADCAST_INTERVAL_MS));
    }
}

/**
 * Main application entry point
 */
void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(TAG, "ESP32 Leader Drone Broadcaster Starting");
    
    // Initialize WiFi and ESP-NOW
    wifi_init_espnow();
    esp_now_init();
    
    // Create broadcast task
    xTaskCreate(broadcast_task, "broadcast_task", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "Leader drone broadcaster initialized successfully");
}
