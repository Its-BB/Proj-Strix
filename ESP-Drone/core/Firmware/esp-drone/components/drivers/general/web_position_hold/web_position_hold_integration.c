/**
 * ESP-Drone Web Position Hold Integration
 * 
 * This file integrates the web position hold system with the existing drone firmware.
 * It should be called from the main system initialization after VL53L0X is ready.
 */

#include "web_position_hold.h"
#include "altitude_logger.h"
#include "vl53l0x_enhanced.h"
#include "i2cdev.h"

#define DEBUG_MODULE "WEB_PH_INIT"
#include "debug_cf.h"

static bool web_position_hold_integrated = false;

/**
 * Function to get distance reading from altitude logger
 * This is a safer approach than sharing the I2C device directly
 */
extern float altitudeLoggerGetDistance(void);
extern bool altitudeLoggerIsValid(void);

// Dummy VL53L0X device for compatibility
static VL53L0X_Dev_t dummy_vl53l0x_dev;

/**
 * Initialize the web position hold system
 * This should be called after altitude logger initialization
 */
bool web_position_hold_integration_init(void) {
    if (web_position_hold_integrated) {
        ESP_LOGW(DEBUG_MODULE, "Web position hold already integrated");
        return true;
    }
    
    ESP_LOGI(DEBUG_MODULE, "Starting Web Position Hold Integration");
    
    // Check if altitude logger is ready 
    if (!altitudeLoggerIsReady()) {
        ESP_LOGW(DEBUG_MODULE, "Altitude logger not ready, waiting...");
        // Wait longer for altitude logger to initialize and sensor to be available
        for (int i = 0; i < 10; i++) {
            vTaskDelay(pdMS_TO_TICKS(1000)); // Wait 1 second each time
            ESP_LOGI(DEBUG_MODULE, "Waiting for altitude logger... attempt %d/10", i + 1);
            if (altitudeLoggerIsReady()) {
                ESP_LOGI(DEBUG_MODULE, "Altitude logger is now ready!");
                break;
            }
        }
        
        if (!altitudeLoggerIsReady()) {
            ESP_LOGE(DEBUG_MODULE, "Altitude logger still not ready after 10 seconds");
            ESP_LOGE(DEBUG_MODULE, "This usually means the VL53L0X sensor is not connected or not working");
            return false;
        }
    }
    
    ESP_LOGI(DEBUG_MODULE, "Altitude logger is ready, using shared sensor data");
    
    // Use dummy device - we'll get readings from altitude logger
    memset(&dummy_vl53l0x_dev, 0, sizeof(VL53L0X_Dev_t));
    VL53L0X_Dev_t *vl53l0x_device = &dummy_vl53l0x_dev;
    
    // Initialize web position hold system
    if (!web_position_hold_init(vl53l0x_device)) {
        ESP_LOGE(DEBUG_MODULE, "Failed to initialize web position hold system");
        return false;
    }
    
    // Start web server
    if (!web_position_hold_start_server()) {
        ESP_LOGE(DEBUG_MODULE, "Failed to start web server");
        return false;
    }
    
    web_position_hold_integrated = true;
    
    ESP_LOGI(DEBUG_MODULE, "🎉 Web Position Hold Integration Complete!");
    ESP_LOGI(DEBUG_MODULE, "📱 Access the web interface at: http://192.168.43.42/");
    ESP_LOGI(DEBUG_MODULE, "🚁 Connect to WiFi hotspot and navigate to the IP address");
    
    return true;
}

/**
 * Test the web position hold integration
 */
bool web_position_hold_integration_test(void) {
    if (!web_position_hold_integrated) {
        ESP_LOGE(DEBUG_MODULE, "Web position hold not integrated");
        return false;
    }
    
    return web_position_hold_test();
}

/**
 * Check if web position hold is integrated and running
 */
bool web_position_hold_integration_is_ready(void) {
    return web_position_hold_integrated;
}
