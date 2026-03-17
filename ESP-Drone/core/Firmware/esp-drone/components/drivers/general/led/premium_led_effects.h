/**
 * Premium LED Effects System for ESP-Drone
 * Beautiful visual feedback for calibration and flight status
 * Author: AI Enhanced Calibration System
 */

#ifndef __PREMIUM_LED_EFFECTS_H__
#define __PREMIUM_LED_EFFECTS_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// LED Effect Types
typedef enum {
    LED_EFFECT_OFF = 0,
    LED_EFFECT_BREATHING,          // Smooth breathing effect
    LED_EFFECT_RAINBOW_CYCLE,      // Beautiful rainbow transition
    LED_EFFECT_PULSE_WAVE,         // Pulsing wave pattern
    LED_EFFECT_SPARKLE,            // Sparkling stars effect
    LED_EFFECT_FIRE,               // Fire simulation
    LED_EFFECT_OCEAN_WAVE,         // Ocean wave pattern
    LED_EFFECT_AURORA,             // Aurora borealis effect
    LED_EFFECT_CALIBRATION_PROGRESS, // Calibration progress bar
    LED_EFFECT_SUCCESS_CELEBRATION,  // Success animation
    LED_EFFECT_ERROR_WARNING,       // Error indication
    LED_EFFECT_FLIGHT_STATUS,       // Flight mode indicators
    LED_EFFECT_CUSTOM              // Custom user pattern
} premium_led_effect_t;

// LED Color Profiles
typedef enum {
    LED_PROFILE_CALM_BLUE = 0,     // Soothing blue tones
    LED_PROFILE_WARM_ORANGE,       // Warm orange/yellow
    LED_PROFILE_FOREST_GREEN,      // Nature green
    LED_PROFILE_ROYAL_PURPLE,      // Elegant purple
    LED_PROFILE_SUNSET,            // Sunset gradient
    LED_PROFILE_OCEAN,             // Ocean blue-green
    LED_PROFILE_FIRE,              // Fire red-orange
    LED_PROFILE_ARCTIC,            // Cool white-blue
    LED_PROFILE_RAINBOW,           // Full spectrum
    LED_PROFILE_MONOCHROME         // Single color
} premium_led_profile_t;

// Calibration Status LED Patterns
typedef enum {
    CALIB_LED_INITIALIZING = 0,    // Gentle pulsing blue
    CALIB_LED_DETECTING_STABLE,    // Slow breathing cyan
    CALIB_LED_GYRO_CALIBRATING,    // Rotating green pattern
    CALIB_LED_ACCEL_CALIBRATING,   // Bouncing yellow pattern
    CALIB_LED_MAG_CALIBRATING,     // Spinning red pattern
    CALIB_LED_AI_LEARNING,         // Neural network pattern
    CALIB_LED_FINE_TUNING,         // Precise white flashes
    CALIB_LED_SUCCESS,             // Celebration rainbow burst
    CALIB_LED_ERROR,               // Pulsing red warning
    CALIB_LED_PERFECT              // Golden sparkle effect
} calibration_led_status_t;

// RGB Color Structure
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t brightness;
} rgb_color_t;

// LED Effect Configuration
typedef struct {
    premium_led_effect_t effect;
    premium_led_profile_t profile;
    uint16_t speed;                // Effect speed (1-1000)
    uint8_t intensity;             // Effect intensity (1-255)
    uint16_t duration_ms;          // Effect duration (0 = infinite)
    bool fade_in;                  // Smooth fade in
    bool fade_out;                 // Smooth fade out
    rgb_color_t custom_color;      // Custom color override
} premium_led_config_t;

// Calibration Progress Structure
typedef struct {
    float gyro_progress;           // 0.0 - 1.0
    float accel_progress;          // 0.0 - 1.0
    float mag_progress;            // 0.0 - 1.0
    float ai_confidence;           // 0.0 - 1.0
    float overall_progress;        // 0.0 - 1.0
    bool is_stable;                // Stability detection
    uint32_t calibration_time_ms;  // Time elapsed
} calibration_progress_t;

// Premium LED System Functions
void premium_led_init(void);
void premium_led_deinit(void);

// Effect Control
void premium_led_set_effect(premium_led_effect_t effect, premium_led_profile_t profile);
void premium_led_set_custom_effect(premium_led_config_t *config);
void premium_led_stop_effect(void);
void premium_led_pause_effect(void);
void premium_led_resume_effect(void);

// Calibration Visual Feedback
void premium_led_calibration_status(calibration_led_status_t status);
void premium_led_calibration_progress(calibration_progress_t *progress);
void premium_led_calibration_milestone(uint8_t milestone);
void premium_led_calibration_complete(float final_accuracy);

// Flight Status Indicators
void premium_led_flight_mode(uint8_t flight_mode);
void premium_led_battery_status(float voltage, bool charging);
void premium_led_connection_status(bool connected, int8_t rssi);
void premium_led_altitude_indicator(float altitude);

// Advanced Effects
void premium_led_breathing_effect(rgb_color_t color, uint16_t period_ms);
void premium_led_rainbow_cycle(uint16_t speed);
void premium_led_fire_simulation(uint8_t intensity);
void premium_led_aurora_effect(void);
void premium_led_sparkle_stars(rgb_color_t base_color, uint8_t density);

// Color Utilities
rgb_color_t premium_led_hsv_to_rgb(uint16_t hue, uint8_t saturation, uint8_t value);
rgb_color_t premium_led_temperature_to_rgb(uint16_t kelvin);
rgb_color_t premium_led_blend_colors(rgb_color_t color1, rgb_color_t color2, float ratio);

// System Integration
void premium_led_update(void);  // Call this regularly for smooth effects
void premium_led_emergency_pattern(void);
void premium_led_power_save_mode(bool enable);

// Configuration
void premium_led_set_brightness(uint8_t brightness);
void premium_led_set_speed_multiplier(float multiplier);
void premium_led_save_preferences(void);
void premium_led_load_preferences(void);

#ifdef __cplusplus
}
#endif

#endif // __PREMIUM_LED_EFFECTS_H__
