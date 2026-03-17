#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "wifi_station.h"
#include "web_server.h"
#include "camera.h"

static const char* TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "🚀 ESP32-CAM Drone Starting...");
    ESP_LOGI(TAG, "💾 Free heap: %lu bytes", esp_get_free_heap_size());
    
    // Initialize camera first
    esp_err_t ret = camera_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Camera initialization failed");
        return;
    }
    
    // Start WiFi station mode (connect to router)
    ret = wifi_station_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ WiFi Station initialization failed");
        return;
    }
    
    // Wait a moment for WiFi to stabilize
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Start web server
    ret = web_server_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ Web server failed to start");
        return;
    }
    
    ESP_LOGI(TAG, "✅ ESP32-CAM Drone Ready!");
    ESP_LOGI(TAG, "🌐 Connected to: GNXS-2.4G-854460");
    ESP_LOGI(TAG, "🔗 Web Interface: http://<your-ip>:80");
    ESP_LOGI(TAG, "📺 Camera Stream: http://<your-ip>:80/stream");
    ESP_LOGI(TAG, "");
    
    // Main loop - monitor system status
    while (1) {
        ESP_LOGI(TAG, "💾 Free heap: %lu bytes | 📡 WiFi: Station Mode | 📹 Camera: %s", 
                 esp_get_free_heap_size(),
                 camera_is_initialized() ? "Online" : "Offline");
        
        vTaskDelay(pdMS_TO_TICKS(60000)); // Status every 60 seconds
    }
}