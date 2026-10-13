#include "App.h"

#include <cstdio>

namespace {

// Layout for 128x64 with the 5x7 font at scale 2:
// one character cell is 11x14 px, four rows fit at 16 px pitch.
// Row 0 (top 16 px) is the yellow zone of the panel.
constexpr int kTextScale = 2;
constexpr int kRowPitch = 16;

void drawChannel(IDisplay& display, int row, char label, uint16_t value)
{
    char line[16];
    std::snprintf(line, sizeof(line), "%c %5u", label, static_cast<unsigned>(value));
    display.drawText(0, row * kRowPitch, line, kTextScale);
}

}  // namespace

App::App(IColorSensor& colorSensor, IDisplay& display, IButton& button)
    : colorSensor_(colorSensor), display_(display), button_(button)
{
}

void App::tick()
{
    if (button_.wasPressed()) {
        mode_ = (mode_ == Mode::Rgb) ? Mode::Climate : Mode::Rgb;
    }

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

void App::renderRgb()
{
    ColorSample sample = {};
    if (!colorSensor_.read(sample)) {
        display_.drawText(0, 0, "Read error", kTextScale);
        return;
    }
    drawChannel(display_, 0, 'W', sample.w);
    drawChannel(display_, 1, 'R', sample.r);
    drawChannel(display_, 2, 'G', sample.g);
    drawChannel(display_, 3, 'B', sample.b);
}

void App::renderClimate()
{
    // Placeholder until the BME688 driver is added.
    display_.drawText(0, 0, "Climate", kTextScale);
    display_.drawText(0, kRowPitch, "soon", kTextScale);
}