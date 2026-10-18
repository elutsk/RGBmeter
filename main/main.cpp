#include "App.h"
#include "Bme688.h"
#include "GpioButton.h"
#include "I2cBus.h"
#include "Ssd1306Display.h"
#include "Veml6040.h"
#include "Ws2812Led.h"
#include "AdcVoltage.h"
#include "esp_err.h"
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
constexpr bool kBusDiagMode = false;                // i2c + vbus diagnostics mode

// Diagnostics: logs every 7-bit address that answers with ACK.
void scanBus(const I2cBus& bus) {
    constexpr uint16_t kFirstAddress = 0x08;
    constexpr uint16_t kLastAddress = 0x77;
    constexpr int kScanTimeoutMs = 50;
    int found = 0;
    for (uint16_t address = kFirstAddress; address <= kLastAddress; ++address) {
        const esp_err_t err = i2c_master_probe(bus.handle(), address, kScanTimeoutMs);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "scan: 0x%02X ACK", address);
            ++found;
        } else if (err != ESP_ERR_NOT_FOUND) {
            ESP_LOGW(TAG, "scan: 0x%02X %s", address, esp_err_to_name(err));
        }
    }
    ESP_LOGI(TAG, "scan: %d device(s) found", found);
}

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

    if (!vbus.init()) fail("VBUS ADC");
    if (kBusDiagMode) {
        // VBUS first: the supply voltage is the first suspect when something fails.
        int startupMillivolts = 0;
        if (vbus.readMillivolts(startupMillivolts)) ESP_LOGI(TAG, "startup VBUS %d mV (uncorrected)", startupMillivolts);
        scanBus(bus);  // scan i2c bus for all connected peripherals
    }

    if (!oled.init()) fail("SSD1306");
    if (!veml.init()) fail("VEML6040");
    if (!bme.init()) fail("BME688");

    led.pulse(0, kLedLevel, 0, 1000);  // all devices are up: green for 1 s
    App app(veml, bme, vbus, oled, led, modeButton, memoryButton, kTickPeriodMs);    

    while (true) {
        app.tick();
        vTaskDelay(pdMS_TO_TICKS(kTickPeriodMs));
    }
}