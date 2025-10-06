#pragma once

#include <optional>

#include "authentication.hpp"

class SerialAuthentication
{
    public:
        class Builder;
        struct Configuration;

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

        void authenticateNextByte(uint8_t byte);
        uint8_t getNextByte();

        SerialAuthentication(const Configuration& configuration);

    protected:
        enum class State
        {
            None,
            ReadReady,

            ReadingToken,
            ReadingUser,
            ReadingPassword,
            ReadingPassword2,
            ReadingName,
            ReadingUserId,
            ReadingPermission,

            SendingToken,

            SendingErrorCode,

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

            ByteReadNotExpected,

            InternalError,
        };

        Authentication* authentication = nullptr;
        std::span<char> currentUsername;
        std::span<char> currentPassword;
        std::span<char> currentPassword2;
        std::span<char> currentName;
        Session::TokenType currentToken;
        User::IdType currentId;
        uint16_t currentPermissionId;
        Permission currentPermission;
        uint16_t currentByteIndex = 0;

        // This variable is used when there's an error
        // to set in a default manner when the error code should
        // be sent. For example, when logging in the master
        // expects 8 bytes for the token and then an error code.
        // This variable should be used to signal that the 9th byte
        // should be the error code.
        uint16_t writesUntilErrorCode = 0;

        const Session* currentSession;

        State state = State::None;
        Error error = Error::None;
        Operation operation = Operation::Idle;

        // Return true if the buffer is full or the null character '\0' was received.
        bool setUsernameByte(uint8_t byte);
        bool setPasswordByte(uint8_t byte);
        bool setPassword2Byte(uint8_t byte);
        bool setNameByte(uint8_t byte);
        bool setIdByte(uint8_t byte);
        bool setPermissionByte(uint8_t byte);
        bool setTokenByte(uint8_t byte);

        bool getUsernameByte(uint8_t* byte);
        bool getPasswordByte(uint8_t* byte);
        bool getPassword2Byte(uint8_t* byte);
        bool getNameByte(uint8_t* byte);
        bool getIdByte(uint8_t* byte);
        bool getPermissionByte(uint8_t* byte);
        bool getTokenByte(uint8_t* byte);

        void readingToken(uint8_t byte, State nextState, Permission permissionNeeded);
        bool readingUser(uint8_t byte, State nextState);
        bool readingPassword(uint8_t byte, State nextState);
        bool readingPassword2(uint8_t byte, State nextState);
        bool readingName(uint8_t byte, State nextState);
        bool readingUserId(uint8_t byte, State nextState);
        bool readingPermission(uint8_t byte, State nextState);

        // Read operations
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

        // Write operations
        uint8_t logIn();
        uint8_t logOut();
        uint8_t createUser();
        uint8_t deleteUser();

        uint8_t modifyOwnUsername();
        uint8_t modifyOwnPassword();
        uint8_t modifyOwnName();

        uint8_t modifyUsername();
        uint8_t modifyPassword();
        uint8_t modifyName();
        uint8_t modifyPermission();

        uint8_t getNextTokenByte();

        // Write states
        uint8_t sendingToken();
};

