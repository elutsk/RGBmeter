#include "GpioButton.h"

#include "esp_err.h"
#include "esp_log.h"

static const char* TAG = "button";

namespace {

constexpr uint32_t kPollPeriodMs = 10;                             // sample the pin every 10 ms
constexpr uint8_t kStableSamples = 3;                              // 3 equal samples = 30 ms stable
constexpr uint32_t kLongClickMs = 1000;                            // hold time for a long click
constexpr uint16_t kLongClickPolls = kLongClickMs / kPollPeriodMs;  // = 100 polls

}  // namespace

GpioButton::GpioButton(gpio_num_t pin) : pin_(pin) {
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
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer_, kPollPeriodMs * 1000));
}

GpioButton::~GpioButton() {
    // Stop the timer before the object dies: otherwise the callback
    // could run with a dangling 'this'.
    if (timer_ != nullptr) {
        esp_timer_stop(timer_);
        esp_timer_delete(timer_);
    }
}

bool GpioButton::wasShortClicked() {
    const bool clicked = shortClick_.exchange(false);  // read and clear in one step
    if (clicked) ESP_LOGI(TAG, "GPIO%d short click", static_cast<int>(pin_));
    return clicked;
}

bool GpioButton::wasLongClicked() {
    const bool clicked = longClick_.exchange(false);
    if (clicked) ESP_LOGI(TAG, "GPIO%d long click", static_cast<int>(pin_));
    return clicked;
}

void GpioButton::onTimer(void* arg) {
    static_cast<GpioButton*>(arg)->poll();
}

void GpioButton::poll() {
    const bool down = (gpio_get_level(pin_) == 0);  // active low

    // 1. Debounce: any change of the raw level restarts the count.
    if (down != lastRaw_) {
        lastRaw_ = down;
        stableCount_ = 0;
        return;
    }
    if (stableCount_ < kStableSamples) ++stableCount_;
    if (stableCount_ < kStableSamples) return;  // not stable long enough yet

    // 2. Debounced edge: press starts timing, release may be a short click.
    if (down != stableDown_) {
        stableDown_ = down;
        if (down) {
            heldPolls_ = 0;
            longReported_ = false;
        } else if (!longReported_) {
            shortClick_.store(true);
        }
        return;
    }

    // 3. Still held: report a long click once, when the hold time is reached.
    if (stableDown_ && !longReported_ && ++heldPolls_ >= kLongClickPolls) {
        longClick_.store(true);
        longReported_ = true;
    }
}