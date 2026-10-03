#pragma once

#include <cstdint>

namespace peer {

// What one service() call of a reader reports.
enum class Step : uint8_t {
    Idle,
    Busy,
    Completed,
    Failed,
};

} // namespace peer
