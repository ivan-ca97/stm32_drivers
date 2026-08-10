#pragma once

#include <stdint.h>

// Register layout of the measurement window: where each value sits and how it is
// encoded. The master reads these registers directly, so this is a wire contract —
// a change here has to be mirrored on the other side.
namespace measurements
{
    // Window-local offsets. Values are float32, little-endian.
    static constexpr uint8_t SequenceOffset = 0x00;   // 1 byte; see MeasurementPublisher
    static constexpr uint8_t ReactivityLogarithmicOffset = 0x04;
    static constexpr uint8_t ReactivityLinearOffset = 0x08;
    static constexpr uint8_t PowerRateLogarithmicOffset = 0x0C;
    static constexpr uint8_t PowerRateLinearOffset = 0x10;

    static constexpr uint8_t FloatSize = 4;

    // Published fields end here. The rest of the window is already committed by the
    // protocol to status/discrepancy registers (see docs/protocol-design.md), just
    // not published yet.
    static constexpr uint8_t UsedLength = PowerRateLinearOffset + FloatSize;
    static constexpr uint8_t ReservedLength = 0x0C;
    static constexpr uint8_t WindowLength = UsedLength + ReservedLength;
}
