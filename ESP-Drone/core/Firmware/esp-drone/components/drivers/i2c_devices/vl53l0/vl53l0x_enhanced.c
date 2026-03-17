/**
 * Enhanced VL53L0X Ground Distance Sensor Driver
 * Simple, reliable version that uses single measurement polling
 * Avoids continuous mode issues that cause sensor hangs
 */

#include <math.h>
#include <string.h>
#include <stdio.h>
#include "vl53l0x_enhanced.h"
#include "i2cdev.h"
#define DEBUG_MODULE "VLX_ENH"
#include "debug_cf.h"

// Internal register definitions for VL53L0X
#define VL53L0X_REG_IDENTIFICATION_MODEL_ID     0xC0
#define VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR      0x0B
#define VL53L0X_REG_SYSRANGE_START              0x00
#define VL53L0X_REG_RESULT_RANGE_STATUS         0x14
#define VL53L0X_REG_RESULT_INTERRUPT_STATUS     0x13
#define VL53L0X_REG_I2C_SLAVE_DEVICE_ADDRESS    0x8A

// Expected model ID for VL53L0X
#define VL53L0X_EXPECTED_DEVICE_ID              0xEEAA

// Status definitions
static const char* STATUS_VALID = "VALID";
static const char* STATUS_ERROR = "ERROR";

// Helper function to read 16-bit register
static bool read_reg16(VL53L0X_Dev_t *dev, uint8_t reg, uint16_t *value) {
    uint8_t data[2];
    if (i2cdevReadReg8(dev->i2cDev, dev->devAddr, reg, 2, data)) {
        *value = (data[0] << 8) | data[1];
        return true;
    }
    return false;
}

// Helper function to write single byte register
static bool write_reg8(VL53L0X_Dev_t *dev, uint8_t reg, uint8_t value) {
    return i2cdevWriteByte(dev->i2cDev, dev->devAddr, reg, value);
}

// Helper function to read single byte register
static bool read_reg8(VL53L0X_Dev_t *dev, uint8_t reg, uint8_t *value) {
    return i2cdevReadByte(dev->i2cDev, dev->devAddr, reg, value);
}

// Simple helper function to get current time
static uint32_t get_time_ms(void) {
    return pdTICKS_TO_MS(xTaskGetTickCount());
}

