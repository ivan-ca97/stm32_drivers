#pragma once

#include <cstdint>

#include "reactor_parameters.hpp"

// Register layout of the parameter window, read by every master: the calibration
// in ReactorParams' flat order, one float32 little-endian per value.
namespace peer::parameter_window {

inline constexpr uint8_t valueSize = 4;

inline constexpr uint8_t length = ReactorParams::Count * valueSize;

constexpr uint8_t offset(uint16_t index)
{
    return static_cast<uint8_t>(index * valueSize);
}

static_assert(sizeof(float) == valueSize, "a parameter is a float32 on the wire");

} // namespace peer::parameter_window
