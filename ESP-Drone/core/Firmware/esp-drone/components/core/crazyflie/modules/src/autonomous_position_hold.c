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
 * autonomous_position_hold.c: Autonomous position hold system for 50cm altitude hold
 */

#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "autonomous_position_hold.h"
#include "commander.h"
#include "stabilizer_types.h"
#include "altitude_logger.h"
#include "motors.h"
#include "system.h"
#include "param.h"
#include "log.h"
#include "pid.h"
#include "num.h"
#include "cf_math.h"

#define DEBUG_MODULE "AUTO_POS"
#include "debug_cf.h"

// Configuration constants
#define TARGET_ALTITUDE_CM 50.0f          // Target altitude in centimeters
#define TARGET_ALTITUDE_M  0.5f           // Target altitude in meters
#define LANDING_THRUST_STEP 200           // Thrust decrement per step during landing
#define CONNECTION_TIMEOUT_MS 2500        // Timeout before emergency landing (ms)
#define ALTITUDE_TOLERANCE_CM 5.0f        // Acceptable altitude error (cm)
#define MIN_VALID_ALTITUDE_CM 5.0f        // Minimum valid altitude reading
#define MAX_VALID_ALTITUDE_CM 200.0f      // Maximum valid altitude reading


// System state variables
static bool isInit = false;
static bool isEnabled = false;
static bool connectionActive = false;
static auto_state_t currentState = AUTO_STATE_DISABLED;
static uint32_t lastConnectionTime = 0;
static uint32_t stateEntryTime = 0;

// Control variables
static float currentAltitude = 0.0f;
static float targetAltitude = TARGET_ALTITUDE_M;
static uint16_t currentThrust = 0;
static setpoint_t autonomousSetpoint;

// PID controllers
static PidObject altitudePID;

// Altitude PID gains (more aggressive for better 50cm altitude reach)
static float altitudeKp = 25000.0f;  // Proportional gain (increased)
static float altitudeKi = 8000.0f;   // Integral gain (increased)
static float altitudeKd = 12000.0f;  // Derivative gain (increased)

// Thrust limits (from position controller)
#ifdef CONFIG_MOTOR_BRUSHED_715
  #ifdef CONFIG_TARGET_ESP32_S2_DRONE_V1_2
    static uint16_t thrustBase = 42000;
    static uint16_t thrustMin = 8000;
    static uint16_t thrustMax = 60000;
  #else
    static uint16_t thrustBase = 36000;
    static uint16_t thrustMin = 20000;
    static uint16_t thrustMax = 60000;
  #endif
#else
    static uint16_t thrustBase = 24000;
    static uint16_t thrustMin = 5000;
    static uint16_t thrustMax = 60000;
#endif

// Statistics
static uint32_t positionHoldTime = 0;
static uint32_t takeoffCount = 0;
static uint32_t landingCount = 0;
static uint32_t connectionLossCount = 0;

// Function prototypes
static void updateState(void);
static void executeStateActions(void);
static void updateAltitudeControl(void);
static bool isAltitudeValid(float altitude);
static void transitionToState(auto_state_t newState);
static void setAutonomousSetpoint(void);

/**
 * Initialize the autonomous position hold system
 */
void autonomousPositionHoldInit(void)
{
    if (isInit) {
        return;
    }

    DEBUG_PRINTI("Initializing Autonomous Position Hold System...\n");

    // Initialize PID controller for altitude
    pidInit(&altitudePID, targetAltitude, altitudeKp, altitudeKi, altitudeKd, 
            1.0f/POSITION_RATE, POSITION_RATE, 20.0f, true);
    pidSetIntegralLimit(&altitudePID, 10000.0f);

    // Initialize setpoint structure
    memset(&autonomousSetpoint, 0, sizeof(autonomousSetpoint));
    
    // Default to disabled state
    currentState = AUTO_STATE_DISABLED;
    stateEntryTime = xTaskGetTickCount();
    
    DEBUG_PRINTI("Target altitude: %.1f cm\n", TARGET_ALTITUDE_CM);
    DEBUG_PRINTI("Thrust base: %d, min: %d, max: %d\n", thrustBase, thrustMin, thrustMax);
    DEBUG_PRINTI("Connection timeout: %d ms\n", CONNECTION_TIMEOUT_MS);
    
    // Enable autonomous mode by default for cfclient compatibility 
    isEnabled = true;
    DEBUG_PRINTI("Autonomous Position Hold initialized and ENABLED - available for cfclient\n");
    
    isInit = true;
}

