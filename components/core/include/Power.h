#pragma once

// Power source and battery state, derived from the VBUS voltage measured
// behind the on-board divider. Pure logic, no hardware access.
struct PowerStatus {
    bool onUsb;          // true: powered from USB; false: from the battery
    int millivolts;      // corrected voltage: VBUS on USB, battery voltage on battery
    int batteryPercent;  // 0..100, meaningful only when onUsb is false
};

// vbusMillivolts: the value reported by the VBUS voltage sensor (uncorrected).
PowerStatus evaluatePower(int vbusMillivolts);