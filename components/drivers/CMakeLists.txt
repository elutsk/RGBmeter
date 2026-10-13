idf_component_register(SRCS "I2cBus.cpp" "Veml6040.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_driver_i2c core)

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++14)