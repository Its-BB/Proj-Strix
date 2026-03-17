/**
 * ESP-Drone Web Position Hold Control System Implementation - API Handlers and Public Functions
 */

#include "web_position_hold.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "altitude_logger.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include <string.h>
#include <math.h>

#define DEBUG_MODULE "WEB_PH"
#include "debug_cf.h"

// Define MIN macro if not already defined
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

// Helper function to get time in milliseconds
static uint64_t get_time_ms(void) {
    return esp_timer_get_time() / 1000;
}

// External references
extern position_hold_status_t ph_status;
extern httpd_handle_t web_server;
extern VL53L0X_Dev_t *vl53l0x_device;
extern bool system_initialized;
extern TaskHandle_t control_task_handle;

// External VLX_ENH functions
extern bool vl53l0xEnhancedTest(void);
extern float vl53l0xEnhancedGetDistance(void);

// HTTP API Handlers

// Serve main HTML page
esp_err_t web_position_hold_handler_root(httpd_req_t *req) {
    extern const char* html_page;
    
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
    
    return ESP_OK;
}

// Get current status as JSON
esp_err_t web_position_hold_handler_api_status(httpd_req_t *req) {
    cJSON *json = cJSON_CreateObject();
    
    cJSON_AddNumberToObject(json, "state", ph_status.state);
    cJSON_AddNumberToObject(json, "target_height", ph_status.target_height_cm);
    cJSON_AddNumberToObject(json, "current_height", ph_status.current_height_cm);
    cJSON_AddNumberToObject(json, "height_error", ph_status.height_error_cm);
    cJSON_AddNumberToObject(json, "thrust_output", ph_status.thrust_output);
    cJSON_AddBoolToObject(json, "sensor_valid", ph_status.sensor_valid);
    cJSON_AddNumberToObject(json, "active_time", ph_status.active_time_ms);
    cJSON_AddBoolToObject(json, "emergency_stop", ph_status.emergency_stop_active);
    cJSON_AddBoolToObject(json, "low_height_warning", ph_status.low_height_warning);
    cJSON_AddBoolToObject(json, "high_height_warning", ph_status.high_height_warning);
    
    char *json_string = cJSON_Print(json);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_string, HTTPD_RESP_USE_STRLEN);
    
    free(json_string);
    cJSON_Delete(json);
    
    return ESP_OK;
}

// Start position hold
esp_err_t web_position_hold_handler_api_start(httpd_req_t *req) {
    ESP_LOGI(DEBUG_MODULE, "API: Start Position Hold button clicked on web interface");
    
    cJSON *json = cJSON_CreateObject();
    
    bool success = web_position_hold_start();
    
    cJSON_AddBoolToObject(json, "success", success);
    if (!success) {
        const char* error_msg = "Failed to start position hold";
        if (ph_status.state != PH_STATE_IDLE) {
            error_msg = "Position hold already active";
        } else if (!ph_status.sensor_valid) {
            error_msg = "Sensor not ready";
        }
        cJSON_AddStringToObject(json, "error", error_msg);
    }
    
    char *json_string = cJSON_Print(json);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_string, HTTPD_RESP_USE_STRLEN);
    
    free(json_string);
    cJSON_Delete(json);
    
    return ESP_OK;
}

// Stop position hold
esp_err_t web_position_hold_handler_api_stop(httpd_req_t *req) {
    ESP_LOGI(DEBUG_MODULE, "API: Stop button clicked on web interface");
    
    web_position_hold_stop();
    
    cJSON *json = cJSON_CreateObject();
    cJSON_AddBoolToObject(json, "success", true);
    
    char *json_string = cJSON_Print(json);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_string, HTTPD_RESP_USE_STRLEN);
    
    free(json_string);
    cJSON_Delete(json);
    
    return ESP_OK;
}

// Initiate landing
esp_err_t web_position_hold_handler_api_land(httpd_req_t *req) {
    ESP_LOGI(DEBUG_MODULE, "API: Land Safely button clicked on web interface");
    
    cJSON *json = cJSON_CreateObject();
    
    bool success = web_position_hold_land();
    
    cJSON_AddBoolToObject(json, "success", success);
    if (!success) {
        cJSON_AddStringToObject(json, "error", "Cannot land - system not active");
    }
    
    char *json_string = cJSON_Print(json);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_string, HTTPD_RESP_USE_STRLEN);
    
    free(json_string);
    cJSON_Delete(json);
    
    return ESP_OK;
}

// Set target height
esp_err_t web_position_hold_handler_api_set_height(httpd_req_t *req) {
    char content[100];
    size_t recv_size = MIN(req->content_len, sizeof(content) - 1);
    
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    cJSON *json = cJSON_Parse(content);
    cJSON *response = cJSON_CreateObject();
    
    if (json && cJSON_HasObjectItem(json, "height")) {
        double height = cJSON_GetObjectItem(json, "height")->valuedouble;
        
        bool success = web_position_hold_set_target((float)height);
        cJSON_AddBoolToObject(response, "success", success);
        
        if (!success) {
            cJSON_AddStringToObject(response, "error", "Invalid height range (5-200 cm)");
        }
    } else {
        cJSON_AddBoolToObject(response, "success", false);
        cJSON_AddStringToObject(response, "error", "Invalid JSON or missing height parameter");
    }
    
    char *response_string = cJSON_Print(response);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, response_string, HTTPD_RESP_USE_STRLEN);
    
    free(response_string);
    cJSON_Delete(response);
    if (json) cJSON_Delete(json);
    
    return ESP_OK;
}

