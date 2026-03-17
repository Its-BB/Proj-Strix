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
 * MERCHANTABILITY or FITNESS FOR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * altitude_logger.c: Continuous altitude logging using VL53L0X TOF sensor
 */

#define DEBUG_MODULE "ALT_LOG"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include <math.h>

#include "system.h"
#include "debug_cf.h"
#include "i2cdev.h"
#include "vl53l0x_simple.h"
#include "stm32_legacy.h"
#include "param.h"
#include "log.h"
#include "flight_ready_state.h"

// External function declarations for VLX_ENH (working sensor)
extern void vl53l0xEnhancedInit(void);
extern bool vl53l0xEnhancedTest(void);
extern float vl53l0xEnhancedGetDistance(void);

// Task configuration
#define ALTITUDE_LOGGER_TASK_NAME "ALT_LOG"
#define ALTITUDE_LOGGER_TASK_STACKSIZE 2048
#define ALTITUDE_LOGGER_TASK_PRI 3

// Altitude logger state
static bool isInit = false;
static bool sensorConnected = false;

// Altitude data (stored in centimeters for consistency)
static float currentAltitude = 0.0f;
static uint32_t lastMeasurementTime = 0;
static uint32_t invalidReadingCount = 0;
static uint32_t validReadingCount = 0;
static uint32_t lastInvalidLogTime = 0;

// Task handle
static TaskHandle_t altitudeLoggerTaskHandle = NULL;

// Error handling constants
#define INVALID_LOG_INTERVAL_MS 2000  // Only log invalid readings every 2 seconds
#define MAX_INVALID_BEFORE_WARNING 50 // Warn after 50 consecutive invalid readings


// Function prototypes
static void altitudeLoggerTask(void* arg);
static float readAltitude(void);

/**
 * Initialize the altitude logger module
 */
void altitudeLoggerInit(void)
{
    if (isInit) {
        return;
    }

    DEBUG_PRINTI("Initializing Altitude Logger (using VLX_ENH directly)...\n");

    // Wait for sensors to be fully initialized
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Initialize VLX_ENH system directly
    vl53l0xEnhancedInit();
    
    // Give it a moment to initialize
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Check if VLX_ENH is available
    if (vl53l0xEnhancedTest()) {
        sensorConnected = true;
        DEBUG_PRINTI("VLX_ENH sensor is available!\n");
        
        // Create the altitude logger task
        xTaskCreate(altitudeLoggerTask, 
                   ALTITUDE_LOGGER_TASK_NAME, 
                   ALTITUDE_LOGGER_TASK_STACKSIZE, 
                   NULL, 
                   ALTITUDE_LOGGER_TASK_PRI, 
                   &altitudeLoggerTaskHandle);
        
        if (altitudeLoggerTaskHandle != NULL) {
            DEBUG_PRINTI("Altitude logger task created successfully!\n");
        } else {
            DEBUG_PRINTE("Failed to create altitude logger task!\n");
        }
    } else {
        DEBUG_PRINTE("VLX_ENH not available!\n");
        DEBUG_PRINTE("This is normal if no VL53L0X sensor is connected.\n");
        sensorConnected = false;
    }

    isInit = true;
}

/**
 * Test the altitude logger module
 */
bool altitudeLoggerTest(void)
{
    return isInit && sensorConnected;
}

/**
 * Get current altitude
 */
float altitudeLoggerGetAltitude(void)
{
    return currentAltitude;
}

/**
 * Check if altitude logger is ready
 */
bool altitudeLoggerIsReady(void)
{
    return isInit && sensorConnected && vl53l0xEnhancedTest();
}

/**
 * Get current distance in centimeters for web position hold
 */
float altitudeLoggerGetDistance(void)
{
    // currentAltitude is already in cm
    return currentAltitude;
}

/**
 * Check if current altitude reading is valid for web position hold
 */
bool altitudeLoggerIsValid(void)
{
    bool valid = (isInit && sensorConnected && vl53l0xEnhancedTest() && currentAltitude >= 0.0f);
    if (!valid) {
        DEBUG_PRINTD("Altitude logger invalid: isInit=%d, sensorConnected=%d, vlx_enh_available=%d, currentAltitude=%.3f\n", 
                    isInit, sensorConnected, vl53l0xEnhancedTest(), currentAltitude);
    }
    return valid;
}




/**
 * Read altitude from VLX_ENH system (returns centimeters)
 */
static float readAltitude(void)
{
    if (!sensorConnected) {
        return -1.0f;
    }

    // Get distance measurement from VLX_ENH (in cm)
    float distance_cm = vl53l0xEnhancedGetDistance();
    
    if (distance_cm > 0.0f) {
        DEBUG_PRINTD("Altitude logger read: %.1f cm\n", distance_cm);
        return distance_cm; // Return cm directly
    } else {
        DEBUG_PRINTD("VLX_ENH sensor read failed or invalid: %.1f cm\n", distance_cm);
        return -1.0f;
    }
}

/**
 * Main altitude logger task
 */
