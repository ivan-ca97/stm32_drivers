#pragma once
#include "serial_authentication.hpp"

struct SerialAuthentication::Configuration
{
    Authentication* authentication = nullptr;
    std::span<char> usernameBuffer;
    std::span<char> nameBuffer;
    std::span<char> passwordBuffer;
    std::span<char> password2Buffer;
};

class SerialAuthentication::Builder
{
    public:
        Builder& setAuthentication(Authentication& authentication);
        Builder& setUsernameBuffer(std::span<char> usernameBuffer);
        Builder& setNameBuffer(std::span<char> nameBuffer);
        Builder& setPasswordBuffer(std::span<char> passwordBuffer);
        Builder& setPassword2Buffer(std::span<char> password2Buffer);

        SerialAuthentication build();

    private:
        Configuration configuration;
};