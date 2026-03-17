/**
 * Smart Calibration Modes for ESP-Drone
 * Professional-grade calibration modes for different flight scenarios
 * Features: Quick/Precision/Competition/Cinematic/Custom modes
 * Author: Smart Flight Enhancement System
 */

#ifndef __SMART_CALIBRATION_MODES_H__
#define __SMART_CALIBRATION_MODES_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Smart Calibration Modes
typedef enum {
    SMART_MODE_QUICK_START = 0,    // 30-second rapid calibration
    SMART_MODE_PRECISION,          // 5-minute ultra-precise calibration
    SMART_MODE_COMPETITION,        // Racing/acrobatic optimization
    SMART_MODE_CINEMATIC,          // Smooth camera/video optimization
    SMART_MODE_ENDURANCE,          // Long-flight efficiency optimization
    SMART_MODE_ACROBATIC,          // High-G maneuver optimization
    SMART_MODE_BEGINNER,           // Safe, stable, forgiving settings
    SMART_MODE_EXPERT,             // Maximum performance, responsiveness
    SMART_MODE_PAYLOAD,            // Optimized for carrying payload
    SMART_MODE_WIND_RESISTANT,     // Outdoor windy conditions
    SMART_MODE_INDOOR_PRECISE,     // Indoor precise hovering
    SMART_MODE_CUSTOM             // User-defined custom mode
} smart_calibration_mode_t;

// Flight Style Preferences
typedef enum {
    FLIGHT_STYLE_STABLE = 0,       // Maximum stability, gentle movements
    FLIGHT_STYLE_SMOOTH,           // Smooth cinematic movements
    FLIGHT_STYLE_BALANCED,         // Balanced performance/stability
    FLIGHT_STYLE_AGILE,           // Quick response, agile movements
    FLIGHT_STYLE_RACING,          // Maximum performance, racing
    FLIGHT_STYLE_ACROBATIC,       // Extreme maneuvers, flips, rolls
    FLIGHT_STYLE_CUSTOM           // User-defined characteristics
} flight_style_t;

// Environment Auto-Detection
typedef enum {
    ENV_AUTO_DETECTING = 0,        // Currently detecting environment
    ENV_INDOOR_CALM,              // Indoor, stable environment
    ENV_INDOOR_VIBRATION,         // Indoor with vibration sources
    ENV_OUTDOOR_CALM,             // Outdoor, minimal wind
    ENV_OUTDOOR_LIGHT_WIND,       // Light wind (<5 m/s)
    ENV_OUTDOOR_MODERATE_WIND,    // Moderate wind (5-10 m/s)
    ENV_OUTDOOR_STRONG_WIND,      // Strong wind (>10 m/s)
    ENV_HIGH_ALTITUDE,            // Above 1000m altitude
    ENV_TEMPERATURE_EXTREME,      // Very hot or cold conditions
    ENV_MAGNETIC_INTERFERENCE,    // High magnetic interference
    ENV_UNKNOWN                   // Unable to determine environment
} auto_detected_environment_t;

// Quick Start Calibration (30 seconds)
typedef struct {
    // Timing
    uint32_t max_duration_ms;      // Maximum 30 seconds
    uint32_t min_stable_time_ms;   // Minimum stability time
    
    // Tolerances (relaxed for speed)
    float gyro_variance_threshold; // Gyro stability threshold
    float accel_variance_threshold;// Accel stability threshold
    float mag_variance_threshold;  // Mag stability threshold
    
    // Quick algorithms
    bool use_fast_convergence;     // Use fast convergence algorithms
    bool skip_fine_tuning;         // Skip detailed fine-tuning
    bool use_previous_data;        // Use previous calibration as baseline
    
    // Quality targets (lower for speed)
    float target_confidence;       // Target confidence level (80%)
    float acceptable_error;        // Acceptable calibration error
} quick_start_config_t;

// Precision Calibration (5 minutes)
typedef struct {
    // Timing
    uint32_t max_duration_ms;      // Maximum 5 minutes
    uint32_t stability_duration_ms;// Required stability time
    
    // Tolerances (strict for precision)
    float gyro_precision_threshold;// Ultra-precise gyro threshold
    float accel_precision_threshold;// Ultra-precise accel threshold
    float mag_precision_threshold; // Ultra-precise mag threshold
    
    // Advanced algorithms
    bool multi_position_calib;     // Multi-position calibration
    bool temperature_compensation; // Temperature drift compensation
    bool vibration_analysis;       // Vibration pattern analysis
    bool cross_axis_correction;    // Cross-axis coupling correction
    
    // Quality targets (highest)
    float target_confidence;       // Target confidence level (98%)
    float maximum_error;           // Maximum allowable error
} precision_config_t;

