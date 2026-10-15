#pragma once

#include <cstdint>

// Hardware-independent single RGB LED.
class IRgbLed {
public:
    virtual ~IRgbLed() = default;

    // 0..255 per channel. Returns false if the LED could not be updated.
    virtual bool setColor(uint8_t red, uint8_t green, uint8_t blue) = 0;

    virtual bool off() = 0;
};