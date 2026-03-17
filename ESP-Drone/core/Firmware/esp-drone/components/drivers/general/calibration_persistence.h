/**
 * Advanced Calibration Persistence System for ESP-Drone
 * Smart data storage, backup, restore, and profile management
 * Features: Multiple profiles, cloud sync, automatic backup, data integrity
 * Author: Calibration Persistence Enhancement System
 */

#ifndef __CALIBRATION_PERSISTENCE_H__
#define __CALIBRATION_PERSISTENCE_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Storage Locations
typedef enum {
    STORAGE_INTERNAL_FLASH = 0,     // Internal flash memory
    STORAGE_EXTERNAL_FLASH,         // External SPI flash
    STORAGE_SD_CARD,                // SD card storage
    STORAGE_NVRAM,                  // Non-volatile RAM
    STORAGE_EEPROM,                 // EEPROM storage
    STORAGE_CLOUD,                  // Cloud storage
    STORAGE_NETWORK_SHARE,          // Network file share
    STORAGE_USB_DRIVE               // USB storage device
} storage_location_t;

// Profile Types
typedef enum {
    PROFILE_TYPE_QUICK = 0,         // Quick calibration profile
    PROFILE_TYPE_PRECISION,         // Precision calibration profile
    PROFILE_TYPE_COMPETITION,       // Competition/racing profile
    PROFILE_TYPE_CINEMATIC,         // Cinematic/smooth profile
    PROFILE_TYPE_BEGINNER,          // Beginner-friendly profile
    PROFILE_TYPE_EXPERT,            // Expert/advanced profile
    PROFILE_TYPE_PAYLOAD,           // Payload carrying profile
    PROFILE_TYPE_WEATHER_SPECIFIC,  // Weather-specific profile
    PROFILE_TYPE_CUSTOM,            // Custom user profile
    PROFILE_TYPE_FACTORY_DEFAULT    // Factory default profile
} calibration_profile_type_t;

// Calibration Data Version
typedef struct {
    uint16_t major;                 // Major version number
    uint16_t minor;                 // Minor version number
    uint16_t patch;                 // Patch version number
    uint32_t build;                 // Build number
    char firmware_version[32];      // Firmware version string
    uint32_t calibration_format_version; // Calibration data format version
} calibration_version_t;

// Environmental Conditions at Calibration
typedef struct {
    float temperature_c;            // Temperature during calibration
    float humidity_percent;         // Humidity level
    float pressure_hpa;             // Atmospheric pressure
    float magnetic_declination_deg; // Magnetic declination
    float altitude_m;               // Altitude during calibration
    double gps_latitude;            // GPS coordinates
    double gps_longitude;
    uint32_t timestamp;             // Unix timestamp
    char location_name[64];         // Human-readable location name
    char weather_conditions[32];    // Weather description
} environmental_snapshot_t;

// Calibration Quality Metrics
typedef struct {
    // Accuracy Metrics
    float gyro_accuracy_percent;    // Gyroscope accuracy
    float accel_accuracy_percent;   // Accelerometer accuracy
    float mag_accuracy_percent;     // Magnetometer accuracy
    float overall_accuracy_percent; // Overall accuracy
    
    // Precision Metrics
    float gyro_repeatability;       // Gyroscope repeatability
    float accel_repeatability;      // Accelerometer repeatability
    float mag_repeatability;        // Magnetometer repeatability
    
    // Stability Metrics
    float short_term_stability;     // Short-term stability
    float long_term_stability;      // Long-term stability
    float temperature_stability;    // Temperature stability
    
    // Performance Metrics
    float convergence_time_s;       // Time to converge
    uint32_t samples_used;          // Number of samples used
    float noise_rejection_db;       // Noise rejection performance
    
    // Quality Score
    float quality_score;            // Overall quality score (0-100)
    char quality_grade;             // Quality grade (A-F)
    bool meets_specifications;      // Meets minimum specifications
} calibration_quality_t;

