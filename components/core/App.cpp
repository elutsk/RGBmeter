#include "App.h"
#include <cstdio>
#include "Power.h"

namespace {

// Layout for 128x64 with the 5x7 font at scale 2:
// one character cell is 11x14 px, four rows fit at 16 px pitch.
// Row 0 (top 16 px) is the yellow zone of the panel.
constexpr int kTextScale = 2;
constexpr int kRowPitch = 16;

void drawChannel(IDisplay& display, int row, char label, uint16_t value) {
    char line[16];
    std::snprintf(line, sizeof(line), "%c %5u", label, static_cast<unsigned>(value));
    display.drawText(0, row * kRowPitch, line, kTextScale);
}

// Memory marker: one 5x7 character at scale 1 in the top right corner.
constexpr int kMarkerX = 128 - 6;
constexpr int kMarkerY = 0;

// Status screen: small font, so longer lines fit (21 characters at scale 1).
constexpr const char* kTitle = "RGBmeter v1";
constexpr int kStatusScale = 2;

// Low battery alert: a short red flash every 10 s while on battery below 20 %.
constexpr int kLowBatteryPercent = 20;
constexpr int kAlertPeriodMs = 10000;
constexpr uint32_t kAlertPulseMs = 100;
constexpr uint8_t kAlertLevel = 64;

// Automatic sleep, both only on battery.
constexpr int kEmptyChecksToSleep = 3;                // battery guard: 3 checks in a row below 0 %, 10 s apart
constexpr int kInactivityTimeoutMs = 10 * 60 * 1000;  // no button events for 10 minutes

// Shows current - saved with a sign: "+11", "-44", and plain "0" when equal.
void drawDelta(IDisplay& display, int row, char label, uint16_t current, uint16_t saved) {
    const int delta = static_cast<int>(current) - static_cast<int>(saved);
    char line[16];
    if (delta == 0) {
        std::snprintf(line, sizeof(line), "%c %6d", label, delta);
    } else {
        std::snprintf(line, sizeof(line), "%c %+6d", label, delta);
    }
    display.drawText(0, row * kRowPitch, line, kTextScale);
}

}  // namespace

App::App(IColorSensor& colorSensor, IClimateSensor& climateSensor, IVoltageSensor& vbus, IDisplay& display, IRgbLed& led, IButton& modeButton, IButton& memoryButton, int tickPeriodMs)
    : colorSensor_(colorSensor), climateSensor_(climateSensor), vbus_(vbus), display_(display), led_(led), modeButton_(modeButton), memoryButton_(memoryButton), tickPeriodMs_(tickPeriodMs) {}

void App::tick() {
    handleButtons();
    display_.clear();
    switch (mode_) {
    case Mode::Rgb:
        renderRgb();
        break;
    case Mode::Climate:
        renderClimate();
        break;
    case Mode::Status:
        renderStatus();
        break;
    }

    // Nothing useful to do on failure here: the next tick will try again.
    (void)display_.flush();

    updateBattery();
    updateInactivity();
}

void App::handleButtons() {
    // Read all events on every tick, even those that are ignored on the
    // current screen: otherwise they stay latched and fire later.
    const bool modeShort = modeButton_.wasShortClicked();
    const bool modeLong = modeButton_.wasLongClicked();
    const bool memoryShort = memoryButton_.wasShortClicked();
    const bool memoryLong = memoryButton_.wasLongClicked();

    // Any button event is activity, even one that is ignored on this screen.
    if (modeShort || modeLong || memoryShort || memoryLong) inactiveMs_ = 0;

    // Button 1 first. Long click: sleep, on any screen.
    if (modeLong) requestSleep(SleepReason::Button);

    // Short click: ignored on the Status screen, reserved for settings.
    if (mode_ != Mode::Status) {
        if (modeShort) {
            mode_ = (mode_ == Mode::Rgb) ? Mode::Climate : Mode::Rgb;
            hasLastShown_ = false;  // it belongs to the previous visit of the RGB screen
        }
    }

    // Then button 2, applied to the screen that is current after button 1.
    switch (mode_) {
    case Mode::Rgb:
        if (memoryShort && hasLastShown_) {
            memory_ = lastShown_;
            hasMemory_ = true;
        }
        if (memoryLong) hasMemory_ = false;
        break;
    case Mode::Climate:
        if (memoryShort) mode_ = Mode::Status;
        break;
    case Mode::Status:
        if (memoryShort) mode_ = Mode::Climate;
        break;
    }
}

