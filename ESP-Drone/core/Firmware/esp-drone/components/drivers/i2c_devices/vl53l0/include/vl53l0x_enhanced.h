/**
 *
 * ESP-Drone Firmware
 * 
 * Copyright 2019-2020  Espressif Systems (Shanghai) 
 * Copyright (C) 2012 BitCraze AB
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, in version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * vl53l0x_enhanced.h: Enhanced VL53L0X Time-of-flight distance sensor driver
 */

#ifndef _VL53L0X_ENHANCED_H_
#define _VL53L0X_ENHANCED_H_

#include "i2cdev.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define VL53L0X_DEFAULT_ADDRESS 0x29

// Configuration constants for MAXIMUM ACCURACY
#define VL53L0X_MEASUREMENT_SAMPLES 5       // More samples for better averaging
#define VL53L0X_MAX_VALID_RANGE 2000         // Maximum valid range in mm (2 meters)
#define VL53L0X_MIN_VALID_RANGE 30           // Minimum valid range in mm (3 cm)
#define VL53L0X_MEASUREMENT_INTERVAL 0       // NO INTERVAL - continuous rapid-fire measurements
#define VL53L0X_TIMING_BUDGET_MS 200         // High accuracy timing budget
#define VL53L0X_OUTLIER_THRESHOLD 30         // Reject measurements >30mm from average

// VL53L0X Register definitions (based on ST API)
#define VL53L0X_REG_IDENTIFICATION_REVISION_ID              0xC2
#define VL53L0X_REG_IDENTIFICATION_DATE_HI                  0xC6
#define VL53L0X_REG_IDENTIFICATION_DATE_LO                  0xC7
#define VL53L0X_REG_IDENTIFICATION_TIME                     0xC9

#define VL53L0X_REG_SYSRANGE_START                          0x00
#define VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG                  0x01
#define VL53L0X_REG_SYSTEM_RANGE_CONFIG                     0x09
#define VL53L0X_REG_SYSTEM_INTERMEASUREMENT_PERIOD          0x04
#define VL53L0X_REG_SYSTEM_INTERRUPT_CONFIG_GPIO             0x0A
#define VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR                  0x0B
#define VL53L0X_REG_SYSTEM_THRESH_HIGH                      0x0C
#define VL53L0X_REG_SYSTEM_THRESH_LOW                       0x0E
#define VL53L0X_REG_RESULT_INTERRUPT_STATUS                 0x13
#define VL53L0X_REG_RESULT_RANGE_STATUS                     0x14
#define VL53L0X_REG_I2C_SLAVE_DEVICE_ADDRESS                0x8A

// Error status definitions
#define VL53L0X_ERROR_NONE                     0
#define VL53L0X_ERROR_CALIBRATION_WARNING      1
#define VL53L0X_ERROR_MIN_CLIPPED              2
#define VL53L0X_ERROR_UNDEFINED                3
#define VL53L0X_ERROR_INVALID_PARAMS           4
#define VL53L0X_ERROR_NOT_SUPPORTED            5
#define VL53L0X_ERROR_RANGE_ERROR              6
#define VL53L0X_ERROR_TIME_OUT                 7
#define VL53L0X_ERROR_MODE_NOT_SUPPORTED       8
#define VL53L0X_ERROR_BUFFER_TOO_SMALL         9
#define VL53L0X_ERROR_GPIO_NOT_EXISTING        10
#define VL53L0X_ERROR_GPIO_FUNCTIONALITY_NOT_SUPPORTED 11
#define VL53L0X_ERROR_CONTROL_INTERFACE        12
#define VL53L0X_ERROR_INVALID_COMMAND          13
#define VL53L0X_ERROR_DIVISION_BY_ZERO         14
#define VL53L0X_ERROR_REF_SPAD_INIT            15
#define VL53L0X_ERROR_NOT_IMPLEMENTED          99

