#pragma once

#include "I2cBus.h"
#include "IColorSensor.h"

// Vishay VEML6040 RGBW color sensor, I2C address 0x10
// Runs in auto mode: the sensor measures continuously,
// read() returns the result of the last finished integration
class Veml6040 : public IColorSensor {
public:
    // Attaches the device to the bus. Does not talk to the sensor yet
    explicit Veml6040(I2cBus& bus);
    ~Veml6040() override;

    Veml6040(const Veml6040&) = delete;
    Veml6040& operator=(const Veml6040&) = delete;

    // Writes the config
    // Returns false if sensor does not respond
    bool init();
    bool read(ColorSample& out) override;

    // Puts the sensor into shutdown (SD bit). Measurements stop until init().
    // Returns false if sensor does not respond
    bool shutdown();

private:
    bool writeConf(uint8_t confLow);
    bool readChannel(uint8_t command, uint16_t& value);

    i2c_master_dev_handle_t dev_ = nullptr;
};