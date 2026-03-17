/**
 * ESP-Drone Web Position Hold Control System Implementation
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "cJSON.h"

#include "web_position_hold.h"
#include "stabilizer.h"  // For thrust control
#include "commander.h"   // For setpoint control
#include "stm32_legacy.h"

#define DEBUG_MODULE "WEB_PH"
#include "debug_cf.h"

// Global variables (non-static so they can be accessed from other files)
httpd_handle_t web_server = NULL;
VL53L0X_Dev_t *vl53l0x_device = NULL;
position_hold_status_t ph_status = {0};
bool system_initialized = false;
TaskHandle_t control_task_handle = NULL;

// HTML page content (embedded) - make non-static for external access
const char* html_page =
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"    <meta charset='UTF-8'>\n"
"    <meta name='viewport' content='width=device-width, initial-scale=1.0'>\n"
"    <title>ESP-Drone Position Hold</title>\n"
"    <style>\n"
"        body {\n"
"            font-family: Arial, sans-serif;\n"
"            margin: 0;\n"
"            padding: 20px;\n"
"            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);\n"
"            color: white;\n"
"            min-height: 100vh;\n"
"        }\n"
"        .container {\n"
"            max-width: 500px;\n"
"            margin: 0 auto;\n"
"            background: rgba(255,255,255,0.1);\n"
"            padding: 30px;\n"
"            border-radius: 15px;\n"
"            backdrop-filter: blur(10px);\n"
"            box-shadow: 0 8px 32px rgba(0,0,0,0.3);\n"
"        }\n"
"        h1 {\n"
"            text-align: center;\n"
"            margin-bottom: 30px;\n"
"            font-size: 28px;\n"
"            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);\n"
"        }\n"
"        .status-panel {\n"
"            background: rgba(0,0,0,0.2);\n"
"            padding: 20px;\n"
"            border-radius: 10px;\n"
"            margin-bottom: 20px;\n"
"        }\n"
"        .status-item {\n"
"            display: flex;\n"
"            justify-content: space-between;\n"
"            margin-bottom: 10px;\n"
"            font-size: 16px;\n"
"        }\n"
"        .status-value {\n"
"            font-weight: bold;\n"
"            color: #4CAF50;\n"
"        }\n"
"        .control-panel {\n"
"            margin-top: 20px;\n"
"        }\n"
"        .input-group {\n"
"            margin-bottom: 15px;\n"
"        }\n"
"        label {\n"
"            display: block;\n"
"            margin-bottom: 5px;\n"
"            font-weight: bold;\n"
"        }\n"
"        input[type='number'] {\n"
"            width: 100%;\n"
"            padding: 10px;\n"
"            border: none;\n"
"            border-radius: 5px;\n"
"            font-size: 16px;\n"
"            background: rgba(255,255,255,0.9);\n"
"            box-sizing: border-box;\n"
"        }\n"
"        .button {\n"
"            width: 100%;\n"
"            padding: 12px;\n"
"            margin: 5px 0;\n"
"            border: none;\n"
"            border-radius: 8px;\n"
"            font-size: 16px;\n"
"            font-weight: bold;\n"
"            cursor: pointer;\n"
"            transition: all 0.3s ease;\n"
"        }\n"
"        .btn-start {\n"
"            background: #4CAF50;\n"
"            color: white;\n"
"        }\n"
"        .btn-stop {\n"
"            background: #f44336;\n"
"            color: white;\n"
"        }\n"
"        .btn-land {\n"
"            background: #FF9800;\n"
"            color: white;\n"
"        }\n"
"        .button:hover {\n"
"            transform: translateY(-2px);\n"
"            box-shadow: 0 4px 8px rgba(0,0,0,0.2);\n"
"        }\n"
"        .button:active {\n"
"            transform: translateY(0);\n"
"        }\n"
"        .alert {\n"
"            padding: 10px;\n"
"            margin: 10px 0;\n"
"            border-radius: 5px;\n"
"            display: none;\n"
"        }\n"
"        .alert-success {\n"
"            background: #d4edda;\n"
"            color: #155724;\n"
"            border: 1px solid #c3e6cb;\n"
"        }\n"
"        .alert-error {\n"
"            background: #f8d7da;\n"
"            color: #721c24;\n"
"            border: 1px solid #f1b2b7;\n"
"        }\n"
"        .emergency {\n"
"            background: #dc3545 !important;\n"
"            animation: pulse 1s infinite;\n"
"        }\n"
"        @keyframes pulse {\n"
"            0% { opacity: 1; }\n"
"            50% { opacity: 0.7; }\n"
"            100% { opacity: 1; }\n"
"        }\n"
"    </style>\n"
"</head>\n"
"<body>\n"
"    <div class='container'>\n"
"        <h1>🚁 ESP-Drone Position Hold</h1>\n"
"        \n"
"        <div class='status-panel'>\n"
"            <div class='status-item'>\n"
"                <span>Status:</span>\n"
"                <span class='status-value' id='status'>Loading...</span>\n"
"            </div>\n"
"            <div class='status-item'>\n"
"                <span>Current Height:</span>\n"
"                <span class='status-value' id='current-height'>-- cm</span>\n"
"            </div>\n"
"            <div class='status-item'>\n"
"                <span>Target Height:</span>\n"
"                <span class='status-value' id='target-height'>-- cm</span>\n"
"            </div>\n"
"            <div class='status-item'>\n"
"                <span>Sensor Status:</span>\n"
"                <span class='status-value' id='sensor-status'>--</span>\n"
"            </div>\n"
"        </div>\n"
"        \n"
"        <div class='control-panel'>\n"
"            <div class='input-group'>\n"
"                <label for='height-input'>Target Height (cm):</label>\n"
"                <input type='number' id='height-input' min='5' max='200' value='20' step='1'>\n"
"            </div>\n"
"            \n"
"            <button class='button btn-start' onclick='startPositionHold()'>Start Position Hold</button>\n"
"            <button class='button btn-stop' onclick='stopPositionHold()'>Stop</button>\n"
"            <button class='button btn-land' onclick='landDrone()'>Land Safely</button>\n"
"            <button class='button btn-stop emergency' onclick='emergencyStop()' style='display:none;' id='emergency-btn'>EMERGENCY STOP</button>\n"
"        </div>\n"
"        \n"
"        <div id='alert' class='alert'></div>\n"
"    </div>\n"
"    \n"
"    <script>\n"
"        let statusInterval;\n"
"        \n"
"        function updateStatus() {\n"
"            fetch('/api/status')\n"
"                .then(response => response.json())\n"
"                .then(data => {\n"
"                    document.getElementById('status').textContent = getStateName(data.state);\n"
"                    document.getElementById('current-height').textContent = data.current_height.toFixed(1) + ' cm';\n"
"                    document.getElementById('target-height').textContent = data.target_height.toFixed(1) + ' cm';\n"
"                    document.getElementById('sensor-status').textContent = data.sensor_valid ? 'OK' : 'ERROR';\n"
"                    \n"
"                    // Show emergency button if active\n"
"                    const emergencyBtn = document.getElementById('emergency-btn');\n"
"                    if (data.state === 1 || data.state === 2 || data.state === 3) {\n"
"                        emergencyBtn.style.display = 'block';\n"
"                    } else {\n"
"                        emergencyBtn.style.display = 'none';\n"
"                    }\n"
"                    \n"
"                    // Update target height input if not focused\n"
"                    const heightInput = document.getElementById('height-input');\n"
"                    if (document.activeElement !== heightInput) {\n"
"                        heightInput.value = data.target_height.toFixed(0);\n"
"                    }\n"
"                })\n"
"                .catch(error => {\n"
"                    console.error('Error updating status:', error);\n"
"                    showAlert('Connection error', 'error');\n"
"                });\n"
"        }\n"
"        \n"
"        function getStateName(state) {\n"
"            const states = ['Idle', 'Taking Off', 'Position Hold', 'Landing', 'Landed', 'Error'];\n"
"            return states[state] || 'Unknown';\n"
"        }\n"
"        \n"
"        function startPositionHold() {\n"
"            const height = document.getElementById('height-input').value;\n"
"            \n"
"            fetch('/api/set_height', {\n"
"                method: 'POST',\n"
"                headers: {'Content-Type': 'application/json'},\n"
"                body: JSON.stringify({height: parseFloat(height)})\n"
"            })\n"
"            .then(() => {\n"
"                return fetch('/api/start', {method: 'POST'});\n"
"            })\n"
"            .then(response => response.json())\n"
"            .then(data => {\n"
"                if (data.success) {\n"
"                    showAlert('Position hold started!', 'success');\n"
"                } else {\n"
"                    showAlert('Failed to start: ' + data.error, 'error');\n"
"                }\n"
"            })\n"
"            .catch(error => {\n"
"                showAlert('Error: ' + error, 'error');\n"
"            });\n"
"        }\n"
"        \n"
"        function stopPositionHold() {\n"
"            fetch('/api/stop', {method: 'POST'})\n"
"                .then(response => response.json())\n"
"                .then(data => {\n"
"                    showAlert('Position hold stopped', 'success');\n"
"                })\n"
"                .catch(error => {\n"
"                    showAlert('Error: ' + error, 'error');\n"
"                });\n"
"        }\n"
"        \n"
"        function landDrone() {\n"
"            fetch('/api/land', {method: 'POST'})\n"
"                .then(response => response.json())\n"
"                .then(data => {\n"
"                    if (data.success) {\n"
"                        showAlert('Landing initiated', 'success');\n"
"                    } else {\n"
"                        showAlert('Landing failed: ' + data.error, 'error');\n"
"                    }\n"
"                })\n"
"                .catch(error => {\n"
"                    showAlert('Error: ' + error, 'error');\n"
"                });\n"
"        }\n"
"        \n"
"        function emergencyStop() {\n"
"            if (confirm('EMERGENCY STOP - This will immediately cut thrust! Continue?')) {\n"
"                fetch('/api/emergency', {method: 'POST'})\n"
"                    .then(() => {\n"
"                        showAlert('EMERGENCY STOP ACTIVATED', 'error');\n"
"                    })\n"
"                    .catch(error => {\n"
"                        showAlert('Emergency stop error: ' + error, 'error');\n"
"                    });\n"
"            }\n"
"        }\n"
"        \n"
"        function showAlert(message, type) {\n"
"            const alert = document.getElementById('alert');\n"
"            alert.textContent = message;\n"
"            alert.className = 'alert alert-' + type;\n"
"            alert.style.display = 'block';\n"
"            \n"
"            setTimeout(() => {\n"
"                alert.style.display = 'none';\n"
"            }, 3000);\n"
"        }\n"
"        \n"
"        // Start status updates\n"
"        updateStatus();\n"
"        statusInterval = setInterval(updateStatus, 500);\n"
"    </script>\n"
"</body>\n"
"</html>";

// Forward declarations
void position_hold_control_task(void *pvParameters);
static uint32_t get_time_ms(void);
static void send_thrust_command(float thrust_value);

// Utility function to get current time in milliseconds
static uint32_t get_time_ms(void) {
    return pdTICKS_TO_MS(xTaskGetTickCount());
}

// Function to send thrust command to flight controller
static void send_thrust_command(float thrust_value) {
    // Clamp thrust to safe limits
    if (thrust_value > THRUST_MAX) thrust_value = THRUST_MAX;
    if (thrust_value < 0) thrust_value = 0;
    
    // Send thrust command to stabilizer
    setpoint_t setpoint;
    memset(&setpoint, 0, sizeof(setpoint));
    
    setpoint.thrust = (uint16_t)thrust_value;
    setpoint.mode.z = modeDisable; // Disable altitude control, we're managing thrust directly
    setpoint.mode.x = modeDisable;
    setpoint.mode.y = modeDisable;
    setpoint.mode.roll = modeDisable;
    setpoint.mode.pitch = modeDisable; 
    setpoint.mode.yaw = modeDisable;
    
    commanderSetSetpoint(&setpoint, 3); // Priority 3 for position hold
}

// PID controller for thrust calculation
float web_position_hold_calculate_thrust(float height_error_cm, float dt_ms) {
    float dt_sec = dt_ms / 1000.0f;
    
    // Update PID terms
    ph_status.pid_integral += height_error_cm * dt_sec;
    
    // Anti-windup: clamp integral term
    if (ph_status.pid_integral > 100.0f) ph_status.pid_integral = 100.0f;
    if (ph_status.pid_integral < -100.0f) ph_status.pid_integral = -100.0f;
    
    ph_status.pid_derivative = (height_error_cm - ph_status.pid_last_error) / dt_sec;
    ph_status.pid_last_error = height_error_cm;
    
    // PID calculation
    float thrust_adjustment = (THRUST_GAIN_P * height_error_cm) +
                             (THRUST_GAIN_I * ph_status.pid_integral) +
                             (THRUST_GAIN_D * ph_status.pid_derivative);
    
    // Base thrust + adjustment
    float thrust_output = THRUST_BASE + thrust_adjustment;
    
    return thrust_output;
}

// External functions to get data directly from VLX_ENH sensor
extern bool vl53l0xEnhancedTest(void);
extern float vl53l0xEnhancedGetDistance(void);

// Update sensor reading and validate
bool web_position_hold_update_sensor_reading(void) {
    // Get sensor reading directly from VLX_ENH sensor
    if (vl53l0xEnhancedTest()) {
        float distance_cm = vl53l0xEnhancedGetDistance();
        
        // Check if distance is reasonable (between 0.1 and 400 cm) - more lenient for testing
        if (distance_cm >= 0.1f && distance_cm <= 400.0f) {
            ph_status.current_height_cm = distance_cm;
            ph_status.sensor_valid = true;
            ESP_LOGD(DEBUG_MODULE, "Direct VLX_ENH reading: %.1f cm", distance_cm);
            return true;
        } else {
            ESP_LOGW(DEBUG_MODULE, "Invalid VLX_ENH distance reading: %.1f cm", distance_cm);
            ph_status.sensor_valid = false;
            return false;
        }
    } else {
        ph_status.sensor_valid = false;
        ESP_LOGW(DEBUG_MODULE, "VLX_ENH sensor not available or not ready");
        return false;
    }
}

// Safety checks
void web_position_hold_safety_checks(void) {
    // Check height limits
    if (ph_status.current_height_cm < POSITION_HOLD_MIN_HEIGHT_CM) {
        ph_status.low_height_warning = true;
        ESP_LOGW(DEBUG_MODULE, "Warning: Below minimum safe height (%.1f cm)", ph_status.current_height_cm);
    } else {
        ph_status.low_height_warning = false;
    }
    
    if (ph_status.current_height_cm > POSITION_HOLD_MAX_HEIGHT_CM) {
        ph_status.high_height_warning = true;
        ESP_LOGW(DEBUG_MODULE, "Warning: Above maximum safe height (%.1f cm)", ph_status.current_height_cm);
    } else {
        ph_status.high_height_warning = false;
    }
    
    // Auto-land if too low for too long
    if (ph_status.low_height_warning && ph_status.state == PH_STATE_POSITION_HOLD) {
        static uint32_t low_height_start = 0;
        if (low_height_start == 0) {
            low_height_start = get_time_ms();
        } else if (get_time_ms() - low_height_start > 2000) { // 2 seconds
            ESP_LOGW(DEBUG_MODULE, "Auto-landing due to low height");
            web_position_hold_land();
            low_height_start = 0;
        }
    }
}

// Main control task
void position_hold_control_task(void *pvParameters) {
    ESP_LOGI(DEBUG_MODULE, "Position hold control task started");
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(POSITION_HOLD_UPDATE_RATE_MS);
    
    while (true) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        
        if (ph_status.emergency_stop_active) {
            send_thrust_command(0);
            continue;
        }
        
        uint32_t current_time = get_time_ms();
        float dt_ms = current_time - ph_status.last_update_ms;
        ph_status.last_update_ms = current_time;
        
        // Update sensor reading
        if (!web_position_hold_update_sensor_reading()) {
            if (ph_status.state == PH_STATE_TAKEOFF || ph_status.state == PH_STATE_POSITION_HOLD || ph_status.state == PH_STATE_LANDING) {
                ESP_LOGE(DEBUG_MODULE, "Sensor error during active control - emergency stop");
                web_position_hold_emergency_stop();
            }
            continue;
        }
        
        // Perform safety checks
        web_position_hold_safety_checks();
        
        // State machine
        switch (ph_status.state) {
            case PH_STATE_IDLE:
                send_thrust_command(0);
                break;
                
            case PH_STATE_TAKEOFF:
                {
                    // Takeoff logic
                    uint32_t takeoff_elapsed = current_time - ph_status.takeoff_start_time;
                    
                    // Check for takeoff timeout
                    if (takeoff_elapsed > TAKEOFF_TIMEOUT_MS) {
                        ESP_LOGE(DEBUG_MODULE, "Takeoff timeout - emergency stop");
                        web_position_hold_emergency_stop();
                        break;
                    }
                    
                    // Check if we've reached takeoff height
                    float height_above_ground = ph_status.current_height_cm - ph_status.takeoff_initial_height;
                    
                    if (height_above_ground >= TAKEOFF_HEIGHT_THRESHOLD_CM) {
                        // Takeoff successful, switch to position hold
                        ph_status.state = PH_STATE_POSITION_HOLD;
                        ph_status.active_time_ms = current_time;
                        ESP_LOGI(DEBUG_MODULE, "Takeoff complete - switching to position hold");
                        
                        // Reset PID for position hold
                        ph_status.pid_integral = 0;
                        ph_status.pid_derivative = 0;
                        ph_status.pid_last_error = 0;
                    } else {
                        // Continue takeoff with gradually increasing thrust
                        float takeoff_progress = (float)takeoff_elapsed / TAKEOFF_TIMEOUT_MS;
                        float thrust = TAKEOFF_INITIAL_THRUST + (TAKEOFF_MAX_THRUST - TAKEOFF_INITIAL_THRUST) * takeoff_progress;
                        
                        ph_status.thrust_output = thrust;
                        send_thrust_command(thrust);
                        
                        ESP_LOGD(DEBUG_MODULE, "Takeoff: Height=%.1f cm (%.1f above ground), Thrust=%.0f", 
                                ph_status.current_height_cm, height_above_ground, thrust);
                    }
                }
                break;
                
            case PH_STATE_POSITION_HOLD:
                ph_status.height_error_cm = ph_status.target_height_cm - ph_status.current_height_cm;
                ph_status.thrust_output = web_position_hold_calculate_thrust(ph_status.height_error_cm, dt_ms);
                send_thrust_command(ph_status.thrust_output);
                
                ESP_LOGD(DEBUG_MODULE, "PH: Target=%.1f, Current=%.1f, Error=%.1f, Thrust=%.0f", 
                        ph_status.target_height_cm, ph_status.current_height_cm, 
                        ph_status.height_error_cm, ph_status.thrust_output);
                break;
                
            case PH_STATE_LANDING:
                // Gradually reduce target height
                ph_status.target_height_cm -= LANDING_DESCENT_RATE * (dt_ms / 1000.0f);
                
                if (ph_status.target_height_cm <= LANDING_FINAL_HEIGHT_CM || 
                    ph_status.current_height_cm <= LANDING_FINAL_HEIGHT_CM) {
                    ph_status.state = PH_STATE_LANDED;
                    ph_status.active_time_ms = get_time_ms();
                    ESP_LOGI(DEBUG_MODULE, "Landing complete - drone landed");
                } else {
                    ph_status.height_error_cm = ph_status.target_height_cm - ph_status.current_height_cm;
                    ph_status.thrust_output = web_position_hold_calculate_thrust(ph_status.height_error_cm, dt_ms);
                    send_thrust_command(ph_status.thrust_output);
                }
                break;
                
            case PH_STATE_LANDED:
                send_thrust_command(0);
                break;
                
            case PH_STATE_ERROR:
                send_thrust_command(0);
                break;
        }
        
        ph_status.active_time_ms = current_time - ph_status.active_time_ms;
    }
}
