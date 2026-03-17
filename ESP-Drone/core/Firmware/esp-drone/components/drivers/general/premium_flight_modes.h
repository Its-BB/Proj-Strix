/**
 * Premium Flight Modes for ESP-Drone
 * Advanced flight modes with cinematic controls, precision hovering, and AI assistance
 * Features: Smooth cinematography, GPS waypoints, advanced stabilization
 * Author: Premium Flight Enhancement System
 */

#ifndef __PREMIUM_FLIGHT_MODES_H__
#define __PREMIUM_FLIGHT_MODES_H__

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// Premium Flight Modes
typedef enum {
    FLIGHT_MODE_STANDARD = 0,      // Standard flight mode
    FLIGHT_MODE_CINEMATIC,         // Ultra-smooth cinematic mode
    FLIGHT_MODE_PRECISION_HOVER,   // Precision hovering mode
    FLIGHT_MODE_FOLLOW_ME,         // GPS follow target mode
    FLIGHT_MODE_ORBIT,             // Circular orbit around target
    FLIGHT_MODE_WAYPOINT,          // GPS waypoint navigation
    FLIGHT_MODE_RETURN_HOME,       // Intelligent return to home
    FLIGHT_MODE_LANDING_ASSIST,    // Precision landing assistance
    FLIGHT_MODE_ACROBATIC,         // Acrobatic/sport mode
    FLIGHT_MODE_BEGINNER_SAFE,     // Safe beginner mode
    FLIGHT_MODE_WIND_RESISTANT,    // High wind compensation
    FLIGHT_MODE_PAYLOAD_STABLE,    // Stable payload carrying
    FLIGHT_MODE_ENERGY_EFFICIENT,  // Battery-saving flight
    FLIGHT_MODE_INDOOR_PRECISE,    // Indoor precision navigation
    FLIGHT_MODE_AUTONOMOUS,        // Full autonomous operation
    FLIGHT_MODE_CUSTOM            // User-defined custom mode
} premium_flight_mode_t;

// Cinematic Movement Types
typedef enum {
    CINEMATIC_DOLLY = 0,           // Forward/backward movement
    CINEMATIC_TRUCK,               // Left/right movement  
    CINEMATIC_PAN,                 // Horizontal rotation
    CINEMATIC_TILT,                // Vertical rotation
    CINEMATIC_PEDESTAL,            // Vertical movement
    CINEMATIC_ZOOM,                // Move closer/further
    CINEMATIC_REVEAL,              // Rising reveal shot
    CINEMATIC_ORBIT_LEFT,          // Orbit counterclockwise
    CINEMATIC_ORBIT_RIGHT,         // Orbit clockwise
    CINEMATIC_SPIRAL_UP,           // Spiral upward
    CINEMATIC_SPIRAL_DOWN,         // Spiral downward
    CINEMATIC_BOOMERANG,           // Boomerang shot
    CINEMATIC_DRONIE,              // Backward and up
    CINEMATIC_HELIX,               // Helical movement
    CINEMATIC_CABLE_CAM,           // Point-to-point movement
    CINEMATIC_CUSTOM               // Custom movement pattern
} cinematic_movement_t;

// GPS Waypoint Structure
typedef struct {
    double latitude;               // GPS latitude
    double longitude;              // GPS longitude
    float altitude;                // Altitude above home (meters)
    float speed;                   // Target speed to waypoint (m/s)
    float heading;                 // Target heading at waypoint (degrees)
    uint16_t dwell_time_ms;        // Time to hover at waypoint
    cinematic_movement_t movement_type; // Movement to next waypoint
    bool camera_action;            // Trigger camera at waypoint
    uint8_t gimbal_pitch;          // Gimbal pitch angle at waypoint
} gps_waypoint_t;