bool vl53l0x_init(VL53L0X_Dev_t *dev, I2C_Dev *i2cDev, float calibration_offset) {
    if (!dev || !i2cDev) {
        return false;
    }

    // Initialize device structure
    memset(dev, 0, sizeof(VL53L0X_Dev_t));
    dev->i2cDev = i2cDev;
    dev->devAddr = VL53L0X_DEFAULT_ADDRESS;
    dev->calibration_offset = calibration_offset;
    dev->measurement_timing_budget_us = VL53L0X_TIMING_BUDGET_MS * 1000;
    
    // Initialize I2C
    i2cdevInit(dev->i2cDev);

    ESP_LOGI(DEBUG_MODULE, "Enhanced VL53L0X Ground Distance Sensor");
    ESP_LOGI(DEBUG_MODULE, "=====================================");

    // Test connection first
    if (!vl53l0x_test_connection(dev)) {
        ESP_LOGE(DEBUG_MODULE, "ERROR: Failed to initialize VL53L0X sensor!");
        ESP_LOGE(DEBUG_MODULE, "Check your wiring and connections.");
        return false;
    }

    // Simplified initialization - just set basic mode
    // Most complex initialization is handled by the sensor's defaults
    ESP_LOGI(DEBUG_MODULE, "VL53L0X detected, performing basic setup...");
    
    // Clear any pending interrupts
    write_reg8(dev, VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Initialize buffer
    for (int i = 0; i < VL53L0X_MEASUREMENT_SAMPLES; i++) {
        dev->distance_buffer[i] = 0.0f;
    }

    ESP_LOGI(DEBUG_MODULE, "Sensor initialized successfully!");
    ESP_LOGI(DEBUG_MODULE, "Calibration offset: %.1f mm", dev->calibration_offset);
    ESP_LOGI(DEBUG_MODULE, "Ready to measure ground distance in centimeters");
    ESP_LOGI(DEBUG_MODULE, "Format: [Status] Distance: XX.X cm");
    ESP_LOGI(DEBUG_MODULE, "-----------------------------------------");

    dev->initialized = true;
    return true;
}

bool vl53l0x_test_connection(VL53L0X_Dev_t *dev) {
    if (!dev) {
        ESP_LOGE(DEBUG_MODULE, "Device pointer is NULL");
        return false;
    }

    ESP_LOGI(DEBUG_MODULE, "Testing VL53L0X connection at I2C address 0x%02X", dev->devAddr);
    
    uint16_t model_id;
    if (read_reg16(dev, VL53L0X_REG_IDENTIFICATION_MODEL_ID, &model_id)) {
        ESP_LOGI(DEBUG_MODULE, "VL53L0X Model ID: 0x%04X (expected: 0x%04X)", model_id, VL53L0X_EXPECTED_DEVICE_ID);
        if (model_id == VL53L0X_EXPECTED_DEVICE_ID) {
            ESP_LOGI(DEBUG_MODULE, "VL53L0X sensor detected successfully!");
            return true;
        } else {
            ESP_LOGW(DEBUG_MODULE, "Model ID mismatch - sensor might not be VL53L0X");
            return false;
        }
    } else {
        ESP_LOGE(DEBUG_MODULE, "Failed to read VL53L0X model ID - I2C communication error");
        ESP_LOGE(DEBUG_MODULE, "Check your wiring: SDA=GPIO21, SCL=GPIO22, VCC=3.3V, GND=GND");
    }
    
    return false;
}

bool vl53l0x_start_continuous(VL53L0X_Dev_t *dev) {
    if (!dev || !dev->initialized) {
        return false;
    }

    // Start continuous ranging
    if (write_reg8(dev, VL53L0X_REG_SYSRANGE_START, 0x02)) {
        dev->continuous_mode = true;
        ESP_LOGI(DEBUG_MODULE, "Started continuous measurements");
        return true;
    }
    
    return false;
}

bool vl53l0x_stop_continuous(VL53L0X_Dev_t *dev) {
    if (!dev || !dev->initialized) {
        return false;
    }

    // Stop continuous ranging
    if (write_reg8(dev, VL53L0X_REG_SYSRANGE_START, 0x01)) {
        dev->continuous_mode = false;
        ESP_LOGI(DEBUG_MODULE, "Stopped continuous measurements");
        return true;
    }
    
    return false;
}

bool vl53l0x_data_ready(VL53L0X_Dev_t *dev) {
    if (!dev || !dev->initialized) {
        return false;
    }

    uint8_t status;
    // Check interrupt status register
    if (read_reg8(dev, VL53L0X_REG_RESULT_INTERRUPT_STATUS, &status)) {
        // Bit 2:0 indicate interrupt status, bit 0 = new sample ready
        bool interrupt_ready = (status & 0x07) != 0;
        if (interrupt_ready) {
            ESP_LOGD(DEBUG_MODULE, "Data ready via interrupt status: 0x%02X", status);
            return true;
        }
    }
    
    // Alternative method: Check if SYSRANGE_START register has cleared
    uint8_t range_status;
    if (read_reg8(dev, VL53L0X_REG_SYSRANGE_START, &range_status)) {
        // When measurement is complete, bit 0 should be 0
        bool start_cleared = (range_status & 0x01) == 0;
        if (start_cleared) {
            ESP_LOGD(DEBUG_MODULE, "Data ready via SYSRANGE_START cleared: 0x%02X", range_status);
            return true;
        }
    }
    
    return false;
}

int vl53l0x_get_ranging_measurement(VL53L0X_Dev_t *dev, VL53L0X_RangingMeasurementData_t *range_data) {
    if (!dev || !range_data || !dev->initialized) {
        return VL53L0X_ERROR_INVALID_PARAMS;
    }

    // Clear the structure
    memset(range_data, 0, sizeof(VL53L0X_RangingMeasurementData_t));

    // Read range result
    uint16_t range_mm;
    if (!read_reg16(dev, VL53L0X_REG_RESULT_RANGE_STATUS + 10, &range_mm)) {
        return VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    // Read range status
    uint8_t range_status;
    if (!read_reg8(dev, VL53L0X_REG_RESULT_RANGE_STATUS, &range_status)) {
        return VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    // Fill measurement data
    range_data->RangeMilliMeter = range_mm;
    range_data->RangeStatus = range_status & 0x0F;
    range_data->TimeStamp = get_time_ms();

    // Clear interrupt for next measurement
    write_reg8(dev, VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01);

    return VL53L0X_ERROR_NONE;
}

bool vl53l0x_is_valid_measurement(VL53L0X_RangingMeasurementData_t *measure) {
    if (!measure) {
        return false;
    }

    // Arduino VL53L0X library accepts range status != 4 as valid
    // Status 4 = phase failure (out of range)
    // Other statuses (0-3, 5-15) can still have valid range data
    if (measure->RangeStatus == 4) {
        return false; // Phase failure - definitely invalid
    }

    // Check range validity (but allow smaller minimum for testing)
    if (measure->RangeMilliMeter < 10 ||  // Allow down to 1cm for testing
        measure->RangeMilliMeter > 2000) {  // 2 meters maximum
        return false;
    }

    return true;
}

void vl53l0x_add_to_buffer(VL53L0X_Dev_t *dev, float distance) {
    if (!dev) {
        return;
    }

    dev->distance_buffer[dev->buffer_index] = distance;
    dev->buffer_index = (dev->buffer_index + 1) % VL53L0X_MEASUREMENT_SAMPLES;

    // Mark buffer as filled after first complete cycle
    if (dev->buffer_index == 0 && !dev->buffer_filled) {
        dev->buffer_filled = true;
    }
}

float vl53l0x_calculate_average(VL53L0X_Dev_t *dev) {
    if (!dev) {
        return 0.0f;
    }

    float sum = 0.0f;
    int count = dev->buffer_filled ? VL53L0X_MEASUREMENT_SAMPLES : dev->buffer_index;

    for (int i = 0; i < count; i++) {
        sum += dev->distance_buffer[i];
    }

    return count > 0 ? sum / count : 0.0f;
}

void vl53l0x_handle_invalid_measurement(VL53L0X_RangingMeasurementData_t *measure, const char **status) {
    if (!measure || !status) {
        return;
    }

    if (measure->RangeStatus != 0) {
        *status = STATUS_ERROR;
        ESP_LOGW(DEBUG_MODULE, "[ERROR] Measurement error - Status code: %d", measure->RangeStatus);
    } else if (measure->RangeMilliMeter < 30) {  // 3 cm minimum
        *status = STATUS_ERROR;
        ESP_LOGW(DEBUG_MODULE, "[ERROR] Too close - minimum range is 3 cm");
    } else if (measure->RangeMilliMeter > 2000) {  // 2m maximum
        *status = STATUS_ERROR;
        ESP_LOGW(DEBUG_MODULE, "[ERROR] Too far - maximum reliable range exceeded");
    } else {
        *status = STATUS_ERROR;
        ESP_LOGW(DEBUG_MODULE, "[ERROR] Out of range - target too far or not detected");
    }
}

// Forward declaration for sensor reset function
static bool vl53l0x_reset_sensor(VL53L0X_Dev_t *dev);

// OPTIMIZED VERSION with better motor interference handling
bool vl53l0x_get_distance(VL53L0X_Dev_t *dev, float *distance_cm, const char **status) {
    if (!dev || !distance_cm || !status || !dev->initialized) {
        return false;
    }

    // Track consecutive failures for recovery (less aggressive)
    static uint8_t consecutive_failures = 0;
    static uint8_t consecutive_zeros = 0;
    static uint32_t last_reset_time = 0;
    static uint8_t recovery_attempts = 0;
    static uint32_t last_successful_reading = 0;
    
    // Clear any existing interrupts first
    write_reg8(dev, VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
    vTaskDelay(pdMS_TO_TICKS(2));
    
    // Start measurement with simple retry
    bool start_success = false;
    for (int attempt = 0; attempt < 2; attempt++) {
        if (write_reg8(dev, VL53L0X_REG_SYSRANGE_START, 0x01)) {
            start_success = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    
    if (!start_success) {
        ESP_LOGD(DEBUG_MODULE, "Start measurement failed, trying I2C recovery");
        i2cdevInit(dev->i2cDev);
        vTaskDelay(pdMS_TO_TICKS(10));
        write_reg8(dev, VL53L0X_REG_SYSRANGE_START, 0x01);
    }
    
    // Wait for measurement with adaptive delay
    uint8_t base_delay = 20; // Increased base delay for stability
    uint8_t penalty = (consecutive_failures > 3) ? 10 : 0;
    vTaskDelay(pdMS_TO_TICKS(base_delay + penalty));
    
    // Try to read measurement result
    uint16_t range_mm = 0;
    bool got_reading = false;
    
    // Try primary register first
    if (read_reg16(dev, VL53L0X_REG_RESULT_RANGE_STATUS + 10, &range_mm)) {
        got_reading = true;
    } else {
        // Try alternative approach with I2C recovery
        ESP_LOGD(DEBUG_MODULE, "Primary register failed, trying I2C recovery");
        i2cdevInit(dev->i2cDev);
        vTaskDelay(pdMS_TO_TICKS(5));
        
        if (read_reg16(dev, VL53L0X_REG_RESULT_RANGE_STATUS + 10, &range_mm)) {
            got_reading = true;
            ESP_LOGD(DEBUG_MODULE, "I2C recovery successful");
        }
    }
    
    // Read status for debugging
    uint8_t range_status = 0;
    read_reg8(dev, VL53L0X_REG_RESULT_RANGE_STATUS, &range_status);
    
    // Clear interrupt for next measurement
    write_reg8(dev, VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
    
    // Process the reading
    if (got_reading) {
        // Check for valid range (be more permissive)
        if (range_mm > 0 && range_mm < 8000 && range_mm != 65535) {
            float distance_mm = range_mm + dev->calibration_offset;
            *distance_cm = distance_mm / 10.0f;
            *status = STATUS_VALID;
            
            // Reset failure counters on success
            consecutive_failures = 0;
            consecutive_zeros = 0;
            last_successful_reading = get_time_ms();
            
            ESP_LOGD(DEBUG_MODULE, "[VALID] Distance: %.1f cm (raw: %d mm, status: 0x%02X)", 
                    *distance_cm, range_mm, range_status);
            return true;
        } else if (range_mm == 0) {
            consecutive_zeros++;
            ESP_LOGD(DEBUG_MODULE, "Zero reading #%d (status: 0x%02X)", consecutive_zeros, range_status);
        } else {
            ESP_LOGD(DEBUG_MODULE, "Invalid range: %d mm (status: 0x%02X)", range_mm, range_status);
        }
    } else {
        consecutive_failures++;
        ESP_LOGD(DEBUG_MODULE, "I2C communication failed #%d", consecutive_failures);
    }
    
    // Only trigger recovery after multiple consecutive failures
    bool needs_recovery = false;
    uint32_t current_time = get_time_ms();
    
    if (consecutive_failures >= 10) { // More tolerant threshold
        needs_recovery = true;
        ESP_LOGW(DEBUG_MODULE, "Too many I2C failures (%d), triggering recovery", consecutive_failures);
    } else if (consecutive_zeros >= 15) { // More tolerant threshold
        needs_recovery = true;
        ESP_LOGW(DEBUG_MODULE, "Too many zero readings (%d), triggering recovery", consecutive_zeros);
    } else if (current_time - last_successful_reading > 5000) { // No valid data for 5 seconds
        needs_recovery = true;
        ESP_LOGW(DEBUG_MODULE, "No valid readings for too long, triggering recovery");
    }
    
    // Perform recovery if needed (less aggressive)
    if (needs_recovery && (current_time - last_reset_time > 2000)) { // At least 2 seconds between recoveries
        ESP_LOGI(DEBUG_MODULE, "Performing sensor recovery (attempt %d)", recovery_attempts + 1);
        
        // Simple recovery sequence
        i2cdevInit(dev->i2cDev);
        vTaskDelay(pdMS_TO_TICKS(20));
        
        // Clear interrupts and reset ranging
        write_reg8(dev, VL53L0X_REG_SYSRANGE_START, 0x00);
        vTaskDelay(pdMS_TO_TICKS(10));
        write_reg8(dev, VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
        vTaskDelay(pdMS_TO_TICKS(10));
        
        // Test if sensor is still responding
        uint16_t model_id;
        if (read_reg16(dev, VL53L0X_REG_IDENTIFICATION_MODEL_ID, &model_id)) {
            if (model_id == VL53L0X_EXPECTED_DEVICE_ID) {
                ESP_LOGI(DEBUG_MODULE, "Sensor recovery successful");
                consecutive_failures = 0;
                consecutive_zeros = 0;
                recovery_attempts = 0;
            } else {
                ESP_LOGW(DEBUG_MODULE, "Sensor recovery failed - unexpected ID: 0x%04X", model_id);
            }
        } else {
            ESP_LOGW(DEBUG_MODULE, "Sensor recovery failed - no I2C communication");
        }
        
        recovery_attempts++;
        last_reset_time = current_time;
    }
    
    *status = STATUS_ERROR;
    return false;
}

bool vl53l0x_set_timing_budget(VL53L0X_Dev_t *dev, uint32_t budget_us) {
    if (!dev) {
        return false;
    }

    // Simplified timing budget setting
    // In a full implementation, this would configure various timing registers
    // For now, we just store the value
    dev->measurement_timing_budget_us = budget_us;
    
    ESP_LOGI(DEBUG_MODULE, "Timing budget set to %u us", budget_us);
    return true;
}

bool vl53l0x_set_address(VL53L0X_Dev_t *dev, uint8_t new_address) {
    if (!dev || !dev->initialized) {
        return false;
    }

    if (write_reg8(dev, VL53L0X_REG_I2C_SLAVE_DEVICE_ADDRESS, new_address)) {
        dev->devAddr = new_address;
        ESP_LOGI(DEBUG_MODULE, "I2C address changed to 0x%02X", new_address);
        return true;
    }
    
    return false;
}

// Sensor reset function to recover from stuck states
static bool vl53l0x_reset_sensor(VL53L0X_Dev_t *dev) {
    if (!dev || !dev->initialized) {
        return false;
    }

    ESP_LOGI(DEBUG_MODULE, "Resetting VL53L0X sensor...");
    
    // Method 1: Soft reset sequence by clearing and reinitializing key registers
    // Clear system interrupt
    write_reg8(dev, VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Reset ranging state
    write_reg8(dev, VL53L0X_REG_SYSRANGE_START, 0x00);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Clear any stuck measurement state
    write_reg8(dev, VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Method 2: Write sequence to force sensor back to known state
    // This mimics some register writes from the original initialization
    write_reg8(dev, 0x80, 0x01);  // Enable register access
    write_reg8(dev, 0xFF, 0x01);  
    write_reg8(dev, 0x00, 0x00);
    write_reg8(dev, 0xFF, 0x00);
    write_reg8(dev, 0x80, 0x00);  // Disable register access
    
    vTaskDelay(pdMS_TO_TICKS(20));
    
    // Clear interrupts again
    write_reg8(dev, VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Verify sensor is responding by checking model ID
    uint16_t model_id;
    if (read_reg16(dev, VL53L0X_REG_IDENTIFICATION_MODEL_ID, &model_id)) {
        if (model_id == VL53L0X_EXPECTED_DEVICE_ID) {
            ESP_LOGI(DEBUG_MODULE, "Sensor reset successful - ID verified: 0x%04X", model_id);
            return true;
        } else {
            ESP_LOGW(DEBUG_MODULE, "Sensor reset: Unexpected ID after reset: 0x%04X", model_id);
            return false;
        }
    } else {
        ESP_LOGE(DEBUG_MODULE, "Sensor reset: Failed to read ID after reset");
        return false;
    }
}

bool vl53l0x_calibrate(VL53L0X_Dev_t *dev, uint16_t target_distance_mm) {
    if (!dev || !dev->initialized) {
        return false;
    }

    ESP_LOGI(DEBUG_MODULE, "Calibration mode - Place sensor at %d mm distance", target_distance_mm);

    // Take several measurements to calculate offset
    float total_error = 0.0f;
    int valid_measurements = 0;
    const int calibration_samples = 10;

    for (int i = 0; i < calibration_samples; i++) {
        VL53L0X_RangingMeasurementData_t measure;
        
        // Wait for data ready
        while (!vl53l0x_data_ready(dev)) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        if (vl53l0x_get_ranging_measurement(dev, &measure) == VL53L0X_ERROR_NONE) {
            if (vl53l0x_is_valid_measurement(&measure)) {
                float error = target_distance_mm - measure.RangeMilliMeter;
                total_error += error;
                valid_measurements++;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    if (valid_measurements > 0) {
        dev->calibration_offset = total_error / valid_measurements;
        ESP_LOGI(DEBUG_MODULE, "Calibration complete! Offset: %.1f mm", dev->calibration_offset);
        return true;
    } else {
        ESP_LOGE(DEBUG_MODULE, "Calibration failed - no valid measurements");
        return false;
    }
}

// ==================================================================
// WRAPPER FUNCTIONS FOR ALTITUDE_LOGGER COMPATIBILITY
// ==================================================================

// Global device instance for the wrapper functions
static VL53L0X_Dev_t g_vl53l0x_device;
static I2C_Dev* g_vl53l0x_i2c_dev = NULL; // Will be initialized at runtime
static bool g_wrapper_initialized = false;

/**
 * Enhanced VL53L0X initialization wrapper for altitude_logger compatibility
 */
void vl53l0xEnhancedInit(void) {
    if (g_wrapper_initialized) {
        ESP_LOGW(DEBUG_MODULE, "VL53L0X already initialized via wrapper");
        return;
    }

    ESP_LOGI(DEBUG_MODULE, "Initializing VL53L0X via enhanced wrapper");
    
    // Initialize I2C device pointer at runtime
    g_vl53l0x_i2c_dev = I2C0_DEV;
    
    // Initialize with default calibration offset
    if (vl53l0x_init(&g_vl53l0x_device, g_vl53l0x_i2c_dev, 0.0f)) {
        g_wrapper_initialized = true;
        ESP_LOGI(DEBUG_MODULE, "VL53L0X enhanced wrapper initialized successfully");
    } else {
        ESP_LOGE(DEBUG_MODULE, "VL53L0X enhanced wrapper initialization failed");
        g_wrapper_initialized = false;
    }
}

/**
 * Enhanced VL53L0X test wrapper for altitude_logger compatibility
 */
bool vl53l0xEnhancedTest(void) {
    if (!g_wrapper_initialized) {
        return false;
    }
    
    // Test connection by attempting to read device ID
    return vl53l0x_test_connection(&g_vl53l0x_device);
}

/**
 * Enhanced VL53L0X distance reading wrapper for altitude_logger compatibility
 * Returns distance in centimeters, or -1.0 on error
 */
float vl53l0xEnhancedGetDistance(void) {
    if (!g_wrapper_initialized) {
        return -1.0f;
    }
    
    float distance_cm = 0.0f;
    const char* status = NULL;
    
    // Get distance using the enhanced driver
    if (vl53l0x_get_distance(&g_vl53l0x_device, &distance_cm, &status)) {
        // Return distance in centimeters
        return distance_cm;
    } else {
        // Return -1.0 to indicate error
        return -1.0f;
    }
}
