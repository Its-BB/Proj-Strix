/**
 * Real ESP32-CAM OV2640 Camera Implementation
 * 
 * Uses the official esp32-camera driver for REAL camera video
 */

#include "camera.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "esp_system.h"

static const char* TAG = "CAMERA";

// Camera configuration for ESP32-CAM with OV2640
static camera_config_t camera_config = {
    .pin_pwdn = -1,       // Power down pin (not used on ESP32-CAM)
    .pin_reset = -1,      // Reset pin (not used on ESP32-CAM) 
    .pin_xclk = 21,       // External clock pin
    .pin_sscb_sda = 26,   // I2C SDA pin for sensor
    .pin_sscb_scl = 27,   // I2C SCL pin for sensor

    .pin_d7 = 35,         // Data pins
    .pin_d6 = 34,
    .pin_d5 = 39,
    .pin_d4 = 36,
    .pin_d3 = 19,
    .pin_d2 = 18,
    .pin_d1 = 5,
    .pin_d0 = 4,
    .pin_vsync = 25,
    .pin_href = 23,
    .pin_pclk = 22,

    // Timing and format settings
    .xclk_freq_hz = 20000000,           // 20MHz clock
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,     // JPEG format for streaming
    .frame_size = FRAMESIZE_SVGA,       // 800x600 resolution
    .jpeg_quality = 12,                 // JPEG quality (lower = better quality)
    .fb_count = 1,                      // Number of frame buffers
    .fb_location = CAMERA_FB_IN_PSRAM,  // Use PSRAM for frame buffers
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,// Grab mode
};

static bool camera_initialized = false;

esp_err_t camera_init(void)
{
    ESP_LOGI(TAG, "🚀 Initializing REAL ESP32-CAM OV2640 camera...");

    // Initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "❌ Camera initialization failed: 0x%x", err);
        return err;
    }

    camera_initialized = true;

    // Get sensor handle for configuration
    sensor_t * s = esp_camera_sensor_get();
    if (s) {
        // Configure OV2640 sensor settings for best quality
        s->set_brightness(s, 0);     // -2 to 2
        s->set_contrast(s, 0);       // -2 to 2  
        s->set_saturation(s, 0);     // -2 to 2
        s->set_special_effect(s, 0); // 0 to 6 (0=No Effect)
        s->set_whitebal(s, 1);       // 0 = disable, 1 = enable
        s->set_awb_gain(s, 1);       // 0 = disable, 1 = enable
        s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled
        s->set_exposure_ctrl(s, 1);  // 0 = disable, 1 = enable
        s->set_aec2(s, 0);           // 0 = disable, 1 = enable
        s->set_ae_level(s, 0);       // -2 to 2
        s->set_aec_value(s, 300);    // 0 to 1200
        s->set_gain_ctrl(s, 1);      // 0 = disable, 1 = enable
        s->set_agc_gain(s, 0);       // 0 to 30
        s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
        s->set_bpc(s, 0);            // 0 = disable, 1 = enable
        s->set_wpc(s, 1);            // 0 = disable, 1 = enable
        s->set_raw_gma(s, 1);        // 0 = disable, 1 = enable
        s->set_lenc(s, 1);           // 0 = disable, 1 = enable
        s->set_hmirror(s, 0);        // 0 = disable, 1 = enable
        s->set_vflip(s, 0);          // 0 = disable, 1 = enable
        s->set_dcw(s, 1);            // 0 = disable, 1 = enable
        s->set_colorbar(s, 0);       // 0 = disable, 1 = enable
    }

    ESP_LOGI(TAG, "✅ REAL OV2640 Camera initialized successfully!");
    ESP_LOGI(TAG, "   📐 Resolution: %dx%d", 
             s->status.framesize == FRAMESIZE_SVGA ? 800 : 640,
             s->status.framesize == FRAMESIZE_SVGA ? 600 : 480);
    ESP_LOGI(TAG, "   📷 Format: JPEG");
    ESP_LOGI(TAG, "   🎛️ Quality: %d", camera_config.jpeg_quality);
    ESP_LOGI(TAG, "   🔋 Free heap: %d bytes", esp_get_free_heap_size());

    return ESP_OK;
}