// Emergency stop handler
esp_err_t web_position_hold_handler_api_emergency(httpd_req_t *req) {
    ESP_LOGI(DEBUG_MODULE, "API: EMERGENCY STOP button clicked on web interface");
    
    web_position_hold_emergency_stop();
    
    cJSON *json = cJSON_CreateObject();
    cJSON_AddBoolToObject(json, "success", true);
    
    char *json_string = cJSON_Print(json);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_string, HTTPD_RESP_USE_STRLEN);
    
    free(json_string);
    cJSON_Delete(json);
    
    return ESP_OK;
}

// Public API Functions Implementation

bool web_position_hold_init(VL53L0X_Dev_t *vl53l0x_dev) {
    if (system_initialized) {
        ESP_LOGW(DEBUG_MODULE, "System already initialized");
        return true;
    }
    
    if (!vl53l0x_dev) {
        ESP_LOGE(DEBUG_MODULE, "VL53L0X device pointer is NULL");
        return false;
    }
    
    ESP_LOGI(DEBUG_MODULE, "Initializing Web Position Hold Control System");
    
    // Store VL53L0X device reference
    vl53l0x_device = vl53l0x_dev;
    
    // Initialize status structure
    memset(&ph_status, 0, sizeof(ph_status));
    ph_status.state = PH_STATE_IDLE;
    ph_status.target_height_cm = POSITION_HOLD_DEFAULT_HEIGHT_CM;
    ph_status.last_update_ms = get_time_ms();
    
    // Test sensor directly via VLX_ENH system
    if (vl53l0xEnhancedTest()) {
        float test_distance = vl53l0xEnhancedGetDistance();
        if (test_distance > 0.0f) {
            ESP_LOGI(DEBUG_MODULE, "VLX_ENH sensor test passed: %.1f cm", test_distance);
        } else {
            ESP_LOGW(DEBUG_MODULE, "VLX_ENH sensor available but not providing valid data yet");
        }
    } else {
        ESP_LOGW(DEBUG_MODULE, "VLX_ENH sensor not available, but continuing initialization");
    }
    
    system_initialized = true;
    ESP_LOGI(DEBUG_MODULE, "Web Position Hold System initialized successfully");
    
    return true;
}

