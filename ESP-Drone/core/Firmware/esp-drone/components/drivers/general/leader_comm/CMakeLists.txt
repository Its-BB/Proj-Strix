# CMakeLists.txt for leader_comm component

idf_component_register(
    SRCS "leader_comm_simple.c"
    INCLUDE_DIRS "."
    REQUIRES esp_wifi freertos log
)
