#pragma once

#include <cstdint>

namespace stm32f4 {

// Holds off every interrupt with the given NVIC preemption priority or a less urgent one, from
// construction to destruction. Nested guards only ever raise the mask, and each one, when
// destroyed, puts back the mask that was in place when it was built. Priority 0 masks nothing.
class InterruptGuard {
public:
    explicit InterruptGuard(uint32_t priority);
    ~InterruptGuard();

    InterruptGuard(const InterruptGuard&) = delete;
    InterruptGuard& operator=(const InterruptGuard&) = delete;

private:
    uint32_t previous_;
};

} // namespace stm32f4