bool web_position_hold_start_server(void) {
    if (web_server != NULL) {
        ESP_LOGW(DEBUG_MODULE, "Web server already running");
        return true;
    }
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = WEB_SERVER_PORT;
    config.max_open_sockets = 5;
    config.lru_purge_enable = true;
    
    ESP_LOGI(DEBUG_MODULE, "Starting web server on port %d", config.server_port);
    
    if (httpd_start(&web_server, &config) != ESP_OK) {
        ESP_LOGE(DEBUG_MODULE, "Failed to start web server");
        return false;
    }
    
    // Register URI handlers
    httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = web_position_hold_handler_root,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(web_server, &root_uri);
    
    httpd_uri_t status_uri = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = web_position_hold_handler_api_status,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(web_server, &status_uri);
    
    httpd_uri_t start_uri = {
        .uri = "/api/start",
        .method = HTTP_POST,
        .handler = web_position_hold_handler_api_start,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(web_server, &start_uri);
    
    httpd_uri_t stop_uri = {
        .uri = "/api/stop",
        .method = HTTP_POST,
        .handler = web_position_hold_handler_api_stop,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(web_server, &stop_uri);
    
    httpd_uri_t land_uri = {
        .uri = "/api/land",
        .method = HTTP_POST,
        .handler = web_position_hold_handler_api_land,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(web_server, &land_uri);
    
    httpd_uri_t set_height_uri = {
        .uri = "/api/set_height",
        .method = HTTP_POST,
        .handler = web_position_hold_handler_api_set_height,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(web_server, &set_height_uri);
    
    httpd_uri_t emergency_uri = {
        .uri = "/api/emergency",
        .method = HTTP_POST,
        .handler = web_position_hold_handler_api_emergency,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(web_server, &emergency_uri);
    
    ESP_LOGI(DEBUG_MODULE, "Web server started successfully");
    ESP_LOGI(DEBUG_MODULE, "Access the interface at: http://192.168.43.42/");
    
    return true;
}

void web_position_hold_stop_server(void) {
    if (web_server) {
        httpd_stop(web_server);
        web_server = NULL;
        ESP_LOGI(DEBUG_MODULE, "Web server stopped");
    }
}

bool web_position_hold_set_target(float height_cm) {
    if (height_cm < POSITION_HOLD_MIN_HEIGHT_CM || height_cm > POSITION_HOLD_MAX_HEIGHT_CM) {
        ESP_LOGW(DEBUG_MODULE, "Invalid target height: %.1f cm (valid range: %.1f - %.1f cm)", 
                height_cm, POSITION_HOLD_MIN_HEIGHT_CM, POSITION_HOLD_MAX_HEIGHT_CM);
        return false;
    }
    
    ph_status.target_height_cm = height_cm;
    ESP_LOGI(DEBUG_MODULE, "Target height set to %.1f cm", height_cm);
    
    return true;
}

bool web_position_hold_start(void) {
    if (!system_initialized || !vl53l0x_device) {
        ESP_LOGE(DEBUG_MODULE, "System not initialized");
        return false;
    }
    
    if (ph_status.state != PH_STATE_IDLE) {
        ESP_LOGW(DEBUG_MODULE, "Position hold already active (state: %d)", ph_status.state);
        return false;
    }
    
    // Test sensor before starting
    if (!web_position_hold_update_sensor_reading()) {
        ESP_LOGE(DEBUG_MODULE, "Sensor not ready - cannot start position hold");
        return false;
    }
    
    // Reset PID controller
    ph_status.pid_integral = 0;
    ph_status.pid_derivative = 0;
    ph_status.pid_last_error = 0;
    ph_status.emergency_stop_active = false;
    
    // Start control task if not running
    if (control_task_handle == NULL) {
        extern void position_hold_control_task(void *pvParameters);
        BaseType_t result = xTaskCreate(
            position_hold_control_task,
            "web_pos_hold",
            4096,
            NULL,
            5,  // High priority for control loop
            &control_task_handle
        );
        
        if (result != pdPASS) {
            ESP_LOGE(DEBUG_MODULE, "Failed to create control task");
            return false;
        }
    }
    
    // Check if drone is already airborne or needs takeoff
    float current_height = ph_status.current_height_cm;
    
    if (current_height < 8.0f) {
        // Drone is on or near ground - start takeoff sequence
        ph_status.state = PH_STATE_TAKEOFF;
        ESP_LOGI(DEBUG_MODULE, "🚁 Starting TAKEOFF mode from %.1f cm", current_height);
        ph_status.takeoff_start_time = get_time_ms();
        ph_status.takeoff_initial_height = current_height;
        ph_status.active_time_ms = get_time_ms();
        ph_status.last_update_ms = get_time_ms();
        
        ESP_LOGI(DEBUG_MODULE, "Starting takeoff from ground level (%.1f cm)", current_height);
    } else {
        // Drone already airborne - go directly to position hold
        ph_status.state = PH_STATE_POSITION_HOLD;
        ph_status.active_time_ms = get_time_ms();
        ph_status.last_update_ms = get_time_ms();
        
        ESP_LOGI(DEBUG_MODULE, "Position hold started - already airborne at %.1f cm", current_height);
    }
    
    ESP_LOGI(DEBUG_MODULE, "Target height: %.1f cm", ph_status.target_height_cm);
    
    return true;
}

void web_position_hold_stop(void) {
    if (ph_status.state == PH_STATE_IDLE) {
        return;
    }
    
    ph_status.state = PH_STATE_IDLE;
    ph_status.emergency_stop_active = false;
    
    ESP_LOGI(DEBUG_MODULE, "Position hold stopped");
}

bool web_position_hold_land(void) {
    if (ph_status.state != PH_STATE_POSITION_HOLD) {
        ESP_LOGW(DEBUG_MODULE, "Cannot land - position hold not active");
        return false;
    }
    
    ph_status.state = PH_STATE_LANDING;
    ph_status.active_time_ms = get_time_ms();
    
    ESP_LOGI(DEBUG_MODULE, "Landing initiated from %.1f cm", ph_status.current_height_cm);
    
    return true;
}

void web_position_hold_emergency_stop(void) {
    ph_status.emergency_stop_active = true;
    ph_status.state = PH_STATE_ERROR;
    
    ESP_LOGW(DEBUG_MODULE, "EMERGENCY STOP ACTIVATED");
}

const position_hold_status_t* web_position_hold_get_status(void) {
    return &ph_status;
}

bool web_position_hold_is_active(void) {
    return (ph_status.state == PH_STATE_POSITION_HOLD || 
            ph_status.state == PH_STATE_LANDING);
}

bool web_position_hold_test(void) {
    if (!system_initialized) {
        ESP_LOGE(DEBUG_MODULE, "System not initialized for testing");
        return false;
    }
    
    // Test sensor directly via VLX_ENH
    if (!vl53l0xEnhancedTest()) {
        ESP_LOGE(DEBUG_MODULE, "VLX_ENH sensor test failed - sensor not available");
        return false;
    }
    
    float distance = vl53l0xEnhancedGetDistance();
    if (distance <= 0.0f) {
        ESP_LOGE(DEBUG_MODULE, "VLX_ENH sensor test failed - invalid reading: %.1f cm", distance);
        return false;
    }
    
    ESP_LOGI(DEBUG_MODULE, "System test passed - VLX_ENH sensor reading: %.1f cm", distance);
    return true;
}
