/**
 * Simple Working VL53L0X Driver
 * Based on your working library approach
 * Uses single measurement polling - no continuous mode issues
 */

#include <stddef.h>
#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "vl53l0x_enhanced.h"
#include "i2cdev.h"
#include "i2c_drv.h"
#include "flight_ready_state.h"
#define DEBUG_MODULE "VLX_ENH" 
#include "debug_cf.h"

// Driver constants  
#define VL53L0X_I2C_ADDR                    0x29  // Default I2C address
#define VL53L0X_MODEL_ID_FULL               0xEEAA  // Expected model ID
#define VL53L0X_READING_INTERVAL_MS         100   // Polling interval (reduced for I2C stability)
#define VL53L0X_MAX_DISTANCE_MM             8000  // 8 meters max
#define VL53L0X_MIN_DISTANCE_MM             10    // 1 cm min

// Register definitions (minimal set needed)
#define VL53L0X_REG_IDENTIFICATION_MODEL_ID 0xC0
#define VL53L0X_REG_SYSRANGE_START          0x00
#define VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR  0x0B
#define VL53L0X_REG_RESULT_INTERRUPT_STATUS 0x13
#define VL53L0X_REG_RESULT_RANGE_STATUS     0x14

// Task management
static TaskHandle_t vl53l0x_task_handle = NULL;
static bool is_init = false;
static bool sensor_available = false;

// Sensor data
static uint16_t current_distance_mm = 0;
static uint32_t last_valid_reading_time = 0;
static uint32_t total_readings = 0;
static uint32_t valid_readings = 0;

// Configuration
static float calibration_offset_mm = 0.0f;

// Static device instance - simplified approach
static struct {
    I2C_Dev *i2cDev;
    uint8_t address;
    bool initialized;
} vl53l0x_device = {0};

