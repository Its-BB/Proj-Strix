/**
 * Premium Audio Feedback System for ESP-Drone
 * Beautiful audio cues for calibration, flight status, and user interaction
 * Features: Musical tones, voice synthesis, spatial audio effects
 * Author: Premium Audio Enhancement System
 */

#ifndef __PREMIUM_AUDIO_FEEDBACK_H__
#define __PREMIUM_AUDIO_FEEDBACK_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Audio Output Types
typedef enum {
    AUDIO_OUTPUT_BUZZER = 0,       // Simple piezo buzzer
    AUDIO_OUTPUT_SPEAKER,          // Small speaker
    AUDIO_OUTPUT_I2S_DAC,          // High-quality I2S DAC
    AUDIO_OUTPUT_PWM,              // PWM-based audio
    AUDIO_OUTPUT_BLUETOOTH         // Bluetooth audio streaming
} audio_output_type_t;

// Audio Effect Types
typedef enum {
    AUDIO_EFFECT_NONE = 0,
    AUDIO_EFFECT_BEEP,             // Simple beep
    AUDIO_EFFECT_CHIME,            // Pleasant chime
    AUDIO_EFFECT_MELODY,           // Musical melody
    AUDIO_EFFECT_SEQUENCE,         // Tone sequence
    AUDIO_EFFECT_SWEEP,            // Frequency sweep
    AUDIO_EFFECT_HARMONY,          // Multiple harmonics
    AUDIO_EFFECT_NATURE_SOUND,     // Nature-inspired sounds
    AUDIO_EFFECT_SCI_FI,           // Futuristic sounds
    AUDIO_EFFECT_VOICE_SYNTH       // Synthetic voice
} audio_effect_type_t;

// Calibration Audio States
typedef enum {
    CALIB_AUDIO_POWER_ON = 0,      // Power on jingle
    CALIB_AUDIO_INITIALIZING,      // Gentle startup melody
    CALIB_AUDIO_DETECTING_STABLE,  // Slow rhythmic tones
    CALIB_AUDIO_GYRO_START,        // Gyro calibration start chime
    CALIB_AUDIO_GYRO_PROGRESS,     // Progress indication tones
    CALIB_AUDIO_GYRO_COMPLETE,     // Gyro completion melody
    CALIB_AUDIO_ACCEL_START,       // Accelerometer start
    CALIB_AUDIO_ACCEL_PROGRESS,    // Accel progress tones
    CALIB_AUDIO_ACCEL_COMPLETE,    // Accel completion
    CALIB_AUDIO_MAG_START,         // Magnetometer start
    CALIB_AUDIO_MAG_PROGRESS,      // Mag progress tones
    CALIB_AUDIO_MAG_COMPLETE,      // Mag completion
    CALIB_AUDIO_AI_LEARNING,       // AI learning sounds
    CALIB_AUDIO_AI_CONVERGED,      // AI convergence chime
    CALIB_AUDIO_STAGE_COMPLETE,    // Stage completion fanfare
    CALIB_AUDIO_ALL_COMPLETE,      // Full calibration success
    CALIB_AUDIO_ERROR,             // Error warning tone
    CALIB_AUDIO_TIMEOUT,           // Timeout warning
    CALIB_AUDIO_PERFECT_CALIB,     // Perfect calibration celebration
    CALIB_AUDIO_FLIGHT_READY       // Ready for flight anthem
} calibration_audio_state_t;

