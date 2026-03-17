/**
 * Premium Telemetry System for ESP-Drone
 * Real-time data streaming, beautiful visualizations, and comprehensive monitoring
 * Features: WiFi/Bluetooth streaming, web dashboard, mobile app integration
 * Author: Premium Telemetry Enhancement System
 */

#ifndef __PREMIUM_TELEMETRY_SYSTEM_H__
#define __PREMIUM_TELEMETRY_SYSTEM_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Telemetry Transport Types
typedef enum {
    TELEMETRY_TRANSPORT_SERIAL = 0, // Serial/UART connection
    TELEMETRY_TRANSPORT_WIFI,       // WiFi connection
    TELEMETRY_TRANSPORT_BLUETOOTH,  // Bluetooth connection
    TELEMETRY_TRANSPORT_USB,        // USB connection
    TELEMETRY_TRANSPORT_RADIO,      // RF radio link
    TELEMETRY_TRANSPORT_WEBSOCKET,  // WebSocket connection
    TELEMETRY_TRANSPORT_UDP,        // UDP packets
    TELEMETRY_TRANSPORT_TCP,        // TCP connection
    TELEMETRY_TRANSPORT_MQTT        // MQTT broker
} telemetry_transport_t;

// Data Streaming Modes
typedef enum {
    STREAM_MODE_CALIBRATION = 0,    // Calibration progress streaming
    STREAM_MODE_FLIGHT_DATA,        // Real-time flight data
    STREAM_MODE_SENSOR_RAW,         // Raw sensor data
    STREAM_MODE_SENSOR_PROCESSED,   // Processed sensor data
    STREAM_MODE_PID_TUNING,         // PID tuning data
    STREAM_MODE_MOTOR_STATUS,       // Motor status and performance
    STREAM_MODE_BATTERY_STATUS,     // Battery monitoring
    STREAM_MODE_GPS_STATUS,         // GPS and navigation data
    STREAM_MODE_SYSTEM_HEALTH,      // System health monitoring
    STREAM_MODE_DEBUG_LOGS,         // Debug and diagnostic logs
    STREAM_MODE_PERFORMANCE_METRICS,// Performance analysis
    STREAM_MODE_AI_LEARNING,        // AI learning progress
    STREAM_MODE_CUSTOM             // Custom user-defined stream
} telemetry_stream_mode_t;

// Real-time Calibration Data
typedef struct {
    // Calibration Progress
    uint8_t overall_progress_percent;
    uint8_t gyro_progress_percent;
    uint8_t accel_progress_percent;
    uint8_t mag_progress_percent;
    uint8_t ai_progress_percent;
    
    // Current Stage Information
    uint8_t current_stage;          // Current calibration stage
    char stage_description[64];     // Human-readable stage description
    uint32_t stage_elapsed_ms;      // Time spent in current stage
    uint32_t stage_estimated_remaining_ms; // Estimated time remaining
    
    // Quality Metrics
    float gyro_confidence[3];       // Per-axis gyro confidence
    float accel_confidence[3];      // Per-axis accel confidence
    float mag_confidence[3];        // Per-axis mag confidence
    float overall_confidence;       // Overall calibration confidence
    float accuracy_estimate;        // Estimated calibration accuracy
    
    // Environmental Data
    float temperature_c;            // Current temperature
    float vibration_level;          // Vibration level detection
    float magnetic_interference;    // Magnetic interference level
    bool stable_conditions;         // Stability detection
    
    // AI Learning Status
    uint32_t ai_samples_processed;  // AI training samples processed
    float ai_learning_rate;         // Current learning rate
    float ai_convergence_rate;      // Rate of convergence
    bool ai_converged;              // AI has converged
    
    // Statistics
    float gyro_bias[3];             // Current gyro bias values
    float accel_offset[3];          // Current accel offset values
    float mag_offset[3];            // Current mag offset values
    float gyro_scale[3];            // Gyro scale factors
    float accel_scale[3];           // Accel scale factors
    
    // Timing
    uint32_t total_calibration_time_ms; // Total calibration time
    uint32_t samples_collected;     // Total samples collected
    float sampling_rate_hz;         // Current sampling rate
} realtime_calibration_data_t;

