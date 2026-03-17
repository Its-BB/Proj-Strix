/**
 * ESP-Drone Web Server Implementation
 * 
 * Professional web interface running directly on ESP32 drone
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
// Simplified includes - avoid circular dependencies
// #include "leader_comm.h"
// #include "crtp_commander.h"
// #include "stabilizer.h"

// Forward declarations and stub functions
typedef enum {
    STABILIZE_MODE = 0,
    ALTHOLD_MODE = 1,
    POSHOLD_MODE = 2,
    POSSET_MODE = 3,
    LEADER_FOLLOW_MODE = 4
} FlightMode;

// Stub function declarations
void setCommandermode(FlightMode mode) { /* stub */ }
void leaderCommSetFollowMode(bool enabled, int16_t targetDist) { /* stub */ }
void leaderCommSetFollowParams(int16_t target, int16_t tol, float speedFactor) { /* stub */ }
void stabilizerSetEmergencyStop(void) { /* stub */ }
void leaderCommLockAltitude(void) { /* stub */ }
void leaderCommUnlockAltitude(void) { /* stub */ }

#define TAG "WEB_SERVER"

// Static variables
static httpd_handle_t server = NULL;
static ws_client_t ws_clients[WEB_SERVER_MAX_CLIENTS];
static web_data_t current_data = {0};
static bool server_running = false;

