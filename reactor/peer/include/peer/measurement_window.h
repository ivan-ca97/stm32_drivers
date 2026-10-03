#pragma once

#include <cstdint>

#include "reactor_measurements.hpp"

// Register layout of the measurement window, read by every master. Values are
// float32, little-endian, at window-local offsets. The window is written under a
// seqlock: a master reads the sequence, the values, and the sequence again, and
// keeps the values only if both reads match and are even.
namespace peer::measurement_window {

inline constexpr uint8_t sequenceOffset = 0x00;
inline constexpr uint8_t reactivityLogarithmicOffset = 0x04;
inline constexpr uint8_t reactivityLinearOffset = 0x08;
inline constexpr uint8_t powerRateLogarithmicOffset = 0x0C;
inline constexpr uint8_t powerRateLinearOffset = 0x10;

inline constexpr uint8_t valueSize = 4;
inline constexpr uint8_t valuesLength = powerRateLinearOffset + valueSize;

// Reserved for alarm and discrepancy flags.
inline constexpr uint8_t statusLength = 0x0C;

inline constexpr uint8_t length = valuesLength + statusLength;

static_assert(
    sizeof(ReactorMeasurements::ValueType) == valueSize,
    "the measurement value type has to match its size on the wire"
);

} // namespace peer::measurement_window
