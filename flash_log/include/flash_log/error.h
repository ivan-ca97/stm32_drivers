#pragma once

#include "base/error.h"

namespace flash_log {

enum class Code {
    MissingPart,
    PageTooSmall,
    KeyOutOfRange,
    TooLarge,
    Full,
    BufferTooSmall,
    CompactionForbidden,
};

class Category final : public base::ErrorCategory {
public:
    const char* name() const override { return "flash_log"; }

    const char* describe(int code) const override
    {
        switch (static_cast<Code>(code)) {
        case Code::MissingPart:
            return "a required part was not given";
        case Code::PageTooSmall:
            return "the pages cannot hold a header and a record";
        case Code::KeyOutOfRange:
            return "no such key";
        case Code::TooLarge:
            return "the data does not fit in a record";
        case Code::Full:
            return "the page is full and compaction is not allowed";
        case Code::BufferTooSmall:
            return "the value does not fit in the buffer";
        case Code::CompactionForbidden:
            return "compaction is forbidden";
        }
        return "unknown error";
    }
};

inline constexpr Category errorCategory{};

inline constexpr base::Error missingPart{errorCategory, static_cast<int>(Code::MissingPart)};
inline constexpr base::Error pageTooSmall{errorCategory, static_cast<int>(Code::PageTooSmall)};
inline constexpr base::Error keyOutOfRange{errorCategory, static_cast<int>(Code::KeyOutOfRange)};
inline constexpr base::Error tooLarge{errorCategory, static_cast<int>(Code::TooLarge)};
inline constexpr base::Error full{errorCategory, static_cast<int>(Code::Full)};
inline constexpr base::Error bufferTooSmall{errorCategory, static_cast<int>(Code::BufferTooSmall)};
inline constexpr base::Error compactionForbidden{
    errorCategory,
    static_cast<int>(Code::CompactionForbidden),
};

} // namespace flash_log
