#include "serial_authentication_builder.hpp"

#include <stdexcept>

SerialAuthentication::Builder& SerialAuthentication::Builder::setAuthentication(Authentication& authentication)
{
    configuration.authentication = &authentication;
    return *this;
}

SerialAuthentication::Builder& SerialAuthentication::Builder::setUsernameBuffer(std::span<char> usernameBuffer)
{
    configuration.usernameBuffer = usernameBuffer;
    return *this;
}

SerialAuthentication::Builder& SerialAuthentication::Builder::setNameBuffer(std::span<char> nameBuffer)
{
    configuration.nameBuffer = nameBuffer;
    return *this;
}

SerialAuthentication::Builder& SerialAuthentication::Builder::setPasswordBuffer(std::span<char> passwordBuffer)
{
    configuration.passwordBuffer = passwordBuffer;
    return *this;
}

SerialAuthentication::Builder& SerialAuthentication::Builder::setPassword2Buffer(std::span<char> password2Buffer)
{
    configuration.password2Buffer = password2Buffer;
    return *this;
}

SerialAuthentication SerialAuthentication::Builder::build()
{
    if(
        configuration.authentication == nullptr ||
        configuration.usernameBuffer.empty() ||
        configuration.nameBuffer.empty() ||
        configuration.passwordBuffer.empty() ||
        configuration.password2Buffer.empty()
    )
        throw std::logic_error("Incomplete configuration.");

    return SerialAuthentication(configuration);
}