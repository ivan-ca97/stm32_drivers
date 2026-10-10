#pragma once

#include <stdint.h>

#include "base/result.h"
#include "watchdog_error.hpp"

// Watchdog — independent watchdog (IWDG): resets the MCU unless kick() is called
// often enough.
// Clocked by the internal low-speed oscillator, which is independent of the crystal
// and PLL that drive the CPU: it still resets the MCU if those fail.
//
// Once started it cannot be stopped — that is the point: code that hangs cannot
// disable its own supervision. Its period can still be changed by code that means to,
// for an operation known to block longer than usual.
//
// Timeouts are approximate: the LSI is specified as 17-47 kHz (32 nominal), so the
// real period can be about a third shorter than requested. Kick several times per
// period — sizing the interval against the requested timeout risks resetting a
// healthy system.
class Watchdog
{
    public:
        // Start supervising with a timeout of `timeoutMs`. Fails with timeoutOutOfRange,
        // and then the watchdog is not started.
        base::Status start(uint32_t timeoutMs);

        // Change the timeout of a running watchdog, and restart the countdown with it.
        // Fails with notRunning or timeoutOutOfRange, and then the previous one stays.
        base::Status setTimeout(uint32_t timeoutMs);

        // Restart the countdown. Call periodically from the supervised loop.
        void kick();

        // Whether the last MCU reset was caused by the watchdog (i.e. something hung).
        // Reading it clears the flag, so it reports a given reset only once.
        bool resetWasCausedByWatchdog();

        // Timeout range, given the nominal LSI frequency. Requests are rounded down
        // to a multiple of 8 ms, the period of one counter tick.
        static constexpr uint32_t MinTimeoutMs = 8;
        static constexpr uint32_t MaxTimeoutMs = 32000;

    private:
        bool running = false;

        // Writes PR/RLR and waits for them to latch: needs the watchdog started.
        void configure(uint32_t timeoutMs);
};