// Hardware Configuration
typedef struct {
    // Board Information
    char board_type[32];            // Board type/model
    char serial_number[64];         // Board serial number
    uint32_t hardware_revision;    // Hardware revision number
    
    // Sensor Information
    char gyro_model[32];            // Gyroscope model
    char accel_model[32];           // Accelerometer model
    char mag_model[32];             // Magnetometer model
    char baro_model[32];            // Barometer model
    
    // Sensor Configurations
    uint16_t gyro_sample_rate;      // Gyro sampling rate
    uint8_t gyro_range;             // Gyro measurement range
    uint16_t accel_sample_rate;     // Accel sampling rate
    uint8_t accel_range;            // Accel measurement range
    uint16_t mag_sample_rate;       // Mag sampling rate
    uint8_t mag_range;              // Mag measurement range
    
    // Motor and ESC Information
    char motor_model[32];           // Motor model
    char esc_model[32];             // ESC model
    uint8_t motor_count;            // Number of motors
    uint16_t motor_kv_rating;       // Motor KV rating
} hardware_configuration_t;

// Complete Calibration Data
typedef struct {
    // Header Information
    uint32_t magic_number;          // File format magic number
    calibration_version_t version;  // Data version information
    uint32_t data_size;             // Total data size
    uint32_t checksum;              // Data integrity checksum
    uint32_t creation_time;         // Creation timestamp
    uint32_t modification_time;     // Last modification timestamp
    
    // Profile Information
    calibration_profile_type_t profile_type;
    char profile_name[64];          // User-friendly profile name
    char profile_description[256];  // Profile description
    char created_by[64];            // Creator/user name
    uint32_t usage_count;           // Number of times used
    float average_flight_time_hours;// Average flight time with profile
    
    // Environmental Context
    environmental_snapshot_t environment;
    
    // Hardware Configuration
    hardware_configuration_t hardware;
    
    // Calibration Quality
    calibration_quality_t quality;
    
    // Sensor Calibration Data
    struct {
        // Gyroscope Calibration
        float gyro_bias[3];          // Bias values [x, y, z]
        float gyro_scale[3];         // Scale factors [x, y, z]
        float gyro_cross_coupling[9]; // Cross-coupling matrix
        float gyro_noise_variance[3]; // Noise characteristics
        float gyro_temperature_compensation[12]; // Temperature compensation
        
        // Accelerometer Calibration  
        float accel_bias[3];         // Bias values [x, y, z]
        float accel_scale[3];        // Scale factors [x, y, z]
        float accel_cross_coupling[9]; // Cross-coupling matrix
        float accel_noise_variance[3]; // Noise characteristics
        float accel_temperature_compensation[12]; // Temperature compensation
        
        // Magnetometer Calibration
        float mag_bias[3];           // Bias values [x, y, z]
        float mag_scale[3];          // Scale factors [x, y, z]
        float mag_cross_coupling[9]; // Cross-coupling matrix
        float mag_noise_variance[3]; // Noise characteristics
        float mag_declination;       // Local magnetic declination
        
        // Advanced Calibration Parameters
        float vibration_compensation[6]; // Vibration compensation
        float thermal_drift_model[16];   // Thermal drift model
        float aging_compensation[8];     // Sensor aging compensation
    } sensor_data;
    
    // Flight Controller Calibration
    struct {
        // PID Parameters
        float pid_roll[3];           // Roll PID [P, I, D]
        float pid_pitch[3];          // Pitch PID [P, I, D]
        float pid_yaw[3];            // Yaw PID [P, I, D]
        float pid_altitude[3];       // Altitude PID [P, I, D]
        
        // Motor Calibration
        float motor_idle_pwm[4];     // Motor idle PWM values
        float motor_max_pwm[4];      // Motor maximum PWM values
        float motor_curve[4][8];     // Motor thrust curves
        float motor_balance[4];      // Motor balance factors
        
        // Filter Settings
        float gyro_filter_cutoff[3]; // Gyro filter cutoff frequencies
        float accel_filter_cutoff[3];// Accel filter cutoff frequencies
        float notch_filter_freq[4];  // Notch filter frequencies
        float notch_filter_bandwidth[4]; // Notch filter bandwidths
    } flight_controller;
    
    // AI Learning Data
    struct {
        uint32_t training_samples;   // Number of training samples
        float learning_rate;         // Final learning rate
        float convergence_metric;    // Convergence quality metric
        uint8_t neural_weights[512]; // Neural network weights (compressed)
        uint32_t training_time_ms;   // Training time
        bool converged;              // Training converged successfully
    } ai_data;
    
    // Usage Statistics
    struct {
        uint32_t total_flight_time_s; // Total flight time with profile
        uint32_t total_flights;       // Total number of flights
        uint32_t crashes;             // Number of crashes
        float average_battery_usage;  // Average battery usage per flight
        float max_altitude_reached;   // Maximum altitude reached
        float max_distance_traveled;  // Maximum distance traveled
        uint32_t last_used_timestamp; // Last time profile was used
    } statistics;
    
} complete_calibration_data_t;

