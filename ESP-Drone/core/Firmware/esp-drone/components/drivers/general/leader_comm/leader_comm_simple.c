/**
 * ESP-Drone Leader Communication Module - Simplified Implementation
 */

#include "leader_comm.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "LEADER_COMM"

// Static variables
static bool isInit = false;
static leader_status_t leader_status = {0};
static leader_follow_config_t follow_config = {0};

// Simple stub implementations
bool leaderCommInit(void)
{
    if (isInit) {
        return true;
    }

    ESP_LOGI(TAG, "Initializing Leader Communication (simplified)...");

    // Initialize status structure
    memset(&leader_status, 0, sizeof(leader_status));
    leader_status.distance_cm = -1;
    leader_status.last_seen_ms = 0;
    leader_status.detected = false;
    
    // Initialize follow config with defaults
    memset(&follow_config, 0, sizeof(follow_config));
    follow_config.enabled = false;
    follow_config.target_distance_cm = 200;  // 2 meters default
    follow_config.distance_tolerance_cm = 50; // 50cm tolerance
    follow_config.follow_speed_factor = 0.5f; // 50% speed
    follow_config.altitude_lock_active = false;
    follow_config.mode = LEADER_FOLLOW_DISABLED;

    isInit = true;
    ESP_LOGI(TAG, "Leader Communication initialized (simplified)");
    return true;
}

bool leaderCommTest(void)
{
    return isInit;
}

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
    follow_config.enabled = enabled;
    follow_config.target_distance_cm = target_distance_cm;
    follow_config.mode = enabled ? LEADER_FOLLOW_ENABLED : LEADER_FOLLOW_DISABLED;
    ESP_LOGI(TAG, "Follow mode: %s, target: %d cm", enabled ? "enabled" : "disabled", target_distance_cm);
}

bool leaderCommUpdateSetpoint(setpoint_t *setpoint, const state_t *state)
{
    // Simplified implementation - just return false (no changes to setpoint)
    return false;
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
    leader_status.packet_count = 0;
    leader_status.sequence_errors = 0;
    ESP_LOGI(TAG, "Stats reset");
}

void leaderCommSetFollowParams(int16_t target_distance_cm, int16_t tolerance_cm, float speed_factor)
{
    follow_config.target_distance_cm = target_distance_cm;
    follow_config.distance_tolerance_cm = tolerance_cm;
    follow_config.follow_speed_factor = speed_factor;
    ESP_LOGI(TAG, "Follow params: target=%d cm, tolerance=%d cm, speed=%.2f", 
             target_distance_cm, tolerance_cm, speed_factor);
}

void leaderCommLockAltitude(void)
{
    follow_config.altitude_lock_active = true;
    ESP_LOGI(TAG, "Altitude locked");
}

void leaderCommUnlockAltitude(void)
{
    follow_config.altitude_lock_active = false;
    ESP_LOGI(TAG, "Altitude unlocked");
}

// Additional function for web server integration
typedef struct {
    bool detected;
    int16_t distance;
    int8_t rssi;
    float signal_quality;
    uint32_t packet_count;
    uint32_t error_count;
} leader_data_t;

leader_data_t leaderCommGetData(void)
{
    leader_data_t data;
    data.detected = leader_status.detected;
    data.distance = leader_status.distance_cm;
    data.rssi = leader_status.rssi;
    data.signal_quality = leader_status.signal_quality;
    data.packet_count = leader_status.packet_count;
    data.error_count = leader_status.sequence_errors;
    return data;
}
