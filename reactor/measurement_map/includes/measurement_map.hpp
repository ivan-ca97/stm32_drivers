#pragma once

#include <stdint.h>

// Register layout of the measurement window: where each value sits and how it is
// encoded. The master reads these registers directly.
namespace measurements
{
    // Window-local offsets. Values are float32, little-endian.
    static constexpr uint8_t SequenceOffset = 0x00;   // 1 byte; see MeasurementPublisher
    static constexpr uint8_t ReactivityLogarithmicOffset = 0x04;
    static constexpr uint8_t ReactivityLinearOffset = 0x08;
    static constexpr uint8_t PowerRateLogarithmicOffset = 0x0C;
    static constexpr uint8_t PowerRateLinearOffset = 0x10;

    // The size on the wire, not the model's: widening the field is a protocol change,
    // not a recompile. Consumers static_assert their value type against this.
    static constexpr uint8_t FloatSize = 4;

    static constexpr uint8_t MeasurementsLength = PowerRateLinearOffset + FloatSize;
    static constexpr uint8_t StatusLength = 0x0C;   // alarm / peer discrepancy flags
    static constexpr uint8_t WindowLength = MeasurementsLength + StatusLength;
}
