#include "App.h"

#include <cstdio>

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

App::App(IColorSensor& colorSensor, IClimateSensor& climateSensor, IDisplay& display, IButton& modeButton, IButton& memoryButton)
    : colorSensor_(colorSensor), climateSensor_(climateSensor), display_(display), modeButton_(modeButton), memoryButton_(memoryButton) {}

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
    }

    // Nothing useful to do on failure here: the next tick will try again.
    (void)display_.flush();
}

void App::handleButtons() {
    if (modeButton_.wasShortClicked()) {
        mode_ = (mode_ == Mode::Rgb) ? Mode::Climate : Mode::Rgb;
        hasLastShown_ = false;  // it belongs to the previous visit of the RGB screen
    }
    if (modeButton_.wasLongClicked()) powerRequested_ = true;  // reserved for power saving

    // Read both memory events on every tick, so they never stay latched,
    // but act on them only on the RGB screen.
    const bool save = memoryButton_.wasShortClicked();
    const bool clear = memoryButton_.wasLongClicked();
    if (mode_ != Mode::Rgb) return;

    if (save && hasLastShown_) {
        memory_ = lastShown_;
        hasMemory_ = true;
    }
    if (clear) hasMemory_ = false;
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