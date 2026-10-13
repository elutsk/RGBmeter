idf_component_register(SRCS "I2cBus.cpp" "Veml6040.cpp" "Ssd1306Display.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_driver_i2c core
                       PRIV_REQUIRES esp_driver_gpio)

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++14)

# PRIV_REQUIRES esp_driver_gpio: ssd1306.h (chill-sam/ssd1306 1.1.2) includes
# driver/gpio.h in its public header, but the library declares esp_driver_gpio
# as a private requirement, so the include path does not reach us.