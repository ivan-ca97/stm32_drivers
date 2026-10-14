#pragma once

#include <cstdint>

#include "stm32f4/stall_listener.h"
#include "watchdog.hpp"

namespace application {

// Stretches the watchdog across a stall of the CPU, which nothing can kick through.
class WatchdogStretcher final : public stm32f4::StallListener {
public:
    WatchdogStretcher(Watchdog& watchdog, uint32_t timeoutMs, uint32_t stallTimeoutMs)
        : watchdog_(watchdog), timeoutMs_(timeoutMs), stallTimeoutMs_(stallTimeoutMs)
    {
    }

    WatchdogStretcher(const WatchdogStretcher&) = delete;
    WatchdogStretcher& operator=(const WatchdogStretcher&) = delete;

    // Before the watchdog starts, setTimeout() fails with notRunning: nothing to stretch.
    void stallStarting() override { (void)watchdog_.setTimeout(stallTimeoutMs_); }
    void stallEnded() override { (void)watchdog_.setTimeout(timeoutMs_); }

private:
    Watchdog& watchdog_;
    uint32_t timeoutMs_;
    uint32_t stallTimeoutMs_;
};

} // namespace application
