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
 * flight_ready_state.h - Flight readiness state management
 *
 * This module manages the system's flight readiness state, ensuring that
 * altitude measurement and other flight-critical tasks only start after
 * all calibrations are complete and the system is truly ready to fly.
 */

#ifndef __FLIGHT_READY_STATE_H__
#define __FLIGHT_READY_STATE_H__

#include <stdbool.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the flight ready state management system
 */
void flightReadyStateInit(void);

/**
 * @brief Set the system as ready to fly
 * 
 * This should be called after all calibrations are complete and 
 * the "Ready to fly" message has been logged.
 */
void flightReadyStateSetReady(void);

/**
 * @brief Check if the system is ready to fly
 * 
 * @return true if system is ready to fly, false otherwise
 */
bool flightReadyStateIsReady(void);

/**
 * @brief Wait for the system to be ready to fly
 * 
 * This function blocks until the system is marked as ready to fly.
 * Should be used by tasks that need to wait for flight readiness
 * before starting their operations.
 * 
 * @param timeout_ms Maximum time to wait in milliseconds, or portMAX_DELAY for infinite wait
 * @return true if system became ready within timeout, false if timeout occurred
 */
bool flightReadyStateWaitReady(uint32_t timeout_ms);

/**
 * @brief Reset the flight ready state
 * 
 * This can be used to reset the state during system reset or
 * when recalibration is needed.
 */
void flightReadyStateReset(void);

#ifdef __cplusplus
}
#endif

#endif /* __FLIGHT_READY_STATE_H__ */
