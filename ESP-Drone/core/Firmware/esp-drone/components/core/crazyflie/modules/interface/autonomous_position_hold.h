/**
 * ESP-Drone Firmware
 *
 * Copyright 2019-2020  Espressif Systems (Shanghai) 
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
 * autonomous_position_hold.h: Autonomous position hold system for 50cm altitude hold
 */

#ifndef __AUTONOMOUS_POSITION_HOLD_H__
#define __AUTONOMOUS_POSITION_HOLD_H__

#include <stdint.h>
#include <stdbool.h>
#include "stabilizer_types.h"

// Forward declarations
typedef enum {
    AUTO_STATE_DISABLED = 0,
    AUTO_STATE_ARMED,
    AUTO_STATE_TAKEOFF,
    AUTO_STATE_POSITION_HOLD,
    AUTO_STATE_EMERGENCY_LANDING,
    AUTO_STATE_LANDED
} auto_state_t;

/**
 * Initialize the autonomous position hold system
 */
void autonomousPositionHoldInit(void);

/**
 * Test the autonomous position hold system
 * @return true if all tests pass, false otherwise
 */
bool autonomousPositionHoldTest(void);

/**
 * Update the autonomous position hold system
 * Should be called from the stabilizer loop
 * @param setpoint Setpoint to modify if autonomous mode is active
 * @param state Current state of the drone
 * @param tick Current tick count
 */
void autonomousPositionHoldUpdate(setpoint_t *setpoint, const state_t *state, uint32_t tick);

/**
 * Enable or disable autonomous position hold
 * @param enable true to enable, false to disable
 */
void autonomousPositionHoldEnable(bool enable);

/**
 * Get the current autonomous state
 * @return Current state
 */
auto_state_t autonomousPositionHoldGetState(void);

/**
 * Check if autonomous position hold is active
 * @return true if active, false if disabled
 */
bool autonomousPositionHoldIsActive(void);

#endif /* __AUTONOMOUS_POSITION_HOLD_H__ */