static void altitudeLoggerTask(void* arg)
{
    systemWaitStart();
    
    DEBUG_PRINTI("Altitude logger task started - waiting for flight ready state...\n");
    
    // Wait for the system to be ready to fly before starting altitude measurements
    // This prevents sensor interference during motor calibration
    if (!flightReadyStateWaitReady(portMAX_DELAY)) {
        DEBUG_PRINTE("Failed to wait for flight ready state - exiting task\n");
        return;
    }
    
    DEBUG_PRINTI("🚁 Flight ready confirmed - starting altitude measurements!\n");
    
    // Wait for VLX_ENH to be available
    DEBUG_PRINTI("⏳ Waiting for VLX_ENH to be available...\n");
    int wait_count = 0;
    while (!vl53l0xEnhancedTest() && wait_count < 50) {
        vTaskDelay(pdMS_TO_TICKS(100)); // Wait 100ms
        wait_count++;
    }
    
    if (!vl53l0xEnhancedTest()) {
        DEBUG_PRINTE("❌ VLX_ENH not available after waiting\n");
        return;
    }
    
    DEBUG_PRINTI("✅ VLX_ENH is available - starting measurements!\n");
    
    // Wait for VLX_ENH to actually start providing data
    DEBUG_PRINTI("⏳ Waiting for VLX_ENH to start providing data...\n");
    wait_count = 0;
    while (wait_count < 100) { // Wait up to 10 seconds for data
        float test_distance = vl53l0xEnhancedGetDistance();
        if (test_distance > 0.0f) {
            DEBUG_PRINTI("✅ VLX_ENH is providing data: %.1f cm\n", test_distance);
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Wait 100ms
        wait_count++;
    }
    
    if (wait_count >= 100) {
        DEBUG_PRINTE("❌ VLX_ENH not providing data after waiting\n");
        return;
    }
    
    uint32_t consecutiveInvalid = 0;
    
    while (1) {
        if (sensorConnected) {
            float altitude = readAltitude();
            uint32_t currentTime = xTaskGetTickCount();
            
            // Debug: Log every 50 readings to see if task is running
            static uint32_t debug_counter = 0;
            if (++debug_counter % 50 == 0) {
                DEBUG_PRINTI("Altitude logger task running: altitude=%.3f, currentAltitude=%.3f\n", 
                            altitude, currentAltitude);
            }

            if (altitude >= 0.0f) {
                // Valid reading - reset error counters
                currentAltitude = altitude;
                lastMeasurementTime = currentTime;
                validReadingCount++;
                consecutiveInvalid = 0;
                
                // altitude is already in cm, convert to meters for display
                float altitudeM = altitude / 100.0f;
                
                // Only log valid readings every 10 readings (1 second) to reduce spam
                if (validReadingCount % 10 == 0) {
                    DEBUG_PRINTI("ALTITUDE: %.1f cm (%.3f m) [Valid readings: %u]\n", 
                                altitude, altitudeM, validReadingCount);
                }
            } else {
                // Invalid reading - count and log intelligently
                invalidReadingCount++;
                consecutiveInvalid++;
                
                // Only log invalid readings every 2 seconds OR if too many consecutive failures
                bool shouldLog = (currentTime - lastInvalidLogTime) > M2T(INVALID_LOG_INTERVAL_MS);
                bool tooManyFailures = consecutiveInvalid == MAX_INVALID_BEFORE_WARNING;
                
                if (shouldLog || tooManyFailures) {
                    if (tooManyFailures) {
                        DEBUG_PRINTW("Sensor issues: %u consecutive invalid readings (Total invalid: %u)\n", 
                                    consecutiveInvalid, invalidReadingCount);
                    } else {
                        DEBUG_PRINTD("Sensor status: %u invalid readings, last valid: %.1f cm\n", 
                                    invalidReadingCount, currentAltitude);
                    }
                    lastInvalidLogTime = currentTime;
                }
            }
        } else {
            // Only warn about disconnected sensor every 5 seconds
            uint32_t currentTime = xTaskGetTickCount();
            if ((currentTime - lastInvalidLogTime) > M2T(5000)) {
                DEBUG_PRINTW("VL53L0X sensor not connected\n");
                lastInvalidLogTime = currentTime;
            }
        }
        
        // 5ms sampling rate for MAXIMUM frequency continuous updates
        vTaskDelay(M2T(5));
    }
}



// Parameter group for configuration
PARAM_GROUP_START(altitude)
PARAM_ADD(PARAM_UINT8 | PARAM_RONLY, enabled, &isInit)
PARAM_ADD(PARAM_UINT8 | PARAM_RONLY, connected, &sensorConnected)
PARAM_ADD(PARAM_FLOAT | PARAM_RONLY, current, &currentAltitude)
PARAM_ADD(PARAM_UINT32 | PARAM_RONLY, validCount, &validReadingCount)
PARAM_ADD(PARAM_UINT32 | PARAM_RONLY, invalidCount, &invalidReadingCount)
PARAM_GROUP_STOP(altitude)

// Logging group for monitoring
LOG_GROUP_START(altitude)
LOG_ADD(LOG_UINT8, enabled, &isInit)
LOG_ADD(LOG_UINT8, connected, &sensorConnected) 
LOG_ADD(LOG_FLOAT, current, &currentAltitude)
LOG_ADD(LOG_UINT32, validCount, &validReadingCount)
LOG_ADD(LOG_UINT32, invalidCount, &invalidReadingCount)
LOG_GROUP_STOP(altitude)
