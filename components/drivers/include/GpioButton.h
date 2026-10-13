#pragma once

#include <atomic>
#include <cstdint>

#include "IButton.h"
#include "driver/gpio.h"
#include "esp_timer.h"

// Push button between a GPIO and GND, internal pull-up (active low).
// A periodic esp_timer samples the pin and debounces it; a confirmed
// press is latched in an atomic flag that wasPressed() reads and clears.
class GpioButton : public IButton {
public:
    explicit GpioButton(gpio_num_t pin);
    ~GpioButton() override;

    GpioButton(const GpioButton&) = delete;
    GpioButton& operator=(const GpioButton&) = delete;

    bool wasPressed() override;

private:
    // esp_timer is a C API: it can call only a plain function with a void*.
    // This static function receives 'this' as void* and forwards to poll().
    static void onTimer(void* arg);
    void poll();

    gpio_num_t pin_;
    esp_timer_handle_t timer_ = nullptr;

    // Used only from the timer callback.
    bool lastRaw_ = false;     // last sampled level (true = pressed)
    bool stableDown_ = false;  // debounced state
    uint8_t stableCount_ = 0;  // how many samples lastRaw_ has been unchanged

    // Shared between the timer task (writer) and the main task (reader).
    std::atomic<bool> pressed_{false};
};