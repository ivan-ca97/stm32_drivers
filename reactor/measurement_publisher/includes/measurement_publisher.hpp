#pragma once

#include "register_window.hpp"
#include "reactor_measurements.hpp"
#include "measurement_map.hpp"

static_assert(sizeof(ReactorMeasurements::ValueType) == measurements::FloatSize,
              "measurement value type does not match the size on the wire");

// MeasurementPublisher — typed publisher for the measurement register window.
// Writes under a seqlock (see publish).
class MeasurementPublisher
{
    public:
        explicit MeasurementPublisher(RegisterWindow& window) : window(window) {}

        void publish(const ReactorMeasurements& measurements);

    protected:
        RegisterWindow& window;
        uint8_t sequence = 0;   // seqlock counter (even = stable, odd = writing)

        void writeFloat(uint8_t offset, ReactorMeasurements::ValueType value);
};
