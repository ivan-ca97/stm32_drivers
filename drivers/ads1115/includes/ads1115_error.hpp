#pragma once

#include "base/error.h"

// Errors of Ads1115, beyond the Adc interface's.
namespace ads1115
{
    enum class Code
    {
        Busy,
    };

    class Category final : public base::ErrorCategory
    {
        public:
            const char* name() const override { return "ads1115"; }

            const char* describe(int code) const override
            {
                switch(static_cast<Code>(code))
                {
                    case Code::Busy: return "an acquisition is already running";
                }
                return "unknown error";
            }
    };

    inline constexpr Category errorCategory{};

    inline constexpr base::Error busy{errorCategory, static_cast<int>(Code::Busy)};
}
