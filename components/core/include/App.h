#pragma once

#include <cstdint>

#include "IButton.h"
#include "IColorSensor.h"
#include "IDisplay.h"
#include "IClimateSensor.h"

// Application logic. Knows nothing about ESP-IDF or concrete hardware:
// it works only through interfaces passed into the constructor.
class App {
public:
    enum class Mode : uint8_t { Rgb, Climate };

    App(IColorSensor& colorSensor, IClimateSensor& climateSensor, IDisplay& display, IButton& button);

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    // One step of the main loop: handle the button, read, render.
    void tick();

    Mode mode() const { return mode_; }

private:
    void renderRgb();
    void renderClimate();

    IColorSensor& colorSensor_;
    IClimateSensor& climateSensor_;   // new
    IDisplay& display_;
    IButton& button_;
    Mode mode_ = Mode::Rgb;  // default mode after power-up
};