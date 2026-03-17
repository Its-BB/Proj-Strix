/**
 *    ||          ____  _ __
 * +------+      / __ )(_) /_______________ _____  ___
 * | 0xBC |     / __  / / __/ ___/ ___/ __ `/_  / / _ \
 * +------+    / /_/ / / /_/ /__/ /  / /_/ / / /_/  __/
 *  ||  ||    /_____/_/\__/\___/_/   \__,_/ /___/\___/
 *
 * Crazyflie control firmware
 *
 * Copyright (C) 2024 ESP-Drone Project
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
 * flight_ready_state.c - Flight readiness state management implementation
 */

#include "flight_ready_state.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#define DEBUG_MODULE "FLIGHT_READY"
#include "debug_cf.h"

// Static variables for flight ready state management
static bool isFlightReady = false;
static SemaphoreHandle_t flightReadySemaphore = NULL;
static bool isInitialized = false;

void flightReadyStateInit(void) {
    if (isInitialized) {
        return;
    }
    
    // Create a binary semaphore for signaling flight ready state
    flightReadySemaphore = xSemaphoreCreateBinary();
    
    if (flightReadySemaphore == NULL) {
        DEBUG_PRINTE("Failed to create flight ready semaphore!");
        return;
    }
    
    // Initialize state as not ready
    isFlightReady = false;
    
    isInitialized = true;
    DEBUG_PRINTD("Flight ready state management initialized");
}

void flightReadyStateSetReady(void) {
    if (!isInitialized) {
        DEBUG_PRINTW("Flight ready state not initialized!");
        return;
    }
    
    if (!isFlightReady) {
        isFlightReady = true;
        
        // Signal all waiting tasks that the system is now ready to fly
        xSemaphoreGive(flightReadySemaphore);
        
        DEBUG_PRINTI("🚁 FLIGHT READY STATE: System is now ready for altitude measurement!");
        DEBUG_PRINTI("✈️  VL53L0X/VL53L1X sensors can now start ground distance measurement");
    }
}

bool flightReadyStateIsReady(void) {
    if (!isInitialized) {
        return false;
    }
    
    return isFlightReady;
}

bool flightReadyStateWaitReady(uint32_t timeout_ms) {
    if (!isInitialized) {
        DEBUG_PRINTE("Flight ready state not initialized!");
        return false;
    }
    
    // If already ready, return immediately
    if (isFlightReady) {
        return true;
    }
    
    TickType_t timeout_ticks;
    if (timeout_ms == portMAX_DELAY) {
        timeout_ticks = portMAX_DELAY;
    } else {
        timeout_ticks = pdMS_TO_TICKS(timeout_ms);
    }
    
    DEBUG_PRINTD("Waiting for flight ready state...");
    
    // Wait for the semaphore to be signaled (flight ready)
    BaseType_t result = xSemaphoreTake(flightReadySemaphore, timeout_ticks);
    
    if (result == pdTRUE) {
        DEBUG_PRINTD("Flight ready state achieved!");
        return true;
    } else {
        DEBUG_PRINTW("Timeout waiting for flight ready state!");
        return false;
    }
}

void flightReadyStateReset(void) {
    if (!isInitialized) {
        return;
    }
    
    isFlightReady = false;
    
    // Reset the semaphore by taking it (if available)
    xSemaphoreTake(flightReadySemaphore, 0);
    
    DEBUG_PRINTI("Flight ready state reset - sensors will wait for new ready signal");
}
