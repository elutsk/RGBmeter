#pragma once

#include "IVoltageSensor.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

// Voltage measured by the ADC behind a resistive divider. The result is
// scaled back to the voltage at the top of the divider. One-shot mode,
// curve-fitting calibration from the factory eFuse data, several samples
// averaged per reading.
class AdcVoltage : public IVoltageSensor {
public:
    // dividerRatio = (Rtop + Rbottom) / Rbottom, for example 2 for 100k + 100k.
    AdcVoltage(gpio_num_t pin, int dividerRatio);
    ~AdcVoltage() override;

    AdcVoltage(const AdcVoltage&) = delete;
    AdcVoltage& operator=(const AdcVoltage&) = delete;

    // Finds the ADC channel of the pin, configures it and the calibration.
    // Returns false on failure.
    bool init();

    bool readMillivolts(int& out) override;

private:
    gpio_num_t pin_;
    int dividerRatio_;
    adc_unit_t unit_ = ADC_UNIT_1;
    adc_channel_t channel_ = ADC_CHANNEL_0;
    adc_oneshot_unit_handle_t adc_ = nullptr;
    adc_cali_handle_t cali_ = nullptr;
};