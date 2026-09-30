#pragma once

#include "base/error.h"
#include "base/result.h"

// Errors of the authentication component.
namespace authentication
{
    enum class Code
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

    class Category final : public base::ErrorCategory
    {
        public:
            const char* name() const override { return "authentication"; }

            const char* describe(int code) const override
            {
                switch(static_cast<Code>(code))
                {
                    case Code::UserIdNotFound: return "user id not found";
                    case Code::UsernameNotFound: return "username not found";
                    case Code::UsernameAlreadyExists: return "username already exists";
                    case Code::InvalidToken: return "invalid token";
                    case Code::InsufficientPermissions: return "insufficient permissions";
                    case Code::IncorrectPassword: return "incorrect password";
                    case Code::UsersBufferFull: return "users buffer full";
                    case Code::SessionBufferFull: return "session buffer full";
                    case Code::Overflow: return "overflow";
                    case Code::UsernameBufferOverflow: return "username too long";
                    case Code::NameBufferOverflow: return "name too long";
                    case Code::PasswordBufferOverflow: return "password too long";
                    case Code::EmptyMandatoryField: return "empty mandatory field";
                    case Code::IntegrityFailure: return "integrity failure";
                }
                return "unknown error";
            }
    };

    inline constexpr Category errorCategory{};

    inline constexpr base::Error userIdNotFound{
        errorCategory,
        static_cast<int>(Code::UserIdNotFound),
    };
    inline constexpr base::Error usernameNotFound{
        errorCategory,
        static_cast<int>(Code::UsernameNotFound),
    };
    inline constexpr base::Error usernameAlreadyExists{
        errorCategory,
        static_cast<int>(Code::UsernameAlreadyExists),
    };
    inline constexpr base::Error invalidToken{errorCategory, static_cast<int>(Code::InvalidToken)};
    inline constexpr base::Error insufficientPermissions{
        errorCategory,
        static_cast<int>(Code::InsufficientPermissions),
    };
    inline constexpr base::Error incorrectPassword{
        errorCategory,
        static_cast<int>(Code::IncorrectPassword),
    };
    inline constexpr base::Error usersBufferFull{
        errorCategory,
        static_cast<int>(Code::UsersBufferFull),
    };
    inline constexpr base::Error sessionBufferFull{
        errorCategory,
        static_cast<int>(Code::SessionBufferFull),
    };
    inline constexpr base::Error overflow{errorCategory, static_cast<int>(Code::Overflow)};
    inline constexpr base::Error usernameBufferOverflow{
        errorCategory,
        static_cast<int>(Code::UsernameBufferOverflow),
    };
    inline constexpr base::Error nameBufferOverflow{
        errorCategory,
        static_cast<int>(Code::NameBufferOverflow),
    };
    inline constexpr base::Error passwordBufferOverflow{
        errorCategory,
        static_cast<int>(Code::PasswordBufferOverflow),
    };
    inline constexpr base::Error emptyMandatoryField{
        errorCategory,
        static_cast<int>(Code::EmptyMandatoryField),
    };
    inline constexpr base::Error integrityFailure{
        errorCategory,
        static_cast<int>(Code::IntegrityFailure),
    };
}

template <typename T>
using Result = base::Result<T>;

using ResultVoid = base::Status;
