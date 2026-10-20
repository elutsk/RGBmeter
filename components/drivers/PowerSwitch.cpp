#include "PowerSwitch.h"

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

const char* TAG = "power";

}  // namespace

PowerSwitch::PowerSwitch(gpio_num_t pin) : pin_(pin) {
    gpio_config_t io = {};
    io.pin_bit_mask = 1ULL << pin_;
    io.mode = GPIO_MODE_OUTPUT;
    io.pull_up_en = GPIO_PULLUP_DISABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&io));
    gpio_set_level(pin_, 0);
}

void PowerSwitch::cut(uint32_t timeoutMs) {
    ESP_LOGI(TAG, "cutting the battery power");
    gpio_set_level(pin_, 1);
    vTaskDelay(pdMS_TO_TICKS(timeoutMs));

    // Still alive: the cut had no effect. Release CS, the caller decides what next.
    gpio_set_level(pin_, 0);
    ESP_LOGW(TAG, "power still on after %u ms", static_cast<unsigned>(timeoutMs));
}