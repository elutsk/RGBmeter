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

    // Some third-party drivers identify the bus by port number, not by handle.
    i2c_port_num_t port() const { return port_; }

    // Current logic level of the lines (1 = high), for diagnostics.
    int sdaLevel() const;
    int sclLevel() const;    

    private:
    gpio_num_t sda_;
    gpio_num_t scl_;
    i2c_port_num_t port_ = I2C_NUM_0;
    i2c_master_bus_handle_t handle_ = nullptr;
};