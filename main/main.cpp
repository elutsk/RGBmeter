#include "App.h"
#include "Bme688.h"
#include "GpioButton.h"
#include "I2cBus.h"
#include "Ssd1306Display.h"
#include "Veml6040.h"
#include "Ws2812Led.h"
#include "AdcVoltage.h"
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
constexpr uint8_t kLedLevel = 64;                  // RGB LED brightness
constexpr gpio_num_t kVbusPin = GPIO_NUM_3;           // VBUS through the on-board 100k/100k divider
constexpr int kVbusDividerRatio = 2;                  // (100k + 100k) / 100k

}  // namespace

// Composition root: creates the concrete hardware objects,
// wires them into App through interfaces, and runs the main loop.
extern "C" void app_main(void) {
    Ws2812Led led(kLedPin);
    I2cBus bus(kSdaPin, kSclPin);
    Veml6040 veml(bus);
    Ssd1306Display oled(bus);
    Bme688 bme(bus);
    GpioButton modeButton(kModeButtonPin);
    GpioButton memoryButton(kMemoryButtonPin);
    AdcVoltage vbus(kVbusPin, kVbusDividerRatio);

    // Startup failure: report it, show red and stop here. No return: leaving
    // app_main would destroy all objects above, including the LED driver.
    auto fail = [&led](const char* what) {
        ESP_LOGE(TAG, "%s init failed", what);
        led.setColor(kLedLevel, 0, 0);
        while (true) vTaskDelay(portMAX_DELAY);
    };

    if (!led.init()) {
        ESP_LOGE(TAG, "LED init failed");
    } else {
        led.ledTest(kLedLevel, 200);        // quick test sequence
    }

    if (!oled.init()) fail("SSD1306");
    if (!veml.init()) fail("VEML6040");
    if (!bme.init()) fail("BME688");
    if (!vbus.init()) fail("VBUS ADC");

    led.setColor(0, kLedLevel, 0);  // all devices are up
    App app(veml, bme, vbus, oled, modeButton, memoryButton);

    uint8_t vLedDimmer = kLedLevel;  // temporary: fade the green LED out
    while (true) {
        app.tick();
        vTaskDelay(pdMS_TO_TICKS(kTickPeriodMs));
        if (vLedDimmer) led.setColor(0, --vLedDimmer, 0);  // green color dimmer
    }
}