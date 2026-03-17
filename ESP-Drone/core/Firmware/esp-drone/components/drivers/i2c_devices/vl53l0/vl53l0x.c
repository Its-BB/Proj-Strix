// Minimal VL53L0X adapter for ESP-Drone
// Uses existing i2cdrv infrastructure instead of creating new I2C driver

#include "vl53l0x.h"
#include "i2cdrv.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define VL53L0X_DEFAULT_ADDR 0x29

// Register definitions (minimal set needed)
#define SYSRANGE_START                      0x00
#define SYSTEM_INTERRUPT_CLEAR              0x0B
#define RESULT_INTERRUPT_STATUS             0x13
#define RESULT_RANGE_STATUS                 0x14
#define IDENTIFICATION_MODEL_ID             0xC0
#define IDENTIFICATION_REVISION_ID          0xC2

// Simplified VL53L0X structure
struct vl53l0x_s
{
    uint8_t address;
    uint16_t timeout;
    uint8_t did_timeout:1;
    uint8_t i2c_fail:1;
};

static const char* TAG = "VL53L0X";

// Simple timing function
static uint32_t millis(void) {
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

// I2C helper functions using existing i2cdrv
static bool vl53l0x_read_reg8(vl53l0x_t *v, uint8_t reg, uint8_t *data) {
    if (!i2cdevReadReg8(I2C0_DEV, v->address, reg, 1, data)) {
        v->i2c_fail = 1;
        return false;
    }
    return true;
}

static bool vl53l0x_write_reg8(vl53l0x_t *v, uint8_t reg, uint8_t data) {
    if (!i2cdevWriteReg8(I2C0_DEV, v->address, reg, data)) {
        v->i2c_fail = 1;
        return false;
    }
    return true;
}

static bool vl53l0x_read_reg16(vl53l0x_t *v, uint8_t reg, uint16_t *data) {
    uint8_t buffer[2];
    if (!i2cdevReadReg8(I2C0_DEV, v->address, reg, 2, buffer)) {
        v->i2c_fail = 1;
        return false;
    }
    *data = (buffer[0] << 8) | buffer[1];
    return true;
}

// Simple VL53L0X implementation
vl53l0x_t *vl53l0x_config(int8_t port, int8_t scl, int8_t sda, int8_t xshut, uint8_t address, uint8_t io_2v8) {
    // Note: We ignore port, scl, sda since we use existing i2cdrv
    // This is just to maintain compatibility with the original API
    
    vl53l0x_t *v = malloc(sizeof(vl53l0x_t));
    if (!v) {
        return NULL;
    }
    
    memset(v, 0, sizeof(vl53l0x_t));
    v->address = address;
    v->timeout = 100; // Default 100ms timeout
    
    return v;
}

const char *vl53l0x_init(vl53l0x_t *v) {
    if (!v) {
        return "Null device";
    }
    
    // Test communication by reading model ID
    uint16_t model_id;
    if (!vl53l0x_read_reg16(v, IDENTIFICATION_MODEL_ID, &model_id)) {
        return "I2C communication failed";
    }
    
    if (model_id != 0xEEAA) {
        return "Wrong model ID";
    }
    
    // Basic initialization - just clear interrupts
    if (!vl53l0x_write_reg8(v, SYSTEM_INTERRUPT_CLEAR, 0x01)) {
        return "Failed to clear interrupts";
    }
    
    vTaskDelay(pdMS_TO_TICKS(10)); // Small delay
    
    return NULL; // Success
}

void vl53l0x_end(vl53l0x_t *v) {
    if (v) {
        free(v);
    }
}

void vl53l0x_setTimeout(vl53l0x_t *v, uint16_t timeout) {
    if (v) {
        v->timeout = timeout;
    }
}

uint16_t vl53l0x_getTimeout(vl53l0x_t *v) {
    return v ? v->timeout : 0;
}

int vl53l0x_timeoutOccurred(vl53l0x_t *v) {
    if (!v) return 0;
    int result = v->did_timeout;
    v->did_timeout = 0;
    return result;
}

int vl53l0x_i2cFail(vl53l0x_t *v) {
    if (!v) return 0;
    int result = v->i2c_fail;
    v->i2c_fail = 0;
    return result;
}

uint16_t vl53l0x_readRangeSingleMillimeters(vl53l0x_t *v) {
    if (!v) {
        return 65535;
    }
    
    // Clear any previous interrupt
    vl53l0x_write_reg8(v, SYSTEM_INTERRUPT_CLEAR, 0x01);
    
    // Start single measurement
    if (!vl53l0x_write_reg8(v, SYSRANGE_START, 0x01)) {
        return 65535;
    }
    
    // Wait for measurement to complete
    uint32_t start_time = millis();
    uint8_t status;
    
    while (true) {
        if (!vl53l0x_read_reg8(v, RESULT_INTERRUPT_STATUS, &status)) {
            return 65535;
        }
        
        if ((status & 0x07) != 0) {
            break; // Measurement ready
        }
        
        if ((millis() - start_time) > v->timeout) {
            v->did_timeout = 1;
            return 65535;
        }
        
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    // Read the distance (at offset 10 from RESULT_RANGE_STATUS)
    uint16_t distance;
    if (!vl53l0x_read_reg16(v, RESULT_RANGE_STATUS + 10, &distance)) {
        return 65535;
    }
    
    // Clear interrupt
    vl53l0x_write_reg8(v, SYSTEM_INTERRUPT_CLEAR, 0x01);
    
    return distance;
}

// Stub implementations for unused functions
void vl53l0x_setAddress(vl53l0x_t *v, uint8_t new_addr) {
    if (v) v->address = new_addr;
}

uint8_t vl53l0x_getAddress(vl53l0x_t *v) {
    return v ? v->address : 0;
}

// Register access functions (basic implementations)
void vl53l0x_writeReg8Bit(vl53l0x_t *v, uint8_t reg, uint8_t value) {
    vl53l0x_write_reg8(v, reg, value);
}

uint8_t vl53l0x_readReg8Bit(vl53l0x_t *v, uint8_t reg) {
    uint8_t value = 0;
    vl53l0x_read_reg8(v, reg, &value);
    return value;
}

// Stub implementations for advanced features
const char *vl53l0x_setSignalRateLimit(vl53l0x_t *v, float limit_Mcps) { return NULL; }
float vl53l0x_getSignalRateLimit(vl53l0x_t *v) { return 0.25f; }
const char *vl53l0x_setMeasurementTimingBudget(vl53l0x_t *v, uint32_t budget_us) { return NULL; }
uint32_t vl53l0x_getMeasurementTimingBudget(vl53l0x_t *v) { return 33000; }
void vl53l0x_writeReg16Bit(vl53l0x_t *v, uint8_t reg, uint16_t value) { /* stub */ }
void vl53l0x_writeReg32Bit(vl53l0x_t *v, uint8_t reg, uint32_t value) { /* stub */ }
uint16_t vl53l0x_readReg16Bit(vl53l0x_t *v, uint8_t reg) { return 0; }
uint32_t vl53l0x_readReg32Bit(vl53l0x_t *v, uint8_t reg) { return 0; }
void vl53l0x_writeMulti(vl53l0x_t *v, uint8_t reg, uint8_t const *src, uint8_t count) { /* stub */ }
void vl53l0x_readMulti(vl53l0x_t *v, uint8_t reg, uint8_t *dst, uint8_t count) { /* stub */ }
const char *vl53l0x_setVcselPulsePeriod(vl53l0x_t *v, vl53l0x_vcselPeriodType type, uint8_t period_pclks) { return NULL; }
uint8_t vl53l0x_getVcselPulsePeriod(vl53l0x_t *v, vl53l0x_vcselPeriodType type) { return 14; }
void vl53l0x_startContinuous(vl53l0x_t *v, uint32_t period_ms) { /* stub */ }
void vl53l0x_stopContinuous(vl53l0x_t *v) { /* stub */ }
uint16_t vl53l0x_readRangeContinuousMillimeters(vl53l0x_t *v) { return vl53l0x_readRangeSingleMillimeters(v); }
