#pragma once

#include "IRgbLed.h"
#include "driver/gpio.h"
#include "esp_timer.h"

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

    // Allocates an RMT channel for the LED and the pulse timer. Returns false on failure.
    bool init();

    bool setColor(uint8_t red, uint8_t green, uint8_t blue) override;
    bool off() override;
    bool pulse(uint8_t red, uint8_t green, uint8_t blue, uint32_t durationMs) override;

    void ledTest(uint8_t level, uint32_t delay);

    // Before deep sleep: turns the LED off, releases the RMT channel and holds the
    // pin low. The board also has a plain red LED (1k to GND) on this pin, which
    // would glow from the pin's default state in deep sleep. After this the object
    // is not usable until init(), which in our case runs after the wake-up reboot.
    void sleep();

private:
    // One-shot esp_timer callback that ends a pulse (trampoline, as in GpioButton).
    static void onPulseEnd(void* arg);

    gpio_num_t pin_;
    esp_timer_handle_t pulseTimer_ = nullptr;
    led_strip_t* handle_ = nullptr;  // same type as the library's led_strip_handle_t
};