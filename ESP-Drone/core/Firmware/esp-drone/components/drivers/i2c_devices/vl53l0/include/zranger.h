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
 * zranger.h: Enhanced Z-ranger (altitude hold) driver using VL53L0X
 */

#ifndef __ZRANGER_H__
#define __ZRANGER_H__

#include <stdint.h>
#include <stdbool.h>
#include "stabilizer_types.h"

#define ZRANGER_TASK_PRI        2
#define ZRANGER_TASK_NAME       "ZRANGER"
// ZRANGER_TASK_STACKSIZE is defined in config.h

// zDistance_t is already defined in stabilizer_types.h
// typedef struct zDistance_s {
//   uint32_t timestamp;
//   float distance;           // m
// } zDistance_t;

/**
 * @brief Initialize the Z-Ranger with enhanced VL53L0X
 */
void zRangerInit(void);

/**
 * @brief Test the Z-Ranger sensor connection
 * @return true if test is successful, false otherwise
 */
bool zRangerTest(void);

/**
 * @brief Z-Ranger task - handles continuous distance measurements
 * @param arg Task argument (unused)
 */
void zRangerTask(void* arg);

/**
 * @brief Read the latest range measurement
 * @param zrange Pointer to store the range data
 * @param tick Current tick for timestamping
 * @return true if new data is available, false otherwise
 */
bool zRangerReadRange(zDistance_t* zrange, const uint32_t tick);

/**
 * @brief Get the current distance measurement in meters
 * @return Distance in meters, -1 if no valid measurement
 */
float zRangerGetDistance(void);

/**
 * @brief Check if the Z-Ranger is initialized
 * @return true if initialized, false otherwise
 */
bool zRangerIsInit(void);

/**
 * @brief Calibrate the Z-Ranger sensor
 * @param target_distance_cm Known target distance in centimeters
 * @return true if calibration successful, false otherwise
 */
bool zRangerCalibrate(float target_distance_cm);

#endif //__ZRANGER_H__
