#pragma once

// Hardware-independent voltage measurement.
class IVoltageSensor {
public:
    virtual ~IVoltageSensor() = default;

    // Voltage at the measured point, in millivolts.
    // On failure returns false and leaves 'out' unchanged.
    virtual bool readMillivolts(int& out) = 0;
};