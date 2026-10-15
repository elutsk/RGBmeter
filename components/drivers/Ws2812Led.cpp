#include "Ws2812Led.h"
#include "led_strip.h"

Ws2812Led::Ws2812Led(gpio_num_t pin) : pin_(pin) {}

Ws2812Led::~Ws2812Led() {
    if (handle_ != nullptr) led_strip_del(handle_);
}

bool Ws2812Led::init() {
    // Zero-initialized fields mean "use the driver defaults": GRB color order
    // (the order WS2812 expects), 10 MHz RMT resolution, default clock source
    // and memory size.
    led_strip_config_t stripConfig = {};
    stripConfig.strip_gpio_num = pin_;
    stripConfig.max_leds = 1;
    stripConfig.led_model = LED_MODEL_WS2812;

    led_strip_rmt_config_t rmtConfig = {};
    return led_strip_new_rmt_device(&stripConfig, &rmtConfig, &handle_) == ESP_OK;
}

void Ws2812Led::ledTest(uint8_t level, uint32_t delay) {
    // LED test: red, green, blue for predefined delay in msec each, then off.
    setColor(level, 0, 0);
    vTaskDelay(pdMS_TO_TICKS(delay));
    setColor(0, level, 0);
    vTaskDelay(pdMS_TO_TICKS(delay));
    setColor(0, 0, level);
    vTaskDelay(pdMS_TO_TICKS(delay));
    off();
}

bool Ws2812Led::setColor(uint8_t red, uint8_t green, uint8_t blue) {
    if (handle_ == nullptr) return false;
    if (led_strip_set_pixel(handle_, 0, red, green, blue) != ESP_OK) return false;
    return led_strip_refresh(handle_) == ESP_OK;  // send the color to the LED
}

bool Ws2812Led::off() {
    return handle_ != nullptr && led_strip_clear(handle_) == ESP_OK;
}