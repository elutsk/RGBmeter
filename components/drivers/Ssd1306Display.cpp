#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Ssd1306Display.h"
#include "ssd1306.h"

namespace {

constexpr uint8_t kAddress = 0x3C;
constexpr uint16_t kWidth = 128;
constexpr uint16_t kHeight = 64;
constexpr int64_t kPowerUpDelayUs = 1000 * 1000;  // panel ignores commands this long after power-up

}  // namespace

Ssd1306Display::Ssd1306Display(const I2cBus& bus) : port_(bus.port()) {}

Ssd1306Display::~Ssd1306Display() {
    if (handle_ != nullptr) {
        ssd1306_del(handle_);
    }
}

bool Ssd1306Display::init() {
    // esp_timer counts from application startup, which is later than power-on,
    // so waiting until this mark is a safe lower bound for the panel warm-up.
    const int64_t sinceStartUs = esp_timer_get_time();
    if (sinceStartUs < kPowerUpDelayUs) {
        const uint32_t waitMs = static_cast<uint32_t>((kPowerUpDelayUs - sinceStartUs) / 1000);
        vTaskDelay((waitMs + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS + 1);  // round up, as in Bme688
    }

    ssd1306_config_t config = {};
    config.bus = SSD1306_I2C;
    config.width = kWidth;
    config.height = kHeight;
    config.fb = nullptr;  // let the library allocate the framebuffer
    config.iface.i2c.port = port_;
    config.iface.i2c.addr = kAddress;
    config.iface.i2c.rst_gpio = GPIO_NUM_NC;  // no reset pin on this module

    if (ssd1306_new_i2c(&config, &handle_) != ESP_OK) {
        // The library sets the handle before the panel init sequence runs,
        // so on failure it may be non-null: release it to avoid a leak.
        if (handle_ != nullptr) {
            ssd1306_del(handle_);
            handle_ = nullptr;
        }
        return false;
    }
    return true;
}

void Ssd1306Display::clear() {
    if (handle_ != nullptr) {
        ssd1306_clear(handle_);
    }
}

void Ssd1306Display::drawText(int x, int y, const char* text, int scale) {
    if (handle_ != nullptr) {
        ssd1306_draw_text_scaled(handle_, x, y, text, true, scale);
    }
}

bool Ssd1306Display::flush() {
    return handle_ != nullptr && ssd1306_display(handle_) == ESP_OK;
}