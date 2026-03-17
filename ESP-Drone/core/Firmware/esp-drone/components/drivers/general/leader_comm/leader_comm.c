/**
 * ESP-Drone Leader Communication Module Implementation
 */

#include "leader_comm.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "log.h"
#include "param.h"
#include "crtp_commander.h"

#define TAG "LEADER_COMM"
#define DEBUG_MODULE "LEADER"
#include "debug_cf.h"

// RSSI to distance conversion constants (calibrated values)
#define RSSI_REFERENCE_DBM -40     // RSSI at 1 meter reference
#define RSSI_PATH_LOSS_EXP 2.0f    // Path loss exponent for free space
#define RSSI_OFFSET_DBM 0          // Calibration offset

// Static variables
static bool isInit = false;
static leader_status_t leader_status;
static leader_follow_config_t follow_config;
static SemaphoreHandle_t status_mutex;
static uint32_t last_sequence = 0;

// Task handles
static TaskHandle_t leader_monitor_task_handle = NULL;

// Function prototypes
static void esp_now_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len);
static void leader_monitor_task(void *pvParameters);
static int16_t rssi_to_distance_cm(int8_t rssi);
static void update_distance_buffer(int16_t new_distance);
static int16_t get_smoothed_distance(void);
static void update_signal_quality(void);
static bool setup_esp_now(void);

bool leaderCommInit(void)
{
    if (isInit) {
        return true;
    }

    DEBUG_PRINTI("Initializing Leader Communication...");

    // Initialize status structure
    memset(&leader_status, 0, sizeof(leader_status));
    leader_status.distance_cm = -1;
    leader_status.last_seen_ms = 0;
    
    // Initialize follow config with defaults
    memset(&follow_config, 0, sizeof(follow_config));
    follow_config.enabled = false;
    follow_config.target_distance_cm = 200;  // 2 meters default
    follow_config.distance_tolerance_cm = 50; // 50cm tolerance
    follow_config.follow_speed_factor = 0.5f; // 50% speed
    follow_config.altitude_lock_active = false;
    follow_config.mode = LEADER_FOLLOW_DISABLED;

    // Create mutex for thread-safe access
    status_mutex = xSemaphoreCreateMutex();
    if (status_mutex == NULL) {
        DEBUG_PRINTE("Failed to create status mutex");
        return false;
    }

    // Setup ESP-NOW
    if (!setup_esp_now()) {
        DEBUG_PRINTE("Failed to setup ESP-NOW");
        return false;
    }

    // Create leader monitor task
    if (xTaskCreate(leader_monitor_task, "leader_monitor", 2048, NULL, 5, &leader_monitor_task_handle) != pdPASS) {
        DEBUG_PRINTE("Failed to create leader monitor task");
        return false;
    }

    isInit = true;
    DEBUG_PRINTI("Leader Communication initialized successfully");
    return true;
}

bool leaderCommTest(void)
{
    return isInit && (status_mutex != NULL);
}

static bool setup_esp_now(void)
{
    // Initialize WiFi in STA mode
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        DEBUG_PRINTE("WiFi init failed: %s", esp_err_to_name(ret));
        return false;
    }

    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        DEBUG_PRINTE("WiFi set mode failed: %s", esp_err_to_name(ret));
        return false;
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        DEBUG_PRINTE("WiFi start failed: %s", esp_err_to_name(ret));
        return false;
    }

    ret = esp_wifi_set_channel(LEADER_COMM_CHANNEL, WIFI_SECOND_CHAN_NONE);
    if (ret != ESP_OK) {
        DEBUG_PRINTE("WiFi set channel failed: %s", esp_err_to_name(ret));
        return false;
    }

    // Initialize ESP-NOW
    ret = esp_now_init();
    if (ret != ESP_OK) {
        DEBUG_PRINTE("ESP-NOW init failed: %s", esp_err_to_name(ret));
        return false;
    }

    // Register receive callback
    ret = esp_now_register_recv_cb(esp_now_recv_cb);
    if (ret != ESP_OK) {
        DEBUG_PRINTE("ESP-NOW register recv cb failed: %s", esp_err_to_name(ret));
        return false;
    }

    DEBUG_PRINTI("ESP-NOW setup completed");
    return true;
}

static void esp_now_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    if (len < sizeof(leader_packet_t)) {
        return; // Invalid packet size
    }

    leader_packet_t *packet = (leader_packet_t *)data;
    
    // Verify packet type
    if (packet->type != 0x42) {
        return; // Not a leader beacon packet
    }

    // Take mutex to update status
    if (xSemaphoreTake(status_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        uint32_t current_time = esp_timer_get_time() / 1000; // Convert to milliseconds
        
        // Update basic status
        leader_status.detected = true;
        leader_status.last_seen_ms = current_time;
        leader_status.rssi = recv_info->rx_ctrl->rssi;
        leader_status.packet_count++;
        
        // Check sequence number for packet loss
        if (last_sequence != 0 && packet->sequence != (last_sequence + 1)) {
            leader_status.sequence_errors++;
        }
        last_sequence = packet->sequence;
        
        // Calculate and smooth distance
        int16_t raw_distance = rssi_to_distance_cm(leader_status.rssi);
        update_distance_buffer(raw_distance);
        leader_status.distance_cm = get_smoothed_distance();
        
        // Update signal quality
        update_signal_quality();
        
        xSemaphoreGive(status_mutex);
    }
}

