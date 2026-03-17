idf_component_register(SRCS "eeprom.c"
                       INCLUDE_DIRS "." "include"
                       REQUIRES core spi_flash)
