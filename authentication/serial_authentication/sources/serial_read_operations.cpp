#include "serial_authentication.hpp"

void SerialAuthentication::readingToken(uint8_t byte, State nextState, Permission permissionNeeded)
{
    bool done = setTokenByte(byte);
    if(!done)
        return;

    const Session* session = authentication->validate(currentToken);
    if(!session)
    {
        error = Error::TokenInvalid;
        return;
    }

    // Don't check if no permission is needed
    if(permissionNeeded != Permission::None)
        if(!session->getUser()->hasPermission(permissionNeeded))
        {
            error = Error::UserNoPermission;
            return;
        }

    state = nextState;
}

bool SerialAuthentication::readingUser(uint8_t byte, State nextState)
{
    bool done = setUsernameByte(byte);
    if(!done)
        return false;

    // These states shouldn't have a repeated username.
    switch(operation)
    {
        case Operation::ModifyOwnUsername:
        case Operation::ModifyUsername:
        case Operation::CreateUser:
            if(authentication->getUserManager()->getUser(currentUsername.data()))
                error = Error::UsernameAlreadyExists;
            break;
        default:
            break;
    }

    state = nextState;
    return true;
}

bool SerialAuthentication::readingPassword(uint8_t byte, State nextState)
{
    bool done = setPasswordByte(byte);
    if(!done)
        return false;

    state = nextState;
    return true;
}

bool SerialAuthentication::readingPassword2(uint8_t byte, State nextState)
{
    bool done = setPassword2Byte(byte);
    if(!done)
        return false;

    state = nextState;
    return true;
}

bool SerialAuthentication::readingName(uint8_t byte, State nextState)
{
    bool done = setNameByte(byte);
    if(!done)
        return false;

    state = nextState;
    return true;
}

bool SerialAuthentication::readingUserId(uint8_t byte, State nextState)
{
    bool done = setIdByte(byte);
    if(!done)
        return false;

    state = nextState;
    return true;
}

bool SerialAuthentication::readingPermission(uint8_t byte, State nextState)
{
    bool done = setPermissionByte(byte);
    if(!done)
        return false;

    switch(currentPermissionId)
    {
        case 0:
            currentPermission = Permission::None;
            break;
        case 1:
            currentPermission = Permission::Observer;
            break;
        case 2:
            currentPermission = Permission::Maintenance;
            break;
        case 3:
            currentPermission = Permission::Superuser;
            break;
        default:
            error = Error::PermissionIdInvalid;
            break;
    }

    state = nextState;
    return true;
}

void SerialAuthentication::logOut(uint8_t byte)
{
    switch(state)
    {
        case State::ReadingToken:
            if(!setTokenByte(byte))
                break;

            try
            {
                authentication->logOut(currentToken);
            }
            catch(...)
            {
                error = Error::TokenInvalid;
            }

            state = State::SendingErrorCode;
            break;

        default:
            break;
    }
}

void SerialAuthentication::logIn(uint8_t byte)
{
    switch(state)
    {
        case State::ReadingUser:
            readingUser(byte, State::ReadingPassword);
            break;

        case State::ReadingPassword:
            if(!readingPassword(byte, State::SendingToken))
                break;

            try
            {
                const Session* session = authentication->authenticate(currentUsername.data(), currentPassword.data());
                currentToken = session->getToken();
            }
            catch(...)
            {
                error = Error::AuthenticationError;
            }
            break;

        default:
            break;
    }
}

void SerialAuthentication::createUser(uint8_t byte)
{
    switch(state)
    {
        case State::ReadingToken:
            readingToken(byte, State::ReadingUser, Permission::Superuser);
            break;

        case State::ReadingUser:
            readingUser(byte, State::ReadingPassword);
            break;

        case State::ReadingPassword:
            readingPassword(byte, State::ReadingPermission);
            break;

        case State::ReadingPermission:
            if(!readingPermission(byte, State::SendingId))
                break;

            try
            {
                currentId = authentication->createUser(currentToken, currentPermission, currentUsername.data(), currentPassword.data(), "");
            }
            catch(...)
            {
                error = Error::InternalError;
            }
            break;

        default:
            break;
    }

}

void SerialAuthentication::deleteUser(uint8_t byte)
{
    switch(state)
    {
        case State::ReadingToken:
            readingToken(byte, State::ReadingUserId, Permission::Superuser);
            break;

        case State::ReadingUserId:
            if(!readingUserId(byte, State::None))
                break;

            try
            {
                authentication->deleteUser(currentToken, currentId);
            }
            catch(...)
            {
                error = Error::InternalError;
            }
            break;

        default:
            break;
    }
}

