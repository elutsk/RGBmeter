#include "Power.h"

namespace {

// Calibration for the ESP32-S3 Super Mini (100k/100k divider, Schottky diode
// between the battery and VBUS), measured against a multimeter.
constexpr int kUsbThresholdMv = 4400;      // above: USB; the battery never gets VBUS this high
constexpr int kUsbCorrectionMv = 130;      // ADC reads low on USB
constexpr int kBatteryCorrectionMv = 440;  // ADC offset plus the diode drop

// Battery range of this device: below 3.4 V the ESP32 shuts down.
constexpr int kBatteryEmptyMv = 3400;  // 0 %
constexpr int kBatteryFullMv = 4200;   // 100 %

}  // namespace

PowerStatus evaluatePower(int vbusMillivolts) {
    PowerStatus status = {};
    status.onUsb = vbusMillivolts > kUsbThresholdMv;

    if (status.onUsb) {
        status.millivolts = vbusMillivolts + kUsbCorrectionMv;
        status.batteryPercent = 0;  // the battery is not visible behind the diode
        return status;
    }

    status.millivolts = vbusMillivolts + kBatteryCorrectionMv;
    if (status.millivolts <= kBatteryEmptyMv) {
        status.batteryPercent = 0;
    } else if (status.millivolts >= kBatteryFullMv) {
        status.batteryPercent = 100;
    } else {
        status.batteryPercent = (status.millivolts - kBatteryEmptyMv) * 100 / (kBatteryFullMv - kBatteryEmptyMv);
    }
    return status;
}