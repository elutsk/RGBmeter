#include "Ws2812Led.h"
#include "led_strip.h"

Ws2812Led::Ws2812Led(gpio_num_t pin) : pin_(pin) {}

Ws2812Led::~Ws2812Led() {
    // Stop the timer first: its callback uses this object and the LED driver.
    if (pulseTimer_ != nullptr) {
        esp_timer_stop(pulseTimer_);
        esp_timer_delete(pulseTimer_);
    }
    if (handle_ != nullptr) led_strip_del(handle_);
}

bool Ws2812Led::init() {
    // After a wake-up from deep sleep the pin is still held low by sleep():
    // release it, otherwise the RMT cannot drive it. Harmless if not held.
    gpio_hold_dis(pin_);

    // Zero-initialized fields mean "use the driver defaults": GRB color order
    // (the order WS2812 expects), 10 MHz RMT resolution, default clock source
    // and memory size.
    led_strip_config_t stripConfig = {};
    stripConfig.strip_gpio_num = pin_;
    stripConfig.max_leds = 1;
    stripConfig.led_model = LED_MODEL_WS2812;

    led_strip_rmt_config_t rmtConfig = {};
    if (led_strip_new_rmt_device(&stripConfig, &rmtConfig, &handle_) != ESP_OK) return false;

    esp_timer_create_args_t timerArgs = {};
    timerArgs.callback = &Ws2812Led::onPulseEnd;
    timerArgs.arg = this;
    timerArgs.dispatch_method = ESP_TIMER_TASK;
    timerArgs.name = "led_pulse";
    return esp_timer_create(&timerArgs, &pulseTimer_) == ESP_OK;
}

bool Ws2812Led::pulse(uint8_t red, uint8_t green, uint8_t blue, uint32_t durationMs) {
    if (pulseTimer_ == nullptr) return false;
    esp_timer_stop(pulseTimer_);  // restart if a pulse is already running; error if not running is fine
    if (!setColor(red, green, blue)) return false;
    return esp_timer_start_once(pulseTimer_, static_cast<uint64_t>(durationMs) * 1000) == ESP_OK;
}

void Ws2812Led::onPulseEnd(void* arg) {
    static_cast<Ws2812Led*>(arg)->off();
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

void Ws2812Led::sleep() {
    if (pulseTimer_ != nullptr) esp_timer_stop(pulseTimer_);  // error if not running is fine
    if (handle_ != nullptr) {
        led_strip_clear(handle_);  // WS2812 off
        led_strip_del(handle_);    // frees the RMT channel: the pin is a plain GPIO again
        handle_ = nullptr;
    }

    gpio_config_t io = {};
    io.pin_bit_mask = 1ULL << pin_;
    io.mode = GPIO_MODE_OUTPUT;
    io.pull_up_en = GPIO_PULLUP_DISABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io);
    gpio_set_level(pin_, 0);

    // GPIO48 is a digital pin, not an RTC one: on the ESP32-S3 it keeps its state
    // in deep sleep only with both calls (see the gpio_hold_en() notes in ESP-IDF).
    gpio_hold_en(pin_);
    gpio_deep_sleep_hold_en();
}