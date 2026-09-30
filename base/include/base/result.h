#pragma once

#include <expected>
#include <utility>

#include "base/error.h"

namespace base {

// The return type of anything that can fail. Discarding one is a compile error.
template <typename T>
class [[nodiscard]] Result : public std::expected<T, Error> {
public:
    using std::expected<T, Error>::expected;

    // Converts what monadic operations return (a std::expected) back to a Result.
    constexpr Result(std::expected<T, Error> other)
        : std::expected<T, Error>(std::move(other))
    {
    }
};

// A Result that carries no value: either success or an Error.
using Status = Result<void>;

// The successful Status. Reads at a return site where `{}` does not.
inline constexpr Status ok{};

} // namespace base
