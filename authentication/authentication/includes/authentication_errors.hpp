#pragma once

#include "result.hpp"

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
using Result = lib::Result<T, AuthenticationError>;

using ResultVoid = Result<void>;

using lib::Error;