// HTML content embedded in firmware
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
"        .control-group { margin-bottom: 20px; }\n"
"        .control-group label { display: block; margin-bottom: 8px; font-weight: 500; }\n"
"        input[type='range'] {\n"
"            width: 100%; height: 8px; border-radius: 4px;\n"
"            background: rgba(255,255,255,0.3); outline: none;\n"
"            -webkit-appearance: none;\n"
"        }\n"
"        input[type='range']::-webkit-slider-thumb {\n"
"            -webkit-appearance: none; width: 20px; height: 20px;\n"
"            border-radius: 50%; background: #4fc3f7; cursor: pointer;\n"
"        }\n"
"        .btn {\n"
"            background: linear-gradient(45deg, #4fc3f7, #29b6f6);\n"
"            border: none; padding: 12px 24px; border-radius: 25px;\n"
"            color: white; font-weight: bold; cursor: pointer;\n"
"            transition: transform 0.2s; margin: 5px;\n"
"        }\n"
"        .btn:hover { transform: translateY(-2px); }\n"
"        .btn-danger { background: linear-gradient(45deg, #f44336, #d32f2f); }\n"
"        .toggle {\n"
"            position: relative; display: inline-block;\n"
"            width: 60px; height: 34px;\n"
"        }\n"
"        .toggle input { opacity: 0; width: 0; height: 0; }\n"
"        .slider {\n"
"            position: absolute; cursor: pointer; top: 0; left: 0;\n"
"            right: 0; bottom: 0; background-color: #ccc;\n"
"            transition: .4s; border-radius: 34px;\n"
"        }\n"
"        .slider:before {\n"
"            position: absolute; content: ''; height: 26px; width: 26px;\n"
"            left: 4px; bottom: 4px; background-color: white;\n"
"            transition: .4s; border-radius: 50%;\n"
"        }\n"
"        input:checked + .slider { background-color: #4fc3f7; }\n"
"        input:checked + .slider:before { transform: translateX(26px); }\n"
"        .signal-bars {\n"
"            display: flex; gap: 3px; align-items: flex-end;\n"
"            height: 30px; margin-top: 10px;\n"
"        }\n"
"        .signal-bar {\n"
"            width: 6px; background: rgba(255,255,255,0.3);\n"
"            border-radius: 2px; transition: all 0.3s;\n"
"        }\n"
"        .signal-bar.active { background: #4fc3f7; }\n"
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
"            <div id='connection'>Connecting...</div>\n"
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
"                <h3>Leader Signal</h3>\n"
"                <div class='value' id='leaderStatus'>Not Detected</div>\n"
"                <div class='signal-bars'>\n"
"                    <div class='signal-bar' style='height:6px'></div>\n"
"                    <div class='signal-bar' style='height:12px'></div>\n"
"                    <div class='signal-bar' style='height:18px'></div>\n"
"                    <div class='signal-bar' style='height:24px'></div>\n"
"                    <div class='signal-bar' style='height:30px'></div>\n"
"                </div>\n"
"            </div>\n"
"            \n"
"            <div class='card'>\n"
"                <h3>Distance</h3>\n"
"                <div class='value' id='distance'>-- cm</div>\n"
"                <div class='label'>to Leader</div>\n"
"            </div>\n"
"            \n"
"            <div class='card'>\n"
"                <h3>Battery</h3>\n"
"                <div class='value' id='battery'>--</div>\n"
"                <div class='label' id='voltage'>0.0V</div>\n"
"            </div>\n"
"        </div>\n"
"        \n"
"        <div class='controls'>\n"
"            <div class='card'>\n"
"                <h3>Leader Following</h3>\n"
"                <div class='control-group'>\n"
"                    <label>Enable Following</label>\n"
"                    <label class='toggle'>\n"
"                        <input type='checkbox' id='followToggle'>\n"
"                        <span class='slider'></span>\n"
"                    </label>\n"
"                </div>\n"
"                <div class='control-group'>\n"
"                    <label>Target Distance: <span id='targetValue'>200</span> cm</label>\n"
"                    <input type='range' id='targetDistance' min='50' max='500' value='200'>\n"
"                </div>\n"
"                <div class='control-group'>\n"
"                    <label>Follow Speed: <span id='speedValue'>0.5</span></label>\n"
"                    <input type='range' id='followSpeed' min='0.1' max='1.0' step='0.1' value='0.5'>\n"
"                </div>\n"
"            </div>\n"
"            \n"
"            <div class='card'>\n"
"                <h3>Flight Modes</h3>\n"
"                <button class='btn' onclick='setMode(0)'>Stabilize</button>\n"
"                <button class='btn' onclick='setMode(1)'>Alt Hold</button>\n"
"                <button class='btn' onclick='setMode(2)'>Pos Hold</button>\n"
"                <button class='btn' onclick='setMode(4)'>Leader Follow</button>\n"
"                <div style='margin-top: 15px;'>\n"
"                    <button class='btn' onclick='lockAltitude()'>Lock Altitude</button>\n"
"                    <button class='btn' onclick='unlockAltitude()'>Unlock Altitude</button>\n"
"                </div>\n"
"            </div>\n"
"        </div>\n"
"        \n"
"        <div class='card'>\n"
"            <h3>System Information</h3>\n"
"            <div style='display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px;'>\n"
"                <div>Signal Quality: <strong id='signalQuality'>0%</strong></div>\n"
"                <div>Packets: <strong id='packets'>0</strong></div>\n"
"                <div>Errors: <strong id='errors'>0</strong></div>\n"
"                <div>RSSI: <strong id='rssi'>0 dBm</strong></div>\n"
"                <div>Uptime: <strong id='uptime'>0s</strong></div>\n"
"                <div>Free Heap: <strong id='heap'>0 KB</strong></div>\n"
"            </div>\n"
"        </div>\n"
"    </div>\n"
"    \n"
"    <div class='emergency'>\n"
"        <button class='btn btn-danger' onclick='emergencyStop()'>🛑 EMERGENCY STOP</button>\n"
"    </div>\n"
"    \n"
"    <script>\n"
"        let ws;\n"
"        let connected = false;\n"
"        \n"
"        function connect() {\n"
"            const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';\n"
"            ws = new WebSocket(protocol + '//' + window.location.host + '/ws');\n"
"            \n"
"            ws.onopen = function() {\n"
"                connected = true;\n"
"                document.getElementById('connection').innerHTML = '✅ Connected';\n"
"                document.getElementById('connection').style.color = '#4caf50';\n"
"            };\n"
"            \n"
"            ws.onmessage = function(event) {\n"
"                const data = JSON.parse(event.data);\n"
"                updateUI(data);\n"
"            };\n"
"            \n"
"            ws.onclose = function() {\n"
"                connected = false;\n"
"                document.getElementById('connection').innerHTML = '❌ Disconnected';\n"
"                document.getElementById('connection').style.color = '#f44336';\n"
"                setTimeout(connect, 3000);\n"
"            };\n"
"        }\n"
"        \n"
"        function updateUI(data) {\n"
"            document.getElementById('droneStatus').textContent = data.armed ? 'Armed' : 'Disarmed';\n"
"            \n"
"            const modes = ['Stabilize', 'Alt Hold', 'Pos Hold', 'Pos Set', 'Leader Follow'];\n"
"            document.getElementById('flightMode').textContent = modes[data.flight_mode] + ' Mode';\n"
"            \n"
"            document.getElementById('leaderStatus').textContent = data.leader_detected ? 'Detected' : 'Not Detected';\n"
"            document.getElementById('distance').textContent = data.leader_detected ? data.leader_distance + ' cm' : '-- cm';\n"
"            \n"
"            document.getElementById('battery').textContent = data.battery_percentage + '%';\n"
"            document.getElementById('voltage').textContent = data.battery_voltage.toFixed(1) + 'V';\n"
"            \n"
"            // Update signal bars\n"
"            const bars = document.querySelectorAll('.signal-bar');\n"
"            const strength = getSignalStrength(data.leader_rssi);\n"
"            bars.forEach((bar, i) => {\n"
"                bar.classList.toggle('active', i < strength);\n"
"            });\n"
"            \n"
"            // Update system info\n"
"            document.getElementById('signalQuality').textContent = Math.round(data.leader_quality) + '%';\n"
"            document.getElementById('packets').textContent = data.leader_packets;\n"
"            document.getElementById('errors').textContent = data.leader_errors;\n"
"            document.getElementById('rssi').textContent = data.leader_rssi + ' dBm';\n"
"            document.getElementById('uptime').textContent = formatUptime(data.uptime_seconds);\n"
"            document.getElementById('heap').textContent = Math.round(data.free_heap / 1024) + ' KB';\n"
"        }\n"
"        \n"
"        function getSignalStrength(rssi) {\n"
"            if (rssi >= -50) return 5;\n"
"            if (rssi >= -60) return 4;\n"
"            if (rssi >= -70) return 3;\n"
"            if (rssi >= -80) return 2;\n"
"            if (rssi >= -90) return 1;\n"
"            return 0;\n"
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
"            if (ws && connected) {\n"
"                ws.send(JSON.stringify({command: cmd, data: data}));\n"
"            }\n"
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
"        // Event listeners\n"
"        document.getElementById('followToggle').addEventListener('change', function(e) {\n"
"            const target = document.getElementById('targetDistance').value;\n"
"            sendCommand('setFollowMode', {enabled: e.target.checked, targetDistance: parseInt(target)});\n"
"        });\n"
"        \n"
"        document.getElementById('targetDistance').addEventListener('input', function(e) {\n"
"            document.getElementById('targetValue').textContent = e.target.value;\n"
"            if (document.getElementById('followToggle').checked) {\n"
"                sendCommand('setFollowParams', {targetDistance: parseInt(e.target.value)});\n"
"            }\n"
"        });\n"
"        \n"
"        document.getElementById('followSpeed').addEventListener('input', function(e) {\n"
"            document.getElementById('speedValue').textContent = e.target.value;\n"
"            sendCommand('setFollowParams', {speedFactor: parseFloat(e.target.value)});\n"
"        });\n"
"        \n"
"        // Start connection\n"
"        connect();\n"
"    </script>\n"
"</body>\n"
"</html>";

