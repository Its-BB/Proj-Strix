/**
 * ESP-Drone Simplified Web Server Implementation
 * Compatible with ESP-IDF v4.4.5 - No WebSocket support
 */

#include "web_server.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "cJSON.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#define TAG "WEB_SERVER"

// Static variables
static httpd_handle_t server = NULL;
static web_data_t current_data = {0};
static bool server_running = false;

// External function declarations - actual implementations are in other components
extern void setCommandermode(uint8_t mode);
extern void leaderCommSetFollowMode(bool enabled, int16_t targetDist);
extern void leaderCommSetFollowParams(int16_t target, int16_t tol, float speedFactor);
extern void stabilizerSetEmergencyStop(void);
extern void leaderCommLockAltitude(void);
extern void leaderCommUnlockAltitude(void);

// HTML content - simplified without WebSocket
static const char* index_html = 
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"    <meta charset='UTF-8'>\n"
"    <meta name='viewport' content='width=device-width, initial-scale=1.0'>\n"
"    <title>ESP-Drone Control</title>\n"
"    <style>\n"
"        * { margin: 0; padding: 0; box-sizing: border-box; }\n"
"        body {\n"
"            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;\n"
"            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);\n"
"            color: white; min-height: 100vh; padding: 20px;\n"
"        }\n"
"        .container { max-width: 1200px; margin: 0 auto; }\n"
"        .header {\n"
"            text-align: center; margin-bottom: 30px;\n"
"            background: rgba(255,255,255,0.1); padding: 20px;\n"
"            border-radius: 15px; backdrop-filter: blur(10px);\n"
"        }\n"
"        .header h1 { font-size: 2.5rem; margin-bottom: 10px; }\n"
"        .status-grid {\n"
"            display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));\n"
"            gap: 20px; margin-bottom: 30px;\n"
"        }\n"
"        .card {\n"
"            background: rgba(255,255,255,0.15); padding: 20px;\n"
"            border-radius: 15px; backdrop-filter: blur(10px);\n"
"            border: 1px solid rgba(255,255,255,0.2);\n"
"        }\n"
"        .card h3 { color: #4fc3f7; margin-bottom: 15px; font-size: 1.2rem; }\n"
"        .value { font-size: 1.8rem; font-weight: bold; margin-bottom: 5px; }\n"
"        .label { font-size: 0.9rem; opacity: 0.8; }\n"
"        .controls {\n"
"            display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));\n"
"            gap: 20px; margin-bottom: 30px;\n"
"        }\n"
"        .btn {\n"
"            background: linear-gradient(45deg, #4fc3f7, #29b6f6);\n"
"            border: none; padding: 12px 24px; border-radius: 25px;\n"
"            color: white; font-weight: bold; cursor: pointer;\n"
"            transition: transform 0.2s; margin: 5px;\n"
"        }\n"
"        .btn:hover { transform: translateY(-2px); }\n"
"        .btn-danger { background: linear-gradient(45deg, #f44336, #d32f2f); }\n"
"        .emergency {\n"
"            position: fixed; bottom: 20px; right: 20px;\n"
"        }\n"
"        @media (max-width: 768px) {\n"
"            .status-grid, .controls { grid-template-columns: 1fr; }\n"
"            .header h1 { font-size: 2rem; }\n"
"        }\n"
"    </style>\n"
"</head>\n"
"<body>\n"
"    <div class='container'>\n"
"        <div class='header'>\n"
"            <h1>🚁 ESP-Drone Control</h1>\n"
"            <div id='connection'>Connected (Polling Mode)</div>\n"
"        </div>\n"
"        \n"
"        <div class='status-grid'>\n"
"            <div class='card'>\n"
"                <h3>Drone Status</h3>\n"
"                <div class='value' id='droneStatus'>Disarmed</div>\n"
"                <div class='label' id='flightMode'>Stabilize Mode</div>\n"
"            </div>\n"
"            \n"
"            <div class='card'>\n"
"                <h3>Battery</h3>\n"
"                <div class='value' id='battery'>--</div>\n"
"                <div class='label' id='voltage'>0.0V</div>\n"
"            </div>\n"
"            \n"
"            <div class='card'>\n"
"                <h3>Altitude</h3>\n"
"                <div class='value' id='altitude'>0.0m</div>\n"
"                <div class='label' id='altitudeAsl'>ASL: 0.0m</div>\n"
"            </div>\n"
"            \n"
"            <div class='card'>\n"
"                <h3>Leader Distance</h3>\n"
"                <div class='value' id='leaderDistance'>--</div>\n"
"                <div class='label' id='leaderStatus'>No Leader</div>\n"
"            </div>\n"
"            \n"
"            <div class='card'>\n"
"                <h3>System Info</h3>\n"
"                <div class='label'>Uptime: <span id='uptime'>0s</span></div>\n"
"                <div class='label'>Free Heap: <span id='heap'>0 KB</span></div>\n"
"            </div>\n"
"        </div>\n"
"        \n"
"        <div class='controls'>\n"
"            <div class='card'>\n"
"                <h3>Flight Modes</h3>\n"
"                <button class='btn' onclick='setMode(0)'>Stabilize</button>\n"
"                <button class='btn' onclick='setMode(1)'>Alt Hold</button>\n"
"                <button class='btn' onclick='setMode(2)'>Pos Hold</button>\n"
"                <button class='btn' onclick='setMode(4)'>Leader Follow</button>\n"
"            </div>\n"
"            \n"
"            <div class='card'>\n"
"                <h3>Controls</h3>\n"
"                <button class='btn' onclick='lockAltitude()'>Lock Altitude</button>\n"
"                <button class='btn' onclick='unlockAltitude()'>Unlock Altitude</button>\n"
"            </div>\n"
"        </div>\n"
"    </div>\n"
"    \n"
"    <div class='emergency'>\n"
"        <button class='btn btn-danger' onclick='emergencyStop()'>🛑 EMERGENCY STOP</button>\n"
"    </div>\n"
"    \n"
"    <script>\n"
"        function updateUI() {\n"
"            fetch('/api/status')\n"
"                .then(response => response.json())\n"
"                .then(data => {\n"
"                    document.getElementById('droneStatus').textContent = data.armed ? 'Armed' : 'Disarmed';\n"
"                    \n"
"                    const modes = ['Stabilize', 'Alt Hold', 'Pos Hold', 'Pos Set', 'Leader Follow'];\n"
"                    document.getElementById('flightMode').textContent = modes[data.flight_mode] + ' Mode';\n"
"                    \n"
"                    document.getElementById('battery').textContent = data.battery_percentage + '%';\n"
"                    document.getElementById('voltage').textContent = data.battery_voltage.toFixed(1) + 'V';\n"
"                    \n"
"                    document.getElementById('uptime').textContent = formatUptime(data.uptime_seconds);\n"
"                    document.getElementById('heap').textContent = Math.round(data.free_heap / 1024) + ' KB';\n"
"                    \n"
"                    // Update altitude data\n"
"                    document.getElementById('altitude').textContent = data.altitude_meters.toFixed(1) + 'm';\n"
"                    document.getElementById('altitudeAsl').textContent = 'ASL: ' + data.altitude_asl.toFixed(1) + 'm';\n"
"                    \n"
"                    // Update leader distance data\n"
"                    if (data.leader_detected && data.leader_distance > 0) {\n"
"                        document.getElementById('leaderDistance').textContent = data.leader_distance + 'cm';\n"
"                        document.getElementById('leaderStatus').textContent = 'Signal: ' + data.leader_rssi + 'dBm (' + Math.round(data.leader_quality) + '%)';\n"
"                    } else {\n"
"                        document.getElementById('leaderDistance').textContent = '--';\n"
"                        document.getElementById('leaderStatus').textContent = 'No Leader Detected';\n"
"                    }\n"
"                })\n"
"                .catch(error => console.error('Error fetching status:', error));\n"
"        }\n"
"        \n"
"        function formatUptime(seconds) {\n"
"            const hours = Math.floor(seconds / 3600);\n"
"            const minutes = Math.floor((seconds % 3600) / 60);\n"
"            const secs = seconds % 60;\n"
"            return hours + 'h ' + minutes + 'm ' + secs + 's';\n"
"        }\n"
"        \n"
"        function sendCommand(cmd, data = {}) {\n"
"            fetch('/api/command', {\n"
"                method: 'POST',\n"
"                headers: { 'Content-Type': 'application/json' },\n"
"                body: JSON.stringify({command: cmd, data: data})\n"
"            })\n"
"            .then(response => response.json())\n"
"            .then(result => console.log('Command result:', result))\n"
"            .catch(error => console.error('Error sending command:', error));\n"
"        }\n"
"        \n"
"        function setMode(mode) {\n"
"            sendCommand('setFlightMode', {mode: mode});\n"
"        }\n"
"        \n"
"        function emergencyStop() {\n"
"            if (confirm('Emergency stop will immediately disable all motors!')) {\n"
"                sendCommand('emergencyStop');\n"
"            }\n"
"        }\n"
"        \n"
"        function lockAltitude() {\n"
"            sendCommand('lockAltitude');\n"
"        }\n"
"        \n"
"        function unlockAltitude() {\n"
"            sendCommand('unlockAltitude');\n"
"        }\n"
"        \n"
"        // Poll for updates every 500ms\n"
"        setInterval(updateUI, 500);\n"
"        updateUI(); // Initial load\n"
"    </script>\n"
"</body>\n"
"</html>";

