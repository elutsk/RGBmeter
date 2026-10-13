#pragma once

#include "driver/i2c_master.h"

// Owns the I2C master bus: creates it in the constructor,
// deletes it in the destructor (RAII). Drivers attach their
// devices to it via handle().
class I2cBus {
public:
    I2cBus(gpio_num_t sda, gpio_num_t scl);
    ~I2cBus();

    // A bus is a unique hardware resource: copying it makes no sense.
    I2cBus(const I2cBus&) = delete;
    I2cBus& operator=(const I2cBus&) = delete;

    i2c_master_bus_handle_t handle() const { return handle_; }

private:
    i2c_master_bus_handle_t handle_ = nullptr;
};