#include "I2cBus.h"
#include "Veml6040.h" 
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "rgbmeter";

constexpr gpio_num_t kSdaPin = GPIO_NUM_12;
constexpr gpio_num_t kSclPin = GPIO_NUM_13;
constexpr int kReadPeriodMs = 200;

extern "C" void app_main(void) {
    I2cBus bus(kSdaPin, kSclPin);
    Veml6040 veml(bus);

    if (!veml.init()) {
        ESP_LOGE(TAG, "VEML6040 init failed");
        return;
    }

    // from here only interface is visible - this is how app will see it
    IColorSensor& ColorSensor = veml;

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(kReadPeriodMs));

        ColorSample sample = {};
        if (ColorSensor.read(sample)) {
            ESP_LOGI(TAG, "R=%5u G=%5u B=%5u W=%5u", static_cast<unsigned>(sample.r), static_cast<unsigned>(sample.g), 
                static_cast<unsigned>(sample.b), static_cast<unsigned>(sample.w));
        } else {
            ESP_LOGW(TAG, "VEML6040 read failed");
        }
    }
}

// bus goes out of scope here: its destructor deletes the I2C bus