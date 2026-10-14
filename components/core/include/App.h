#pragma once

#include <cstdint>

#include "IButton.h"
#include "IClimateSensor.h"
#include "IColorSensor.h"
#include "IDisplay.h"

// Application logic. Knows nothing about ESP-IDF or concrete hardware:
// it works only through interfaces passed into the constructor.
class App {
public:
    enum class Mode : uint8_t { Rgb, Climate };

    App(IColorSensor& colorSensor, IClimateSensor& climateSensor, IDisplay& display, IButton& modeButton, IButton& memoryButton);

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    // One step of the main loop: handle the buttons, read, render.
    void tick();

    Mode mode() const { return mode_; }
    bool powerRequested() const { return powerRequested_; }

private:
    void handleButtons();
    void renderRgb();
    void renderClimate();

    IColorSensor& colorSensor_;
    IClimateSensor& climateSensor_;
    IDisplay& display_;
    IButton& modeButton_;    // button 1: short = next screen, long = power (reserved)
    IButton& memoryButton_;  // button 2: memory on the RGB screen (next step)
    Mode mode_ = Mode::Rgb;  // default mode after power-up
    bool powerRequested_ = false;  // set by a long click on button 1; not used yet
};