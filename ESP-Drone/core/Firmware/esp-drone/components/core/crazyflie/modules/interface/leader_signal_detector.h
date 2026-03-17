/**
 * ESP-Drone Leader Signal Detection Module
 * 
 * Detects ESP32 broadcast signals and enables automatic follower mode
 * Features:
 * - ESP-NOW signal detection
 * - Signal strength tracking (RSSI)
 * - Automatic takeoff on signal detection
 * - Position hold at 35cm altitude
 * - Signal following/tracking
 */

#ifndef __LEADER_SIGNAL_DETECTOR_H__
#define __LEADER_SIGNAL_DETECTOR_H__

#include <stdint.h>
#include <stdbool.h>
#include "stabilizer_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Signal detection states
typedef enum {
    SIGNAL_STATE_IDLE = 0,           // No signal detected
    SIGNAL_STATE_DETECTED,           // Signal detected, waiting for takeoff
    SIGNAL_STATE_FOLLOWING,          // Following leader signal
    SIGNAL_STATE_LOST,               // Signal lost during flight
    SIGNAL_STATE_LANDING             // Landing due to signal loss
} signal_state_t;

// Leader position data structure
typedef struct {
    float x;                         // X position (meters)
    float y;                         // Y position (meters)
    float z;                         // Z position (meters, altitude)
    float vx;                        // X velocity
    float vy;                        // Y velocity
    float vz;                        // Z velocity
    int8_t rssi;                     // Signal strength (dBm)
    uint32_t timestamp;              // Timestamp of last update
    uint8_t leader_id;               // Leader drone ID
} leader_data_t;

// Configuration constants
#define SIGNAL_DETECTION_TIMEOUT_MS  5000    // Timeout for signal loss detection
#define TARGET_ALTITUDE_CM_FOLLOWER  35      // Target altitude 35cm
#define TARGET_ALTITUDE_M_FOLLOWER   0.35f   // Target altitude in meters
#define SIGNAL_RSSI_THRESHOLD        -80     // Minimum RSSI for valid signal
#define POSITION_UPDATE_RATE_HZ      10      // Position update rate

/**
 * Initialize the leader signal detection module
 */
void leaderSignalDetectorInit(void);

/**
 * Test the leader signal detection module
 * @return true if initialization successful
 */
bool leaderSignalDetectorTest(void);

/**
 * Update leader signal detection and following
 * Should be called from the stabilizer loop
 * @param setpoint Setpoint to modify if following mode is active
 * @param state Current state of the drone
 * @param tick Current tick count
 */
void leaderSignalDetectorUpdate(setpoint_t *setpoint, const state_t *state, uint32_t tick);

/**
 * Callback for receiving leader position data via ESP-NOW
 * @param leader_data Pointer to received leader data
 */
void leaderSignalDetectorReceiveData(const leader_data_t *leader_data);

/**
 * Get current signal detection state
 * @return Current signal state
 */
signal_state_t leaderSignalDetectorGetState(void);

/**
 * Get last received leader data
 * @return Pointer to last leader data
 */
const leader_data_t* leaderSignalDetectorGetLeaderData(void);

/**
 * Check if signal is currently detected
 * @return true if signal detected and valid
 */
bool leaderSignalDetectorIsSignalValid(void);

/**
 * Get signal strength (RSSI)
 * @return RSSI value in dBm
 */
int8_t leaderSignalDetectorGetRSSI(void);

/**
 * Enable/disable follower mode
 * @param enable true to enable, false to disable
 */
void leaderSignalDetectorEnable(bool enable);

/**
 * Check if follower mode is enabled
 * @return true if enabled
 */
bool leaderSignalDetectorIsEnabled(void);

#ifdef __cplusplus
}
#endif

#endif /* __LEADER_SIGNAL_DETECTOR_H__ */