// Precision Hover Configuration
typedef struct {
    // Position accuracy targets
    float horizontal_accuracy_cm;   // Horizontal position accuracy
    float vertical_accuracy_cm;     // Vertical position accuracy
    float heading_accuracy_deg;     // Heading accuracy in degrees
    
    // Hover behavior
    float max_drift_speed_cm_s;     // Maximum allowed drift speed
    float position_correction_rate; // Position correction aggressiveness
    float altitude_hold_strength;   // Altitude hold strength
    float heading_hold_strength;    // Heading hold strength
    
    // Environmental compensation
    bool enable_wind_compensation;  // Compensate for wind
    bool enable_gps_fusion;         // Fuse GPS for position hold
    bool enable_optical_flow;       // Use optical flow sensors
    bool enable_downward_camera;    // Use downward camera for positioning
    
    // Advanced features
    bool adaptive_gains;            // Adaptive PID gains based on conditions
    bool predictive_correction;     // Predict and preemptively correct drift
    float stability_timeout_s;      // Time to achieve stable hover
} precision_hover_config_t;

// Cinematic Mode Configuration
typedef struct {
    // Movement characteristics
    float max_velocity_m_s;         // Maximum movement velocity
    float max_acceleration_m_s2;    // Maximum acceleration
    float max_angular_velocity_deg_s; // Maximum rotation speed
    float max_angular_accel_deg_s2; // Maximum angular acceleration
    
    // Smoothness settings
    float motion_smoothing_factor;  // Motion smoothing (0-1)
    float rotation_smoothing_factor;// Rotation smoothing (0-1)
    float expo_curve_strength;      // Exponential curve strength
    bool enable_motion_blur_compensation; // Compensate for motion blur
    
    // Camera integration
    bool gimbal_follow_mode;        // Gimbal follows aircraft movement
    float gimbal_smoothing;         // Gimbal movement smoothing
    bool enable_focus_tracking;     // Track focus point during movement
    bool auto_exposure_lock;        // Lock exposure during movement
    
    // Advanced cinematic features
    bool enable_cable_cam_mode;     // Cable cam simulation
    bool enable_smooth_transitions; // Smooth transitions between shots
    float transition_time_s;        // Time for mode transitions
} cinematic_config_t;

// Follow-Me Mode Configuration
typedef struct {
    // Target tracking
    double target_latitude;         // Target GPS position
    double target_longitude;
    float target_altitude;
    float follow_distance_m;        // Distance to maintain from target
    float follow_height_m;          // Height above target
    float follow_angle_deg;         // Angle relative to target movement
    
    // Tracking behavior
    float max_follow_speed_m_s;     // Maximum speed when following
    float tracking_smoothing;       // Target tracking smoothing
    bool maintain_orientation;      // Keep facing target
    bool predictive_following;      // Predict target movement
    
    // Safety features
    float max_follow_distance_m;    // Maximum distance from target
    float min_follow_distance_m;    // Minimum distance from target
    float obstacle_avoidance_distance_m; // Obstacle avoidance distance
    bool return_if_target_lost;     // Return home if target lost
    uint32_t target_lost_timeout_s; // Timeout before returning home
} follow_me_config_t;

// Orbit Mode Configuration
typedef struct {
    // Orbit parameters
    double center_latitude;         // Orbit center GPS
    double center_longitude;
    float center_altitude;
    float orbit_radius_m;           // Orbit radius in meters
    float orbit_speed_deg_s;        // Orbit speed in degrees per second
    float orbit_altitude_m;         // Altitude during orbit
    
    // Orbit behavior
    bool clockwise_direction;       // Orbit direction
    bool maintain_camera_on_center; // Keep camera pointed at center
    float altitude_variation_m;     // Altitude variation during orbit
    bool enable_spiral_orbit;       // Spiral inward/outward during orbit
    float spiral_rate_m_per_deg;    // Spiral rate
    
    // Dynamic adjustments
    bool adaptive_speed;            // Adjust speed based on conditions
    bool wind_compensation;         // Compensate for wind during orbit
    float max_orbit_speed_m_s;      // Maximum linear speed during orbit
} orbit_config_t;

// Waypoint Navigation Configuration
typedef struct {
    gps_waypoint_t waypoints[16];   // Up to 16 waypoints
    uint8_t waypoint_count;         // Number of active waypoints
    uint8_t current_waypoint;       // Currently navigating to this waypoint
    
    // Navigation behavior
    float waypoint_radius_m;        // Acceptance radius for waypoints
    float cruise_speed_m_s;         // Cruise speed between waypoints
    float approach_speed_m_s;       // Speed when approaching waypoints
    bool smooth_turns;              // Smooth turns between waypoints
    float turn_anticipation_m;      // Distance to start turning
    
    // Mission settings
    bool repeat_mission;            // Repeat waypoint mission
    bool return_home_after;         // Return home after mission
    bool pause_at_waypoints;        // Pause at each waypoint
    uint32_t mission_timeout_s;     // Maximum mission time
    
    // Safety features
    float max_altitude_m;           // Maximum altitude during mission
    float min_altitude_m;           // Minimum altitude during mission
    bool obstacle_avoidance;        // Enable obstacle avoidance
    float safe_corridor_width_m;    // Safe corridor width
} waypoint_navigation_config_t;