/**
 * Test the autonomous position hold system
 */
bool autonomousPositionHoldTest(void)
{
    return isInit && altitudeLoggerTest();
}

/**
 * Update the autonomous position hold system
 * Should be called from the stabilizer loop
 */
void autonomousPositionHoldUpdate(setpoint_t *setpoint, const state_t *state, uint32_t tick)
{
    if (!isInit || !isEnabled) {
        return;
    }

    // Update connection status based on commander activity
    uint32_t inactivityTime = commanderGetInactivityTime();
    uint32_t currentTime = xTaskGetTickCount();
    
    if (inactivityTime < M2T(CONNECTION_TIMEOUT_MS)) {
        if (!connectionActive) {
            DEBUG_PRINTI("Connection established!\n");
            connectionActive = true;
        }
        lastConnectionTime = currentTime;
    } else {
        if (connectionActive) {
            DEBUG_PRINTW("Connection lost! Initiating emergency landing...\n");
            connectionActive = false;
            connectionLossCount++;
        }
    }

    // Get current altitude from sensor
    currentAltitude = altitudeLoggerGetAltitude();
    
    // Update state machine
    updateState();
    
    // Execute actions for current state
    executeStateActions();
    
    // Apply autonomous control if active (including during takeoff)
    if (currentState != AUTO_STATE_DISABLED && currentState != AUTO_STATE_ARMED) {
        setAutonomousSetpoint();
        *setpoint = autonomousSetpoint;
    }
}

/**
 * Enable/disable autonomous position hold
 */
void autonomousPositionHoldEnable(bool enable)
{
    if (!isInit) {
        return;
    }

    if (enable && !isEnabled) {
        DEBUG_PRINTI("Autonomous Position Hold ENABLED\n");
        isEnabled = true;
        transitionToState(AUTO_STATE_ARMED);
    } else if (!enable && isEnabled) {
        DEBUG_PRINTI("Autonomous Position Hold DISABLED\n");
        isEnabled = false;
        transitionToState(AUTO_STATE_DISABLED);
        currentThrust = 0;
    }
}

/**
 * Get current autonomous state
 */
auto_state_t autonomousPositionHoldGetState(void)
{
    return currentState;
}

/**
 * Check if system is active (not disabled)
 */
bool autonomousPositionHoldIsActive(void)
{
    return isInit && isEnabled && (currentState != AUTO_STATE_DISABLED);
}

// ==================== PRIVATE FUNCTIONS ====================

/**
 * Update the state machine
 */
static void updateState(void)
{
    uint32_t currentTime = xTaskGetTickCount();
    uint32_t timeInState = currentTime - stateEntryTime;
    
    switch (currentState) {
        case AUTO_STATE_DISABLED:
            // Stay disabled until explicitly enabled
            break;
            
        case AUTO_STATE_ARMED:
            // Wait for connection before takeoff
            if (connectionActive && systemIsArmed()) {
                DEBUG_PRINTI("Connection active and system armed - starting takeoff!\n");
                transitionToState(AUTO_STATE_TAKEOFF);
            }
            break;
            
        case AUTO_STATE_TAKEOFF:
            // Check if we reached target altitude
            if (isAltitudeValid(currentAltitude)) {
                float altitudeCm = currentAltitude * 100.0f;
                if (altitudeCm >= (TARGET_ALTITUDE_CM - ALTITUDE_TOLERANCE_CM)) {
                    DEBUG_PRINTI("Target altitude reached! Switching to position hold.\n");
                    transitionToState(AUTO_STATE_POSITION_HOLD);
                }
            }
            
            // Safety timeout for takeoff (30 seconds)
            if (timeInState > M2T(30000)) {
                DEBUG_PRINTW("Takeoff timeout - switching to position hold anyway\n");
                transitionToState(AUTO_STATE_POSITION_HOLD);
            }
            break;
            
        case AUTO_STATE_POSITION_HOLD:
            // Check for connection loss
            if (!connectionActive) {
                DEBUG_PRINTW("Connection lost during position hold - emergency landing!\n");
                transitionToState(AUTO_STATE_EMERGENCY_LANDING);
            }
            
            // Update position hold time
            positionHoldTime++;
            break;
            
        case AUTO_STATE_EMERGENCY_LANDING:
            // Check if landed (very low altitude or timeout)
            if (isAltitudeValid(currentAltitude)) {
                float altitudeCm = currentAltitude * 100.0f;
                if (altitudeCm < MIN_VALID_ALTITUDE_CM) {
                    DEBUG_PRINTI("Emergency landing complete.\n");
                    transitionToState(AUTO_STATE_LANDED);
                }
            }
            
            // Safety timeout for landing (60 seconds)
            if (timeInState > M2T(60000)) {
                DEBUG_PRINTW("Landing timeout - assuming landed\n");
                transitionToState(AUTO_STATE_LANDED);
            }
            break;
            
        case AUTO_STATE_LANDED:
            // Stay landed until manually reset
            currentThrust = 0;
            break;
    }
}