// Flight Audio States
typedef enum {
    FLIGHT_AUDIO_ARM = 0,          // Motors armed confirmation
    FLIGHT_AUDIO_DISARM,           // Motors disarmed confirmation
    FLIGHT_AUDIO_TAKEOFF,          // Takeoff sequence
    FLIGHT_AUDIO_LANDING,          // Landing sequence
    FLIGHT_AUDIO_LOW_BATTERY,      // Low battery warning
    FLIGHT_AUDIO_CRITICAL_BATTERY, // Critical battery alarm
    FLIGHT_AUDIO_GPS_LOCK,         // GPS lock acquired
    FLIGHT_AUDIO_GPS_LOST,         // GPS signal lost
    FLIGHT_AUDIO_ALTITUDE_WARNING, // Altitude limit warning
    FLIGHT_AUDIO_RANGE_WARNING,    // Range limit warning
    FLIGHT_AUDIO_EMERGENCY,        // Emergency situation
    FLIGHT_AUDIO_RETURN_HOME,      // Return to home activated
    FLIGHT_AUDIO_MISSION_START,    // Mission start confirmation
    FLIGHT_AUDIO_MISSION_COMPLETE, // Mission completion
    FLIGHT_AUDIO_PHOTO_CAPTURE,    // Photo captured
    FLIGHT_AUDIO_VIDEO_START,      // Video recording start
    FLIGHT_AUDIO_VIDEO_STOP        // Video recording stop
} flight_audio_state_t;

// Musical Note Structure
typedef struct {
    uint16_t frequency;            // Note frequency in Hz
    uint16_t duration_ms;          // Note duration
    uint8_t volume;                // Volume level (0-255)
    uint8_t waveform;              // Waveform type (sine, square, etc.)
} musical_note_t;

// Audio Sequence Structure
typedef struct {
    musical_note_t notes[32];      // Up to 32 notes per sequence
    uint8_t note_count;            // Number of notes in sequence
    uint16_t repeat_count;         // How many times to repeat (0 = infinite)
    uint16_t pause_between_ms;     // Pause between repetitions
    bool fade_in;                  // Fade in effect
    bool fade_out;                 // Fade out effect
} audio_sequence_t;

// Voice Synthesis Structure
typedef struct {
    char text[128];                // Text to synthesize
    uint8_t voice_type;            // Voice character (0-7)
    uint8_t speed;                 // Speech speed (1-10)
    uint8_t pitch;                 // Voice pitch (1-10)
    uint8_t volume;                // Voice volume (1-10)
    bool robotic_filter;           // Apply robotic effect
} voice_synthesis_t;

// Audio Configuration
typedef struct {
    audio_output_type_t output_type;
    uint8_t master_volume;         // Global volume (0-255)
    uint8_t calibration_volume;    // Calibration sounds volume
    uint8_t flight_volume;         // Flight sounds volume
    uint8_t voice_volume;          // Voice synthesis volume
    uint8_t alert_volume;          // Alert sounds volume
    bool enable_calibration_audio; // Enable/disable calibration sounds
    bool enable_flight_audio;      // Enable/disable flight sounds
    bool enable_voice_feedback;    // Enable/disable voice
    bool enable_musical_modes;     // Enable/disable musical effects
    bool spatial_audio_enabled;    // 3D spatial audio effects
    uint16_t audio_sample_rate;    // Sample rate (8000, 16000, 22050, 44100)
    uint8_t audio_bit_depth;       // Bit depth (8, 16)
} premium_audio_config_t;

// Real-time Audio Status
typedef struct {
    bool is_playing;               // Currently playing audio
    bool is_muted;                 // Audio muted
    uint8_t current_volume;        // Current volume level
    uint16_t current_frequency;    // Current tone frequency
    uint32_t playback_time_ms;     // Current playback time
    calibration_audio_state_t calib_state; // Current calibration audio state
    flight_audio_state_t flight_state;     // Current flight audio state
    float cpu_usage_percent;       // Audio processing CPU usage
    uint16_t buffer_usage_percent; // Audio buffer usage
} premium_audio_status_t;

// Main Audio System Functions
void premium_audio_init(premium_audio_config_t *config);
void premium_audio_deinit(void);
void premium_audio_update(void);  // Call regularly for smooth playback

// Configuration
void premium_audio_set_config(premium_audio_config_t *config);
void premium_audio_get_config(premium_audio_config_t *config);
void premium_audio_set_master_volume(uint8_t volume);
void premium_audio_mute(bool mute);

// Basic Audio Functions
void premium_audio_play_tone(uint16_t frequency, uint16_t duration_ms, uint8_t volume);
void premium_audio_play_beep(uint8_t beep_type);
void premium_audio_play_chime(uint8_t chime_type);
void premium_audio_stop_audio(void);
void premium_audio_pause_audio(void);
void premium_audio_resume_audio(void);