// Helper function for timing
static uint32_t get_time_ms(void) {
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

// Forward declarations
static bool vl53l0x_take_single_measurement(uint16_t* distance_mm);
static void vl53l0x_task(void* parameters);

// Public API functions for altitude logger integration
bool vl53l0x_simple_get_distance(uint16_t* distance_mm, bool* valid) {
    if (!sensor_available || !distance_mm || !valid) {
        return false;
    }
    
    *distance_mm = current_distance_mm;
    *valid = (current_distance_mm > 0);
    return true;
}

bool vl53l0x_simple_is_available(void) {
    ESP_LOGD(DEBUG_MODULE, "Simple sensor availability check: sensor_available=%d", sensor_available);
    return sensor_available;
}

// I2C helper functions
static bool vl53l0x_read_reg8(uint8_t reg, uint8_t* data) {
    return i2cdevReadByte(vl53l0x_device.i2cDev, vl53l0x_device.address, reg, data);
}

static bool vl53l0x_write_reg8(uint8_t reg, uint8_t data) {
    return i2cdevWriteByte(vl53l0x_device.i2cDev, vl53l0x_device.address, reg, data);
}

static bool vl53l0x_read_reg16(uint8_t reg, uint16_t* data) {
    uint8_t buffer[2];
    if (i2cdevReadReg8(vl53l0x_device.i2cDev, vl53l0x_device.address, reg, 2, buffer)) {
        *data = (buffer[0] << 8) | buffer[1];
        return true;
    }
    return false;
}

// I2C Scanner - automatically detect device address
static uint8_t vl53l0x_scan_i2c_bus(void) {
    ESP_LOGI(DEBUG_MODULE, "Scanning I2C bus for VL53L0X sensor...");
    ESP_LOGI(DEBUG_MODULE, "I2C Configuration: SDA=GPIO21, SCL=GPIO22");
    
    // Common VL53L0X addresses to try (including more possibilities)
    uint8_t addresses_to_try[] = {0x29, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36};
    int num_addresses = sizeof(addresses_to_try) / sizeof(addresses_to_try[0]);
    
    for (int i = 0; i < num_addresses; i++) {
        uint8_t addr = addresses_to_try[i];
        ESP_LOGI(DEBUG_MODULE, "Trying I2C address 0x%02X...", addr);
        
        // Temporarily set address for testing
        uint8_t old_addr = vl53l0x_device.address;
        vl53l0x_device.address = addr;
        
        // Try to read model ID register
        uint16_t model_id;
        if (vl53l0x_read_reg16(VL53L0X_REG_IDENTIFICATION_MODEL_ID, &model_id)) {
            ESP_LOGI(DEBUG_MODULE, "Response from 0x%02X: Model ID = 0x%04X", addr, model_id);
            
            // Check if it's a valid VL53L0X
            if (model_id == VL53L0X_MODEL_ID_FULL || model_id == 0xEEAA) {
                ESP_LOGI(DEBUG_MODULE, "✓ Found VL53L0X sensor at I2C address 0x%02X!", addr);
                return addr;
            }
            
            // Check for other possible model IDs
            if ((model_id & 0xFF00) == 0xEE00) {
                ESP_LOGI(DEBUG_MODULE, "✓ Found possible VL53L0X variant at 0x%02X (ID: 0x%04X)", addr, model_id);
                return addr;
            }
        } else {
            ESP_LOGD(DEBUG_MODULE, "No response from address 0x%02X", addr);
        }
        
        // Restore old address
        vl53l0x_device.address = old_addr;
        vTaskDelay(pdMS_TO_TICKS(50)); // Small delay between attempts
    }
    
    // If we get here, try a full bus scan
    ESP_LOGI(DEBUG_MODULE, "Standard addresses failed, performing full bus scan...");
    
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        vl53l0x_device.address = addr;
        
        // Try a simple read to see if device responds
        uint8_t dummy;
        if (i2cdevReadByte(vl53l0x_device.i2cDev, addr, 0x00, &dummy)) {
            ESP_LOGI(DEBUG_MODULE, "Device found at address 0x%02X", addr);
            
            // Try to read model ID
            uint16_t model_id;
            if (vl53l0x_read_reg16(VL53L0X_REG_IDENTIFICATION_MODEL_ID, &model_id)) {
                ESP_LOGI(DEBUG_MODULE, "Address 0x%02X - Model ID: 0x%04X", addr, model_id);
                
                if (model_id == VL53L0X_MODEL_ID_FULL || (model_id & 0xFF00) == 0xEE00) {
                    ESP_LOGI(DEBUG_MODULE, "✓ VL53L0X sensor detected at address 0x%02X!", addr);
                    return addr;
                }
            }
        }
        
        if (addr % 16 == 0) {
            vTaskDelay(pdMS_TO_TICKS(1)); // Small delay every 16 addresses
        }
    }
    
    return 0; // Not found
}