// Forward declarations
static esp_err_t index_handler(httpd_req_t *req);
static esp_err_t status_handler(httpd_req_t *req);
static esp_err_t command_handler(httpd_req_t *req);

bool webServerInit(void) {
    // Clear current data
    memset(&current_data, 0, sizeof(web_data_t));
    
    ESP_LOGI(TAG, "Web server initialized (simplified)");
    return true;
}

bool webServerStart(void) {
    if (server != NULL) {
        ESP_LOGW(TAG, "Web server already running");
        return true;
    }
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = WEB_SERVER_PORT;
    config.max_open_sockets = 4;  // Reduced to 4 (max allowed is 7, but we need some buffer)
    config.stack_size = WEB_SERVER_TASK_STACK_SIZE;
    config.task_priority = WEB_SERVER_TASK_PRIORITY;
    config.lru_purge_enable = true;
    
    if (httpd_start(&server, &config) == ESP_OK) {
        // Register URI handlers
        httpd_uri_t index_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = index_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &index_uri);
        
        httpd_uri_t status_uri = {
            .uri = "/api/status",
            .method = HTTP_GET,
            .handler = status_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &status_uri);
        
        httpd_uri_t command_uri = {
            .uri = "/api/command",
            .method = HTTP_POST,
            .handler = command_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &command_uri);
        
        server_running = true;
        
        ESP_LOGI(TAG, "Web server started on port %d", WEB_SERVER_PORT);
        return true;
    }
    
    ESP_LOGE(TAG, "Failed to start web server");
    return false;
}