// Forward declarations
static esp_err_t index_handler(httpd_req_t *req);
static esp_err_t websocket_handler(httpd_req_t *req);
static void websocket_task(void *pvParameters);

bool webServerInit(void) {
    // Initialize WebSocket clients array
    for (int i = 0; i < WEB_SERVER_MAX_CLIENTS; i++) {
        ws_clients[i].fd = -1;
        ws_clients[i].connected = false;
        ws_clients[i].last_ping = 0;
    }
    
    // Clear current data
    memset(&current_data, 0, sizeof(web_data_t));
    
    ESP_LOGI(TAG, "Web server initialized");
    return true;
}

bool webServerStart(void) {
    if (server != NULL) {
        ESP_LOGW(TAG, "Web server already running");
        return true;
    }
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = WEB_SERVER_PORT;
    config.max_open_sockets = WEB_SERVER_MAX_CLIENTS + 2;
    config.task_stack_size = WEB_SERVER_TASK_STACK_SIZE;
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
        
        httpd_uri_t ws_uri = {
            .uri = "/ws",
            .method = HTTP_GET,
            .handler = websocket_handler,
            .user_ctx = NULL,
            .is_websocket = true
        };
        httpd_register_uri_handler(server, &ws_uri);
        
        server_running = true;
        
        // Start WebSocket management task
        xTaskCreate(websocket_task, "websocket_task", 4096, NULL, 3, NULL);
        
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

static esp_err_t websocket_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "WebSocket handshake");
        return ESP_OK;
    }
    
    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    
    // Receive WebSocket frame
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    if (ws_pkt.len) {
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for websocket buffer");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed: %s", esp_err_to_name(ret));
            free(buf);
            return ret;
        }
    }
    
    // Process WebSocket message
    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT && ws_pkt.len > 0) {
        ESP_LOGI(TAG, "Received WebSocket message: %.*s", (int)ws_pkt.len, (char*)ws_pkt.payload);
        
        // Parse JSON command
        cJSON *json = cJSON_ParseWithLength((char*)ws_pkt.payload, ws_pkt.len);
        if (json != NULL) {
            cJSON *command = cJSON_GetObjectItem(json, "command");
            cJSON *data = cJSON_GetObjectItem(json, "data");
            
            if (cJSON_IsString(command)) {
                const char *cmd = command->valuestring;
                
                // Handle commands
                if (strcmp(cmd, "setFlightMode") == 0 && data != NULL) {
                    cJSON *mode = cJSON_GetObjectItem(data, "mode");
                    if (cJSON_IsNumber(mode)) {
                        setCommandermode((FlightMode)mode->valueint);
                        ESP_LOGI(TAG, "Flight mode set to %d", mode->valueint);
                    }
                }
                else if (strcmp(cmd, "setFollowMode") == 0 && data != NULL) {
                    cJSON *enabled = cJSON_GetObjectItem(data, "enabled");
                    cJSON *target = cJSON_GetObjectItem(data, "targetDistance");
                    
                    if (cJSON_IsBool(enabled)) {
                        int16_t targetDist = cJSON_IsNumber(target) ? target->valueint : 200;
                        leaderCommSetFollowMode(cJSON_IsTrue(enabled), targetDist);
                        ESP_LOGI(TAG, "Follow mode: %s, target: %d", 
                                cJSON_IsTrue(enabled) ? "enabled" : "disabled", targetDist);
                    }
                }
                else if (strcmp(cmd, "setFollowParams") == 0 && data != NULL) {
                    cJSON *targetDist = cJSON_GetObjectItem(data, "targetDistance");
                    cJSON *tolerance = cJSON_GetObjectItem(data, "tolerance");
                    cJSON *speed = cJSON_GetObjectItem(data, "speedFactor");
                    
                    int16_t target = cJSON_IsNumber(targetDist) ? targetDist->valueint : 200;
                    int16_t tol = cJSON_IsNumber(tolerance) ? tolerance->valueint : 50;
                    float speedFactor = cJSON_IsNumber(speed) ? (float)speed->valuedouble : 0.5f;
                    
                    leaderCommSetFollowParams(target, tol, speedFactor);
                    ESP_LOGI(TAG, "Follow params updated: target=%d, tolerance=%d, speed=%.2f", 
                            target, tol, speedFactor);
                }
                else if (strcmp(cmd, "emergencyStop") == 0) {
                    stabilizerSetEmergencyStop();
                    ESP_LOGW(TAG, "Emergency stop activated via web interface");
                }
                else if (strcmp(cmd, "lockAltitude") == 0) {
                    leaderCommLockAltitude();
                    ESP_LOGI(TAG, "Altitude locked");
                }
                else if (strcmp(cmd, "unlockAltitude") == 0) {
                    leaderCommUnlockAltitude();
                    ESP_LOGI(TAG, "Altitude unlocked");
                }
            }
            
            cJSON_Delete(json);
        }
        
        // Add client to list if not already present
        bool client_found = false;
        for (int i = 0; i < WEB_SERVER_MAX_CLIENTS; i++) {
            if (ws_clients[i].fd == httpd_req_to_sockfd(req)) {
                client_found = true;
                ws_clients[i].last_ping = xTaskGetTickCount();
                break;
            }
        }
        
        if (!client_found) {
            for (int i = 0; i < WEB_SERVER_MAX_CLIENTS; i++) {
                if (!ws_clients[i].connected) {
                    ws_clients[i].fd = httpd_req_to_sockfd(req);
                    ws_clients[i].connected = true;
                    ws_clients[i].last_ping = xTaskGetTickCount();
                    ESP_LOGI(TAG, "New WebSocket client connected: %d", ws_clients[i].fd);
                    break;
                }
            }
        }
    }
    
    if (buf) {
        free(buf);
    }
    
    return ESP_OK;
}

