#pragma once

// One temperature / humidity / pressure measurement.
struct ClimateSample {
    float tempC;
    float humidityPct;
    float pressureHpa;
};

// Hardware-independent climate sensor interface.
class IClimateSensor {
public:
    virtual ~IClimateSensor() = default;

    // On success fills 'out' and returns true.
    // On failure returns false and leaves 'out' unchanged.
    virtual bool read(ClimateSample& out) = 0;
};