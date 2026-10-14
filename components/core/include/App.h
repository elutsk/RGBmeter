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
    bool memoryActive() const { return hasMemory_; }

private:
    void handleButtons();
    void renderRgb();
    void renderClimate();

    IColorSensor& colorSensor_;
    IClimateSensor& climateSensor_;
    IDisplay& display_;
    IButton& modeButton_;    // button 1: short = next screen, long = power (reserved)
    IButton& memoryButton_;  // button 2 on the RGB screen: short = save, long = clear memory
    Mode mode_ = Mode::Rgb;  // default mode after power-up
    bool powerRequested_ = false;  // set by a long click on button 1; not used yet

    // The sample shown on the RGB screen by the previous tick: this is
    // what the user saw when pressing button 2, so this is what gets saved.
    ColorSample lastShown_ = {};
    bool hasLastShown_ = false;

    // Saved reference (memory). While active, the RGB screen shows differences.
    ColorSample memory_ = {};
    bool hasMemory_ = false;
};