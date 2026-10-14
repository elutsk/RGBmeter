#pragma once

#include "I2cBus.h"
#include "IClimateSensor.h"
#include "bme68x.h"

// Bosch BME688 (BME68x family) over I2C, address 0x77.
// Adapter: implements IClimateSensor on top of the Bosch BME68x SensorAPI.
// Uses forced mode for temperature, humidity and pressure; the gas
// heater is disabled.
class Bme688 : public IClimateSensor {
public:
    // Attaches the device to the bus. Does not talk to the sensor yet.
    explicit Bme688(I2cBus& bus);
    ~Bme688() override;

    Bme688(const Bme688&) = delete;
    Bme688& operator=(const Bme688&) = delete;

    // Reads the chip ID and calibration data, applies the configuration.
    // Returns false if the sensor does not respond or is not a BME68x.
    bool init();

    // Starts one measurement, waits for it to finish and reads the result.
    // Blocks for the measurement time (about 40 ms with this configuration).
    bool read(ClimateSample& out) override;

private:
    // Bus callbacks for the Bosch API. It is a C API, so these must be
    // plain functions; 'intf' carries 'this' (see bme_.intf_ptr).
    static BME68X_INTF_RET_TYPE busRead(uint8_t reg, uint8_t* data, uint32_t len, void* intf);
    static BME68X_INTF_RET_TYPE busWrite(uint8_t reg, const uint8_t* data, uint32_t len, void* intf);
    static void delayUs(uint32_t us, void* intf);

    i2c_master_dev_handle_t dev_ = nullptr;

    // Stored by value: the Bosch API keeps calibration data inside it.
    // That is why the full "bme68x.h" is needed here, not a forward declaration.
    bme68x_dev bme_ = {};
    bme68x_conf conf_ = {};
};