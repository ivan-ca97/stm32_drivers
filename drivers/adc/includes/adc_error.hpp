#pragma once

#include "base/error.h"

// Errors of the Adc interface.
namespace adc
{
    enum class Code
    {
        InvalidChannel,
        NoData,
    };

    class Category final : public base::ErrorCategory
    {
        public:
            const char* name() const override { return "adc"; }

            const char* describe(int code) const override
            {
                switch(static_cast<Code>(code))
                {
                    case Code::InvalidChannel: return "no such channel";
                    case Code::NoData: return "no reading for the channel";
                }
                return "unknown error";
            }
    };

    inline constexpr Category errorCategory{};

    inline constexpr base::Error invalidChannel{
        errorCategory,
        static_cast<int>(Code::InvalidChannel),
    };
    inline constexpr base::Error noData{errorCategory, static_cast<int>(Code::NoData)};
}