/**
 * Execute actions for the current state
 */
static void executeStateActions(void)
{
    switch (currentState) {
        case AUTO_STATE_DISABLED:
            currentThrust = 0;
            break;
            
        case AUTO_STATE_ARMED:
            currentThrust = 0;
            break;
            
        case AUTO_STATE_TAKEOFF:
            // Let PID controller determine thrust needed to reach target altitude
            updateAltitudeControl();
            break;
            
        case AUTO_STATE_POSITION_HOLD:
            updateAltitudeControl();
            break;
            
        case AUTO_STATE_EMERGENCY_LANDING:
            // Gradual thrust decrease
            if (currentThrust > thrustMin) {
                currentThrust = MAX(currentThrust - LANDING_THRUST_STEP, thrustMin);
            } else {
                currentThrust = 0;
            }
            break;
            
        case AUTO_STATE_LANDED:
            currentThrust = 0;
            break;
    }
    
    // Ensure thrust limits
    currentThrust = constrain(currentThrust, 0, thrustMax);
}

/**
 * Update altitude control using PID
 */
static void updateAltitudeControl(void)
{
    if (!isAltitudeValid(currentAltitude)) {
        // Hold current thrust if sensor reading is invalid
        return;
    }
    
    // PID control for altitude
    pidSetDesired(&altitudePID, targetAltitude);
    float thrustAdjustment = pidUpdate(&altitudePID, currentAltitude, true);
    
    // Apply thrust adjustment to base thrust - NO LIMITS, let PID decide
    int newThrust = (int)(thrustBase + thrustAdjustment);
    currentThrust = (uint16_t)MAX(0, newThrust);  // Only prevent negative thrust
}

/**
 * Check if altitude reading is valid
 */
static bool isAltitudeValid(float altitude)
{
    if (altitude <= 0.0f) return false;
    
    float altitudeCm = altitude * 100.0f;
    return (altitudeCm >= MIN_VALID_ALTITUDE_CM && altitudeCm <= MAX_VALID_ALTITUDE_CM);
}

/**
 * Transition to a new state
 */
static void transitionToState(auto_state_t newState)
{
    if (newState == currentState) {
        return;
    }
    
    DEBUG_PRINTI("State transition: %d -> %d\n", currentState, newState);
    
    // Exit actions for old state
    switch (currentState) {
        case AUTO_STATE_TAKEOFF:
            takeoffCount++;
            break;
        case AUTO_STATE_EMERGENCY_LANDING:
            landingCount++;
            break;
        default:
            break;
    }
    
    // Update state
    currentState = newState;
    stateEntryTime = xTaskGetTickCount();
    
    // Entry actions for new state
    switch (newState) {
        case AUTO_STATE_DISABLED:
            currentThrust = 0;
            pidReset(&altitudePID);
            break;
            
        case AUTO_STATE_ARMED:
            currentThrust = 0;
            pidReset(&altitudePID);
            DEBUG_PRINTI("System armed - waiting for connection...\n");
            break;
            
        case AUTO_STATE_TAKEOFF:
            // Start with base thrust as initial estimate, PID will adjust from there
            currentThrust = thrustBase;
            pidReset(&altitudePID);
            DEBUG_PRINTI("Starting takeoff to %.1f cm with position hold active...\n", TARGET_ALTITUDE_CM);
            break;
            
        case AUTO_STATE_POSITION_HOLD:
            DEBUG_PRINTI("Position hold active at %.1f cm\n", currentAltitude * 100.0f);
            break;
            
        case AUTO_STATE_EMERGENCY_LANDING:
            DEBUG_PRINTW("Emergency landing initiated!\n");
            break;
            
        case AUTO_STATE_LANDED:
            currentThrust = 0;
            DEBUG_PRINTI("Landed safely.\n");
            break;
    }
}

