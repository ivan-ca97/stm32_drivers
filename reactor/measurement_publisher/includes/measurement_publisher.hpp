#pragma once

#include "register_window.hpp"
#include "reactor_measurements.hpp"
#include "measurement_map.hpp"

// MeasurementPublisher — typed publisher for the measurement register window.
// Writes the latest measurements with a seqlock (seq odd while writing, even when
// stable) so the master reads a consistent snapshot and can tell whether the data
// is new. The window scopes what it can write, the raw byte layout stays hidden.
class MeasurementPublisher
{
    public:
        explicit MeasurementPublisher(RegisterWindow& window) : window(window) {}

        void publish(const ReactorMeasurements& measurements);

    protected:
        RegisterWindow& window;
        uint8_t         sequence = 0;   // seqlock counter (even = stable, odd = writing)

        void writeFloat(uint8_t offset, float value);
};