void SerialAuthentication::modifyOwnUsername(uint8_t byte)
{
    switch(state)
    {
        case State::ReadingToken:
            readingToken(byte, State::ReadingUser);
            break;

        case State::ReadingUser:
            if(!readingUser(byte, State::None))
                break;

            try
            {
                authentication->modifyOwnUsername(currentToken, currentUsername.data());
            }
            catch(...)
            {
                error = Error::InternalError;
            }
            break;

        default:
            break;
    }
}

void SerialAuthentication::modifyOwnPassword(uint8_t byte)
{
    switch(state)
    {
        case State::ReadingToken:
            readingToken(byte, State::ReadingPassword);
            break;

        case State::ReadingPassword:
            readingPassword(byte, State::ReadingPassword2);
            break;

        case State::ReadingPassword2:
            if(!readingPassword2(byte, State::None))
                break;

            try
            {
                authentication->modifyOwnPassword(currentToken, currentPassword.data(), currentPassword2.data());
            }
            catch(...)
            {
                error = Error::InternalError;
            }
            break;

        default:
            break;
    }
}

void SerialAuthentication::modifyOwnName(uint8_t byte)
{
    switch(state)
    {
        case State::ReadingToken:
            readingToken(byte, State::ReadingName);
            break;

        case State::ReadingName:
            if(!readingName(byte, State::None))
                break;

            try
            {
                authentication->modifyOwnName(currentToken, currentName.data());
            }
            catch(...)
            {
                error = Error::InternalError;
            }
            break;

        default:
            break;
    }
}

void SerialAuthentication::modifyUsername(uint8_t byte)
{
    switch(state)
    {
        case State::ReadingToken:
            readingToken(byte, State::ReadingUserId, Permission::Superuser);
            break;

        case State::ReadingUserId:
            if(!readingUserId(byte, State::ReadingUser))
                break;

            if(!authentication->getUserManager()->getUser(currentId))
                error = Error::UserIdDoesNotExist;
            break;

        case State::ReadingUser:
            if(!readingUser(byte, State::None))
                break;

            try
            {
                authentication->modifyUsername(currentToken, currentId, currentUsername.data());
            }
            catch(...)
            {
                error = Error::InternalError;
            }
            break;

        default:
            break;
    }
}

void SerialAuthentication::modifyPassword(uint8_t byte)
{
    switch(state)
    {
        case State::ReadingToken:
            readingToken(byte, State::ReadingUserId, Permission::Superuser);
            break;

        case State::ReadingUserId:
            if(!readingUserId(byte, State::ReadingPassword))
                break;

            if(!authentication->getUserManager()->getUser(currentId))
                error = Error::UserIdDoesNotExist;
            break;

        case State::ReadingPassword:
            if(!readingPassword(byte, State::None))
                break;

            try
            {
                authentication->modifyPassword(currentToken, currentId, currentPassword.data());
            }
            catch(...)
            {
                error = Error::InternalError;
            }
            break;

        default:
            break;
    }
}

void SerialAuthentication::modifyName(uint8_t byte)
{
    switch(state)
    {
        case State::ReadingToken:
            readingToken(byte, State::ReadingUserId, Permission::Superuser);
            break;

        case State::ReadingUserId:
            if(!readingUserId(byte, State::ReadingName))
                break;

            if(!authentication->getUserManager()->getUser(currentId))
                error = Error::UserIdDoesNotExist;
            break;

        case State::ReadingName:
            if(!readingName(byte, State::None))
                break;

            try
            {
                authentication->modifyName(currentToken, currentId, currentName.data());
            }
            catch(const std::exception& e)
            {
                error = Error::InternalError;
            }
            break;

        default:
            break;
    }
}

void SerialAuthentication::modifyPermission(uint8_t byte)
{
    switch(state)
    {
        case State::ReadingToken:
            readingToken(byte, State::ReadingUserId, Permission::Superuser);
            break;

        case State::ReadingUserId:
            if(!readingUserId(byte, State::ReadingPermission))
                break;

            if(!authentication->getUserManager()->getUser(currentId))
                error = Error::UserIdDoesNotExist;
            break;

        case State::ReadingPermission:
            if(!readingPermission(byte, State::None))
                break;

            try
            {
                authentication->modifyPermission(currentToken, currentId, currentPermission);
            }
            catch(const std::exception& e)
            {
                error = Error::InternalError;
            }
            break;

        default:
            break;
    }
}