/**
 * Set the autonomous setpoint for position hold
 */
static void setAutonomousSetpoint(void)
{
    // Clear the setpoint
    memset(&autonomousSetpoint, 0, sizeof(autonomousSetpoint));
    
    // Set timestamp
    autonomousSetpoint.timestamp = xTaskGetTickCount();
    
    // Configure for stabilized attitude control with level flight
    autonomousSetpoint.mode.x = modeDisable;       // No position control
    autonomousSetpoint.mode.y = modeDisable;       // No position control
    autonomousSetpoint.mode.z = modeDisable;       // Direct thrust control
    autonomousSetpoint.mode.roll = modeAbs;        // Absolute roll angle
    autonomousSetpoint.mode.pitch = modeAbs;       // Absolute pitch angle
    autonomousSetpoint.mode.yaw = modeVelocity;    // Zero yaw rate (maintain heading)
    
    // Zero velocities (not used in this mode)
    autonomousSetpoint.velocity.x = 0.0f;
    autonomousSetpoint.velocity.y = 0.0f;
    autonomousSetpoint.velocity.z = 0.0f;
    
    // Set attitudes for level flight (prevents drift)
    autonomousSetpoint.attitude.roll = 0.0f;   // Keep perfectly level
    autonomousSetpoint.attitude.pitch = 0.0f;  // Keep perfectly level
    autonomousSetpoint.attitude.yaw = 0.0f;    // Maintain initial heading
    
    // Zero angular rates
    autonomousSetpoint.attitudeRate.roll = 0.0f;
    autonomousSetpoint.attitudeRate.pitch = 0.0f;
    autonomousSetpoint.attitudeRate.yaw = 0.0f;    // No rotation
    
    // Set thrust for altitude control
    autonomousSetpoint.thrust = currentThrust;
    
    // Debug info (only during position hold to avoid spam during takeoff)
    if (currentState == AUTO_STATE_POSITION_HOLD) {
        static uint32_t debugCounter = 0;
        debugCounter++;
        if (debugCounter % 200 == 0) {  // Print every 2 seconds at 100Hz
            DEBUG_PRINTI("Position hold: Alt=%.1fcm Thrust=%d Level flight active\n",
                        currentAltitude * 100.0f, currentThrust);
        }
    }
}

// ==================== LOGGING AND PARAMETERS ====================

PARAM_GROUP_START(autoPos)
PARAM_ADD(PARAM_UINT8, enable, &isEnabled)
PARAM_ADD(PARAM_FLOAT, targetAlt, &targetAltitude)
PARAM_ADD(PARAM_FLOAT, altKp, &altitudeKp)
PARAM_ADD(PARAM_FLOAT, altKi, &altitudeKi)
PARAM_ADD(PARAM_FLOAT, altKd, &altitudeKd)
PARAM_ADD(PARAM_UINT16, thrustBase, &thrustBase)
PARAM_ADD(PARAM_UINT16, thrustMin, &thrustMin)
PARAM_ADD(PARAM_UINT16, thrustMax, &thrustMax)
PARAM_GROUP_STOP(autoPos)

LOG_GROUP_START(autoPos)
LOG_ADD(LOG_UINT8, enabled, &isEnabled)
LOG_ADD(LOG_UINT8, state, &currentState)
LOG_ADD(LOG_UINT8, connected, &connectionActive)
LOG_ADD(LOG_FLOAT, currentAlt, &currentAltitude)
LOG_ADD(LOG_FLOAT, targetAlt, &targetAltitude)
LOG_ADD(LOG_UINT16, thrust, &currentThrust)
LOG_ADD(LOG_UINT32, posHoldTime, &positionHoldTime)
LOG_ADD(LOG_UINT32, takeoffCount, &takeoffCount)
LOG_ADD(LOG_UINT32, landingCount, &landingCount)
LOG_ADD(LOG_UINT32, connLossCount, &connectionLossCount)
LOG_GROUP_STOP(autoPos)
