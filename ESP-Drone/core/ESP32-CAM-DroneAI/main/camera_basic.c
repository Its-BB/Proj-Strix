/**
 * Basic Camera Implementation
 * 
 * Simple camera functionality that generates test images
 * This version works without external camera libraries
 */

#include "camera.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include <stdlib.h>
#include <string.h>

static const char* TAG = "CAMERA";

// Camera state
static bool camera_initialized = false;
static camera_config_t camera_config = {
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_SVGA,
    .jpeg_quality = 12,
    .fb_count = 1
};

// Simple JPEG header for test images
static const uint8_t jpeg_header[] = {
    0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01,
    0x01, 0x01, 0x00, 0x48, 0x00, 0x48, 0x00, 0x00, 0xFF, 0xDB, 0x00, 0x43,
    0x00, 0x08, 0x06, 0x06, 0x07, 0x06, 0x05, 0x08, 0x07, 0x07, 0x07, 0x09,
    0x09, 0x08, 0x0A, 0x0C, 0x14, 0x0D, 0x0C, 0x0B, 0x0B, 0x0C, 0x19, 0x12,
    0x13, 0x0F, 0x14, 0x1D, 0x1A, 0x1F, 0x1E, 0x1D, 0x1A, 0x1C, 0x1C, 0x20,
    0x24, 0x2E, 0x27, 0x20, 0x22, 0x2C, 0x23, 0x1C, 0x1C, 0x28, 0x37, 0x29,
    0x2C, 0x30, 0x31, 0x34, 0x34, 0x34, 0x1F, 0x27, 0x39, 0x3D, 0x38, 0x32,
    0x3C, 0x2E, 0x33, 0x34, 0x32, 0xFF, 0xC0, 0x00, 0x11, 0x08, 0x02, 0x58,
    0x03, 0x20, 0x03, 0x01, 0x22, 0x00, 0x02, 0x11, 0x01, 0x03, 0x11, 0x01,
    0xFF, 0xC4, 0x00, 0x1F, 0x00
};

static const uint8_t jpeg_footer[] = { 0xFF, 0xD9 };

esp_err_t camera_init(void)
{
    ESP_LOGI(TAG, "📷 Initializing basic camera module...");
    
    // Simulate camera initialization
    camera_initialized = true;
    
    ESP_LOGI(TAG, "✅ Camera initialized successfully");
    ESP_LOGI(TAG, "   📐 Default resolution: SVGA (800x600)");
    ESP_LOGI(TAG, "   🎛️ Default quality: 12");
    ESP_LOGI(TAG, "   📷 Format: JPEG");
    
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
    
    // Clamp quality to valid range
    if (quality < 4) quality = 4;
    if (quality > 63) quality = 63;
    
    camera_config.jpeg_quality = quality;
    ESP_LOGI(TAG, "🎛️ Quality set to %d", quality);
    
    return ESP_OK;
}

esp_err_t camera_set_framesize(framesize_t framesize)
{
    if (!camera_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    camera_config.frame_size = framesize;
    
    const char* size_names[] = {
        "96x96", "QQVGA", "QCIF", "HQVGA", "240x240", "QVGA", 
        "VGA", "SVGA", "XGA", "HD", "SXGA", "UXGA"
    };
    
    if (framesize <= FRAMESIZE_UXGA) {
        ESP_LOGI(TAG, "📐 Frame size set to %s", size_names[framesize]);
    }
    
    return ESP_OK;
}

esp_err_t camera_set_brightness(int brightness)
{
    if (!camera_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "☀️ Brightness set to %d (simulated)", brightness);
    return ESP_OK;
}

esp_err_t camera_set_contrast(int contrast)
{
    if (!camera_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "🎭 Contrast set to %d (simulated)", contrast);
    return ESP_OK;
}

static void generate_test_pattern(uint8_t* buffer, size_t width, size_t height)
{
    // Generate a simple test pattern
    static uint32_t frame_counter = 0;
    frame_counter++;
    
    // Create a simple gradient pattern with frame counter
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            size_t idx = (y * width + x) * 3; // RGB
            if (idx + 2 < width * height * 3) {
                buffer[idx] = (x + frame_counter) % 256;      // R
                buffer[idx + 1] = (y + frame_counter) % 256;  // G  
                buffer[idx + 2] = ((x + y + frame_counter) / 2) % 256; // B
            }
        }
    }
}

camera_fb_t* camera_capture_frame(void)
{
    if (!camera_initialized) {
        ESP_LOGE(TAG, "Camera not initialized");
        return NULL;
    }
    
    // Use small, fixed dimensions to avoid memory issues
    size_t width = 160, height = 120; // QQVGA - small and memory-efficient
    
    // Create a very simple test JPEG with minimal memory usage
    size_t jpeg_size = sizeof(jpeg_header) + 500 + sizeof(jpeg_footer); // Much smaller
    uint8_t* jpeg_buffer = malloc(jpeg_size);
    if (!jpeg_buffer) {
        ESP_LOGE(TAG, "Failed to allocate JPEG buffer (%d bytes)", jpeg_size);
        return NULL;
    }
    
    // Simple JPEG construction without large intermediate buffers
    memcpy(jpeg_buffer, jpeg_header, sizeof(jpeg_header));
    
    // Add simple test pattern data (much smaller)
    static uint32_t counter = 0;
    counter++;
    for (int i = 0; i < 500; i++) {
        // Create a simple gradient pattern
        uint8_t value = (counter + i) % 256;
        jpeg_buffer[sizeof(jpeg_header) + i] = value;
    }
    
    memcpy(jpeg_buffer + sizeof(jpeg_header) + 500, jpeg_footer, sizeof(jpeg_footer));
    
    // Create frame buffer
    camera_fb_t* fb = malloc(sizeof(camera_fb_t));
    if (!fb) {
        free(jpeg_buffer);
        ESP_LOGE(TAG, "Failed to allocate frame buffer");
        return NULL;
    }
    
    fb->buf = jpeg_buffer;
    fb->len = jpeg_size;
    fb->width = width;
    fb->height = height;
    fb->format = PIXFORMAT_JPEG;
    
    ESP_LOGI(TAG, "📷 Generated test frame: %dx%d, %d bytes (free heap: %d)", 
             width, height, jpeg_size, esp_get_free_heap_size());
    
    return fb;
}

void camera_return_frame(camera_fb_t* fb)
{
    if (fb) {
        if (fb->buf) {
            free(fb->buf);
        }
        free(fb);
    }
}

bool camera_is_initialized(void)
{
    return camera_initialized;
}

// Placeholder for frame2jpg function used in http_server.c
bool frame2jpg(camera_fb_t* fb, uint8_t quality, uint8_t** out_buf, size_t* out_len)
{
    if (!fb || !out_buf || !out_len) {
        return false;
    }
    
    // Since we already have "JPEG" data, just copy it
    *out_len = fb->len;
    *out_buf = malloc(fb->len);
    if (!*out_buf) {
        return false;
    }
    
    memcpy(*out_buf, fb->buf, fb->len);
    return true;
}