// Advanced Stabilization System
typedef struct {
    // Multi-layer stabilization
    struct {
        float gyro_stabilization;   // Gyroscopic stabilization strength
        float accelerometer_fusion; // Accelerometer fusion weight
        float magnetometer_fusion;  // Magnetometer fusion weight
        float gps_fusion;           // GPS fusion weight
        float optical_flow_fusion;  // Optical flow fusion weight
    } sensor_fusion;
    
    // Predictive stabilization
    struct {
        bool enable_predictive;     // Enable predictive stabilization
        float prediction_horizon_ms; // Prediction time horizon
        float prediction_confidence; // Confidence in predictions
        bool adapt_to_flight_style;  // Adapt to pilot's flight style
    } predictive;
    
    // Environmental adaptation
    struct {
        bool wind_detection;        // Automatic wind detection
        float wind_compensation;    // Wind compensation strength
        bool turbulence_dampening;  // Turbulence dampening
        float vibration_isolation;  // Vibration isolation factor
    } environmental;
    
    // Advanced algorithms
    struct {
        bool neural_network_assist; // AI assistance for stabilization
        bool kalman_filtering;      // Advanced Kalman filtering
        bool complementary_filtering; // Complementary filtering
        bool adaptive_pid_tuning;   // Adaptive PID tuning
    } algorithms;
} advanced_stabilization_t;

// Flight Mode Status
typedef struct {
    premium_flight_mode_t current_mode;
    premium_flight_mode_t previous_mode;
    uint32_t mode_start_time_ms;
    bool mode_transition_active;
    float transition_progress;      // 0.0 - 1.0
    
    // Current flight state
    float current_velocity[3];      // [x, y, z] velocity in m/s
    float current_position[3];      // [x, y, z] position relative to home
    float current_heading_deg;      // Current heading in degrees
    float current_altitude_m;       // Current altitude in meters
    
    // Target state
    float target_velocity[3];       // Target velocity
    float target_position[3];       // Target position
    float target_heading_deg;       // Target heading
    float target_altitude_m;        // Target altitude
    
    // Performance metrics
    float position_accuracy_cm;     // Current position accuracy
    float velocity_accuracy_cm_s;   // Current velocity accuracy
    float stability_index;          // Overall stability (0-100)
    float smoothness_index;         // Movement smoothness (0-100)
    float energy_efficiency;        // Energy efficiency (0-100)
    
    // Status flags
    bool gps_available;             // GPS signal available
    bool optical_flow_available;    // Optical flow available
    bool stable_hover_achieved;     // Stable hover achieved
    bool waypoint_navigation_active;// Waypoint navigation active
    bool obstacle_detected;         // Obstacle detected
    bool emergency_mode_active;     // Emergency mode active
} flight_mode_status_t;

// Function Prototypes

// Main Flight Mode Functions
void premium_flight_modes_init(void);
void premium_flight_modes_deinit(void);
void premium_flight_modes_update(void);

// Mode Control
void premium_flight_set_mode(premium_flight_mode_t mode);
premium_flight_mode_t premium_flight_get_mode(void);
bool premium_flight_is_mode_available(premium_flight_mode_t mode);
void premium_flight_enable_smooth_transitions(bool enable);

// Cinematic Mode
void premium_flight_cinematic_init(cinematic_config_t *config);
void premium_flight_cinematic_start_movement(cinematic_movement_t movement, float duration_s);
void premium_flight_cinematic_set_target(float target_position[3], float target_heading);
void premium_flight_cinematic_execute_shot(cinematic_movement_t shot_type, float parameters[4]);
bool premium_flight_cinematic_is_shot_complete(void);