static void leader_monitor_task(void *pvParameters)
{
    TickType_t last_wake_time = xTaskGetTickCount();
    
    while (1) {
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(100)); // Run at 10Hz
        
        if (xSemaphoreTake(status_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            uint32_t current_time = esp_timer_get_time() / 1000;
            
            // Check for signal timeout
            if (leader_status.detected && 
                (current_time - leader_status.last_seen_ms) > LEADER_SIGNAL_TIMEOUT_MS) {
                leader_status.detected = false;
                leader_status.distance_cm = -1;
                leader_status.signal_quality = 0.0f;
                
                // Update follow mode if signal lost
                if (follow_config.enabled) {
                    follow_config.mode = LEADER_FOLLOW_LOST_SIGNAL;
                    DEBUG_PRINTW("Leader signal lost");
                }
            }
            
            xSemaphoreGive(status_mutex);
        }
    }
}

static int16_t rssi_to_distance_cm(int8_t rssi)
{
    if (rssi == 0) return -1;
    
    // Convert RSSI to distance using path loss model
    // Distance = 10^((RSSI_ref - RSSI) / (10 * n))
    // Where n is path loss exponent
    
    float distance_m = powf(10.0f, (RSSI_REFERENCE_DBM - rssi - RSSI_OFFSET_DBM) / (10.0f * RSSI_PATH_LOSS_EXP));
    int16_t distance_cm = (int16_t)(distance_m * 100.0f);
    
    // Clamp to reasonable limits
    if (distance_cm < MIN_LEADER_DISTANCE_CM) distance_cm = MIN_LEADER_DISTANCE_CM;
    if (distance_cm > MAX_LEADER_DISTANCE_CM) distance_cm = MAX_LEADER_DISTANCE_CM;
    
    return distance_cm;
}

static void update_distance_buffer(int16_t new_distance)
{
    leader_status.distance_buffer[leader_status.buffer_index] = new_distance;
    leader_status.buffer_index = (leader_status.buffer_index + 1) % DISTANCE_SMOOTHING_SAMPLES;
    
    if (!leader_status.buffer_full && leader_status.buffer_index == 0) {
        leader_status.buffer_full = true;
    }
}

static int16_t get_smoothed_distance(void)
{
    if (!leader_status.buffer_full && leader_status.buffer_index == 0) {
        return leader_status.distance_buffer[0]; // Only one sample
    }
    
    int32_t sum = 0;
    int count = leader_status.buffer_full ? DISTANCE_SMOOTHING_SAMPLES : leader_status.buffer_index;
    
    for (int i = 0; i < count; i++) {
        sum += leader_status.distance_buffer[i];
    }
    
    return (int16_t)(sum / count);
}

static void update_signal_quality(void)
{
    // Calculate signal quality based on RSSI and packet loss
    float rssi_quality = 0.0f;
    if (leader_status.rssi >= -50) {
        rssi_quality = 100.0f;
    } else if (leader_status.rssi <= -90) {
        rssi_quality = 0.0f;
    } else {
        rssi_quality = 100.0f * (90 + leader_status.rssi) / 40.0f;
    }
    
    // Factor in packet loss
    float packet_loss_rate = 0.0f;
    if (leader_status.packet_count > 0) {
        packet_loss_rate = (float)leader_status.sequence_errors / leader_status.packet_count;
    }
    float packet_quality = 100.0f * (1.0f - packet_loss_rate);
    
    // Combine both factors
    leader_status.signal_quality = (rssi_quality + packet_quality) / 2.0f;
}

// Public API implementations
const leader_status_t* leaderCommGetStatus(void)
{
    return &leader_status;
}

const leader_follow_config_t* leaderCommGetFollowConfig(void)
{
    return &follow_config;
}

