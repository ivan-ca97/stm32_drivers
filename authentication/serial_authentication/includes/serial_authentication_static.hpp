#pragma once
#include "serial_authentication.hpp"
#include "serial_authentication_builder.hpp"

template <size_t BuffersSize>
class SerialAuthenticationStatic : public SerialAuthentication
{
    protected:
        std::array<char, BuffersSize> usernameBuffer, password1Buffer, password2Buffer, nameBuffer;

    public:
        SerialAuthenticationStatic(Authentication& authentication) :
            SerialAuthentication(
                SerialAuthentication::Builder()
                    .setAuthentication(authentication)
                    .setUsernameBuffer(usernameBuffer)
                    .setPasswordBuffer(password1Buffer)
                    .setPassword2Buffer(password2Buffer)
                    .setNameBuffer(nameBuffer)
                    .build()
            )
        {}
};