// Backup Configuration
typedef struct {
    bool auto_backup_enabled;       // Enable automatic backups
    uint16_t backup_interval_hours; // Backup interval in hours
    uint8_t max_backups;            // Maximum number of backups
    storage_location_t backup_locations[4]; // Backup storage locations
    bool backup_to_cloud;           // Enable cloud backup
    bool compress_backups;          // Compress backup files
    bool encrypt_backups;           // Encrypt backup files
    char encryption_key[32];        // Encryption key
} backup_configuration_t;

// Cloud Sync Configuration
typedef struct {
    bool cloud_sync_enabled;        // Enable cloud synchronization
    char cloud_provider[32];        // Cloud provider name
    char api_endpoint[128];         // Cloud API endpoint
    char api_key[64];               // Cloud API key
    char username[32];              // Cloud username
    bool auto_sync;                 // Automatic synchronization
    uint16_t sync_interval_minutes; // Sync interval
    bool sync_on_calibration;       // Sync after calibration
    bool sync_bidirectional;        // Two-way sync
} cloud_sync_configuration_t;

// Profile Management Status
typedef struct {
    uint8_t total_profiles;         // Total number of profiles
    uint8_t active_profile_index;   // Currently active profile
    char active_profile_name[64];   // Active profile name
    uint32_t active_profile_load_time; // When active profile was loaded
    
    // Storage Status
    uint32_t storage_used_bytes;    // Storage space used
    uint32_t storage_available_bytes; // Available storage space
    float storage_fragmentation_percent; // Storage fragmentation
    
    // Backup Status
    uint32_t last_backup_time;      // Last backup timestamp
    uint8_t backup_count;           // Number of backups
    bool backup_healthy;            // Backup system healthy
    
    // Cloud Sync Status
    uint32_t last_cloud_sync_time;  // Last cloud sync timestamp
    bool cloud_connected;           // Cloud connection status
    bool sync_in_progress;          // Sync currently in progress
    uint8_t sync_conflicts;         // Number of sync conflicts
} profile_management_status_t;

// Function Prototypes

// Initialization and Configuration
void calibration_persistence_init(void);
void calibration_persistence_deinit(void);
void calibration_persistence_set_storage_location(storage_location_t location);
void calibration_persistence_configure_backup(backup_configuration_t *config);
void calibration_persistence_configure_cloud_sync(cloud_sync_configuration_t *config);

// Profile Management
bool calibration_persistence_create_profile(const char *profile_name, 
                                           calibration_profile_type_t type,
                                           complete_calibration_data_t *data);
bool calibration_persistence_load_profile(const char *profile_name, 
                                         complete_calibration_data_t *data);
bool calibration_persistence_save_profile(const char *profile_name, 
                                         complete_calibration_data_t *data);
bool calibration_persistence_delete_profile(const char *profile_name);
bool calibration_persistence_rename_profile(const char *old_name, const char *new_name);
bool calibration_persistence_copy_profile(const char *source_name, const char *dest_name);

// Profile Discovery and Listing
uint8_t calibration_persistence_list_profiles(char profile_names[][64], uint8_t max_profiles);
bool calibration_persistence_profile_exists(const char *profile_name);
bool calibration_persistence_get_profile_info(const char *profile_name, 
                                              complete_calibration_data_t *info);
calibration_profile_type_t calibration_persistence_get_profile_type(const char *profile_name);

// Active Profile Management
bool calibration_persistence_set_active_profile(const char *profile_name);
const char* calibration_persistence_get_active_profile_name(void);
bool calibration_persistence_load_active_profile(complete_calibration_data_t *data);
bool calibration_persistence_save_active_profile(complete_calibration_data_t *data);

// Default Profiles
bool calibration_persistence_create_factory_defaults(void);
bool calibration_persistence_reset_to_factory_defaults(void);
bool calibration_persistence_restore_factory_profile(calibration_profile_type_t type);

