#pragma once

#include <cstdint>

#include "IButton.h"
#include "IClimateSensor.h"
#include "IColorSensor.h"
#include "IDisplay.h"
#include "IRgbLed.h"
#include "IVoltageSensor.h"

// Application logic. Knows nothing about ESP-IDF or concrete hardware:
// it works only through interfaces passed into the constructor.
class App {
public:
    enum class Mode : uint8_t { Rgb, Climate, Status };

    // tickPeriodMs: how often tick() is called; App measures time by counting ticks.
    App(IColorSensor& colorSensor, IClimateSensor& climateSensor, IVoltageSensor& vbus, IDisplay& display, IRgbLed& led, IButton& modeButton, IButton& memoryButton, int tickPeriodMs);

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    // One step of the main loop: handle the buttons, read, render, battery alert.
    void tick();

    Mode mode() const { return mode_; }
    bool powerRequested() const { return powerRequested_; }
    bool memoryActive() const { return hasMemory_; }

private:
    void handleButtons();
    void renderRgb();
    void renderClimate();
    void renderStatus();
    void updateBatteryAlert();

    IColorSensor& colorSensor_;
    IClimateSensor& climateSensor_;
    IVoltageSensor& vbus_;
    IDisplay& display_;
    IRgbLed& led_;
    IButton& modeButton_;    // button 1: short = RGB/Climate, long = power (reserved); ignored on Status
    IButton& memoryButton_;  // button 2: RGB = memory (short save, long clear), Climate/Status = enter/leave Status
    int tickPeriodMs_;
    Mode mode_ = Mode::Rgb;  // default mode after power-up
    bool powerRequested_ = false;  // set by a long click on button 1; not used yet

    // The sample shown on the RGB screen by the previous tick: this is
    // what the user saw when pressing button 2, so this is what gets saved.
    ColorSample lastShown_ = {};
    bool hasLastShown_ = false;

    // Saved reference (memory). While active, the RGB screen shows differences.
    ColorSample memory_ = {};
    bool hasMemory_ = false;

    int alertElapsedMs_ = 0;  // time since the last low battery check
};