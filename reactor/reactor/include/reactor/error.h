#pragma once

#include "base/error.h"

namespace reactor {

enum class Code {
    MissingPart,
    InvalidOpcodeRange,
    OverlappingSections,
    TooManySections,
};

class Category final : public base::ErrorCategory {
public:
    const char* name() const override { return "reactor"; }

    const char* describe(int code) const override
    {
        switch (static_cast<Code>(code)) {
        case Code::MissingPart:
            return "a required part was not given";
        case Code::InvalidOpcodeRange:
            return "opcode range ends before it starts";
        case Code::OverlappingSections:
            return "two sections claim the same opcode";
        case Code::TooManySections:
            return "more sections than the router holds";
        }
        return "unknown error";
    }
};

inline constexpr Category errorCategory{};

inline constexpr base::Error missingPart{errorCategory, static_cast<int>(Code::MissingPart)};
inline constexpr base::Error invalidOpcodeRange{
    errorCategory,
    static_cast<int>(Code::InvalidOpcodeRange),
};
inline constexpr base::Error overlappingSections{
    errorCategory,
    static_cast<int>(Code::OverlappingSections),
};
inline constexpr base::Error tooManySections{
    errorCategory,
    static_cast<int>(Code::TooManySections),
};

} // namespace reactor
