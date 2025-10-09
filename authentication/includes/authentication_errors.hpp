#pragma once

#include <expected>
#include <optional>

enum class AuthenticationError
{
    UserIdNotFound,
    UsernameNotFound,
    UsernameAlreadyExists,

    InvalidToken,
    InsufficientPermissions,
    IncorrectPassword,

    UsersBufferFull,
    SessionBufferFull,

    Overflow,
    UsernameBufferOverflow,
    NameBufferOverflow,
    PasswordBufferOverflow,

    EmptyMandatoryField,
    IntegrityFailure,
};

template <typename T>
class [[nodiscard]] Result : public std::expected<T, AuthenticationError>
{
    using std::expected<T, AuthenticationError>::expected;
};

using ResultVoid = Result<void>;

template <typename E>
constexpr auto Error(E e)
{
    return std::unexpected(e);
}
