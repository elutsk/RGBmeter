#include "I2cBus.h"
#include "esp_log.h"

static const char* TAG = "rgbmeter";

constexpr gpio_num_t kSdaPin = GPIO_NUM_12;
constexpr gpio_num_t kSclPin = GPIO_NUM_13;
constexpr uint16_t kVemlAddress = 0x10;
constexpr int kProbeTimeoutMs = 50;

extern "C" void app_main(void) {
    I2cBus bus(kSdaPin, kSclPin);

    const bool present =
        (i2c_master_probe(bus.handle(), kVemlAddress, kProbeTimeoutMs) == ESP_OK);
    ESP_LOGI(TAG, "VEML6040 at 0x%02X: %s", kVemlAddress, present ? "present" : "missing");
}   // bus goes out of scope here: its destructor deletes the I2C bus