#include "GpioButton.h"

#include "esp_err.h"

namespace {

constexpr uint64_t kPollPeriodUs = 10 * 1000;  // sample the pin every 10 ms
constexpr uint8_t kStableSamples = 3;          // 3 equal samples = 30 ms stable

}  // namespace

GpioButton::GpioButton(gpio_num_t pin)
    : pin_(pin)
{
    gpio_config_t io = {};
    io.pin_bit_mask = 1ULL << pin_;
    io.mode = GPIO_MODE_INPUT;
    io.pull_up_en = GPIO_PULLUP_ENABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.intr_type = GPIO_INTR_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&io));

    // All members are already initialized at this point, so it is safe
    // to hand 'this' to the timer and start it.
    esp_timer_create_args_t args = {};
    args.callback = &GpioButton::onTimer;
    args.arg = this;
    args.dispatch_method = ESP_TIMER_TASK;
    args.name = "button";
    ESP_ERROR_CHECK(esp_timer_create(&args, &timer_));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer_, kPollPeriodUs));
}

GpioButton::~GpioButton()
{
    // Stop the timer before the object dies: otherwise the callback
    // could run with a dangling 'this'.
    if (timer_ != nullptr) {
        esp_timer_stop(timer_);
        esp_timer_delete(timer_);
    }
}

bool GpioButton::wasPressed()
{
    // Read the flag and clear it in one indivisible step.
    return pressed_.exchange(false);
}

void GpioButton::onTimer(void* arg)
{
    static_cast<GpioButton*>(arg)->poll();
}

void GpioButton::poll()
{
    const bool down = (gpio_get_level(pin_) == 0);  // active low

    if (down != lastRaw_) {
        // Level changed: restart counting (this is where bounce is filtered).
        lastRaw_ = down;
        stableCount_ = 0;
        return;
    }

    if (stableCount_ < kStableSamples) {
        ++stableCount_;
    }

    if (stableCount_ == kStableSamples && down != stableDown_) {
        stableDown_ = down;
        if (down) {
            pressed_.store(true);  // report the press edge only, not the release
        }
    }
}