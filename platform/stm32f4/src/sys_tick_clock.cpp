#include <cstdint>

#include "stm32f4xx_hal.h"

#include "stm32f4/sys_tick_clock.h"

namespace stm32f4 {

uint32_t SysTickClock::milliseconds() const
{
    return HAL_GetTick();
}

} // namespace stm32f4