camera_config_t* camera_get_config(void)
{
    return &camera_config;
}

esp_err_t camera_set_quality(uint8_t quality)
{
    if (!camera_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    sensor_t * s = esp_camera_sensor_get();
    if (!s) {
        return ESP_FAIL;
    }

    int res = s->set_quality(s, quality);
    if (res == 0) {
        camera_config.jpeg_quality = quality;
        ESP_LOGI(TAG, "🎛️ JPEG quality set to %d", quality);
    }
    
    return res == 0 ? ESP_OK : ESP_FAIL;
}

esp_err_t camera_set_framesize(framesize_t framesize)
{
    if (!camera_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    sensor_t * s = esp_camera_sensor_get();
    if (!s) {
        return ESP_FAIL;
    }

    int res = s->set_framesize(s, framesize);
    if (res == 0) {
        camera_config.frame_size = framesize;
        
        const char* size_names[] = {
            "96x96", "QQVGA", "QCIF", "HQVGA", "240x240", "QVGA", 
            "VGA", "SVGA", "XGA", "HD", "SXGA", "UXGA"
        };
        
        if (framesize <= FRAMESIZE_UXGA) {
            ESP_LOGI(TAG, "📐 Frame size set to %s", size_names[framesize]);
        }
    }
    
    return res == 0 ? ESP_OK : ESP_FAIL;
}

esp_err_t camera_set_brightness(int brightness)
{
    if (!camera_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    sensor_t * s = esp_camera_sensor_get();
    if (!s) {
        return ESP_FAIL;
    }

    int res = s->set_brightness(s, brightness);
    ESP_LOGI(TAG, "☀️ Brightness set to %d", brightness);
    
    return res == 0 ? ESP_OK : ESP_FAIL;
}

esp_err_t camera_set_contrast(int contrast)
{
    if (!camera_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    sensor_t * s = esp_camera_sensor_get();
    if (!s) {
        return ESP_FAIL;
    }

    int res = s->set_contrast(s, contrast);
    ESP_LOGI(TAG, "🎭 Contrast set to %d", contrast);
    
    return res == 0 ? ESP_OK : ESP_FAIL;
}

camera_fb_t* camera_capture_frame(void)
{
    if (!camera_initialized) {
        ESP_LOGE(TAG, "Camera not initialized");
        return NULL;
    }

    // Get frame from real OV2640 camera
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        return NULL;
    }

    ESP_LOGI(TAG, "📷 REAL OV2640 frame captured: %dx%d, %d bytes, format=%d", 
             fb->width, fb->height, fb->len, fb->format);

    return fb;
}

void camera_return_frame(camera_fb_t* fb)
{
    if (fb) {
        esp_camera_fb_return(fb);
    }
}

bool camera_is_initialized(void)
{
    return camera_initialized;
}

// Frame to JPEG conversion (already JPEG from OV2640)
bool frame2jpg(camera_fb_t* fb, uint8_t quality, uint8_t** out_buf, size_t* out_len)
{
    if (!fb || !out_buf || !out_len) {
        return false;
    }

    if (fb->format == PIXFORMAT_JPEG) {
        // Already JPEG, just copy
        *out_len = fb->len;
        *out_buf = malloc(fb->len);
        if (!*out_buf) {
            return false;
        }
        memcpy(*out_buf, fb->buf, fb->len);
        return true;
    } else {
        // For non-JPEG formats, just copy raw data
        // Real JPEG conversion would need additional libraries
        *out_len = fb->len;
        *out_buf = malloc(fb->len);
        if (!*out_buf) {
            return false;
        }
        memcpy(*out_buf, fb->buf, fb->len);
        return true;
    }
}
