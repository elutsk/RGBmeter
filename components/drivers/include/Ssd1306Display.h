#pragma once

#include "I2cBus.h"
#include "IDisplay.h"

// Forward declaration of the library's internal struct. This keeps
// "ssd1306.h" out of our public header: users of Ssd1306Display
// do not depend on the third-party library at all.
struct ssd1306_t;

// SSD1306 128x64 OLED over I2C (address 0x3C).
// Adapter: implements our IDisplay on top of the chill-sam/ssd1306 C library.
class Ssd1306Display : public IDisplay {
public:
    explicit Ssd1306Display(const I2cBus& bus);
    ~Ssd1306Display() override;

    Ssd1306Display(const Ssd1306Display&) = delete;
    Ssd1306Display& operator=(const Ssd1306Display&) = delete;

    // Creates the library handle and runs the panel init sequence.
    // Returns false if the panel does not respond.
    bool init();

    void clear() override;
    void drawText(int x, int y, const char* text, int scale) override;
    bool flush() override;

private:
    i2c_port_num_t port_;
    i2c_master_bus_handle_t bus_;  // only for the diagnostic probe
    ssd1306_t* handle_ = nullptr;  // same type as the library's ssd1306_handle_t
};