void App::renderRgb() {
    if (hasMemory_) display_.drawText(kMarkerX, kMarkerY, "M", 1);

    ColorSample sample = {};
    if (!colorSensor_.read(sample)) {
        hasLastShown_ = false;
        display_.drawText(0, 0, "Read error", kTextScale);
        return;
    }
    lastShown_ = sample;
    hasLastShown_ = true;

    if (hasMemory_) {
        drawDelta(display_, 0, 'W', sample.w, memory_.w);
        drawDelta(display_, 1, 'R', sample.r, memory_.r);
        drawDelta(display_, 2, 'G', sample.g, memory_.g);
        drawDelta(display_, 3, 'B', sample.b, memory_.b);
    } else {
        drawChannel(display_, 0, 'W', sample.w);
        drawChannel(display_, 1, 'R', sample.r);
        drawChannel(display_, 2, 'G', sample.g);
        drawChannel(display_, 3, 'B', sample.b);
    }
}

void App::renderClimate() {
    ClimateSample sample = {};
    if (!climateSensor_.read(sample)) {
        display_.drawText(0, 0, "Read error", kTextScale);
        return;
    }

    // The font has no degree sign, so plain "C" is used.
    char line[16];
    std::snprintf(line, sizeof(line), "T %5.1f C", static_cast<double>(sample.tempC));
    display_.drawText(0, 0 * kRowPitch, line, kTextScale);
    std::snprintf(line, sizeof(line), "H %5.1f %%", static_cast<double>(sample.humidityPct));
    display_.drawText(0, 1 * kRowPitch, line, kTextScale);
    std::snprintf(line, sizeof(line), "hPa %5.0f", static_cast<double>(sample.pressureHpa));
    display_.drawText(0, 2 * kRowPitch, line, kTextScale);
    std::snprintf(line, sizeof(line), "mmHg %4.1f", static_cast<double>(sample.pressureHpa * 0.75f));
    display_.drawText(0, 3 * kRowPitch, line, kTextScale);
}

void App::renderStatus() {
    display_.drawText(0, 0, kTitle, kStatusScale);

    int vbusMillivolts = 0;
    if (!vbus_.readMillivolts(vbusMillivolts)) {
        display_.drawText(0, kRowPitch, "vBus error", kStatusScale);
        return;
    }

    const PowerStatus power = evaluatePower(vbusMillivolts);
    char line[24];
    std::snprintf(line, sizeof(line), power.onUsb ? "vBus %.2f V" : "vBat %.2f V", power.millivolts / 1000.0);
    display_.drawText(0, kRowPitch, line, kStatusScale);
}

void App::updateBattery() {
    alertElapsedMs_ += tickPeriodMs_;
    if (alertElapsedMs_ < kAlertPeriodMs) return;
    alertElapsedMs_ = 0;

    int vbusMillivolts = 0;
    if (!vbus_.readMillivolts(vbusMillivolts)) return;  // a failed read changes nothing

    const PowerStatus power = evaluatePower(vbusMillivolts);
    onBattery_ = !power.onUsb;
    if (onBattery_) {
        if (power.batteryPercent < kLowBatteryPercent) {
            led_.pulse(kAlertLevel, 0, 0, kAlertPulseMs);
        } else {
            led_.pulse(0, kAlertLevel, 0, kAlertPulseMs);
        }
    }

    // Battery guard: only several empty readings in a row put the device to sleep,
    // a single low reading may be noise or a short dip under load.
    emptyChecks_ = power.batteryEmpty ? emptyChecks_ + 1 : 0;
    if (emptyChecks_ >= kEmptyChecksToSleep) requestSleep(SleepReason::LowBattery);
}

void App::updateInactivity() {
    // Counts only on battery. On USB it stays at 0, so the 10 minutes start
    // from the moment the device switches to the battery.
    if (!onBattery_) {
        inactiveMs_ = 0;
        return;
    }
    inactiveMs_ += tickPeriodMs_;
    if (inactiveMs_ >= kInactivityTimeoutMs) requestSleep(SleepReason::Inactivity);
}

void App::requestSleep(SleepReason reason) {
    if (sleepRequest_ == SleepReason::None) sleepRequest_ = reason;  // the first reason wins
}

const char* App::sleepReasonName(SleepReason reason) {
    switch (reason) {
    case SleepReason::None:
        return "none";
    case SleepReason::Button:
        return "button";
    case SleepReason::LowBattery:
        return "low battery";
    case SleepReason::Inactivity:
        return "inactivity";
    }
    return "unknown";  // not reached: the switch covers all values
}