void webServerStop(void) {
    if (server != NULL) {
        httpd_stop(server);
        server = NULL;
        server_running = false;
        ESP_LOGI(TAG, "Web server stopped");
    }
}

static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, index_html, strlen(index_html));
    return ESP_OK;
}

static esp_err_t status_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    
    cJSON *json = cJSON_CreateObject();
    cJSON_AddBoolToObject(json, "armed", current_data.armed);
    cJSON_AddNumberToObject(json, "flight_mode", current_data.flight_mode);
    cJSON_AddNumberToObject(json, "battery_voltage", current_data.battery_voltage);
    cJSON_AddNumberToObject(json, "battery_percentage", current_data.battery_percentage);
    cJSON_AddBoolToObject(json, "leader_detected", current_data.leader_detected);
    cJSON_AddNumberToObject(json, "leader_distance", current_data.leader_distance);
    cJSON_AddNumberToObject(json, "leader_rssi", current_data.leader_rssi);
    cJSON_AddNumberToObject(json, "leader_quality", current_data.leader_quality);
    cJSON_AddNumberToObject(json, "leader_packets", current_data.leader_packets);
    cJSON_AddNumberToObject(json, "leader_errors", current_data.leader_errors);
    cJSON_AddNumberToObject(json, "uptime_seconds", current_data.uptime_seconds);
    cJSON_AddNumberToObject(json, "free_heap", current_data.free_heap);
    cJSON_AddNumberToObject(json, "altitude_meters", current_data.altitude_meters);
    cJSON_AddNumberToObject(json, "altitude_asl", current_data.altitude_asl);
    
    char *json_string = cJSON_Print(json);
    if (json_string != NULL) {
        httpd_resp_send(req, json_string, strlen(json_string));
        free(json_string);
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "JSON creation failed");
    }
    
    cJSON_Delete(json);
    return ESP_OK;
}

