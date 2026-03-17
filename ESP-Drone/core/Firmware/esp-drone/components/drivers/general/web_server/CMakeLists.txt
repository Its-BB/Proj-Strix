# CMakeLists.txt for web_server component

idf_component_register(
    SRCS "web_server_simple.c"
    INCLUDE_DIRS "."
    REQUIRES esp_http_server esp_wifi json freertos
)
