#include <cstdint>

#include "stm32f4xx.h"

#include "stm32f4/interrupt_guard.h"

namespace stm32f4 {

InterruptGuard::InterruptGuard(uint32_t priority) : previous_(__get_BASEPRI())
{
    __set_BASEPRI_MAX(priority << (8 - __NVIC_PRIO_BITS));
}

InterruptGuard::~InterruptGuard()
{
    __set_BASEPRI(previous_);
}

} // namespace stm32f4
