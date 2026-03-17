/**
 * ESP-Drone Web Position Hold Control System
 * 
 * This system provides a simple web interface for controlling
 * position hold using the VL53L0X distance sensor.
 * 
 * Features:
 * - Simple web interface accessible via WiFi hotspot
 * - Automatic altitude holding using VL53L0X sensor
 * - RESTful API for control
 * - Safe landing functionality
 */

#ifndef WEB_POSITION_HOLD_H_
#define WEB_POSITION_HOLD_H_

#include <stdbool.h>
#include <stdint.h>
#include "esp_http_server.h"
#include "vl53l0x_enhanced.h"

// Web server configuration
#define WEB_SERVER_PORT 80
#define WEB_SERVER_MAX_HANDLERS 10
#define WEB_SERVER_URI_MATCH_WILDCARD true

// Position hold configuration
#define POSITION_HOLD_MIN_HEIGHT_CM     1.0f    // Minimum safe height (1 cm) - lowered for testing
#define POSITION_HOLD_MAX_HEIGHT_CM     200.0f  // Maximum height (2 meters)
#define POSITION_HOLD_DEFAULT_HEIGHT_CM 20.0f   // Default target height (20 cm)
#define POSITION_HOLD_TOLERANCE_CM      1.0f    // Acceptable distance tolerance
#define POSITION_HOLD_UPDATE_RATE_MS    50      // Control loop update rate

// Takeoff configuration
#define TAKEOFF_INITIAL_THRUST          25000   // Initial takeoff thrust
#define TAKEOFF_MAX_THRUST              45000   // Maximum takeoff thrust
#define TAKEOFF_TARGET_HEIGHT_CM        15.0f   // Initial takeoff height
#define TAKEOFF_TIMEOUT_MS              10000   // Maximum takeoff time (10 seconds)
#define TAKEOFF_HEIGHT_THRESHOLD_CM     8.0f    // Height to consider takeoff successful

// Thrust control parameters
#define THRUST_BASE                     35000   // Base thrust for hovering
#define THRUST_MIN                      10000   // Minimum thrust
#define THRUST_MAX                      50000   // Maximum thrust
#define THRUST_GAIN_P                   800.0f  // Proportional gain
#define THRUST_GAIN_I                   50.0f   // Integral gain
#define THRUST_GAIN_D                   200.0f  // Derivative gain

// Landing parameters
#define LANDING_DESCENT_RATE            0.5f    // cm/s descent rate during landing
#define LANDING_FINAL_HEIGHT_CM         3.0f    // Height to consider as "landed"

// Position hold states
typedef enum {
    PH_STATE_IDLE = 0,           // System idle, not controlling
    PH_STATE_TAKEOFF,            // Automatic takeoff in progress
    PH_STATE_POSITION_HOLD,      // Active position hold mode
    PH_STATE_LANDING,            // Controlled landing in progress
    PH_STATE_LANDED,             // Safely landed and stopped
    PH_STATE_ERROR               // Error state
} position_hold_state_t;

// Position hold status structure
typedef struct {
    position_hold_state_t state;
    float target_height_cm;         // Target height to maintain
    float current_height_cm;        // Current measured height
    float height_error_cm;          // Height error (target - current)
    float thrust_output;            // Current thrust output (0-65535)
    bool sensor_valid;              // VL53L0X sensor reading valid
    uint32_t active_time_ms;        // Time in current state
    uint32_t last_update_ms;        // Last control update time
    
    // PID controller state
    float pid_integral;             // Integral term accumulator
    float pid_derivative;           // Derivative term
    float pid_last_error;           // Previous error for derivative
    
    // Safety flags
    bool emergency_stop_active;     // Emergency stop triggered
    bool low_height_warning;        // Below minimum safe height
    bool high_height_warning;       // Above maximum safe height
    
    // Takeoff state
    uint32_t takeoff_start_time;    // When takeoff started
    float takeoff_initial_height;   // Ground height at takeoff start
} position_hold_status_t;

// Function declarations

/**
 * Initialize the web-based position hold system
 * @param vl53l0x_dev Pointer to initialized VL53L0X device
 * @return true if successful, false otherwise
 */
bool web_position_hold_init(VL53L0X_Dev_t *vl53l0x_dev);

/**
 * Start the web server
 * @return true if successful, false otherwise
 */
bool web_position_hold_start_server(void);

/**
 * Stop the web server
 */
void web_position_hold_stop_server(void);

/**
 * Main position hold control task - should be called regularly
 */
void web_position_hold_task(void *pvParameters);

/**
 * Set target height for position hold
 * @param height_cm Target height in centimeters
 * @return true if height is valid and set, false otherwise
 */
bool web_position_hold_set_target(float height_cm);

/**
 * Start position hold mode (with automatic takeoff if needed)
 * @return true if started successfully, false otherwise
 */
bool web_position_hold_start(void);

/**
 * Start takeoff sequence
 * @return true if takeoff started successfully, false otherwise
 */
bool web_position_hold_takeoff(void);

/**
 * Stop position hold mode (immediate stop)
 */
void web_position_hold_stop(void);

/**
 * Initiate controlled landing
 * @return true if landing started, false otherwise
 */
bool web_position_hold_land(void);

/**
 * Emergency stop - immediate thrust cut and shutdown
 */
void web_position_hold_emergency_stop(void);

/**
 * Get current position hold status
 * @return Pointer to current status structure
 */
const position_hold_status_t* web_position_hold_get_status(void);

/**
 * Check if position hold system is active
 * @return true if active, false otherwise
 */
bool web_position_hold_is_active(void);

/**
 * Test function to verify system functionality
 * @return true if all tests pass, false otherwise
 */
bool web_position_hold_test(void);

// HTTP handler functions (internal)
esp_err_t web_position_hold_handler_root(httpd_req_t *req);
esp_err_t web_position_hold_handler_api_status(httpd_req_t *req);
esp_err_t web_position_hold_handler_api_start(httpd_req_t *req);
esp_err_t web_position_hold_handler_api_stop(httpd_req_t *req);
esp_err_t web_position_hold_handler_api_land(httpd_req_t *req);
esp_err_t web_position_hold_handler_api_set_height(httpd_req_t *req);

// Internal control functions
float web_position_hold_calculate_thrust(float height_error_cm, float dt_ms);
bool web_position_hold_update_sensor_reading(void);
void web_position_hold_apply_thrust(float thrust_value);
void web_position_hold_safety_checks(void);

// Integration functions
bool web_position_hold_integration_init(void);
bool web_position_hold_integration_test(void);
bool web_position_hold_integration_is_ready(void);

#endif // WEB_POSITION_HOLD_H_
