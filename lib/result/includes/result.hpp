#pragma once

#include <expected>

namespace lib
{
    template <typename T, typename E>
    class [[nodiscard]] Result : public std::expected<T, E>
    {
        using std::expected<T, E>::expected;
    };

    template <typename E>
    constexpr auto Error(E e)
    {
        return std::unexpected(e);
    }
}