// Real-time Flight Data
typedef struct {
    // Attitude and Position
    float roll_deg;                 // Roll angle in degrees
    float pitch_deg;                // Pitch angle in degrees
    float yaw_deg;                  // Yaw angle in degrees
    float altitude_m;               // Altitude in meters
    float velocity[3];              // Velocity vector [x, y, z] m/s
    float position[3];              // Position vector [x, y, z] m
    
    // Control Inputs
    float throttle_percent;         // Throttle percentage
    float roll_input;               // Roll stick input
    float pitch_input;              // Pitch stick input
    float yaw_input;                // Yaw stick input
    
    // Motor Outputs
    uint16_t motor_pwm[4];          // Motor PWM values
    float motor_thrust[4];          // Motor thrust estimates
    float motor_rpm[4];             // Motor RPM estimates
    float motor_temperature[4];     // Motor temperatures
    
    // Flight Mode and Status
    uint8_t flight_mode;            // Current flight mode
    bool motors_armed;              // Motors armed status
    bool gps_lock;                  // GPS lock status
    uint8_t satellite_count;        // Number of GPS satellites
    float battery_voltage;          // Battery voltage
    float battery_current;          // Battery current draw
    float battery_remaining_percent;// Battery remaining percentage
    
    // Performance Metrics
    float loop_time_us;             // Main loop execution time
    float cpu_usage_percent;        // CPU usage percentage
    float memory_usage_kb;          // Memory usage in KB
    uint16_t radio_rssi;            // Radio signal strength
} realtime_flight_data_t;

// System Health Monitoring
typedef struct {
    // System Status
    uint32_t uptime_ms;             // System uptime
    float cpu_temperature_c;        // CPU temperature
    float board_temperature_c;      // Board temperature
    uint8_t system_load_percent;    // Overall system load
    
    // Sensor Health
    bool gyro_healthy;              // Gyroscope health status
    bool accel_healthy;             // Accelerometer health status
    bool mag_healthy;               // Magnetometer health status
    bool baro_healthy;              // Barometer health status
    bool gps_healthy;               // GPS health status
    
    // Communication Health
    bool radio_connected;           // Radio connection status
    bool wifi_connected;            // WiFi connection status
    bool bluetooth_connected;       // Bluetooth connection status
    uint16_t packet_loss_percent;   // Communication packet loss
    
    // Storage and Memory
    uint32_t flash_free_kb;         // Free flash memory
    uint32_t ram_free_kb;           // Free RAM
    bool sd_card_present;           // SD card present
    uint32_t sd_card_free_mb;       // Free SD card space
    
    // Error Counters
    uint16_t sensor_errors;         // Sensor error count
    uint16_t communication_errors;  // Communication error count
    uint16_t system_errors;         // System error count
    uint16_t watchdog_resets;       // Watchdog reset count
} system_health_data_t;

// Telemetry Configuration
typedef struct {
    telemetry_transport_t transport;
    telemetry_stream_mode_t stream_mode;
    
    // Connection Settings
    char wifi_ssid[32];             // WiFi network name
    char wifi_password[64];         // WiFi password
    char bluetooth_name[32];        // Bluetooth device name
    uint16_t tcp_port;              // TCP port number
    uint16_t udp_port;              // UDP port number
    char mqtt_broker[64];           // MQTT broker address
    char mqtt_topic[32];            // MQTT topic
    
    // Streaming Configuration
    uint16_t update_rate_hz;        // Data update rate
    bool enable_compression;        // Enable data compression
    bool enable_encryption;         // Enable data encryption
    uint8_t data_precision;         // Float precision (decimals)
    bool include_timestamps;        // Include timestamps in data
    
    // Data Selection
    bool stream_calibration_data;   // Stream calibration data
    bool stream_flight_data;        // Stream flight data
    bool stream_sensor_data;        // Stream raw sensor data
    bool stream_system_health;      // Stream system health
    bool stream_debug_logs;         // Stream debug information
    
    // Web Dashboard Settings
    bool enable_web_dashboard;      // Enable web dashboard
    uint16_t web_port;              // Web server port
    char web_username[32];          // Web authentication username
    char web_password[64];          // Web authentication password
    bool enable_real_time_graphs;   // Real-time graphing
    
    // Mobile App Integration
    bool enable_mobile_app;         // Enable mobile app support
    char app_api_key[64];           // Mobile app API key
    bool push_notifications;        // Enable push notifications
    bool location_sharing;          // Share drone location
} telemetry_config_t;

