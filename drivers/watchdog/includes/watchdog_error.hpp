#pragma once

#include "base/error.h"

// Errors of Watchdog.
namespace watchdog
{
    enum class Code
    {
        TimeoutOutOfRange,
        NotRunning,
    };

    class Category final : public base::ErrorCategory
    {
        public:
            const char* name() const override { return "watchdog"; }

            const char* describe(int code) const override
            {
                switch(static_cast<Code>(code))
                {
                    case Code::TimeoutOutOfRange: return "timeout outside what the hardware can express";
                    case Code::NotRunning: return "the watchdog has not been started";
                }
                return "unknown error";
            }
    };

    inline constexpr Category errorCategory{};

    inline constexpr base::Error timeoutOutOfRange{
        errorCategory,
        static_cast<int>(Code::TimeoutOutOfRange),
    };
    inline constexpr base::Error notRunning{errorCategory, static_cast<int>(Code::NotRunning)};
}