void leaderCommSetFollowMode(bool enabled, int16_t target_distance_cm)
{
    if (xSemaphoreTake(status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        follow_config.enabled = enabled;
        follow_config.target_distance_cm = target_distance_cm;
        
        if (enabled) {
            follow_config.mode = LEADER_FOLLOW_ENABLED;
            DEBUG_PRINTI("Leader follow mode enabled, target distance: %d cm", target_distance_cm);
        } else {
            follow_config.mode = LEADER_FOLLOW_DISABLED;
            follow_config.altitude_lock_active = false;
            DEBUG_PRINTI("Leader follow mode disabled");
        }
        
        xSemaphoreGive(status_mutex);
    }
}

bool leaderCommUpdateSetpoint(setpoint_t *setpoint, const state_t *state)
{
    if (!follow_config.enabled || !leader_status.detected) {
        return false;
    }
    
    bool modified = false;
    
    if (xSemaphoreTake(status_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        int16_t distance_error = leader_status.distance_cm - follow_config.target_distance_cm;
        
        // Lock altitude when follow mode starts
        if (!follow_config.altitude_lock_active) {
            follow_config.altitude_lock = state->position.z;
            follow_config.altitude_lock_active = true;
            DEBUG_PRINTI("Altitude locked at %.2f m", follow_config.altitude_lock);
        }
        
        // Update follow mode based on distance
        if (abs(distance_error) <= follow_config.distance_tolerance_cm) {
            follow_config.mode = LEADER_FOLLOW_ENABLED;
        } else if (leader_status.distance_cm < follow_config.target_distance_cm) {
            follow_config.mode = LEADER_FOLLOW_TOO_CLOSE;
        } else {
            follow_config.mode = LEADER_FOLLOW_TOO_FAR;
        }
        
        // Apply following logic based on mode
        switch (follow_config.mode) {
            case LEADER_FOLLOW_ENABLED:
                // Fine adjustment mode - small corrections
                if (abs(distance_error) > 10) { // 10cm deadband
                    setpoint->mode.x = modeVelocity;
                    setpoint->velocity.x = (distance_error > 0 ? 1.0f : -1.0f) * 
                                          follow_config.follow_speed_factor * 0.3f; // Slow approach
                    modified = true;
                }
                break;
                
            case LEADER_FOLLOW_TOO_CLOSE:
                // Move away from leader
                setpoint->mode.x = modeVelocity;
                setpoint->velocity.x = -follow_config.follow_speed_factor * 0.8f; // Back away faster
                modified = true;
                break;
                
            case LEADER_FOLLOW_TOO_FAR:
                // Move towards leader
                setpoint->mode.x = modeVelocity;
                setpoint->velocity.x = follow_config.follow_speed_factor * 1.0f; // Approach at full speed
                modified = true;
                break;
                
            default:
                break;
        }
        
        // Maintain locked altitude
        if (follow_config.altitude_lock_active) {
            setpoint->mode.z = modeAbs;
            setpoint->position.z = follow_config.altitude_lock;
            modified = true;
        }
        
        // Keep other axes stable
        setpoint->mode.y = modeVelocity;
        setpoint->velocity.y = 0.0f;
        setpoint->mode.yaw = modeVelocity;
        setpoint->attitudeRate.yaw = 0.0f;
        
        xSemaphoreGive(status_mutex);
    }
    
    return modified;
}

int16_t leaderCommGetDistance(void)
{
    return leader_status.distance_cm;
}

int8_t leaderCommGetSignalStrength(void)
{
    return leader_status.rssi;
}

bool leaderCommIsDetected(void)
{
    return leader_status.detected;
}

float leaderCommGetSignalQuality(void)
{
    return leader_status.signal_quality;
}

void leaderCommResetStats(void)
{
    if (xSemaphoreTake(status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        leader_status.packet_count = 0;
        leader_status.sequence_errors = 0;
        leader_status.signal_quality = 0.0f;
        last_sequence = 0;
        
        xSemaphoreGive(status_mutex);
    }
}

void leaderCommSetFollowParams(int16_t target_distance_cm, 
                              int16_t tolerance_cm, 
                              float speed_factor)
{
    if (xSemaphoreTake(status_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        follow_config.target_distance_cm = target_distance_cm;
        follow_config.distance_tolerance_cm = tolerance_cm;
        follow_config.follow_speed_factor = speed_factor;
        
        DEBUG_PRINTI("Follow params updated: target=%dcm, tolerance=%dcm, speed=%.2f", 
                    target_distance_cm, tolerance_cm, speed_factor);
        
        xSemaphoreGive(status_mutex);
    }
}

void leaderCommLockAltitude(void)
{
    follow_config.altitude_lock_active = true;
}

void leaderCommUnlockAltitude(void)
{
    follow_config.altitude_lock_active = false;
}

// Parameter and log groups for cfclient integration
PARAM_GROUP_START(leader)
PARAM_ADD(PARAM_UINT8, followEnabled, &follow_config.enabled)
PARAM_ADD(PARAM_UINT16, targetDistance, &follow_config.target_distance_cm)
PARAM_ADD(PARAM_UINT16, tolerance, &follow_config.distance_tolerance_cm)
PARAM_ADD(PARAM_FLOAT, speedFactor, &follow_config.follow_speed_factor)
PARAM_ADD(PARAM_UINT8, altLockActive, &follow_config.altitude_lock_active)
PARAM_GROUP_STOP(leader)

LOG_GROUP_START(leader)
LOG_ADD(LOG_UINT8, detected, &leader_status.detected)
LOG_ADD(LOG_INT16, distance, &leader_status.distance_cm)
LOG_ADD(LOG_INT8, rssi, &leader_status.rssi)
LOG_ADD(LOG_UINT32, packets, &leader_status.packet_count)
LOG_ADD(LOG_UINT32, errors, &leader_status.sequence_errors)
LOG_ADD(LOG_FLOAT, quality, &leader_status.signal_quality)
LOG_ADD(LOG_UINT8, followMode, &follow_config.mode)
LOG_ADD(LOG_FLOAT, altLock, &follow_config.altitude_lock)
LOG_GROUP_STOP(leader)
