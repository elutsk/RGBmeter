#pragma once

#include <cstdint>

// Hardware-independent single RGB LED.
class IRgbLed {
public:
    virtual ~IRgbLed() = default;

    // 0..255 per channel. Returns false if the LED could not be updated.
    virtual bool setColor(uint8_t red, uint8_t green, uint8_t blue) = 0;

    virtual bool off() = 0;

    // Lights the LED and returns at once; the LED turns itself off after
    // durationMs. A new pulse restarts the time.
    virtual bool pulse(uint8_t red, uint8_t green, uint8_t blue, uint32_t durationMs) = 0;
};