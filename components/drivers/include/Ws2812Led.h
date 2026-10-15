#pragma once

#include "IRgbLed.h"
#include "driver/gpio.h"

// Forward declaration of the library's internal struct: keeps "led_strip.h"
// out of this header, like in Ssd1306Display.
struct led_strip_t;

// Single WS2812 RGB LED (on-board LED of the ESP32-S3 Super Mini, GPIO48).
// Adapter: implements IRgbLed on top of the Espressif led_strip component,
// which generates the WS2812 timing with the RMT peripheral.
class Ws2812Led : public IRgbLed {
public:
    explicit Ws2812Led(gpio_num_t pin);
    ~Ws2812Led() override;

    Ws2812Led(const Ws2812Led&) = delete;
    Ws2812Led& operator=(const Ws2812Led&) = delete;

    // Allocates an RMT channel for the LED. Returns false on failure.
    bool init();

    bool setColor(uint8_t red, uint8_t green, uint8_t blue) override;
    bool off() override;

private:
    gpio_num_t pin_;
    led_strip_t* handle_ = nullptr;  // same type as the library's led_strip_handle_t
};