// Dashboard Widget Types
typedef enum {
    WIDGET_ATTITUDE_INDICATOR = 0,  // 3D attitude indicator
    WIDGET_ALTITUDE_GRAPH,          // Altitude vs time graph
    WIDGET_VELOCITY_GRAPH,          // Velocity vs time graph
    WIDGET_BATTERY_GAUGE,           // Battery level gauge
    WIDGET_GPS_MAP,                 // GPS position map
    WIDGET_MOTOR_STATUS,            // Motor status indicators
    WIDGET_SENSOR_GRAPHS,           // Raw sensor data graphs
    WIDGET_CALIBRATION_PROGRESS,    // Calibration progress bars
    WIDGET_SYSTEM_HEALTH,           // System health indicators
    WIDGET_PID_TUNING,              // PID tuning interface
    WIDGET_FLIGHT_LOG,              // Flight log viewer
    WIDGET_3D_VISUALIZATION,        // 3D drone visualization
    WIDGET_CUSTOM                   // Custom user widget
} dashboard_widget_t;

// Web Dashboard Configuration
typedef struct {
    dashboard_widget_t widgets[16]; // Up to 16 dashboard widgets
    uint8_t widget_count;           // Number of active widgets
    char dashboard_title[64];       // Dashboard title
    bool dark_theme;                // Use dark theme
    bool auto_refresh;              // Auto-refresh data
    uint16_t refresh_rate_ms;       // Refresh rate in milliseconds
    bool enable_alerts;             // Enable alert notifications
    float alert_battery_low;        // Low battery alert threshold
    float alert_temperature_high;   // High temperature alert
} web_dashboard_config_t;

// Mobile App Notification
typedef struct {
    char title[64];                 // Notification title
    char message[256];              // Notification message
    uint8_t priority;               // Priority (0=low, 5=high)
    bool include_location;          // Include drone location
    bool include_battery_status;    // Include battery status
    bool include_flight_time;       // Include flight time
    uint32_t timestamp;             // Notification timestamp
} mobile_notification_t;

// Data Logging Configuration
typedef struct {
    bool enable_logging;            // Enable data logging
    char log_filename_prefix[32];   // Log filename prefix
    bool log_to_sd_card;            // Log to SD card
    bool log_to_flash;              // Log to internal flash
    bool log_to_cloud;              // Log to cloud storage
    
    // Log Content Selection
    bool log_calibration_data;      // Log calibration sessions
    bool log_flight_data;           // Log flight sessions
    bool log_system_health;         // Log system health data
    bool log_debug_info;            // Log debug information
    
    // Log Management
    uint32_t max_log_size_mb;       // Maximum log file size
    uint8_t max_log_files;          // Maximum number of log files
    bool auto_delete_old_logs;      // Auto-delete old logs
    uint16_t log_retention_days;    // Log retention period
} data_logging_config_t;

// Function Prototypes

// Main Telemetry System
void premium_telemetry_init(telemetry_config_t *config);
void premium_telemetry_deinit(void);
void premium_telemetry_update(void);
void premium_telemetry_start_streaming(void);
void premium_telemetry_stop_streaming(void);

// Configuration Management
void premium_telemetry_set_config(telemetry_config_t *config);
void premium_telemetry_get_config(telemetry_config_t *config);
void premium_telemetry_set_transport(telemetry_transport_t transport);
void premium_telemetry_set_stream_mode(telemetry_stream_mode_t mode);
void premium_telemetry_set_update_rate(uint16_t rate_hz);

// Data Streaming Functions
void premium_telemetry_stream_calibration_data(realtime_calibration_data_t *data);
void premium_telemetry_stream_flight_data(realtime_flight_data_t *data);
void premium_telemetry_stream_system_health(system_health_data_t *data);
void premium_telemetry_stream_custom_data(const char *data_name, void *data, size_t data_size);

