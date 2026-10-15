idf_component_register(SRCS "I2cBus.cpp" "Veml6040.cpp" "Ssd1306Display.cpp" "GpioButton.cpp" "Bme688.cpp" "Ws2812Led.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES esp_driver_i2c esp_driver_gpio esp_timer bme68x core)

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++14)

# esp_driver_gpio is public: GpioButton.h includes driver/gpio.h.
# It also covers ssd1306.h (chill-sam/ssd1306 1.1.2), which includes
# driver/gpio.h but declares that dependency as private.