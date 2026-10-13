#pragma once

#include "base/error.h"

namespace stm32f4 {

enum class Code {
    MissingPart,
    SameSector,
    EraseFailed,
    ProgramFailed,
};

class Category final : public base::ErrorCategory {
public:
    const char* name() const override { return "stm32f4"; }

    const char* describe(int code) const override
    {
        switch (static_cast<Code>(code)) {
        case Code::MissingPart:
            return "a required part was not given";
        case Code::SameSector:
            return "both pages were given the same flash sector";
        case Code::EraseFailed:
            return "flash sector erase failed";
        case Code::ProgramFailed:
            return "flash programming failed or did not read back";
        }
        return "unknown error";
    }
};

inline constexpr Category errorCategory{};

inline constexpr base::Error missingPart{errorCategory, static_cast<int>(Code::MissingPart)};
inline constexpr base::Error sameSector{errorCategory, static_cast<int>(Code::SameSector)};
inline constexpr base::Error eraseFailed{errorCategory, static_cast<int>(Code::EraseFailed)};
inline constexpr base::Error programFailed{errorCategory, static_cast<int>(Code::ProgramFailed)};

} // namespace stm32f4