// Data Validation and Integrity
bool calibration_persistence_validate_data(complete_calibration_data_t *data);
uint32_t calibration_persistence_calculate_checksum(complete_calibration_data_t *data);
bool calibration_persistence_verify_checksum(complete_calibration_data_t *data);
bool calibration_persistence_repair_data(complete_calibration_data_t *data);

// Backup and Restore
bool calibration_persistence_create_backup(const char *backup_name);
bool calibration_persistence_restore_backup(const char *backup_name);
uint8_t calibration_persistence_list_backups(char backup_names[][64], uint8_t max_backups);
bool calibration_persistence_delete_backup(const char *backup_name);
bool calibration_persistence_auto_backup(void);

// Cloud Synchronization
bool calibration_persistence_sync_to_cloud(const char *profile_name);
bool calibration_persistence_sync_from_cloud(const char *profile_name);
bool calibration_persistence_sync_all_profiles(void);
uint8_t calibration_persistence_get_cloud_profiles(char profile_names[][64], uint8_t max_profiles);
bool calibration_persistence_resolve_sync_conflict(const char *profile_name, bool prefer_local);

// Import and Export
bool calibration_persistence_export_profile(const char *profile_name, const char *export_path);
bool calibration_persistence_import_profile(const char *import_path, const char *profile_name);
bool calibration_persistence_export_all_profiles(const char *export_directory);
bool calibration_persistence_import_profiles_from_directory(const char *import_directory);

// Migration and Compatibility
bool calibration_persistence_migrate_old_format(const char *old_data_path);
bool calibration_persistence_upgrade_data_format(complete_calibration_data_t *data);
bool calibration_persistence_is_compatible_version(calibration_version_t *version);
bool calibration_persistence_convert_format(complete_calibration_data_t *data, uint32_t target_version);

// Storage Management
uint32_t calibration_persistence_get_storage_usage(void);
uint32_t calibration_persistence_get_available_storage(void);
bool calibration_persistence_cleanup_storage(void);
bool calibration_persistence_defragment_storage(void);
bool calibration_persistence_optimize_storage(void);

// Search and Query
bool calibration_persistence_search_profiles(const char *search_term, 
                                            char results[][64], uint8_t max_results);
bool calibration_persistence_find_profiles_by_type(calibration_profile_type_t type,
                                                   char results[][64], uint8_t max_results);
bool calibration_persistence_find_profiles_by_quality(float min_quality,
                                                      char results[][64], uint8_t max_results);
bool calibration_persistence_find_profiles_by_date(uint32_t start_date, uint32_t end_date,
                                                   char results[][64], uint8_t max_results);

// Statistics and Analytics
void calibration_persistence_get_usage_statistics(const char *profile_name, 
                                                  complete_calibration_data_t *stats);
void calibration_persistence_update_usage_statistics(const char *profile_name, 
                                                     uint32_t flight_time_s, bool crashed);
float calibration_persistence_calculate_profile_score(const char *profile_name);
void calibration_persistence_generate_report(const char *profile_name, 
                                             char *report_buffer, size_t buffer_size);

// System Status and Health
void calibration_persistence_get_system_status(profile_management_status_t *status);
bool calibration_persistence_system_health_check(void);
bool calibration_persistence_verify_all_profiles(void);
void calibration_persistence_cleanup_orphaned_data(void);

// Advanced Features
bool calibration_persistence_create_profile_diff(const char *profile1, const char *profile2,
                                                 char *diff_buffer, size_t buffer_size);
bool calibration_persistence_merge_profiles(const char *base_profile, const char *merge_profile,
                                            const char *result_profile);
bool calibration_persistence_create_profile_template(calibration_profile_type_t type,
                                                     const char *template_name);
bool calibration_persistence_apply_template(const char *template_name, const char *profile_name);

// Security and Encryption
bool calibration_persistence_enable_encryption(const char *encryption_key);
bool calibration_persistence_disable_encryption(void);
bool calibration_persistence_change_encryption_key(const char *old_key, const char *new_key);
bool calibration_persistence_verify_encryption_integrity(void);

#ifdef __cplusplus
}
#endif

#endif // __CALIBRATION_PERSISTENCE_H__
