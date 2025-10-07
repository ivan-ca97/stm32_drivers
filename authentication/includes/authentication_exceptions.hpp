#include <stdexcept>

class BufferFullError : public std::runtime_error
{
    public:
        explicit BufferFullError(const std::string& msg)
            : std::runtime_error(msg) {}
};

class AuthenticationError : public std::runtime_error
{
    public:
        explicit AuthenticationError(const std::string& msg)
            : std::runtime_error(msg) {}
};

class UserNotFoundError : public std::runtime_error
{
    public:
        explicit UserNotFoundError(const std::string& msg)
            : std::runtime_error(msg) {}
};

class UsernameAlreadyExistsError : public std::runtime_error
{
    public:
        explicit UsernameAlreadyExistsError(const std::string& msg)
            : std::runtime_error(msg) {}
};

class InvalidTokenError : public std::runtime_error
{
    public:
        explicit InvalidTokenError(const std::string& msg)
            : std::runtime_error(msg) {}
};

class InsuficientPermissionsError : public std::runtime_error
{
    public:
        explicit InsuficientPermissionsError(const std::string& msg)
            : std::runtime_error(msg) {}
};

class IntegrityError : public std::runtime_error
{
    public:
        explicit IntegrityError(const std::string& msg)
            : std::runtime_error(msg) {}
};

class InsuficientDataError : public std::runtime_error
{
    public:
        explicit InsuficientDataError(const std::string& msg)
            : std::runtime_error(msg) {}
};