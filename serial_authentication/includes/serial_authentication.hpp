#pragma once

#include "authentication.hpp"

class SerialAuthentication
{
    public:
        class Builder;

        enum class Operation
        {
            Idle,

            LogIn,
            LogOut,
            CreateUser,
            DeleteUser,

            ModifyOwnUsername,
            ModifyOwnPassword,
            ModifyOwnName,

            ModifyUsername,
            ModifyPassword,
            ModifyName,
            ModifyPermission,
        };

        void setOperation(Operation newOperation);

        uint8_t authenticateNextByte(uint8_t byte);

        SerialAuthentication(Authentication& authentication, std::span<char> usernameBuffer, std::span<char> nameBuffer, std::span<char> passwordBuffer, std::span<char> password2Buffer);

    protected:
        enum class State
        {
            None,
            ReadingUser,
            ReadingPassword,
            ReadingPassword2,
            ReadingName,
            ReadingUserId,
            ReadingPermission,
            TokenReady,

            ReadingToken,
            Error,
        };

        enum class Error
        {
            None,

            UsernameOverflow,
            PasswordOverflow,
            NameOverflow,

            TokenInvalid,
            UserNoPermission,
            AuthenticationError,
            UserIdDoesNotExist,
            RepeatedUsername,

            InternalError,
        };

        Authentication* authentication = nullptr;
        std::span<char> currentUsername;
        std::span<char> currentPassword;
        std::span<char> currentPassword2;
        std::span<char> currentName;
        TokenType currentToken;
        uint16_t currentId;
        uint16_t currentPermissionId;
        Permission currentPermission;
        uint16_t currentByteIndex = 0;

        const Session* currentSession;

        State state = State::None;
        Error error = Error::None;
        Operation operation = Operation::Idle;

        // Return true if the buffer is full or the null character '\0' was received.
        bool writeUsernameByte(uint8_t byte);
        bool writePasswordByte(uint8_t byte);
        bool writePassword2Byte(uint8_t byte);
        bool writeNameByte(uint8_t byte);
        bool writeTokenByte(uint8_t byte);
        bool writeIdByte(uint8_t byte);
        bool writePermissionByte(uint8_t byte);

        void readingToken(uint8_t byte, State nextState, Permission permissionNeeded);
        bool readingUser(uint8_t byte, State nextState);
        bool readingPassword(uint8_t byte, State nextState);
        bool readingPassword2(uint8_t byte, State nextState);
        bool readingName(uint8_t byte, State nextState);
        bool readingUserId(uint8_t byte, State nextState);
        bool readingPermission(uint8_t byte, State nextState);

        uint8_t getNextTokenByte();

        void logIn(uint8_t byte);
        void logOut(uint8_t byte);
        void createUser(uint8_t byte);
        void deleteUser(uint8_t byte);

        void modifyOwnUsername(uint8_t byte);
        void modifyOwnPassword(uint8_t byte);
        void modifyOwnName(uint8_t byte);

        void modifyUsername(uint8_t byte);
        void modifyPassword(uint8_t byte);
        void modifyName(uint8_t byte);
        void modifyPermission(uint8_t byte);
};

