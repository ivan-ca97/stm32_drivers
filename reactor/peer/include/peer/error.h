#pragma once

#include "base/error.h"

namespace peer {

enum class Code {
    Busy,
    NoData,
    InvalidIndex,
    VerifyFailed,
    MissingPart,
};

class Category final : public base::ErrorCategory {
public:
    const char* name() const override { return "peer"; }

    const char* describe(int code) const override
    {
        switch (static_cast<Code>(code)) {
        case Code::Busy:
            return "a read is already running";
        case Code::NoData:
            return "no read has completed yet";
        case Code::InvalidIndex:
            return "no parameter at that index";
        case Code::VerifyFailed:
            return "the register read back differs from what was written";
        case Code::MissingPart:
            return "a required part was not given";
        }
        return "unknown error";
    }
};

inline constexpr Category errorCategory{};

inline constexpr base::Error busy{errorCategory, static_cast<int>(Code::Busy)};
inline constexpr base::Error noData{errorCategory, static_cast<int>(Code::NoData)};
inline constexpr base::Error invalidIndex{errorCategory, static_cast<int>(Code::InvalidIndex)};
inline constexpr base::Error verifyFailed{errorCategory, static_cast<int>(Code::VerifyFailed)};
inline constexpr base::Error missingPart{errorCategory, static_cast<int>(Code::MissingPart)};

} // namespace peer
