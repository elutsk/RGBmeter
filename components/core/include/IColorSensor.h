#pragma once

#include <cstdint>

// Raw 16-bit counts from an RGBW color sensor
struct ColorSample {
    uint16_t r;
    uint16_t g;
    uint16_t b;
    uint16_t w;
};

// Hardware independent color sensor interface
class IColorSensor {
public:
    virtual ~IColorSensor() = default;

    // on success fill out and return true
    // on failure returns false and leaves 'out' unchanged
    virtual bool read(ColorSample& out) = 0;
};