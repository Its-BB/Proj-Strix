/**
 * ESP-Drone Leader Signal Detection Module Implementation
 * 
 * Detects ESP32 broadcast signals and enables automatic follower mode
 */

#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "leader_signal_detector.h"
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

#define DEBUG_MODULE "LEADER_SIG"
#include "debug_cf.h"

// System state variables
static bool isInit = false;
static bool isEnabled = false;
static signal_state_t currentState = SIGNAL_STATE_IDLE;
static uint32_t lastSignalTime = 0;
static uint32_t stateEntryTime = 0;

// Leader data
static leader_data_t lastLeaderData;
static leader_data_t currentLeaderData;

// Control variables
static float currentAltitude = 0.0f;
static float targetAltitude = TARGET_ALTITUDE_M_FOLLOWER;
static uint16_t currentThrust = 0;
static setpoint_t followerSetpoint;

// PID controllers
static PidObject altitudePID;
static PidObject positionXPID;
static PidObject positionYPID;

// PID gains
static float altitudeKp = 25000.0f;
static float altitudeKi = 8000.0f;
static float altitudeKd = 12000.0f;

static float positionKp = 5000.0f;
static float positionKi = 1000.0f;
static float positionKd = 2000.0f;

// Thrust limits
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
static uint32_t followingTime = 0;
static uint32_t signalLossCount = 0;
static int8_t lastRSSI = -100;

// Function prototypes
static void updateState(void);
static void executeStateActions(void);
static void updateAltitudeControl(void);
static void updatePositionControl(void);
static bool isAltitudeValid(float altitude);
static void transitionToState(signal_state_t newState);
static void setFollowerSetpoint(void);

/**
 * Initialize the leader signal detection module
 */
void leaderSignalDetectorInit(void)
{
    if (isInit) {
        return;
    }

    DEBUG_PRINTI("Initializing Leader Signal Detection Module...\n");

    // Initialize PID controllers for altitude
    pidInit(&altitudePID, targetAltitude, altitudeKp, altitudeKi, altitudeKd, 
            1.0f/POSITION_RATE, POSITION_RATE, 20.0f, true);
    pidSetIntegralLimit(&altitudePID, 10000.0f);

    // Initialize PID controllers for position
    pidInit(&positionXPID, 0.0f, positionKp, positionKi, positionKd,
            1.0f/POSITION_RATE, POSITION_RATE, 5.0f, true);
    pidSetIntegralLimit(&positionXPID, 2000.0f);

    pidInit(&positionYPID, 0.0f, positionKp, positionKi, positionKd,
            1.0f/POSITION_RATE, POSITION_RATE, 5.0f, true);
    pidSetIntegralLimit(&positionYPID, 2000.0f);

    // Initialize data structures
    memset(&lastLeaderData, 0, sizeof(leader_data_t));
    memset(&currentLeaderData, 0, sizeof(leader_data_t));
    memset(&followerSetpoint, 0, sizeof(followerSetpoint));
    
    // Default to idle state
    currentState = SIGNAL_STATE_IDLE;
    stateEntryTime = xTaskGetTickCount();
    lastSignalTime = xTaskGetTickCount();
    
    DEBUG_PRINTI("Target altitude: %.1f cm\n", TARGET_ALTITUDE_CM_FOLLOWER);
    DEBUG_PRINTI("Signal detection enabled\n");
    
    // Enable follower mode by default
    isEnabled = true;
    DEBUG_PRINTI("Leader Signal Detection initialized and ENABLED\n");
    
    isInit = true;
}

/**
 * Test the leader signal detection module
 */
bool leaderSignalDetectorTest(void)
{
    return isInit;
}

/**
 * Update leader signal detection and following
 */
void leaderSignalDetectorUpdate(setpoint_t *setpoint, const state_t *state, uint32_t tick)
{
    if (!isInit || !isEnabled) {
        return;
    }

    // Get current altitude
    currentAltitude = altitudeLoggerGetAltitude();
    
    // Check for signal timeout
    uint32_t currentTime = xTaskGetTickCount();
    uint32_t timeSinceLastSignal = currentTime - lastSignalTime;
    
    if (timeSinceLastSignal > M2T(SIGNAL_DETECTION_TIMEOUT_MS)) {
        if (currentState == SIGNAL_STATE_FOLLOWING) {
            DEBUG_PRINTW("Signal lost! Initiating emergency landing...\n");
            transitionToState(SIGNAL_STATE_LANDING);
            signalLossCount++;
        }
    }

    // Update state machine
    updateState();
    
    // Execute actions for current state
    executeStateActions();
    
    // Apply follower control if active
    if (currentState == SIGNAL_STATE_FOLLOWING) {
        setFollowerSetpoint();
        *setpoint = followerSetpoint;
    }
}

/**
 * Callback for receiving leader position data
 */
void leaderSignalDetectorReceiveData(const leader_data_t *leader_data)
{
    if (!isInit || !isEnabled) {
        return;
    }

    // Update leader data
    lastLeaderData = currentLeaderData;
    currentLeaderData = *leader_data;
    lastSignalTime = xTaskGetTickCount();
    lastRSSI = leader_data->rssi;

    DEBUG_PRINTD("Leader signal received: pos(%.2f, %.2f, %.2f) RSSI=%d\n",
                 leader_data->x, leader_data->y, leader_data->z, leader_data->rssi);
}