// Calibration Audio Feedback
void premium_audio_calibration_state(calibration_audio_state_t state);
void premium_audio_calibration_progress(float progress_percent);
void premium_audio_calibration_milestone(uint8_t milestone);
void premium_audio_calibration_error(uint8_t error_code);
void premium_audio_calibration_success(float quality_score);

// Flight Audio Feedback
void premium_audio_flight_state(flight_audio_state_t state);
void premium_audio_battery_warning(float battery_percent);
void premium_audio_altitude_warning(float current_altitude, float limit);
void premium_audio_gps_status(bool has_lock, uint8_t satellite_count);
void premium_audio_emergency_alert(uint8_t emergency_type);

// Advanced Audio Effects
void premium_audio_play_sequence(audio_sequence_t *sequence);
void premium_audio_play_melody(musical_note_t *notes, uint8_t note_count);
void premium_audio_frequency_sweep(uint16_t start_freq, uint16_t end_freq, uint16_t duration_ms);
void premium_audio_harmony_chord(uint16_t frequencies[4], uint16_t duration_ms);

// Voice Synthesis
void premium_audio_speak_text(const char *text);
void premium_audio_speak_with_config(voice_synthesis_t *voice_config);
void premium_audio_speak_number(float number, const char *units);
void premium_audio_speak_calibration_status(float progress, const char *sensor);
void premium_audio_speak_flight_info(const char *info);

// Musical Modes
void premium_audio_play_startup_jingle(void);
void premium_audio_play_success_fanfare(void);
void premium_audio_play_error_melody(void);
void premium_audio_play_celebration(void);
void premium_audio_play_ambient_background(uint8_t ambient_type);

// Spatial Audio (for stereo setups)
void premium_audio_set_spatial_position(float x, float y, float z);
void premium_audio_spatial_warning_direction(float angle_degrees);
void premium_audio_3d_audio_effect(uint16_t frequency, float x, float y, float z);

// Custom Audio Patterns
void premium_audio_create_custom_pattern(const char *pattern_name, musical_note_t *notes, uint8_t count);
void premium_audio_play_custom_pattern(const char *pattern_name);
void premium_audio_delete_custom_pattern(const char *pattern_name);

// Audio Preferences & Profiles
void premium_audio_save_preferences(void);
void premium_audio_load_preferences(void);
void premium_audio_create_audio_profile(const char *profile_name);
void premium_audio_load_audio_profile(const char *profile_name);

// Diagnostic & Status
void premium_audio_get_status(premium_audio_status_t *status);
bool premium_audio_test_output(void);
void premium_audio_calibrate_output(void);
float premium_audio_get_cpu_usage(void);
uint16_t premium_audio_get_memory_usage(void);

// Predefined Audio Sequences
extern const audio_sequence_t AUDIO_SEQUENCE_STARTUP;
extern const audio_sequence_t AUDIO_SEQUENCE_CALIBRATION_START;
extern const audio_sequence_t AUDIO_SEQUENCE_CALIBRATION_SUCCESS;
extern const audio_sequence_t AUDIO_SEQUENCE_FLIGHT_READY;
extern const audio_sequence_t AUDIO_SEQUENCE_EMERGENCY;
extern const audio_sequence_t AUDIO_SEQUENCE_LOW_BATTERY;
extern const audio_sequence_t AUDIO_SEQUENCE_GPS_LOCK;
extern const audio_sequence_t AUDIO_SEQUENCE_MISSION_COMPLETE;

// Musical Scales and Chords
extern const uint16_t MAJOR_SCALE[8];
extern const uint16_t MINOR_SCALE[8];
extern const uint16_t PENTATONIC_SCALE[5];
extern const uint16_t MAJOR_CHORD[3];
extern const uint16_t MINOR_CHORD[3];

#ifdef __cplusplus
}
#endif

#endif // __PREMIUM_AUDIO_FEEDBACK_H__
