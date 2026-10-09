#pragma once

#include <cstdint>

#include "clock.hpp"

namespace stm32f4 {

// Milliseconds since start-up, from the HAL's SysTick count.
class SysTickClock final : public Clock {
public:
    uint32_t milliseconds() const override;
};

} // namespace stm32f4
