#include "App.h"
#include "Bme688.h"
#include "GpioButton.h"
#include "I2cBus.h"
#include "Ssd1306Display.h"
#include "Veml6040.h"
#include "Ws2812Led.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "rgbmeter";

namespace {

constexpr gpio_num_t kSdaPin = GPIO_NUM_12;
constexpr gpio_num_t kSclPin = GPIO_NUM_13;
constexpr gpio_num_t kModeButtonPin = GPIO_NUM_11;    // button 1
constexpr gpio_num_t kMemoryButtonPin = GPIO_NUM_10;  // button 2
constexpr gpio_num_t kLedPin = GPIO_NUM_48;           // on-board WS2812
constexpr int kTickPeriodMs = 200;                    // longer than the 160 ms VEML6040 integration time

}  // namespace

// Composition root: creates the concrete hardware objects,
// wires them into App through interfaces, and runs the main loop.
extern "C" void app_main(void) {
    Ws2812Led led(kLedPin);
    if (!led.init()) ESP_LOGE(TAG, "LED init failed");

    // Temporary LED test: red, green, blue for 1 s each, then off.
    led.setColor(64, 0, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
    led.setColor(0, 64, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
    led.setColor(0, 0, 64);
    vTaskDelay(pdMS_TO_TICKS(1000));
    led.off();

    I2cBus bus(kSdaPin, kSclPin);
    Veml6040 veml(bus);
    Ssd1306Display oled(bus);
    Bme688 bme(bus);
    GpioButton modeButton(kModeButtonPin);
    GpioButton memoryButton(kMemoryButtonPin);

    if (!veml.init()) {
        ESP_LOGE(TAG, "VEML6040 init failed");
        return;
    }
    if (!oled.init()) {
        ESP_LOGE(TAG, "SSD1306 init failed");
        return;
    }
    if (!bme.init()) {
        ESP_LOGE(TAG, "BME688 init failed");
        return;
    }

    App app(veml, bme, oled, modeButton, memoryButton);

    while (true) {
        app.tick();
        vTaskDelay(pdMS_TO_TICKS(kTickPeriodMs));
    }
}