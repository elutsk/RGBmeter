#include "Bme688.h"

#include <cstring>

#include "esp_err.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

constexpr uint16_t kAddress = BME68X_I2C_ADDR_HIGH;  // 0x77, SDO tied to VCC
constexpr uint32_t kSclSpeedHz = 400000;
constexpr int kTimeoutMs = 50;

// The Bosch API never writes more than this many bytes in one call
// (register/value pairs are interleaved in a buffer of this size).
constexpr uint32_t kMaxWriteLen = BME68X_LEN_INTERLEAVE_BUFF;

// Below this, busy-wait; above, sleep and let other tasks run.
constexpr uint32_t kBusyWaitLimitUs = 1000;

constexpr BME68X_INTF_RET_TYPE kBusOk = 0;
constexpr BME68X_INTF_RET_TYPE kBusError = BME68X_E_COM_FAIL;

}  // namespace

Bme688::Bme688(I2cBus& bus)
{
    i2c_device_config_t config = {};
    config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    config.device_address = kAddress;
    config.scl_speed_hz = kSclSpeedHz;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus.handle(), &config, &dev_));
}

Bme688::~Bme688()
{
    if (dev_ != nullptr) {
        i2c_master_bus_rm_device(dev_);
    }
}

bool Bme688::init()
{
    bme_.intf = BME68X_I2C_INTF;
    bme_.intf_ptr = this;  // passed back to busRead / busWrite / delayUs
    bme_.read = &Bme688::busRead;
    bme_.write = &Bme688::busWrite;
    bme_.delay_us = &Bme688::delayUs;
    bme_.amb_temp = 25;  // used by the API only for heater calculations

    // Soft reset, chip ID check, calibration data read.
    if (bme68x_init(&bme_) != BME68X_OK) {
        return false;
    }

    // Oversampling as in the Bosch forced mode example.
    conf_.os_hum = BME68X_OS_16X;
    conf_.os_temp = BME68X_OS_2X;
    conf_.os_pres = BME68X_OS_1X;
    conf_.filter = BME68X_FILTER_OFF;
    conf_.odr = BME68X_ODR_NONE;
    if (bme68x_set_conf(&conf_, &bme_) != BME68X_OK) {
        return false;
    }

    bme68x_heatr_conf heater = {};
    heater.enable = BME68X_DISABLE;  // no gas measurement
    return bme68x_set_heatr_conf(BME68X_FORCED_MODE, &heater, &bme_) == BME68X_OK;
}

bool Bme688::read(ClimateSample& out)
{
    if (bme68x_set_op_mode(BME68X_FORCED_MODE, &bme_) != BME68X_OK) {
        return false;
    }

    // The API tells how long the measurement takes for our oversampling.
    delayUs(bme68x_get_meas_dur(BME68X_FORCED_MODE, &conf_, &bme_), this);

    bme68x_data data = {};
    uint8_t count = 0;
    if (bme68x_get_data(BME68X_FORCED_MODE, &data, &count, &bme_) != BME68X_OK || count == 0) {
        return false;
    }

    // BME68X_USE_FPU is on by default: values are floats.
    out.tempC = data.temperature;
    out.humidityPct = data.humidity;
    out.pressureHpa = data.pressure / 100.0f;  // Pa -> hPa
    return true;
}

BME68X_INTF_RET_TYPE Bme688::busRead(uint8_t reg, uint8_t* data, uint32_t len, void* intf)
{
    Bme688* self = static_cast<Bme688*>(intf);
    // Write the register address, repeated START, read 'len' bytes.
    const esp_err_t err = i2c_master_transmit_receive(self->dev_, &reg, 1, data, len, kTimeoutMs);
    return err == ESP_OK ? kBusOk : kBusError;
}

BME68X_INTF_RET_TYPE Bme688::busWrite(uint8_t reg, const uint8_t* data, uint32_t len, void* intf)
{
    Bme688* self = static_cast<Bme688*>(intf);
    if (len > kMaxWriteLen) {
        return kBusError;
    }
    // Register address and data must go out in one transaction,
    // so put them into one buffer.
    uint8_t frame[1 + kMaxWriteLen];
    frame[0] = reg;
    std::memcpy(&frame[1], data, len);
    const esp_err_t err = i2c_master_transmit(self->dev_, frame, 1 + len, kTimeoutMs);
    return err == ESP_OK ? kBusOk : kBusError;
}

void Bme688::delayUs(uint32_t us, void* /*intf*/)
{
    if (us < kBusyWaitLimitUs) {
        esp_rom_delay_us(us);
        return;
    }
    // vTaskDelay works in whole ticks (10 ms with the default 100 Hz tick).
    // Round up, and add one tick: the current tick is already partly over,
    // so vTaskDelay(n) can return up to one tick early.
    const uint32_t ms = (us + 999) / 1000;
    const TickType_t ticks = (ms + portTICK_PERIOD_MS - 1) / portTICK_PERIOD_MS + 1;
    vTaskDelay(ticks);
}