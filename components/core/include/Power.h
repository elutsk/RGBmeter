#pragma once

#include <cstdint>

// Power source and battery state, derived from the VBUS voltage measured
// behind the on-board divider. Pure logic, no hardware access.
struct PowerStatus {
    bool onUsb;          // true: powered from USB; false: from the battery
    int millivolts;      // corrected voltage: VBUS on USB, battery voltage on battery
    int batteryPercent;  // 0..100, meaningful only when onUsb is false
    bool batteryEmpty;   // true: on battery and below 0 % (the battery guard threshold)
};

// vbusMillivolts: the value reported by the VBUS voltage sensor (uncorrected).
PowerStatus evaluatePower(int vbusMillivolts);

// How the device was started, as far as the power logic cares.
enum class StartCause : uint8_t { PowerOn, ButtonWake, Other };

// What to do at startup: start normally, or first check that button 1 is held.
// A start by the button needs a long press, an accidental short one switches
// the device off again.
enum class StartAction : uint8_t { Start, ConfirmButton };

// PowerOn on battery comes from button 1 (after the protection IC had cut the
// power) or from connecting the battery: both need the button confirmation.
// PowerOn on USB, and any other reset (flashing, panic, watchdog): normal start.
StartAction decideStart(StartCause cause, bool onUsb);