// Simple sensor initialization 
bool vl53l0x_simple_init(void) {
    if (is_init) {
        return sensor_available;
    }
    
    ESP_LOGI(DEBUG_MODULE, "Initializing VL53L0X with auto-detection...");
    
    // Initialize the device structure first
    vl53l0x_device.i2cDev = I2C0_DEV;
    vl53l0x_device.address = VL53L0X_I2C_ADDR; // Default address
    vl53l0x_device.initialized = false;
    
    // Auto-detect the sensor address
    uint8_t detected_addr = vl53l0x_scan_i2c_bus();
    
    if (detected_addr == 0) {
        ESP_LOGE(DEBUG_MODULE, "❌ No VL53L0X sensor found on I2C bus!");
        ESP_LOGE(DEBUG_MODULE, "Performing full I2C bus scan...");
        
        // Full I2C bus scan
        for (uint8_t addr = 0x08; addr < 0x78; addr++) {
            uint8_t test_byte;
            if (vl53l0x_read_reg8(0x00, &test_byte)) {
                ESP_LOGI(DEBUG_MODULE, "Found device at address 0x%02X", addr);
            }
        }
        
        ESP_LOGE(DEBUG_MODULE, "Check your wiring:");
        ESP_LOGE(DEBUG_MODULE, "  - SDA = GPIO21");
        ESP_LOGE(DEBUG_MODULE, "  - SCL = GPIO22");
        ESP_LOGE(DEBUG_MODULE, "  - VCC = 3.3V");
        ESP_LOGE(DEBUG_MODULE, "  - GND = Ground");
        ESP_LOGE(DEBUG_MODULE, "  - XSHUT = 3.3V (if available)");
        return false;
    }
    
    // Update device address with detected address
    vl53l0x_device.address = detected_addr;
    
    // Read and verify model ID one more time
    uint16_t model_id;
    if (!vl53l0x_read_reg16(VL53L0X_REG_IDENTIFICATION_MODEL_ID, &model_id)) {
        ESP_LOGE(DEBUG_MODULE, "Failed to read model ID from detected sensor");
        return false;
    }
    
    ESP_LOGI(DEBUG_MODULE, "VL53L0X Model ID: 0x%04X (expected: 0x%04X)", model_id, VL53L0X_MODEL_ID_FULL);
    
    if (model_id != VL53L0X_MODEL_ID_FULL) {
        ESP_LOGE(DEBUG_MODULE, "Model ID mismatch! Got 0x%04X, expected 0x%04X", model_id, VL53L0X_MODEL_ID_FULL);
        return false;
    }
    
    ESP_LOGI(DEBUG_MODULE, "VL53L0X sensor detected successfully!");
    ESP_LOGI(DEBUG_MODULE, "VL53L0X detected, performing basic setup...");
    
    // Simplified VL53L0X initialization - just clear interrupts and wait
    // The sensor should work with minimal initialization
    if (!vl53l0x_write_reg8(VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01)) {
        ESP_LOGE(DEBUG_MODULE, "Failed to clear system interrupts");
        return false;
    }
    
    // Wait for sensor to stabilize
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_LOGI(DEBUG_MODULE, "Basic initialization complete, sensor should be ready");
    
    ESP_LOGI(DEBUG_MODULE, "Sensor initialized successfully!");
    ESP_LOGI(DEBUG_MODULE, "Calibration offset: %.1f mm", calibration_offset_mm);
    
    // Test the sensor with a single measurement
    uint16_t test_distance;
    ESP_LOGI(DEBUG_MODULE, "Testing sensor communication...");
    
    // First, try to read a known register to verify communication
    uint8_t test_byte;
    if (vl53l0x_read_reg8(0xC0, &test_byte)) {
        ESP_LOGI(DEBUG_MODULE, "✓ I2C communication working, read 0xC0: 0x%02X", test_byte);
    } else {
        ESP_LOGE(DEBUG_MODULE, "✗ I2C communication failed - check wiring!");
        return false;
    }
    
    if (vl53l0x_take_single_measurement(&test_distance)) {
        ESP_LOGI(DEBUG_MODULE, "✓ Test measurement successful: %d mm", test_distance);
    } else {
        ESP_LOGW(DEBUG_MODULE, "⚠ Test measurement failed - sensor may need more time to stabilize");
    }
    
    ESP_LOGI(DEBUG_MODULE, "Ready to measure ground distance in centimeters");
    ESP_LOGI(DEBUG_MODULE, "Format: [Status] Distance: XX.X cm");
    ESP_LOGI(DEBUG_MODULE, "-----------------------------------------");
    
    // Create the sensor task
    BaseType_t result = xTaskCreate(vl53l0x_task, 
                                    "VL53L0X_SIMPLE", 
                                    2048, 
                                    NULL, 
                                    3,  // Priority
                                    &vl53l0x_task_handle);
    
    if (result != pdPASS) {
        ESP_LOGE(DEBUG_MODULE, "Failed to create VL53L0X task");
        return false;
    }
    
    // Mark sensor as available and initialized
    sensor_available = true;
    vl53l0x_device.initialized = true;
    is_init = true;
    
    ESP_LOGI(DEBUG_MODULE, "VL53L0X simple sensor initialized successfully!");
    
    return true;
}