/**
 * Get current signal detection state
 */
signal_state_t leaderSignalDetectorGetState(void)
{
    return currentState;
}

/**
 * Get last received leader data
 */
const leader_data_t* leaderSignalDetectorGetLeaderData(void)
{
    return &currentLeaderData;
}

/**
 * Check if signal is currently detected
 */
bool leaderSignalDetectorIsSignalValid(void)
{
    uint32_t currentTime = xTaskGetTickCount();
    uint32_t timeSinceLastSignal = currentTime - lastSignalTime;
    
    return (timeSinceLastSignal < M2T(SIGNAL_DETECTION_TIMEOUT_MS)) && 
           (lastRSSI > SIGNAL_RSSI_THRESHOLD);
}

/**
 * Get signal strength (RSSI)
 */
int8_t leaderSignalDetectorGetRSSI(void)
{
    return lastRSSI;
}

/**
 * Enable/disable follower mode
 */
void leaderSignalDetectorEnable(bool enable)
{
    if (!isInit) {
        return;
    }

    if (enable && !isEnabled) {
        DEBUG_PRINTI("Leader Signal Detection ENABLED\n");
        isEnabled = true;
        transitionToState(SIGNAL_STATE_IDLE);
    } else if (!enable && isEnabled) {
        DEBUG_PRINTI("Leader Signal Detection DISABLED\n");
        isEnabled = false;
        transitionToState(SIGNAL_STATE_IDLE);
        currentThrust = 0;
    }
}

/**
 * Check if follower mode is enabled
 */
bool leaderSignalDetectorIsEnabled(void)
{
    return isInit && isEnabled;
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
        case SIGNAL_STATE_IDLE:
            // Wait for signal detection
            if (leaderSignalDetectorIsSignalValid()) {
                DEBUG_PRINTI("Signal detected! Preparing for takeoff...\n");
                transitionToState(SIGNAL_STATE_DETECTED);
            }
            break;
            
        case SIGNAL_STATE_DETECTED:
            // Signal detected, ready to takeoff
            if (systemIsArmed()) {
                DEBUG_PRINTI("System armed - starting automatic takeoff!\n");
                transitionToState(SIGNAL_STATE_FOLLOWING);
            }
            break;
            
        case SIGNAL_STATE_FOLLOWING:
            // Check if we have valid signal
            if (!leaderSignalDetectorIsSignalValid()) {
                DEBUG_PRINTW("Signal lost during flight!\n");
                transitionToState(SIGNAL_STATE_LANDING);
            }
            
            // Update following time
            followingTime++;
            break;
            
        case SIGNAL_STATE_LOST:
            // Signal lost, waiting for recovery
            if (leaderSignalDetectorIsSignalValid()) {
                DEBUG_PRINTI("Signal recovered! Resuming following...\n");
                transitionToState(SIGNAL_STATE_FOLLOWING);
            }
            
            // Safety timeout (30 seconds)
            if (timeInState > M2T(30000)) {
                DEBUG_PRINTW("Signal recovery timeout - landing\n");
                transitionToState(SIGNAL_STATE_LANDING);
            }
            break;
            
        case SIGNAL_STATE_LANDING:
            // Check if landed
            if (isAltitudeValid(currentAltitude)) {
                float altitudeCm = currentAltitude * 100.0f;
                if (altitudeCm < 5.0f) {
                    DEBUG_PRINTI("Landing complete.\n");
                    transitionToState(SIGNAL_STATE_IDLE);
                }
            }
            
            // Safety timeout (60 seconds)
            if (timeInState > M2T(60000)) {
                DEBUG_PRINTW("Landing timeout\n");
                transitionToState(SIGNAL_STATE_IDLE);
            }
            break;
    }
}

/**
 * Execute actions for the current state
 */
static void executeStateActions(void)
{
    switch (currentState) {
        case SIGNAL_STATE_IDLE:
            currentThrust = 0;
            break;
            
        case SIGNAL_STATE_DETECTED:
            currentThrust = 0;
            break;
            
        case SIGNAL_STATE_FOLLOWING:
            updateAltitudeControl();
            updatePositionControl();
            break;
            
        case SIGNAL_STATE_LOST:
            // Hold current thrust
            break;
            
        case SIGNAL_STATE_LANDING:
            // Gradual thrust decrease
            if (currentThrust > thrustMin) {
                currentThrust = MAX(currentThrust - 200, thrustMin);
            } else {
                currentThrust = 0;
            }
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
        return;
    }
    
    // PID control for altitude
    pidSetDesired(&altitudePID, targetAltitude);
    float thrustAdjustment = pidUpdate(&altitudePID, currentAltitude, true);
    
    // Apply thrust adjustment to base thrust
    int newThrust = (int)(thrustBase + thrustAdjustment);
    currentThrust = (uint16_t)MAX(0, newThrust);
}

/**
 * Update position control to follow leader
 */
