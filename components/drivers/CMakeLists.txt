idf_component_register(SRCS "I2cBus.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_driver_i2c)

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++14)