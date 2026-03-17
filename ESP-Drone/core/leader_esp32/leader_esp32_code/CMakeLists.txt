# This is the project CMakeLists.txt file for the ESP32 Leader Drone Broadcaster
# It is assumed to be used in an ESP-IDF project

cmake_minimum_required(VERSION 3.16)

project(esp32_leader_broadcaster)

set(SOURCES "main.c")

idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS "."
                       REQUIRES esp_wifi esp_now nvs_flash esp_event esp_netif)