static void updatePositionControl(void)
{
    // Get desired position from leader data
    float desiredX = currentLeaderData.x;
    float desiredY = currentLeaderData.y;
    
    // Calculate position errors (simplified - would need actual position feedback)
    // For now, use velocity commands based on leader position
    
    // This is a placeholder - actual implementation would use position feedback
    // from external positioning system (e.g., Lighthouse, Loco Positioning)
}

/**
 * Check if altitude reading is valid
 */
static bool isAltitudeValid(float altitude)
{
    if (altitude <= 0.0f) return false;
    
    float altitudeCm = altitude * 100.0f;
    return (altitudeCm >= 5.0f && altitudeCm <= 200.0f);
}

/**
 * Transition to a new state
 */
static void transitionToState(signal_state_t newState)
{
    if (newState == currentState) {
        return;
    }
    
    DEBUG_PRINTI("Signal state transition: %d -> %d\n", currentState, newState);
    
    // Update state
    currentState = newState;
    stateEntryTime = xTaskGetTickCount();
    
    // Entry actions for new state
    switch (newState) {
        case SIGNAL_STATE_IDLE:
            currentThrust = 0;
            pidReset(&altitudePID);
            pidReset(&positionXPID);
            pidReset(&positionYPID);
            DEBUG_PRINTI("Idle - waiting for signal...\n");
            break;
            
        case SIGNAL_STATE_DETECTED:
            currentThrust = 0;
            DEBUG_PRINTI("Signal detected - ready for takeoff\n");
            break;
            
        case SIGNAL_STATE_FOLLOWING:
            currentThrust = thrustBase;
            pidReset(&altitudePID);
            pidReset(&positionXPID);
            pidReset(&positionYPID);
            DEBUG_PRINTI("Following leader at %.1f cm altitude\n", TARGET_ALTITUDE_CM_FOLLOWER);
            break;
            
        case SIGNAL_STATE_LOST:
            DEBUG_PRINTW("Signal lost - attempting recovery\n");
            break;
            
        case SIGNAL_STATE_LANDING:
            DEBUG_PRINTI("Landing initiated\n");
            break;
    }
}

/**
 * Set the follower setpoint for position hold and following
 */
static void setFollowerSetpoint(void)
{
    // Clear the setpoint
    memset(&followerSetpoint, 0, sizeof(followerSetpoint));
    
    // Set timestamp
    followerSetpoint.timestamp = xTaskGetTickCount();
    
    // Configure for stabilized attitude control with level flight
    followerSetpoint.mode.x = modeDisable;       // No position control
    followerSetpoint.mode.y = modeDisable;       // No position control
    followerSetpoint.mode.z = modeDisable;       // Direct thrust control
    followerSetpoint.mode.roll = modeAbs;        // Absolute roll angle
    followerSetpoint.mode.pitch = modeAbs;       // Absolute pitch angle
    followerSetpoint.mode.yaw = modeVelocity;    // Zero yaw rate
    
    // Zero velocities
    followerSetpoint.velocity.x = 0.0f;
    followerSetpoint.velocity.y = 0.0f;
    followerSetpoint.velocity.z = 0.0f;
    
    // Set attitudes for level flight
    followerSetpoint.attitude.roll = 0.0f;
    followerSetpoint.attitude.pitch = 0.0f;
    followerSetpoint.attitude.yaw = 0.0f;
    
    // Zero angular rates
    followerSetpoint.attitudeRate.roll = 0.0f;
    followerSetpoint.attitudeRate.pitch = 0.0f;
    followerSetpoint.attitudeRate.yaw = 0.0f;
    
    // Set thrust for altitude control
    followerSetpoint.thrust = currentThrust;
}

// ==================== LOGGING AND PARAMETERS ====================

PARAM_GROUP_START(leaderSig)
PARAM_ADD(PARAM_UINT8, enable, &isEnabled)
PARAM_ADD(PARAM_FLOAT, targetAlt, &targetAltitude)
PARAM_ADD(PARAM_FLOAT, altKp, &altitudeKp)
PARAM_ADD(PARAM_FLOAT, altKi, &altitudeKi)
PARAM_ADD(PARAM_FLOAT, altKd, &altitudeKd)
PARAM_ADD(PARAM_UINT16, thrustBase, &thrustBase)
PARAM_ADD(PARAM_UINT16, thrustMin, &thrustMin)
PARAM_ADD(PARAM_UINT16, thrustMax, &thrustMax)
PARAM_GROUP_STOP(leaderSig)

LOG_GROUP_START(leaderSig)
LOG_ADD(LOG_UINT8, enabled, &isEnabled)
LOG_ADD(LOG_UINT8, state, &currentState)
LOG_ADD(LOG_INT8, rssi, &lastRSSI)
LOG_ADD(LOG_FLOAT, currentAlt, &currentAltitude)
LOG_ADD(LOG_FLOAT, targetAlt, &targetAltitude)
LOG_ADD(LOG_UINT16, thrust, &currentThrust)
LOG_ADD(LOG_UINT32, followTime, &followingTime)
LOG_ADD(LOG_UINT32, sigLossCount, &signalLossCount)
LOG_GROUP_STOP(leaderSig)
