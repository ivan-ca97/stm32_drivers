#include "serial_authentication.hpp"
#include "serial_authentication_builder.hpp"

#include <stdexcept>
#include <utility>

SerialAuthentication::SerialAuthentication(const Configuration& configuration) :
    authentication(configuration.authentication),
    currentUsername(configuration.usernameBuffer),
    currentPassword(configuration.passwordBuffer),
    currentPassword2(configuration.password2Buffer),
    currentName(configuration.nameBuffer)
{

}

void SerialAuthentication::authenticateNextByte(uint8_t byte)
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

    if(error != Error::None)
        state = State::Error;

    if(previousState != state)
        currentByteIndex = 0;
}

uint8_t SerialAuthentication::getNextByte()
{
    uint8_t byte = 0;
    State previousState = state;

    // Error handling, common for all operations.
    if(state == State::Error)
        if(writesUntilErrorCode-- == 0)
            state = State::SendingErrorCode;

    if(state == State::SendingErrorCode)
    {
        state = State::None;
        return std::to_underlying(error);
    }

    switch(operation)
    {
        case Operation::Idle:
            break;

        case Operation::LogIn:
            byte = logIn();
            break;
        case Operation::LogOut:
            // Error code only.
            break;
        case Operation::CreateUser:
            byte = createUser();
            break;
        case Operation::DeleteUser:
            break;

        case Operation::ModifyOwnUsername:
            break;
        case Operation::ModifyOwnPassword:
            break;
        case Operation::ModifyOwnName:
            break;

        case Operation::ModifyUsername:
            break;
        case Operation::ModifyPassword:
            break;
        case Operation::ModifyName:
            break;
        case Operation::ModifyPermission:
            break;
    }


    if(error != Error::None)
        state = State::Error;

    if(previousState != state)
        currentByteIndex = 0;

    return byte;
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
            writesUntilErrorCode = sizeof(Session::TokenType);
            state = State::ReadingUser;
            break;

        case Operation::LogOut:
            writesUntilErrorCode = 0;
            state = State::ReadingToken;
            break;

        case Operation::CreateUser:
            writesUntilErrorCode = sizeof(User::IdType);
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

bool SerialAuthentication::setUsernameByte(uint8_t byte)
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

bool SerialAuthentication::setPasswordByte(uint8_t byte)
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

bool SerialAuthentication::setPassword2Byte(uint8_t byte)
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

bool SerialAuthentication::setNameByte(uint8_t byte)
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

bool SerialAuthentication::setIdByte(uint8_t byte)
{
    if(currentByteIndex < sizeof(currentId))
        reinterpret_cast<uint8_t*>(&currentId)[currentByteIndex++] = byte;

    return currentByteIndex >= sizeof(currentId);
}

bool SerialAuthentication::setPermissionByte(uint8_t byte)
{
    if(currentByteIndex < sizeof(currentPermissionId))
        reinterpret_cast<uint8_t*>(&currentPermissionId)[currentByteIndex++] = byte;

    return currentByteIndex >= sizeof(currentPermissionId);
}

bool SerialAuthentication::setTokenByte(uint8_t byte)
{
    if(currentByteIndex < sizeof(currentToken))
        reinterpret_cast<uint8_t*>(&currentToken)[currentByteIndex++] = byte;

    return currentByteIndex >= sizeof(currentToken);
}

bool SerialAuthentication::getIdByte(uint8_t* byte)
{
    if(currentByteIndex < sizeof(currentId))
        *byte = reinterpret_cast<uint8_t*>(&currentId)[currentByteIndex++];

    return currentByteIndex >= sizeof(currentId);
}

bool SerialAuthentication::getTokenByte(uint8_t* byte)
{
    if(currentByteIndex < sizeof(currentToken))
        *byte = reinterpret_cast<uint8_t*>(&currentToken)[currentByteIndex++];

    return currentByteIndex >= sizeof(currentToken);
}