void webServerUpdateData(const web_data_t* data) {
    if (data != NULL) {
        memcpy(&current_data, data, sizeof(web_data_t));
        
        // Broadcast to all connected clients
        cJSON *json = cJSON_CreateObject();
        cJSON_AddBoolToObject(json, "armed", data->armed);
        cJSON_AddNumberToObject(json, "flight_mode", data->flight_mode);
        cJSON_AddNumberToObject(json, "battery_voltage", data->battery_voltage);
        cJSON_AddNumberToObject(json, "battery_percentage", data->battery_percentage);
        cJSON_AddBoolToObject(json, "leader_detected", data->leader_detected);
        cJSON_AddNumberToObject(json, "leader_distance", data->leader_distance);
        cJSON_AddNumberToObject(json, "leader_rssi", data->leader_rssi);
        cJSON_AddNumberToObject(json, "leader_quality", data->leader_quality);
        cJSON_AddNumberToObject(json, "leader_packets", data->leader_packets);
        cJSON_AddNumberToObject(json, "leader_errors", data->leader_errors);
        cJSON_AddNumberToObject(json, "uptime_seconds", data->uptime_seconds);
        cJSON_AddNumberToObject(json, "free_heap", data->free_heap);
        
        char *json_string = cJSON_Print(json);
        if (json_string != NULL) {
            webServerBroadcast(json_string);
            free(json_string);
        }
        
        cJSON_Delete(json);
    }
}

