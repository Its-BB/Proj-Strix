// LEADER_CODE.c
// Enhanced ESP32 Leader: Periodically broadcasts packets using ESP-NOW
// Compatible with ESP-Drone Leader Following System
// Copy this to your leader ESP32 project

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#define TAG "LEADER"
#define ESPNOW_CHANNEL 1
#define BROADCAST_MAC {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}
#define BROADCAST_INTERVAL_MS 200

// Status LED pin
#define STATUS_LED_PIN 2

static uint8_t broadcast_mac[] = BROADCAST_MAC;
static uint32_t packet_sequence = 0;
static bool leader_active = true;

// Enhanced packet structure matching drone expectations
typedef struct __attribute__((packed)) {
    uint8_t type;           // Packet type (0x42 for beacon)
    uint32_t sequence;      // Sequence number for packet tracking
    uint32_t timestamp;     // Timestamp from leader
    uint8_t battery_level;  // Leader battery level (0-100)
    uint8_t signal_strength; // Self-reported signal strength
    uint16_t status_flags;   // Status flags (bit 0: active, bit 1: emergency, etc.)
    uint8_t reserved[4];     // Reserved for future use
} leader_packet_t;

// Status flags
#define STATUS_FLAG_ACTIVE    (1 << 0)
#define STATUS_FLAG_EMERGENCY (1 << 1)
#define STATUS_FLAG_LOW_BATTERY (1 << 2)

void status_led_task(void *pvParameter) {
    gpio_set_direction(STATUS_LED_PIN, GPIO_MODE_OUTPUT);
    
    while (1) {
        if (leader_active) {
            // Fast blink when active
            gpio_set_level(STATUS_LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(100));
            gpio_set_level(STATUS_LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            // Slow blink when inactive
            gpio_set_level(STATUS_LED_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(500));
            gpio_set_level(STATUS_LED_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

void send_broadcast_task(void *pvParameter) {
    leader_packet_t packet = {0};
    
    while (1) {
        if (leader_active) {
            // Prepare packet
            packet.type = 0x42;
            packet.sequence = ++packet_sequence;
            packet.timestamp = esp_timer_get_time() / 1000; // Convert to milliseconds
            packet.battery_level = 95; // Simulated battery level
            packet.signal_strength = 85; // Simulated signal strength
            packet.status_flags = STATUS_FLAG_ACTIVE;
            
            // Add low battery flag if needed (simulated)
            if (packet.battery_level < 20) {
                packet.status_flags |= STATUS_FLAG_LOW_BATTERY;
            }
            
            esp_err_t result = esp_now_send(broadcast_mac, (uint8_t*)&packet, sizeof(packet));
            if (result == ESP_OK) {
                ESP_LOGI(TAG, "Broadcast sent - Seq: %lu, Time: %lu", 
                        packet.sequence, packet.timestamp);
            } else {
                ESP_LOGE(TAG, "Broadcast failed: %d", result);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(BROADCAST_INTERVAL_MS));
    }
}

// ESP-NOW send callback
void on_data_sent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        // Packet sent successfully
    } else {
        ESP_LOGW(TAG, "Send failed");
    }
}

// ESP-NOW receive callback (for potential bidirectional communication)
void on_data_recv(const uint8_t *mac_addr, const uint8_t *data, int len) {
    ESP_LOGI(TAG, "Received data from drone, length: %d", len);
    
    // Handle any commands from drone if needed
    if (len > 0) {
        switch (data[0]) {
            case 0x01: // Emergency stop command
                leader_active = false;
                ESP_LOGW(TAG, "Emergency stop received from drone");
                break;
            case 0x02: // Resume command
                leader_active = true;
                ESP_LOGI(TAG, "Resume command received from drone");
                break;
            default:
                ESP_LOGD(TAG, "Unknown command: 0x%02X", data[0]);
                break;
        }
    }
}

void wifi_init() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));
    ESP_LOGI(TAG, "WiFi initialized on channel %d", ESPNOW_CHANNEL);
}

void espnow_init() {
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t *)"pmk1234567890123"));
    
    // Register callbacks
    ESP_ERROR_CHECK(esp_now_register_send_cb(on_data_sent));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(on_data_recv));
    
    // Add broadcast peer
    esp_now_peer_info_t peerInfo = {0};
    memcpy(peerInfo.peer_addr, broadcast_mac, 6);
    peerInfo.channel = ESPNOW_CHANNEL;
    peerInfo.ifidx = WIFI_IF_STA;
    peerInfo.encrypt = false;
    ESP_ERROR_CHECK(esp_now_add_peer(&peerInfo));
    
    ESP_LOGI(TAG, "ESP-NOW initialized successfully");
}

void app_main() {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(TAG, "ESP32 Leader starting...");
    ESP_LOGI(TAG, "Firmware version: 1.0.0");
    ESP_LOGI(TAG, "Broadcast interval: %d ms", BROADCAST_INTERVAL_MS);
    ESP_LOGI(TAG, "ESP-NOW channel: %d", ESPNOW_CHANNEL);
    
    // Initialize WiFi and ESP-NOW
    wifi_init();
    espnow_init();
    
    // Create tasks
    xTaskCreate(send_broadcast_task, "send_broadcast_task", 4096, NULL, 5, NULL);
    xTaskCreate(status_led_task, "status_led_task", 2048, NULL, 3, NULL);
    
    ESP_LOGI(TAG, "Leader is now broadcasting...");
    ESP_LOGI(TAG, "Status LED on GPIO %d", STATUS_LED_PIN);
    ESP_LOGI(TAG, "Send emergency stop: 0x01, Resume: 0x02");
    
    // Main loop for monitoring
    while (1) {
        // Print status every 10 seconds
        ESP_LOGI(TAG, "Leader active: %s, Packets sent: %lu", 
                leader_active ? "YES" : "NO", packet_sequence);
        
        // Simulate battery drain (for demonstration)
        if (packet_sequence % 100 == 0 && packet_sequence > 0) {
            ESP_LOGD(TAG, "Simulated battery check - Still healthy");
        }
        
        vTaskDelay(pdMS_TO_TICKS(10000)); // 10 second status updates
    }
}