// Competition/Racing Mode
typedef struct {
    // Performance priorities
    float response_priority;       // Prioritize quick response (0.9)
    float stability_priority;      // Stability importance (0.3)
    float agility_priority;        // Agility importance (1.0)
    
    // PID tuning for racing
    struct {
        float kp_boost[3];          // P gain boost factors
        float ki_reduction[3];      // I gain reduction factors
        float kd_boost[3];          // D gain boost factors
        float filter_frequency[3];  // Higher filter frequencies
    } racing_pids;
    
    // Motor optimization for racing
    struct {
        float max_throttle_rate;    // Maximum throttle change rate
        float motor_response_boost; // Motor response enhancement
        float thrust_linearization;// Thrust curve linearization
    } racing_motors;
    
    // Advanced racing features
    bool enable_acro_mode;         // Enable acrobatic mode features
    bool enable_air_mode;          // Enable air mode (props never stop)
    bool enable_dynamic_filtering; // Dynamic notch filtering
    float crash_detection_threshold; // Crash detection sensitivity
} competition_config_t;

// Cinematic/Camera Mode
typedef struct {
    // Smoothness priorities
    float smoothness_priority;     // Maximum smoothness (1.0)
    float response_priority;       // Reduced response (0.3)
    float vibration_dampening;     // Maximum vibration dampening (1.0)
    
    // PID tuning for smoothness
    struct {
        float kp_reduction[3];      // Reduced P gains for smoothness
        float ki_boost[3];          // Boosted I gains for stability
        float kd_smoothing[3];      // Enhanced D filtering
        float low_pass_cutoff[3];   // Lower filter frequencies
    } cinematic_pids;
    
    // Gimbal optimization
    struct {
        bool enable_gimbal_integration; // Integrate with gimbal control
        float gimbal_compensation[3];   // Gimbal movement compensation
        float vibration_isolation;      // Vibration isolation factor
    } gimbal_config;
    
    // Camera-specific features
    bool enable_expo_curves;       // Exponential stick curves
    bool enable_gentle_rates;      // Gentle rate limits
    float max_angular_velocity[3]; // Maximum angular velocities
    float acceleration_limits[3];  // Maximum accelerations
} cinematic_config_t;

// Environment Detection System
typedef struct {
    // Sensor data for detection
    float vibration_level;         // Current vibration level
    float temperature;             // Current temperature
    float magnetic_field_strength; // Magnetic field strength
    float pressure;                // Atmospheric pressure
    float gyro_noise_level[3];     // Gyro noise characteristics
    float accel_noise_level[3];    // Accel noise characteristics
    
    // Detection results
    auto_detected_environment_t detected_env;
    float detection_confidence;    // Confidence in detection (0-1)
    uint32_t detection_time_ms;    // Time taken to detect
    
    // Environmental parameters
    float estimated_wind_speed;    // Estimated wind speed
    float estimated_altitude;      // Estimated altitude
    float magnetic_declination;    // Local magnetic declination
    float temperature_drift_rate;  // Temperature change rate
    
    // Adaptive parameters based on environment
    struct {
        float gyro_filter_cutoff[3];    // Environment-specific filtering
        float accel_filter_cutoff[3];   // Accelerometer filtering
        float mag_filter_cutoff[3];     // Magnetometer filtering
        float pid_adjustment[3];        // PID adjustments for environment
    } env_adaptations;
} environment_detection_t;

// Smart Mode Configuration
typedef struct {
    smart_calibration_mode_t mode;
    flight_style_t flight_style;
    
    union {
        quick_start_config_t quick_config;
        precision_config_t precision_config;
        competition_config_t competition_config;
        cinematic_config_t cinematic_config;
    } mode_config;
    
    environment_detection_t env_detection;
    
    // Common settings
    bool auto_environment_detect;   // Enable auto environment detection
    bool adaptive_algorithms;       // Enable adaptive algorithms
    bool save_as_profile;          // Save as custom profile
    char profile_name[32];         // Profile name if saving
    
    // Quality requirements
    float minimum_confidence;       // Minimum confidence to accept
    float maximum_calibration_time; // Maximum time allowed
    uint32_t retry_count;          // Retry attempts if failed
    
    // Status
    bool is_active;                // Mode is currently active
    bool is_complete;              // Calibration complete
    float current_progress;        // Current progress (0-1)
    uint32_t elapsed_time_ms;      // Elapsed calibration time
} smart_mode_config_t;