static esp_err_t command_handler(httpd_req_t *req) {
    char content[1024];
    size_t recv_size = MIN(req->content_len, sizeof(content) - 1);
    
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    
    content[ret] = '\0';
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "POST");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    
    // Parse JSON command
    cJSON *json = cJSON_Parse(content);
    cJSON *response = cJSON_CreateObject();
    
    if (json != NULL) {
        cJSON *command = cJSON_GetObjectItem(json, "command");
        cJSON *data = cJSON_GetObjectItem(json, "data");
        
        if (cJSON_IsString(command)) {
            const char *cmd = command->valuestring;
            
            // Handle commands
            if (strcmp(cmd, "setFlightMode") == 0 && data != NULL) {
                cJSON *mode = cJSON_GetObjectItem(data, "mode");
                if (cJSON_IsNumber(mode)) {
                    setCommandermode((uint8_t)mode->valueint);
                    cJSON_AddStringToObject(response, "status", "success");
                    cJSON_AddStringToObject(response, "message", "Flight mode set");
                }
            }
            else if (strcmp(cmd, "setFollowMode") == 0 && data != NULL) {
                cJSON *enabled = cJSON_GetObjectItem(data, "enabled");
                cJSON *target = cJSON_GetObjectItem(data, "targetDistance");
                
                if (cJSON_IsBool(enabled)) {
                    int16_t targetDist = cJSON_IsNumber(target) ? target->valueint : 200;
                    leaderCommSetFollowMode(cJSON_IsTrue(enabled), targetDist);
                    cJSON_AddStringToObject(response, "status", "success");
                    cJSON_AddStringToObject(response, "message", "Follow mode updated");
                }
            }
            else if (strcmp(cmd, "emergencyStop") == 0) {
                stabilizerSetEmergencyStop();
                cJSON_AddStringToObject(response, "status", "success");
                cJSON_AddStringToObject(response, "message", "Emergency stop activated");
            }
            else if (strcmp(cmd, "lockAltitude") == 0) {
                leaderCommLockAltitude();
                cJSON_AddStringToObject(response, "status", "success");
                cJSON_AddStringToObject(response, "message", "Altitude locked");
            }
            else if (strcmp(cmd, "unlockAltitude") == 0) {
                leaderCommUnlockAltitude();
                cJSON_AddStringToObject(response, "status", "success");
                cJSON_AddStringToObject(response, "message", "Altitude unlocked");
            }
            else {
                cJSON_AddStringToObject(response, "status", "error");
                cJSON_AddStringToObject(response, "message", "Unknown command");
            }
        }
        
        cJSON_Delete(json);
    } else {
        cJSON_AddStringToObject(response, "status", "error");
        cJSON_AddStringToObject(response, "message", "Invalid JSON");
    }
    
    char *response_string = cJSON_Print(response);
    if (response_string != NULL) {
        httpd_resp_send(req, response_string, strlen(response_string));
        free(response_string);
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Response creation failed");
    }
    
    cJSON_Delete(response);
    return ESP_OK;
}

void webServerUpdateData(const web_data_t* data) {
    if (data != NULL) {
        memcpy(&current_data, data, sizeof(web_data_t));
        // In polling mode, data is simply stored and served when requested
    }
}

void webServerBroadcast(const char* json_data) {
    // Not used in polling mode - data is served via /api/status endpoint
    (void)json_data; // Suppress unused parameter warning
}

bool webServerIsRunning(void) {
    return server_running;
}
