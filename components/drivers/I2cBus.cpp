#include "I2cBus.h"
#include "esp_err.h"
#include "driver/gpio.h"

I2cBus::I2cBus(gpio_num_t sda, gpio_num_t scl) : sda_(sda), scl_(scl) {
    i2c_master_bus_config_t config = {};
    config.i2c_port = port_;
    config.sda_io_num = sda;
    config.scl_io_num = scl;
    config.clk_source = I2C_CLK_SRC_DEFAULT;
    config.glitch_ignore_cnt = 7;
    config.flags.enable_internal_pullup = true;

    // ESP-IDF builds without C++ exceptions by default, so a constructor
    // has no way to report failure. A bus that cannot be created means
    // wrong pins or config: stop here with a clear error message.
    ESP_ERROR_CHECK(i2c_new_master_bus(&config, &handle_));
}

I2cBus::~I2cBus() {
    if (handle_ != nullptr) {
        i2c_del_master_bus(handle_);
    }
}

int I2cBus::sdaLevel() const {
    return gpio_get_level(sda_);
}

int I2cBus::sclLevel() const {
    return gpio_get_level(scl_);
}