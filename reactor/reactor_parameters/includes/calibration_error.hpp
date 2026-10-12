#pragma once

#include "base/error.h"

// Errors of CalibrationStorage, beyond those of its backend.
namespace calibration
{
    enum class Code
    {
        NoneStored,
        Malformed,
    };

    class Category final : public base::ErrorCategory
    {
        public:
            const char* name() const override { return "calibration"; }

            const char* describe(int code) const override
            {
                switch(static_cast<Code>(code))
                {
                    case Code::NoneStored: return "no calibration has been stored";
                    case Code::Malformed: return "the stored calibration does not decode";
                }
                return "unknown error";
            }
    };

    inline constexpr Category errorCategory{};

    inline constexpr base::Error noneStored{errorCategory, static_cast<int>(Code::NoneStored)};
    inline constexpr base::Error malformed{errorCategory, static_cast<int>(Code::Malformed)};
}
