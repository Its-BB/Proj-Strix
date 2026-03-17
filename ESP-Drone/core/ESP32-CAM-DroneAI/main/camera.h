/**
 * ESP32-CAM Camera Wrapper
 * 
 * Wrapper around esp_camera driver for OV2640 sensor
 */

#ifndef CAMERA_H
#define CAMERA_H

#include "esp_err.h"
#include "esp_camera.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * Initialize camera with default settings
 */
esp_err_t camera_init(void);

/**
 * Get camera configuration
 */
camera_config_t* camera_get_config(void);

/**
 * Update camera settings
 */
esp_err_t camera_set_quality(uint8_t quality);
esp_err_t camera_set_framesize(framesize_t framesize);
esp_err_t camera_set_brightness(int brightness);
esp_err_t camera_set_contrast(int contrast);

/**
 * Capture a single frame
 */
camera_fb_t* camera_capture_frame(void);

/**
 * Return frame buffer (must be called after capture)
 */
void camera_return_frame(camera_fb_t* fb);

/**
 * Get camera status
 */
bool camera_is_initialized(void);

/**
 * Convert frame to JPEG (utility function)
 */
bool frame2jpg(camera_fb_t* fb, uint8_t quality, uint8_t** out_buf, size_t* out_len);

#endif // CAMERA_H
