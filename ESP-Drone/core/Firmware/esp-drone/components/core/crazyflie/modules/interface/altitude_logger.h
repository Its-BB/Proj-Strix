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
 * altitude_logger.h: Continuous altitude logging using VL53L0X TOF sensor
 */

#ifndef _ALTITUDE_LOGGER_H_
#define _ALTITUDE_LOGGER_H_

#include <stdbool.h>
#include <stdint.h>

/**
 * Initialize the altitude logger module
 */
void altitudeLoggerInit(void);

/**
 * Test the altitude logger module
 */
bool altitudeLoggerTest(void);

/**
 * Get current altitude in meters
 */
float altitudeLoggerGetAltitude(void);

/**
 * Check if altitude logger is ready
 */
bool altitudeLoggerIsReady(void);

/**
 * Get current distance in centimeters for web position hold
 */
float altitudeLoggerGetDistance(void);

/**
 * Check if current altitude reading is valid for web position hold
 */
bool altitudeLoggerIsValid(void);

#endif // _ALTITUDE_LOGGER_H_
