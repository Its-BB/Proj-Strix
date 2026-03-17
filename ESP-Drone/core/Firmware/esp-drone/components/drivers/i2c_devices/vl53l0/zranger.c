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
 * zranger.c: Enhanced Z-ranger (altitude hold) driver using VL53L0X
 */

#define DEBUG_MODULE "VLX_ZRANGER"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "system.h"
#include "debug_cf.h"
#include "log.h"
#include "param.h"
#include "range.h"
#include "config.h"
#include "i2cdev.h"
#include "zranger.h"
#include "vl53l0x_enhanced.h"
#include "stabilizer_types.h"
#include "estimator.h"
#include "cf_math.h"
#include "flight_ready_state.h"

// Measurement noise model parameters
static const float expPointA = 1.0f;
static const float expStdA = 0.0025f; // STD at elevation expPointA [m]
static const float expPointB = 1.3f;
static const float expStdB = 0.2f;    // STD at elevation expPointB [m]
static float expCoeff;

#define RANGE_OUTLIER_LIMIT 3.0f  // Maximum valid range in meters
#define ZRANGER_UPDATE_RATE_MS 50 // Update rate in milliseconds

static float range_last = 0.0f;
static uint32_t last_update_tick = 0;
static bool isInit = false;
static VL53L0X_Dev_t vl53l0x_dev;

// Task handle
static TaskHandle_t zRangerTaskHandle = NULL;

void zRangerInit(void) {
    if (isInit) {
        return;
    }

    ESP_LOGI(DEBUG_MODULE, "Initializing Enhanced Z-Ranger...");

    // Initialize the enhanced VL53L0X sensor with default calibration offset
    if (!vl53l0x_init(&vl53l0x_dev, I2C0_DEV, 0.0f)) {
        ESP_LOGE(DEBUG_MODULE, "Failed to initialize VL53L0X sensor");
        return;
    }

    // Start continuous measurements
    if (!vl53l0x_start_continuous(&vl53l0x_dev)) {
        ESP_LOGE(DEBUG_MODULE, "Failed to start continuous measurements");
        return;
    }

    // Wait a moment for the sensor to stabilize
    vTaskDelay(pdMS_TO_TICKS(100));

    // Create the Z-Ranger task
    if (xTaskCreate(zRangerTask, ZRANGER_TASK_NAME, ZRANGER_TASK_STACKSIZE, 
                    NULL, ZRANGER_TASK_PRI, &zRangerTaskHandle) != pdPASS) {
        ESP_LOGE(DEBUG_MODULE, "Failed to create Z-Ranger task");
        return;
    }

    // Pre-compute constant in the measurement noise model for Kalman filter
    expCoeff = logf(expStdB / expStdA) / (expPointB - expPointA);

    ESP_LOGI(DEBUG_MODULE, "Enhanced Z-Ranger initialized successfully");
    isInit = true;
}

bool zRangerTest(void) {
    if (!isInit) {
        return false;
    }

    return vl53l0x_test_connection(&vl53l0x_dev);
}

