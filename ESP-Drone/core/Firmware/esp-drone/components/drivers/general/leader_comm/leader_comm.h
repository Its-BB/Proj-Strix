/**
 * ESP-Drone Leader Communication Module
 * 
 * This module handles ESP-NOW communication with the leader ESP32,
 * calculates distance based on RSSI, and manages follow-leader mode.
 */

#ifndef __LEADER_COMM_H__
#define __LEADER_COMM_H__

#include <stdint.h>
#include <stdbool.h>
// Forward declarations - avoid circular dependencies
typedef struct setpoint_s setpoint_t;
typedef struct state_s state_t;

// Configuration constants
#define LEADER_COMM_CHANNEL 1
#define LEADER_MAC_SIZE 6
#define MAX_LEADER_DISTANCE_CM 1000  // Maximum tracking distance in centimeters
#define MIN_LEADER_DISTANCE_CM 50    // Minimum safe distance
#define LEADER_SIGNAL_TIMEOUT_MS 2000 // Timeout for leader signal loss
#define DISTANCE_SMOOTHING_SAMPLES 5  // Number of samples for distance smoothing

// Leader packet structure (must match leader ESP32)
typedef struct __attribute__((packed)) {
    uint8_t type;           // Packet type identifier (0x42 for beacon)
    uint32_t sequence;      // Sequence number for packet tracking
    uint32_t timestamp;     // Timestamp from leader
    uint8_t reserved[8];    // Reserved for future use
} leader_packet_t;

// Leader tracking data
typedef struct {
    bool detected;              // Is leader currently detected
    int16_t distance_cm;        // Distance to leader in centimeters
    int8_t rssi;               // Signal strength
    uint32_t last_seen_ms;     // Last time leader was detected
    uint32_t packet_count;     // Total packets received
    uint32_t sequence_errors;  // Sequence number errors
    float signal_quality;      // Signal quality percentage (0-100)
    
    // For distance smoothing
    int16_t distance_buffer[DISTANCE_SMOOTHING_SAMPLES];
    uint8_t buffer_index;
    bool buffer_full;
} leader_status_t;

// Flight mode for leader following
typedef enum {
    LEADER_FOLLOW_DISABLED = 0,
    LEADER_FOLLOW_ENABLED = 1,
    LEADER_FOLLOW_LOST_SIGNAL = 2,
    LEADER_FOLLOW_TOO_CLOSE = 3,
    LEADER_FOLLOW_TOO_FAR = 4
} leader_follow_mode_t;

// Leader following parameters
typedef struct {
    bool enabled;                    // Follow mode enabled
    int16_t target_distance_cm;      // Target following distance
    int16_t distance_tolerance_cm;   // Distance tolerance
    float follow_speed_factor;       // Speed multiplier for following (0.0-1.0)
    float altitude_lock;            // Locked altitude when following starts
    bool altitude_lock_active;      // Whether altitude lock is active
    leader_follow_mode_t mode;      // Current following mode
} leader_follow_config_t;

/**
 * Initialize the leader communication system
 * 
 * @return true if initialization successful, false otherwise
 */
bool leaderCommInit(void);

/**
 * Test the leader communication system
 * 
 * @return true if test passed, false otherwise
 */
bool leaderCommTest(void);

/**
 * Get current leader status
 * 
 * @return pointer to leader status structure
 */
const leader_status_t* leaderCommGetStatus(void);

/**
 * Get current leader following configuration
 * 
 * @return pointer to leader following config
 */
const leader_follow_config_t* leaderCommGetFollowConfig(void);

/**
 * Enable/disable leader following mode
 * 
 * @param enabled true to enable, false to disable
 * @param target_distance_cm target following distance in centimeters
 */
void leaderCommSetFollowMode(bool enabled, int16_t target_distance_cm);

/**
 * Update leader following setpoint based on current leader position
 * Called from stabilizer loop to adjust flight controls
 * 
 * @param setpoint current setpoint to modify
 * @param state current drone state
 * @return true if setpoint was modified, false otherwise
 */
bool leaderCommUpdateSetpoint(setpoint_t *setpoint, const state_t *state);

/**
 * Get distance to leader in centimeters
 * 
 * @return distance in cm, -1 if leader not detected
 */
int16_t leaderCommGetDistance(void);

/**
 * Get signal strength to leader
 * 
 * @return RSSI value, 0 if leader not detected
 */
int8_t leaderCommGetSignalStrength(void);

/**
 * Check if leader is currently detected
 * 
 * @return true if leader detected within timeout period
 */
bool leaderCommIsDetected(void);

/**
 * Get signal quality percentage
 * 
 * @return signal quality 0-100%
 */
float leaderCommGetSignalQuality(void);

/**
 * Reset leader communication statistics
 */
void leaderCommResetStats(void);

/**
 * Set leader following parameters
 * 
 * @param target_distance_cm target following distance
 * @param tolerance_cm distance tolerance
 * @param speed_factor speed multiplier (0.0-1.0)
 */
void leaderCommSetFollowParams(int16_t target_distance_cm, 
                              int16_t tolerance_cm, 
                              float speed_factor);

/**
 * Lock current altitude for leader following
 */
void leaderCommLockAltitude(void);

/**
 * Unlock altitude lock
 */
void leaderCommUnlockAltitude(void);

#endif /* __LEADER_COMM_H__ */