// Ranging measurement data structure
typedef struct {
    uint32_t TimeStamp;              /*!< 32-bit time stamp. */
    uint32_t MeasurementTimeUsec;    /*!< Give the Measurement time needed by the device to do the measurement.*/
    uint16_t RangeMilliMeter;        /*!< range distance in millimeter. */
    uint16_t RangeReclipMm;          /*!< range distance in millimeter before clip. */
    uint8_t RangeStatus;             /*!< Range Status for the current measurement. */
    uint8_t RangeDMaxMilliMeter;     /*!< Tells what is the maximum detection distance. */
    int32_t SignalRateRtnMegaCps;    /*!< Return signal rate (MCPS)\n these is a 16.16 fix point value. */
    uint32_t AmbientRateRtnMegaCps;  /*!< Return ambient rate (MCPS)\n these is a 16.16 fix point value. */
    uint16_t EffectiveSpadRtnCount;  /*!< Return the effective SPAD count for the return signal. */
    uint8_t ZoneId;                  /*!< Denotes which zone and range scheduler stage the range data came from. */
    uint8_t RangeFractionalPart;     /*!< Fractional part of range distance. Final value is a RangeMilliMeter + RangeFractionalPart/256. */
} VL53L0X_RangingMeasurementData_t;

// Enhanced VL53L0X device structure
typedef struct {
    I2C_Dev *i2cDev;
    uint8_t devAddr;
    
    // Configuration
    float calibration_offset;        // Calibration offset in mm
    
    // Averaging buffer
    float distance_buffer[VL53L0X_MEASUREMENT_SAMPLES];
    int buffer_index;
    bool buffer_filled;
    
    // Timing
    uint32_t last_measurement;
    uint32_t measurement_timing_budget_us;
    
    // Status
    bool initialized;
    bool continuous_mode;
    
} VL53L0X_Dev_t;

// Function declarations
/**
 * @brief Initialize the VL53L0X sensor
 * @param dev Pointer to device structure
 * @param i2cDev I2C device handle
 * @param calibration_offset Calibration offset in mm
 * @return true if successful, false otherwise
 */
bool vl53l0x_init(VL53L0X_Dev_t *dev, I2C_Dev *i2cDev, float calibration_offset);

/**
 * @brief Test connection to the VL53L0X sensor
 * @param dev Pointer to device structure
 * @return true if connection is valid, false otherwise
 */
bool vl53l0x_test_connection(VL53L0X_Dev_t *dev);

/**
 * @brief Start continuous ranging measurements
 * @param dev Pointer to device structure
 * @return true if successful, false otherwise
 */
bool vl53l0x_start_continuous(VL53L0X_Dev_t *dev);

/**
 * @brief Stop continuous ranging measurements
 * @param dev Pointer to device structure
 * @return true if successful, false otherwise
 */
bool vl53l0x_stop_continuous(VL53L0X_Dev_t *dev);

/**
 * @brief Get a single range measurement with averaging and validation
 * @param dev Pointer to device structure
 * @param distance_cm Pointer to store distance in centimeters
 * @param status Pointer to store measurement status
 * @return true if measurement is valid, false otherwise
 */
bool vl53l0x_get_distance(VL53L0X_Dev_t *dev, float *distance_cm, const char **status);

/**
 * @brief Check if new measurement data is available
 * @param dev Pointer to device structure
 * @return true if data is ready, false otherwise
 */
bool vl53l0x_data_ready(VL53L0X_Dev_t *dev);

/**
 * @brief Set measurement timing budget
 * @param dev Pointer to device structure
 * @param budget_us Timing budget in microseconds
 * @return true if successful, false otherwise
 */
bool vl53l0x_set_timing_budget(VL53L0X_Dev_t *dev, uint32_t budget_us);

/**
 * @brief Set I2C address of the sensor
 * @param dev Pointer to device structure
 * @param new_address New I2C address (7-bit)
 * @return true if successful, false otherwise
 */
bool vl53l0x_set_address(VL53L0X_Dev_t *dev, uint8_t new_address);

/**
 * @brief Calibrate the sensor
 * @param dev Pointer to device structure
 * @param target_distance_mm Known target distance in mm
 * @return true if successful, false otherwise
 */
bool vl53l0x_calibrate(VL53L0X_Dev_t *dev, uint16_t target_distance_mm);

/**
 * @brief Read raw range data from sensor
 * @param dev Pointer to device structure
 * @param range_data Pointer to store range measurement data
 * @return VL53L0X error code
 */
int vl53l0x_get_ranging_measurement(VL53L0X_Dev_t *dev, VL53L0X_RangingMeasurementData_t *range_data);

// Internal helper functions
bool vl53l0x_is_valid_measurement(VL53L0X_RangingMeasurementData_t *measure);
void vl53l0x_add_to_buffer(VL53L0X_Dev_t *dev, float distance);
float vl53l0x_calculate_average(VL53L0X_Dev_t *dev);
void vl53l0x_handle_invalid_measurement(VL53L0X_RangingMeasurementData_t *measure, const char **status);

#endif /* _VL53L0X_ENHANCED_H_ */
