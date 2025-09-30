#include "serial_authentication.hpp"

#include <stdexcept>

SerialAuthentication::SerialAuthentication(Authentication& authentication, std::span<char> usernameBuffer, std::span<char> nameBuffer, std::span<char> passwordBuffer, std::span<char> password2Buffer)
    : authentication(&authentication), currentUsername(usernameBuffer), currentName(nameBuffer), currentPassword(passwordBuffer), currentPassword2(password2Buffer)
{

}

uint8_t SerialAuthentication::authenticateNextByte(uint8_t byte)
{
    State previousState = state;
    switch(operation)
    {
        case Operation::Idle:
            break;

        case Operation::LogIn:
            logIn(byte);
            break;
        case Operation::LogOut:
            logOut(byte);
            break;
        case Operation::CreateUser:
            createUser(byte);
            break;
        case Operation::DeleteUser:
            deleteUser(byte);
            break;

        case Operation::ModifyOwnUsername:
            modifyOwnUsername(byte);
            break;
        case Operation::ModifyOwnPassword:
            modifyOwnPassword(byte);
            break;
        case Operation::ModifyOwnName:
            modifyOwnName(byte);
            break;

        case Operation::ModifyUsername:
            modifyUsername(byte);
            break;
        case Operation::ModifyPassword:
            modifyPassword(byte);
            break;
        case Operation::ModifyName:
            modifyName(byte);
            break;
        case Operation::ModifyPermission:
            modifyPermission(byte);
            break;
    }

    if(previousState != state)
        currentByteIndex = 0;

    if(error != Error::None)
    {
        state = State::Error;
        currentByteIndex = 0;
    }

    return 0;
}


void SerialAuthentication::setOperation(Operation newOperation)
{
    operation = newOperation;
    error = Error::None;
    currentByteIndex = 0;
    switch(operation)
    {
        case Operation::Idle:
            break;

        case Operation::LogIn:
            state = State::ReadingUser;
            break;

        case Operation::LogOut:
            state = State::ReadingToken;
            break;

        case Operation::CreateUser:
            state = State::ReadingToken;
            break;

        case Operation::DeleteUser:
            state = State::ReadingToken;
            break;

        case Operation::ModifyOwnUsername:
            state = State::ReadingToken;
            break;

        case Operation::ModifyOwnPassword:
            state = State::ReadingToken;
            break;

        case Operation::ModifyOwnName:
            state = State::ReadingToken;
            break;

        case Operation::ModifyUsername:
            state = State::ReadingToken;
            break;

        case Operation::ModifyPassword:
            state = State::ReadingToken;
            break;

        case Operation::ModifyName:
            state = State::ReadingToken;
            break;

        case Operation::ModifyPermission:
            state = State::ReadingToken;
            break;
    }
}

bool SerialAuthentication::writeUsernameByte(uint8_t byte)
{
    if(currentByteIndex >= currentUsername.size())
    {
        error = Error::UsernameOverflow;
        return true;
    }

    currentUsername[currentByteIndex++] = byte;
    if(byte == '\0')
        return true;

    return false;
}

bool SerialAuthentication::writePasswordByte(uint8_t byte)
{
    if(currentByteIndex >= currentPassword.size())
    {
        error = Error::UsernameOverflow;
        return true;
    }

    currentPassword[currentByteIndex++] = byte;
    if(byte == '\0')
        return true;

    return false;
}

bool SerialAuthentication::writePassword2Byte(uint8_t byte)
{
    if(currentByteIndex >= currentPassword2.size())
    {
        error = Error::UsernameOverflow;
        return true;
    }

    currentPassword2[currentByteIndex++] = byte;
    if(byte == '\0')
        return true;

    return false;
}

bool SerialAuthentication::writeNameByte(uint8_t byte)
{
    if(currentByteIndex >= currentName.size())
    {
        error = Error::UsernameOverflow;
        return true;
    }

    currentName[currentByteIndex++] = byte;
    if(byte == '\0')
        return true;

    return false;
}

bool SerialAuthentication::writeTokenByte(uint8_t byte)
{
    if (currentByteIndex < sizeof(currentToken))
        reinterpret_cast<uint8_t*>(&currentToken)[currentByteIndex++] = byte;

    return currentByteIndex >= sizeof(currentToken);
}

bool SerialAuthentication::writeIdByte(uint8_t byte)
{
    if(currentByteIndex < sizeof(currentId))
        reinterpret_cast<uint8_t*>(&currentId)[currentByteIndex++] = byte;

    return currentByteIndex >= sizeof(currentId);
}

bool SerialAuthentication::writePermissionByte(uint8_t byte)
{
    if(currentByteIndex < sizeof(currentPermissionId))
        reinterpret_cast<uint8_t*>(&currentPermissionId)[currentByteIndex++] = byte;

    return currentByteIndex >= sizeof(currentPermissionId);
}

uint8_t SerialAuthentication::getNextTokenByte()
{
    if(state != State::TokenReady)
        throw std::logic_error("Token not ready.");

    uint8_t byte = reinterpret_cast<uint8_t*>(&currentToken)[currentByteIndex++];

    if(currentByteIndex >= sizeof(TokenType))
        state = State::None;

    return byte;
}