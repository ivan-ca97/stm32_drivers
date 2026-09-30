#pragma once

#include "base/error.h"

// Errors of RegisterBank.
namespace register_bank
{
    enum class Code
    {
        OutOfBank,
        EmptySpan,
        Overlap,
        TooManyWindows,
    };

    class Category final : public base::ErrorCategory
    {
        public:
            const char* name() const override { return "register_bank"; }

            const char* describe(int code) const override
            {
                switch(static_cast<Code>(code))
                {
                    case Code::OutOfBank: return "span does not fit in the bank";
                    case Code::EmptySpan: return "zero-length reservation";
                    case Code::Overlap: return "span overlaps a reserved one";
                    case Code::TooManyWindows: return "reservation table is full";
                }
                return "unknown error";
            }
    };

    inline constexpr Category errorCategory{};

    inline constexpr base::Error outOfBank{errorCategory, static_cast<int>(Code::OutOfBank)};
    inline constexpr base::Error emptySpan{errorCategory, static_cast<int>(Code::EmptySpan)};
    inline constexpr base::Error overlap{errorCategory, static_cast<int>(Code::Overlap)};
    inline constexpr base::Error tooManyWindows{
        errorCategory,
        static_cast<int>(Code::TooManyWindows),
    };
}
