#pragma once

#include <cstdint>

#include "driver/gpio.h"

// Deep sleep of the ESP32-S3 with wake-up by one button (ext0, active low).
// Free functions instead of a class: there is no state to own, and the chip is one.
// Waking up from deep sleep is a reboot: app_main() starts from scratch.
namespace DeepSleep {

// True if this boot is a wake-up from deep sleep caused by the button.
bool wokeByButton();

// Call right after wokeByButton() returned true, before anything else uses the pin.
// Returns true if the button stays pressed for holdMs; it then also waits for the
// release, so the same press is not seen as a click later. Returns false if the
// button was released earlier (an accidental press).
bool confirmWake(gpio_num_t pin, uint32_t holdMs);

// Waits for the button release, arms the ext0 wake-up on it and enters deep sleep.
// Never returns: the next wake-up is a reboot.
[[noreturn]] void enter(gpio_num_t wakePin);

}  // namespace DeepSleep