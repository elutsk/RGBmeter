#pragma once

#include <cstdint>

#include "driver/gpio.h"

// Software power switch through the battery protection IC (DW01A + 8205A).
// The pin drives the DW01A CS input through a resistor (4.7k, with 1k from CS
// to the device ground on the protection board). Held high, it makes the DW01A
// see a discharge overcurrent: the discharge MOSFET opens and the whole device
// loses power. It stays off until button 1 pulls CS to the cell minus.
// Charging keeps working. On USB the device is powered from VBUS anyway.
class PowerSwitch {
public:
    // Configures the pin as an output, low: the battery stays connected.
    explicit PowerSwitch(gpio_num_t pin);

    PowerSwitch(const PowerSwitch&) = delete;
    PowerSwitch& operator=(const PowerSwitch&) = delete;

    // Cuts the power. Returns only if the device is still powered after
    // timeoutMs (on USB, or no protection IC); the pin is then low again.
    void cut(uint32_t timeoutMs);

private:
    gpio_num_t pin_;
};