// Performance Metrics
typedef struct {
    // Calibration quality metrics
    float gyro_accuracy;           // Gyroscope accuracy (0-100%)
    float accel_accuracy;          // Accelerometer accuracy (0-100%)
    float mag_accuracy;            // Magnetometer accuracy (0-100%)
    float overall_accuracy;        // Overall calibration accuracy
    
    // Performance characteristics
    float response_time_ms;        // Control response time
    float stability_index;         // Stability index (0-100)
    float agility_index;           // Agility index (0-100)
    float efficiency_index;        // Power efficiency (0-100)
    
    // Environmental adaptation
    float wind_rejection;          // Wind disturbance rejection
    float vibration_dampening;     // Vibration dampening effectiveness
    float temperature_stability;   // Temperature stability
    
    // Flight characteristics
    float hover_precision_mm;      // Hover precision in mm
    float max_safe_wind_speed;     // Maximum safe wind speed
    float battery_life_multiplier; // Battery life impact factor
} performance_metrics_t;

// Function Prototypes

// Main Smart Mode Functions
void smart_calibration_modes_init(void);
void smart_calibration_modes_deinit(void);

// Mode Selection and Configuration
void smart_mode_select(smart_calibration_mode_t mode);
void smart_mode_set_flight_style(flight_style_t style);
void smart_mode_configure(smart_mode_config_t *config);
smart_mode_config_t* smart_mode_get_current_config(void);

// Environment Detection
void smart_mode_start_environment_detection(void);
bool smart_mode_update_environment_detection(float sensor_data[9], float temperature, float pressure);
auto_detected_environment_t smart_mode_get_detected_environment(float *confidence);
void smart_mode_force_environment(auto_detected_environment_t env);

// Calibration Execution
void smart_mode_start_calibration(void);
bool smart_mode_update_calibration(float gyro[3], float accel[3], float mag[3], 
                                  float temperature, uint32_t timestamp);
bool smart_mode_is_calibration_complete(void);
void smart_mode_abort_calibration(void);
void smart_mode_finalize_calibration(void);

// Mode-Specific Functions
void smart_mode_quick_start_init(quick_start_config_t *config);
void smart_mode_precision_init(precision_config_t *config);
void smart_mode_competition_init(competition_config_t *config);
void smart_mode_cinematic_init(cinematic_config_t *config);

// Performance Optimization
void smart_mode_optimize_for_environment(auto_detected_environment_t env);
void smart_mode_optimize_for_payload(float payload_mass_grams);
void smart_mode_optimize_for_battery(float battery_voltage);
void smart_mode_optimize_pids(flight_style_t style);

// Real-time Monitoring
float smart_mode_get_progress(void);
void smart_mode_get_status(char *status_buffer, size_t buffer_size);
performance_metrics_t* smart_mode_get_performance_metrics(void);
void smart_mode_get_detailed_report(char *report_buffer, size_t buffer_size);

// Profile Management
void smart_mode_save_profile(const char *profile_name);
bool smart_mode_load_profile(const char *profile_name);
void smart_mode_delete_profile(const char *profile_name);
void smart_mode_list_profiles(char *profiles_buffer, size_t buffer_size);

// Adaptive Features
void smart_mode_enable_adaptive_learning(bool enable);
void smart_mode_update_from_flight_data(float flight_metrics[10]);
void smart_mode_adapt_to_conditions(float environmental_data[8]);
void smart_mode_learn_from_pilot_input(float stick_inputs[4], uint32_t duration_ms);

// Advanced Calibration Features
void smart_mode_multi_position_calibration(uint8_t positions[6]);
void smart_mode_dynamic_weight_adjustment(float weight_factors[9]);
void smart_mode_vibration_signature_analysis(float vibration_data[3]);
void smart_mode_magnetic_interference_compensation(float mag_data[3]);

// Quality Assessment
float smart_mode_calculate_calibration_score(void);
bool smart_mode_validate_calibration_quality(void);
void smart_mode_generate_quality_certificate(char *cert_buffer, size_t buffer_size);
void smart_mode_benchmark_performance(performance_metrics_t *metrics);

// Diagnostic and Testing
bool smart_mode_self_test(void);
void smart_mode_calibration_health_check(void);
float smart_mode_estimate_calibration_time(smart_calibration_mode_t mode);
void smart_mode_predict_performance(smart_calibration_mode_t mode, performance_metrics_t *predicted);

#ifdef __cplusplus
}
#endif

#endif // __SMART_CALIBRATION_MODES_H__
