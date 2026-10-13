#pragma once

namespace stm32f4 {

// Told when the CPU is about to stall on a flash operation, and when it runs again. Called
// on either side of the stall, from the code that causes it.
class StallListener {
public:
    virtual ~StallListener() = default;

    virtual void stallStarting() = 0;
    virtual void stallEnded() = 0;
};

} // namespace stm32f4
