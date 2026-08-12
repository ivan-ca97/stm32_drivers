#pragma once

#include <stdint.h>

// Clock — injected time source. The physical clock (SysTick / RTC / timer) is
// supplied by the application.
class Clock
{
    public:
        virtual ~Clock() = default;

        virtual uint32_t milliseconds() const = 0;

        uint32_t seconds() const { return milliseconds() / 1000; }

        // Time since a stamp taken from this clock. The count wraps (about every 49
        // days in milliseconds).
        uint32_t millisecondsSince(uint32_t stamp) const { return milliseconds() - stamp; }
        uint32_t secondsSince(uint32_t stamp) const { return seconds() - stamp; }
};
