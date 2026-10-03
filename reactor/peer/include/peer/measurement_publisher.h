#pragma once

#include <cstdint>

#include "reactor_measurements.hpp"
#include "register_window.hpp"

namespace peer {

// Writes this board's measurements into its measurement window, under the
// seqlock described in measurement_window.h.
class MeasurementPublisher {
public:
    explicit MeasurementPublisher(RegisterWindow& window);

    MeasurementPublisher(const MeasurementPublisher&) = delete;
    MeasurementPublisher& operator=(const MeasurementPublisher&) = delete;

    void publish(const ReactorMeasurements& measurements);

private:
    void writeValue(uint8_t offset, ReactorMeasurements::ValueType value);

    RegisterWindow& window_;
    uint8_t sequence_ = 0;
};

} // namespace peer
