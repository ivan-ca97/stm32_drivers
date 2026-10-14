#pragma once

#include <cstdint>

// Where things sit on the wire: the register bank layout every master reads, and the
// two addresses the boards answer to. The full register map is in the protocol design.
namespace application::layout {

inline constexpr uint8_t measurementsBase = 0x00;
inline constexpr uint8_t parametersBase = 0x20;

inline constexpr uint8_t firstBoardAddress = 0x42;
inline constexpr uint8_t secondBoardAddress = 0x43;

} // namespace application::layout
