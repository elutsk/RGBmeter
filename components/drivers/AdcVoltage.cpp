#include "AdcVoltage.h"

namespace {

constexpr adc_atten_t kAtten = ADC_ATTEN_DB_12;  // input range up to about 3.1 V
constexpr int kSamples = 16;                     // raw samples averaged per reading

}  // namespace

AdcVoltage::AdcVoltage(gpio_num_t pin, int dividerRatio) : pin_(pin), dividerRatio_(dividerRatio) {}

AdcVoltage::~AdcVoltage() {
    // Release whatever init() managed to create, even if it failed halfway.
    if (cali_ != nullptr) adc_cali_delete_scheme_curve_fitting(cali_);
    if (adc_ != nullptr) adc_oneshot_del_unit(adc_);
}

bool AdcVoltage::init() {
    // Which ADC unit and channel this GPIO belongs to (on ESP32-S3: GPIO3 = ADC1 channel 2).
    if (adc_oneshot_io_to_channel(pin_, &unit_, &channel_) != ESP_OK) return false;

    adc_oneshot_unit_init_cfg_t unitConfig = {};
    unitConfig.unit_id = unit_;
    if (adc_oneshot_new_unit(&unitConfig, &adc_) != ESP_OK) return false;

    adc_oneshot_chan_cfg_t channelConfig = {};
    channelConfig.atten = kAtten;
    channelConfig.bitwidth = ADC_BITWIDTH_DEFAULT;
    if (adc_oneshot_config_channel(adc_, channel_, &channelConfig) != ESP_OK) return false;

    // Calibration converts raw counts to millivolts using factory data from eFuse.
    adc_cali_curve_fitting_config_t caliConfig = {};
    caliConfig.unit_id = unit_;
    caliConfig.chan = channel_;
    caliConfig.atten = kAtten;
    caliConfig.bitwidth = ADC_BITWIDTH_DEFAULT;
    return adc_cali_create_scheme_curve_fitting(&caliConfig, &cali_) == ESP_OK;
}

bool AdcVoltage::readMillivolts(int& out) {
    if (adc_ == nullptr || cali_ == nullptr) return false;

    // The ADC is noisy: average several raw samples, then convert once.
    int sum = 0;
    for (int i = 0; i < kSamples; ++i) {
        int raw = 0;
        if (adc_oneshot_read(adc_, channel_, &raw) != ESP_OK) return false;
        sum += raw;
    }

    int pinMillivolts = 0;
    if (adc_cali_raw_to_voltage(cali_, sum / kSamples, &pinMillivolts) != ESP_OK) return false;
    out = pinMillivolts * dividerRatio_;  // back to the voltage at the top of the divider
    return true;
}