#include <cstdio>

#include "I2cBus.h"
#include "Ssd1306Display.h"
#include "Veml6040.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "rgbmeter";

namespace {

constexpr gpio_num_t kSdaPin = GPIO_NUM_12;
constexpr gpio_num_t kSclPin = GPIO_NUM_13;
constexpr int kReadPeriodMs = 200;  // longer than the 160 ms integration time

// Layout for 128x64 with the 5x7 font at scale 2:
// one character cell is 11x14 px, four rows fit at 16 px pitch.
constexpr int kTextScale = 2;
constexpr int kRowPitch = 16;

void drawChannel(IDisplay& display, int row, char label, uint16_t value) {
    char line[16];
    std::snprintf(line, sizeof(line), "%c %5u", label, static_cast<unsigned>(value));
    display.drawText(0, row * kRowPitch, line, kTextScale);
}

}  // namespace

extern "C" void app_main(void) {
    I2cBus bus(kSdaPin, kSclPin);
    Veml6040 veml(bus);
    Ssd1306Display oled(bus);

    if (!veml.init()) {
        ESP_LOGE(TAG, "VEML6040 init failed");
        return;
    }
    if (!oled.init()) {
        ESP_LOGE(TAG, "SSD1306 init failed");
        return;
    }

    // From here on only the interfaces are used.
    IColorSensor& colorSensor = veml;
    IDisplay& display = oled;

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(kReadPeriodMs));

        display.clear();
        ColorSample sample = {};
        if (colorSensor.read(sample)) {
            drawChannel(display, 1, 'R', sample.r);
            drawChannel(display, 2, 'G', sample.g);
            drawChannel(display, 3, 'B', sample.b);
            drawChannel(display, 0, 'W', sample.w);
        } else {
            display.drawText(0, 0, "Sensor error", kTextScale);
        }
        if (!display.flush()) {
            ESP_LOGW(TAG, "display flush failed");
        }
    }
}