// Real-time Data Collection
void premium_telemetry_update_calibration_data(realtime_calibration_data_t *data);
void premium_telemetry_update_flight_data(realtime_flight_data_t *data);
void premium_telemetry_update_system_health(system_health_data_t *data);

// Web Dashboard
void premium_telemetry_start_web_dashboard(web_dashboard_config_t *config);
void premium_telemetry_stop_web_dashboard(void);
void premium_telemetry_add_dashboard_widget(dashboard_widget_t widget);
void premium_telemetry_remove_dashboard_widget(dashboard_widget_t widget);
void premium_telemetry_update_dashboard_data(void);

// Mobile App Integration
void premium_telemetry_enable_mobile_app(const char *api_key);
void premium_telemetry_send_notification(mobile_notification_t *notification);
void premium_telemetry_update_app_location(double lat, double lon, float alt);
void premium_telemetry_app_heartbeat(void);

// Data Logging
void premium_telemetry_start_logging(data_logging_config_t *config);
void premium_telemetry_stop_logging(void);
void premium_telemetry_log_calibration_session(realtime_calibration_data_t *session_data);
void premium_telemetry_log_flight_session(realtime_flight_data_t *session_data);
bool premium_telemetry_export_logs(const char *export_path);

// Network and Communication
bool premium_telemetry_wifi_connect(const char *ssid, const char *password);
void premium_telemetry_wifi_disconnect(void);
bool premium_telemetry_bluetooth_enable(const char *device_name);
void premium_telemetry_bluetooth_disable(void);
bool premium_telemetry_is_connected(void);
uint16_t premium_telemetry_get_connection_quality(void);

// Data Visualization Helpers
void premium_telemetry_create_realtime_graph(const char *graph_name, float *data, size_t data_count);
void premium_telemetry_update_realtime_graph(const char *graph_name, float new_data_point);
void premium_telemetry_create_3d_visualization(float attitude[3], float position[3]);
void premium_telemetry_update_attitude_indicator(float roll, float pitch, float yaw);

// Alert and Notification System
void premium_telemetry_set_alert_threshold(const char *alert_name, float threshold);
void premium_telemetry_enable_alert(const char *alert_name, bool enable);
void premium_telemetry_check_alerts(void);
void premium_telemetry_send_emergency_alert(const char *message);

// Performance Monitoring
float premium_telemetry_get_cpu_usage(void);
uint16_t premium_telemetry_get_memory_usage(void);
uint16_t premium_telemetry_get_network_throughput(void);
float premium_telemetry_get_data_rate_kbps(void);

// Calibration-Specific Telemetry
void premium_telemetry_calibration_started(const char *calibration_type);
void premium_telemetry_calibration_progress(uint8_t progress_percent, const char *status);
void premium_telemetry_calibration_milestone(const char *milestone, float confidence);
void premium_telemetry_calibration_completed(float final_accuracy, uint32_t total_time_ms);
void premium_telemetry_calibration_error(const char *error_message);

// Advanced Analytics
void premium_telemetry_enable_analytics(bool enable);
void premium_telemetry_generate_flight_report(char *report_buffer, size_t buffer_size);
void premium_telemetry_generate_calibration_report(char *report_buffer, size_t buffer_size);
void premium_telemetry_calculate_performance_metrics(void);
void premium_telemetry_export_analytics_data(const char *export_format);

// Cloud Integration
void premium_telemetry_enable_cloud_sync(const char *cloud_api_key);
void premium_telemetry_upload_flight_data(const char *flight_id);
void premium_telemetry_download_calibration_profiles(void);
void premium_telemetry_sync_with_cloud(void);

// Security and Privacy
void premium_telemetry_enable_encryption(const char *encryption_key);
void premium_telemetry_set_privacy_mode(bool enable_privacy);
void premium_telemetry_anonymize_data(bool enable_anonymization);
void premium_telemetry_clear_stored_data(void);

// Diagnostic and Testing
bool premium_telemetry_self_test(void);
void premium_telemetry_test_connection(telemetry_transport_t transport);
void premium_telemetry_benchmark_performance(void);
void premium_telemetry_generate_diagnostic_report(char *report_buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // __PREMIUM_TELEMETRY_SYSTEM_H__