// Single measurement function - simplified approach
static bool vl53l0x_take_single_measurement(uint16_t* distance_mm) {
    ESP_LOGD(DEBUG_MODULE, "Starting measurement...");
    
    // Clear any previous interrupt
    if (!vl53l0x_write_reg8(VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01)) {
        ESP_LOGD(DEBUG_MODULE, "Failed to clear interrupts");
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Start single measurement 
    if (!vl53l0x_write_reg8(VL53L0X_REG_SYSRANGE_START, 0x01)) {
        ESP_LOGD(DEBUG_MODULE, "Failed to start measurement");
        return false;
    }
    ESP_LOGD(DEBUG_MODULE, "Measurement started, waiting...");
    
    // Simple wait approach - VL53L0X takes about 30ms for measurement
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Read the distance directly from the result register
    uint16_t raw_distance;
    if (!vl53l0x_read_reg16(0x1E, &raw_distance)) { // 0x1E is the distance register
        ESP_LOGD(DEBUG_MODULE, "Failed to read distance from register 0x1E");
        return false;
    }
    
    ESP_LOGD(DEBUG_MODULE, "Raw distance read: %d mm", raw_distance);
    
    // Convert from 16-bit to actual distance
    // VL53L0X returns distance in mm, but we need to check the format
    if (raw_distance == 0 || raw_distance > VL53L0X_MAX_DISTANCE_MM) {
        ESP_LOGD(DEBUG_MODULE, "Invalid reading: %d mm", raw_distance);
        return false;
    }
    
    *distance_mm = raw_distance;
    ESP_LOGD(DEBUG_MODULE, "Valid measurement: %d mm", raw_distance);
    
    // Clear interrupt after reading
    vl53l0x_write_reg8(VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
    
    return true;
}

// Main sensor task - simple polling loop like your working example
static void vl53l0x_task(void* parameters) {
    ESP_LOGI(DEBUG_MODULE, "VL53L0X sensor task started - waiting for flight ready state...");
    
    // Wait for the system to be ready to fly before starting altitude measurements
    // This prevents sensor interference during motor calibration
    if (!flightReadyStateWaitReady(portMAX_DELAY)) {
        ESP_LOGE(DEBUG_MODULE, "Failed to wait for flight ready state - exiting task");
        return;
    }
    
    ESP_LOGI(DEBUG_MODULE, "🚁 Flight ready confirmed - starting VL53L0X simple measurements!");
    
    // Give the system a moment to stabilize after flight ready state
    // This prevents interference from motor calibration and other systems
    ESP_LOGI(DEBUG_MODULE, "⏳ Waiting for system to stabilize after flight ready...");
    vTaskDelay(pdMS_TO_TICKS(2000)); // 2 second stabilization period
    
    ESP_LOGI(DEBUG_MODULE, "✅ System stabilized - beginning VL53L0X measurements!");
    
    TickType_t last_wake_time = xTaskGetTickCount();
    
    while (true) {
        if (!sensor_available) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        
        uint16_t raw_distance_mm = 0;
        total_readings++;
        
        // Take a single measurement 
        bool success = vl53l0x_take_single_measurement(&raw_distance_mm);
        
        ESP_LOGI(DEBUG_MODULE, "Raw: %d mm, Got: %s", raw_distance_mm, success ? "YES" : "NO");
        
        // Check if measurement is valid
        if (success && raw_distance_mm > VL53L0X_MIN_DISTANCE_MM && raw_distance_mm < VL53L0X_MAX_DISTANCE_MM) {
            
            // Apply calibration offset
            float calibrated_distance_mm = (float)raw_distance_mm + calibration_offset_mm;
            
            if (calibrated_distance_mm > 0) {
                current_distance_mm = (uint16_t)calibrated_distance_mm;
                last_valid_reading_time = get_time_ms();
                valid_readings++;
                
                ESP_LOGI(DEBUG_MODULE, "[VALID] Distance: %.1f cm", calibrated_distance_mm / 10.0f);
            } else {
                ESP_LOGI(DEBUG_MODULE, "[CALIBRATED OUT] Calibration resulted in negative distance");
            }
        } else {
            ESP_LOGI(DEBUG_MODULE, "[RETRY] Will try again next time - raw was %d mm", raw_distance_mm);
            
            // If we get too many consecutive 0 readings, give the sensor a brief rest
            static uint8_t consecutive_zeros = 0;
            if (raw_distance_mm == 0) {
                consecutive_zeros++;
                if (consecutive_zeros >= 5) {
                    ESP_LOGD(DEBUG_MODULE, "Too many zeros, giving sensor a brief rest...");
                    vTaskDelay(pdMS_TO_TICKS(500)); // Longer 500ms rest for I2C recovery
                    
                    // Try to reinitialize the sensor if it's been failing
                    if (consecutive_zeros >= 10) {
                        ESP_LOGI(DEBUG_MODULE, "Attempting sensor reinitialization...");
                        vl53l0x_simple_init();
                        consecutive_zeros = 0;
                    }
                }
            } else {
                consecutive_zeros = 0;
            }
        }
        
        // Sleep until next measurement
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(VL53L0X_READING_INTERVAL_MS));
    }
}

// Public API functions to match the existing interface

void vl53l0xEnhancedInit(void) {
    if (is_init) {
        return;
    }
    
    ESP_LOGI(DEBUG_MODULE, "Enhanced VL53L0X Ground Distance Sensor");
    ESP_LOGI(DEBUG_MODULE, "=====================================");
    
    // Initialize device structure
    vl53l0x_device.i2cDev = I2C0_DEV;  // Use existing I2C0 device
    vl53l0x_device.address = VL53L0X_I2C_ADDR;
    
    // Initialize I2C 
    i2cdevInit(vl53l0x_device.i2cDev);
    
    // Try to initialize the sensor
    sensor_available = vl53l0x_simple_init();
    
    if (sensor_available) {
        vl53l0x_device.initialized = true;
        
        // Create the sensor task
        BaseType_t result = xTaskCreate(vl53l0x_task, 
                                        "VL53L0X_SIMPLE", 
                                        2048, 
                                        NULL, 
                                        3,  // Priority
                                        &vl53l0x_task_handle);
        
        if (result != pdPASS) {
            ESP_LOGE(DEBUG_MODULE, "Failed to create VL53L0X task");
            sensor_available = false;
            vl53l0x_device.initialized = false;
        }
    }
    
    is_init = true;
}

bool vl53l0xEnhancedTest(void) {
    return sensor_available && vl53l0x_device.initialized;
}

float vl53l0xEnhancedGetDistance(void) {
    if (!sensor_available || !vl53l0x_device.initialized) {
        return -1.0f;
    }
    
    uint32_t current_time = get_time_ms();
    
    // Check if we have recent data (within last 200ms)
    if (current_time - last_valid_reading_time > 200) {
        return -1.0f; // Stale data
    }
    
    return (float)current_distance_mm / 10.0f; // Convert mm to cm
}

void vl53l0xEnhancedGetStats(uint32_t* total, uint32_t* valid) {
    if (total) *total = total_readings;
    if (valid) *valid = valid_readings;
}

void vl53l0xEnhancedSetCalibration(float offset_mm) {
    calibration_offset_mm = offset_mm;
    ESP_LOGI(DEBUG_MODULE, "Calibration offset set to %.1f mm", offset_mm);
}

// Functions needed by altitude_logger and other modules
// These provide compatibility with the old API

bool vl53l0x_init(VL53L0X_Dev_t *dev, I2C_Dev *i2cDev, float calibration_offset) {
    // Initialize using the new simple approach
    vl53l0xEnhancedInit();
    calibration_offset_mm = calibration_offset;
    
    // Fill device structure for compatibility (though we don't really use it)
    if (dev) {
        memset(dev, 0, sizeof(VL53L0X_Dev_t));
        dev->i2cDev = i2cDev;
        dev->devAddr = VL53L0X_I2C_ADDR;
        dev->calibration_offset = calibration_offset;
        dev->initialized = vl53l0xEnhancedTest();
    }
    
    return vl53l0xEnhancedTest();
}

bool vl53l0x_get_distance(VL53L0X_Dev_t *dev, float *distance_cm, const char **status) {
    // Use our simple distance function
    float distance = vl53l0xEnhancedGetDistance();
    
    if (distance > 0) {
        *distance_cm = distance;
        if (status) *status = "VALID";
        return true;
    } else {
        *distance_cm = 0.0f;
        if (status) *status = "ERROR";
        return false;
    }
}

// Additional compatibility functions that might be needed
bool vl53l0x_test_connection(VL53L0X_Dev_t *dev) {
    return vl53l0xEnhancedTest();
}
