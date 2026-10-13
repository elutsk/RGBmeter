#include "App.h"
#include "GpioButton.h"
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
constexpr gpio_num_t kButtonPin = GPIO_NUM_11;
constexpr int kTickPeriodMs = 200;  // longer than the 160 ms VEML6040 integration time

}  // namespace

// Composition root: creates the concrete hardware objects,
// wires them into App through interfaces, and runs the main loop.
extern "C" void app_main(void)
{
    I2cBus bus(kSdaPin, kSclPin);
    Veml6040 veml(bus);
    Ssd1306Display oled(bus);
    GpioButton button(kButtonPin);

    if (!veml.init()) {
        ESP_LOGE(TAG, "VEML6040 init failed");
        return;
    }
    if (!oled.init()) {
        ESP_LOGE(TAG, "SSD1306 init failed");
        return;
    }

    App app(veml, oled, button);

    while (true) {
        app.tick();
        vTaskDelay(pdMS_TO_TICKS(kTickPeriodMs));
    }
}