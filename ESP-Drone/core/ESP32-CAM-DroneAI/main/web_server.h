#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "esp_err.h"

/**
 * @brief Initialize and start the web server
 * @return ESP_OK on success
 */
esp_err_t web_server_start(void);

/**
 * @brief Stop the web server
 * @return ESP_OK on success
 */
esp_err_t web_server_stop(void);

#endif // WEB_SERVER_H