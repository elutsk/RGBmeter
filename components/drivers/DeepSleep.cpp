#include "DeepSleep.h"

#include "driver/rtc_io.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

const char* TAG = "sleep";
constexpr uint32_t kPollPeriodMs = 10;  // same sampling as GpioButton
constexpr int kStableSamples = 3;       // 3 equal samples = 30 ms stable

bool isPressed(gpio_num_t pin) {
    return gpio_get_level(pin) == 0;  // active low
}

}  // namespace

void DeepSleep::waitForRelease(gpio_num_t pin) {
    // Released for kStableSamples polls in a row.
    int released = 0;
    while (released < kStableSamples) {
        released = isPressed(pin) ? 0 : released + 1;
        vTaskDelay(pdMS_TO_TICKS(kPollPeriodMs));
    }
}

bool DeepSleep::wokeByButton() {
    return esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0;
}

bool DeepSleep::confirmWake(gpio_num_t pin, uint32_t holdMs) {
    // After deep sleep the wake-up pad is still an RTC IO: make it a normal input again.
    // After a power-on it is not an RTC IO, and the call changes nothing.
    ESP_ERROR_CHECK(rtc_gpio_deinit(pin));
    gpio_config_t io = {};
    io.pin_bit_mask = 1ULL << pin;
    io.mode = GPIO_MODE_INPUT;
    io.pull_up_en = GPIO_PULLUP_ENABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&io));

    // A stable release before holdMs means an accidental press.
    int released = 0;
    for (uint32_t elapsedMs = 0; elapsedMs < holdMs; elapsedMs += kPollPeriodMs) {
        released = isPressed(pin) ? 0 : released + 1;
        if (released >= kStableSamples) {
            ESP_LOGI(TAG, "wake-up not confirmed");
            return false;
        }
        vTaskDelay(pdMS_TO_TICKS(kPollPeriodMs));
    }
    ESP_LOGI(TAG, "wake-up confirmed, waiting for release");
    waitForRelease(pin);
    return true;
}

void DeepSleep::enter(gpio_num_t wakePin) {
    waitForRelease(wakePin);  // a pressed button would wake the chip at once

    // Only the RTC domain stays powered in deep sleep: the normal GPIO pull-up
    // does not work there, the RTC one does. Call order as in the ESP-IDF
    // deep_sleep example: the pin itself is switched to RTC IO by esp_deep_sleep_start().
    ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup(wakePin, 0));  // 0 = wake on low level
    ESP_ERROR_CHECK(rtc_gpio_pullup_en(wakePin));
    ESP_ERROR_CHECK(rtc_gpio_pulldown_dis(wakePin));

    ESP_LOGI(TAG, "entering deep sleep");
    esp_deep_sleep_start();
}