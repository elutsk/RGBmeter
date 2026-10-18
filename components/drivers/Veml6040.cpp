#include "Veml6040.h"
#include "esp_err.h"

namespace {

constexpr uint16_t kAddress = 0x10;
constexpr uint32_t kSclSpeedHz = 100000;
constexpr int kTimeoutMs = 50;

// Command codes (register addresses), see the VEML6040 datasheet.
constexpr uint8_t kCmdConf = 0x00;
constexpr uint8_t kCmdRed = 0x08;
constexpr uint8_t kCmdGreen = 0x09;
constexpr uint8_t kCmdBlue = 0x0A;
constexpr uint8_t kCmdWhite = 0x0B;

// CONF low byte: IT[6:4] = integration time, TRIG = bit 2,
// AF = bit 1 (0 = auto mode), SD = bit 0 (0 = sensor enabled).
constexpr uint8_t kIt160ms = 0x2 << 4;
constexpr uint8_t kSd = 0x01;  // shutdown
constexpr uint8_t kConfLow = kIt160ms;
constexpr uint8_t kConfHigh = 0x00;

}  // namespace

Veml6040::Veml6040(I2cBus& bus) {
    i2c_device_config_t config = {};
    config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    config.device_address = kAddress;
    config.scl_speed_hz = kSclSpeedHz;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus.handle(), &config, &dev_));
}

Veml6040::~Veml6040() {
    if (dev_ != nullptr) {
        i2c_master_bus_rm_device(dev_);
    }
}

bool Veml6040::init() {
    return writeConf(kConfLow);
}

bool Veml6040::shutdown() {
    // Same configuration, only SD set: init() later restores it by clearing SD.
    return writeConf(kConfLow | kSd);
}

bool Veml6040::writeConf(uint8_t confLow) {
    // Write frame: command code, then data low byte, then data high byte.
    const uint8_t frame[3] = {kCmdConf, confLow, kConfHigh};
    return i2c_master_transmit(dev_, frame, sizeof(frame), kTimeoutMs) == ESP_OK;
}

bool Veml6040::read(ColorSample& out) {
    ColorSample sample = {};
    if (!readChannel(kCmdRed, sample.r) ||
        !readChannel(kCmdGreen, sample.g) ||
        !readChannel(kCmdBlue, sample.b) ||
        !readChannel(kCmdWhite, sample.w)) {
        return false;
    }
    out = sample;
    return true;
}

bool Veml6040::readChannel(uint8_t command, uint16_t& value) {
    // Read frame: write the command code, repeated START, read 2 bytes.
    uint8_t data[2] = {};
    if (i2c_master_transmit_receive(dev_, &command, 1, data, sizeof(data), kTimeoutMs) != ESP_OK) {
        return false;
    }
    value = static_cast<uint16_t>(data[0] | (data[1] << 8));  // LSB comes first
    return true;
}