void zRangerTask(void* arg) {
    systemWaitStart();
    
    ESP_LOGI(DEBUG_MODULE, "Z-Ranger task started - waiting for flight ready state...");
    
    // Wait for the system to be ready to fly before starting altitude measurements
    // This prevents sensor interference during motor calibration
    if (!flightReadyStateWaitReady(portMAX_DELAY)) {
        ESP_LOGE(DEBUG_MODULE, "Failed to wait for flight ready state - exiting task");
        return;
    }
    
    ESP_LOGI(DEBUG_MODULE, "🚁 Flight ready confirmed - starting ground distance measurements!");
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(ZRANGER_UPDATE_RATE_MS));

        float distance_cm;
        const char* status;
        
        // Get enhanced distance measurement with averaging and validation
        if (vl53l0x_get_distance(&vl53l0x_dev, &distance_cm, &status)) {
            // Convert cm to meters
            float distance_m = distance_cm / 100.0f;
            
            // Update global range value
            range_last = distance_m;
            last_update_tick = xTaskGetTickCount();
            
            // Set range for the control system
            rangeSet(rangeDown, distance_m);
            
            ESP_LOGI(DEBUG_MODULE, "Altitude: %.1f cm [%s]", distance_cm, status);

            // Check if range is feasible and push into the estimator
            if (distance_m < RANGE_OUTLIER_LIMIT && distance_m > 0.03f) {
                // Calculate standard deviation for noise model
                float stdDev = expStdA * (1.0f + expf(expCoeff * (distance_m - expPointA)));
                
                // Enqueue measurement in estimator with noise model
                rangeEnqueueDownRangeInEstimator(distance_m, stdDev, xTaskGetTickCount());
                
                ESP_LOGV(DEBUG_MODULE, "Enqueued in estimator: %.3f m, stdDev: %.4f", distance_m, stdDev);
            } else {
                ESP_LOGW(DEBUG_MODULE, "Range outlier rejected: %.3f m", distance_m);
            }
        } else {
            // Log status of invalid measurements periodically
            static uint32_t error_count = 0;
            error_count++;
            if (error_count % 20 == 0) { // Log every 20 failed measurements (1 second)
                ESP_LOGW(DEBUG_MODULE, "Invalid measurement: %s", status ? status : "UNKNOWN");
            }
        }
    }
}

bool zRangerReadRange(zDistance_t* zrange, const uint32_t tick) {
    if (!zrange || !isInit) {
        return false;
    }

    // Check if we have a recent update
    uint32_t tick_diff = tick - last_update_tick;
    if (tick_diff < pdMS_TO_TICKS(ZRANGER_UPDATE_RATE_MS * 2)) {
        zrange->distance = range_last;
        zrange->timestamp = last_update_tick;
        return true;
    }

    return false;
}

float zRangerGetDistance(void) {
    if (!isInit) {
        return -1.0f;
    }
    
    // Check if the last measurement is recent
    uint32_t current_tick = xTaskGetTickCount();
    uint32_t tick_diff = current_tick - last_update_tick;
    
    if (tick_diff < pdMS_TO_TICKS(ZRANGER_UPDATE_RATE_MS * 3)) {
        return range_last;
    }
    
    return -1.0f; // No recent valid measurement
}

bool zRangerIsInit(void) {
    return isInit;
}

bool zRangerCalibrate(float target_distance_cm) {
    if (!isInit) {
        ESP_LOGE(DEBUG_MODULE, "Z-Ranger not initialized");
        return false;
    }

    ESP_LOGI(DEBUG_MODULE, "Starting Z-Ranger calibration at %.1f cm", target_distance_cm);
    
    // Convert cm to mm for the VL53L0X calibration function
    uint16_t target_distance_mm = (uint16_t)(target_distance_cm * 10.0f);
    
    // Temporarily stop continuous measurements for calibration
    vl53l0x_stop_continuous(&vl53l0x_dev);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Perform calibration
    bool calibration_success = vl53l0x_calibrate(&vl53l0x_dev, target_distance_mm);
    
    // Restart continuous measurements
    vTaskDelay(pdMS_TO_TICKS(100));
    vl53l0x_start_continuous(&vl53l0x_dev);
    
    if (calibration_success) {
        ESP_LOGI(DEBUG_MODULE, "Z-Ranger calibration completed successfully");
    } else {
        ESP_LOGE(DEBUG_MODULE, "Z-Ranger calibration failed");
    }
    
    return calibration_success;
}

// Parameter and logging support
PARAM_GROUP_START(zranger)
PARAM_ADD(PARAM_UINT8, enabled, &isInit)
PARAM_GROUP_STOP(zranger)

LOG_GROUP_START(zranger)
LOG_ADD(LOG_FLOAT, distance, &range_last)
LOG_ADD(LOG_UINT8, status, &isInit)
LOG_GROUP_STOP(zranger)
