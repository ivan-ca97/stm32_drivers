#pragma once

#include "base/error.h"

namespace safety {

enum class Code {
    MissingPart,
};

class Category final : public base::ErrorCategory {
public:
    const char* name() const override { return "safety"; }

    const char* describe(int code) const override
    {
        switch (static_cast<Code>(code)) {
        case Code::MissingPart:
            return "a required part was not given";
        }
        return "unknown error";
    }
};

inline constexpr Category errorCategory{};

inline constexpr base::Error missingPart{errorCategory, static_cast<int>(Code::MissingPart)};

} // namespace safety
