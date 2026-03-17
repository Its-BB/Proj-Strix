# Conditionally select VL53L0X driver based on configuration
if(CONFIG_SENSORS_ENABLE_RANGE_VL53L0X_ENHANCED)
    set(VL53L0X_SRCS "vl53l0x_enhanced.c")
    message(STATUS "Using enhanced VL53L0X driver")
else()
    set(VL53L0X_SRCS "vl53l0x_simple.c")
    message(STATUS "Using simple VL53L0X driver")
endif()

idf_component_register(SRCS ${VL53L0X_SRCS} "zranger.c"
                       INCLUDE_DIRS "." "include"
                       REQUIRES i2c_bus core platform config)