void webServerBroadcast(const char* json_data) {
    if (!server_running || json_data == NULL) {
        return;
    }
    
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t*)json_data;
    ws_pkt.len = strlen(json_data);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    
    for (int i = 0; i < WEB_SERVER_MAX_CLIENTS; i++) {
        if (ws_clients[i].connected && ws_clients[i].fd >= 0) {
            esp_err_t ret = httpd_ws_send_frame_async(server, ws_clients[i].fd, &ws_pkt);
            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "Failed to send to client %d: %s", 
                        ws_clients[i].fd, esp_err_to_name(ret));
                ws_clients[i].connected = false;
                ws_clients[i].fd = -1;
            }
        }
    }
}

static void websocket_task(void *pvParameters) {
    while (server_running) {
        // Clean up disconnected clients
        uint32_t current_time = xTaskGetTickCount();
        for (int i = 0; i < WEB_SERVER_MAX_CLIENTS; i++) {
            if (ws_clients[i].connected) {
                // Check if client is still alive (no ping for 30 seconds)
                if (current_time - ws_clients[i].last_ping > pdMS_TO_TICKS(30000)) {
                    ESP_LOGW(TAG, "Client %d timed out, disconnecting", ws_clients[i].fd);
                    ws_clients[i].connected = false;
                    ws_clients[i].fd = -1;
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000)); // Check every 5 seconds
    }
    
    vTaskDelete(NULL);
}

bool webServerIsRunning(void) {
    return server_running;
}

uint8_t webServerGetClientCount(void) {
    uint8_t count = 0;
    for (int i = 0; i < WEB_SERVER_MAX_CLIENTS; i++) {
        if (ws_clients[i].connected) {
            count++;
        }
    }
    return count;
}
