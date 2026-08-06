#pragma once

#include <stdint.h>

// Register layout of the measurement window: where each value sits in the bank and
// how it is encoded. The master reads these registers directly, so this is a wire
// contract — a change here has to be mirrored on the other side.
namespace measurements
{
    // Global base + window length (immediate-register window [0x00, 0x20)).
    static constexpr uint8_t WindowBase   = 0x00;
    static constexpr uint8_t WindowLength = 0x20;

    // Window-local offsets. Values are float32, little-endian.
    static constexpr uint8_t SequenceOffset              = 0x00;  // 1 byte; see MeasurementPublisher
    static constexpr uint8_t ReactivityLogarithmicOffset = 0x04;
    static constexpr uint8_t ReactivityLinearOffset      = 0x08;
    static constexpr uint8_t PowerRateLogarithmicOffset  = 0x0C;
    static constexpr uint8_t PowerRateLinearOffset       = 0x10;
    // 0x14-0x1F reserved (flags / status).

    static constexpr uint8_t FloatSize = 4;
}