// Precision Hover Mode
void premium_flight_precision_hover_init(precision_hover_config_t *config);
void premium_flight_precision_hover_set_target(float position[3], float heading);
bool premium_flight_precision_hover_is_stable(void);
float premium_flight_precision_hover_get_accuracy(void);
void premium_flight_precision_hover_emergency_land(void);

// Follow-Me Mode
void premium_flight_follow_me_init(follow_me_config_t *config);
void premium_flight_follow_me_set_target(double lat, double lon, float alt);
void premium_flight_follow_me_update_target(double lat, double lon, float alt);
bool premium_flight_follow_me_target_in_range(void);
void premium_flight_follow_me_adjust_distance(float distance_m);

// Orbit Mode
void premium_flight_orbit_init(orbit_config_t *config);
void premium_flight_orbit_set_center(double lat, double lon, float alt);
void premium_flight_orbit_start(float radius_m, float speed_deg_s);
void premium_flight_orbit_adjust_radius(float new_radius_m);
void premium_flight_orbit_change_direction(void);

// Waypoint Navigation
void premium_flight_waypoint_init(waypoint_navigation_config_t *config);
void premium_flight_waypoint_add(gps_waypoint_t *waypoint);
void premium_flight_waypoint_start_mission(void);
void premium_flight_waypoint_pause_mission(void);
void premium_flight_waypoint_resume_mission(void);
void premium_flight_waypoint_skip_current(void);
bool premium_flight_waypoint_mission_complete(void);

// Advanced Stabilization
void premium_flight_stabilization_init(advanced_stabilization_t *config);
void premium_flight_stabilization_update(float sensor_data[9]);
void premium_flight_stabilization_tune_for_conditions(void);
void premium_flight_stabilization_enable_predictive(bool enable);

// Safety Systems
bool premium_flight_safety_check(void);
void premium_flight_emergency_land(void);
void premium_flight_return_to_home(void);
void premium_flight_set_geofence(float radius_m, float max_altitude_m);
bool premium_flight_is_in_geofence(void);

// GPS and Navigation
bool premium_flight_gps_ready(void);
void premium_flight_set_home_position(double lat, double lon, float alt);
void premium_flight_get_current_position(double *lat, double *lon, float *alt);
float premium_flight_get_distance_to_home(void);
float premium_flight_get_bearing_to_home(void);

// Flight Characteristics Tuning
void premium_flight_set_agility(float agility_factor);      // 0.0 = stable, 1.0 = agile
void premium_flight_set_smoothness(float smoothness_factor);// 0.0 = responsive, 1.0 = smooth
void premium_flight_set_efficiency(float efficiency_factor);// 0.0 = performance, 1.0 = efficient
void premium_flight_auto_tune_for_payload(float payload_mass_g);

// Status and Monitoring
void premium_flight_get_status(flight_mode_status_t *status);
float premium_flight_get_battery_time_remaining(void);
void premium_flight_get_performance_report(char *report_buffer, size_t buffer_size);
bool premium_flight_calibration_valid(void);

// Custom Mode Configuration
void premium_flight_create_custom_mode(const char *mode_name, void *config);
bool premium_flight_load_custom_mode(const char *mode_name);
void premium_flight_save_flight_profile(const char *profile_name);
bool premium_flight_load_flight_profile(const char *profile_name);

// AI-Assisted Flight
void premium_flight_enable_ai_assist(bool enable);
void premium_flight_ai_learn_from_flight(void);
void premium_flight_ai_suggest_optimizations(char *suggestions_buffer, size_t buffer_size);
void premium_flight_ai_auto_tune(void);

// Obstacle Avoidance
void premium_flight_enable_obstacle_avoidance(bool enable);
void premium_flight_set_obstacle_sensors(bool front, bool back, bool left, bool right, bool top, bool bottom);
bool premium_flight_obstacle_detected(void);
float premium_flight_get_obstacle_distance(uint8_t direction);

// Camera and Gimbal Integration
void premium_flight_camera_set_target(float target_position[3]);
void premium_flight_camera_set_mode(uint8_t camera_mode);
void premium_flight_gimbal_control(float pitch, float yaw, float roll);
void premium_flight_gimbal_follow_target(bool enable);

#ifdef __cplusplus
}
#endif

#endif // __PREMIUM_FLIGHT_MODES_H__
