/**
 * ESP-Drone Web Server Component
 * 
 * Provides a professional web interface running directly on the ESP32 drone
 * for monitoring and controlling the leader following functionality.
 */

#ifndef __WEB_SERVER_H__
#define __WEB_SERVER_H__

#include <stdbool.h>
#include <stdint.h>
#include "esp_http_server.h"

// Configuration
#define WEB_SERVER_PORT 80
#define WEB_SERVER_MAX_CLIENTS 4
#define WEB_SERVER_TASK_STACK_SIZE 8192
#define WEB_SERVER_TASK_PRIORITY 3

// WebSocket frame types
#define WS_FRAME_TEXT 0x01
#define WS_FRAME_BINARY 0x02
#define WS_FRAME_CLOSE 0x08
#define WS_FRAME_PING 0x09
#define WS_FRAME_PONG 0x0A

// WebSocket client structure
typedef struct {
    int fd;
    bool connected;
    uint32_t last_ping;
} ws_client_t;

// Web server data structure for real-time updates
typedef struct {
    // Drone status
    bool armed;
    uint8_t flight_mode;
    float battery_voltage;
    uint8_t battery_percentage;
    
    // Leader data
    bool leader_detected;
    int16_t leader_distance;
    int8_t leader_rssi;
    float leader_quality;
    uint32_t leader_packets;
    uint32_t leader_errors;
    
    // System data
    uint32_t uptime_seconds;
    uint32_t free_heap;
    float cpu_usage;
    
    // Flight data
    float altitude_meters;
    float altitude_asl;  // Above sea level
} web_data_t;

/**
 * Initialize the web server
 * 
 * @return true if initialization successful, false otherwise
 */
bool webServerInit(void);

/**
 * Start the web server
 * 
 * @return true if server started successfully, false otherwise
 */
bool webServerStart(void);

/**
 * Stop the web server
 */
void webServerStop(void);

/**
 * Update data for web clients
 * 
 * @param data pointer to web data structure
 */
void webServerUpdateData(const web_data_t* data);

/**
 * Send real-time data to all connected WebSocket clients
 * 
 * @param json_data JSON string to send
 */
void webServerBroadcast(const char* json_data);

/**
 * Check if web server is running
 * 
 * @return true if running, false otherwise
 */
bool webServerIsRunning(void);

/**
 * Get number of connected clients
 * 
 * @return number of connected WebSocket clients
 */
uint8_t webServerGetClientCount(void);

#endif /* __WEB_SERVER_H__ */
