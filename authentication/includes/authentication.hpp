#pragma once

#include "user.hpp"
#include "user_manager.hpp"
#include "session_manager.hpp"

#include <span>
#include <string>
#include <stdint.h>

class Authentication
{
    public:
        Authentication(UserManager& userManager, SessionManager& sessionManager);

        const Session* authenticate(std::string_view username, std::string_view password);

        const Session* validate(TokenType token);

        // Returns pointer to session if the user has the desired permission, nullptr otherwise.
        const Session* validateWithPermission(TokenType token, Permission permission);

        void updateSessions();

        UserManager* getUserManager();
        SessionManager* getSessionManager();

        void createUser(TokenType token, Permission newPermission, std::string_view newUsername, std::string_view newPassword, std::string_view newName);

        void deleteUser(TokenType token, const User& user);
        void deleteUser(TokenType token, uint16_t userId);

        void logOut(TokenType token);

        void modifyOwnUsername(TokenType token, std::string_view newUsername);
        void modifyOwnPassword(TokenType token, std::string_view oldPassword, std::string_view newPassword);
        void modifyOwnName(TokenType token, std::string_view newName);

        void modifyUsername(TokenType token, uint16_t id, std::string_view newUsername);
        void modifyPassword(TokenType token, uint16_t id, std::string_view newPassword);
        void modifyName(TokenType token, uint16_t id, std::string_view newName);
        void modifyPermission(TokenType token, uint16_t id, Permission newPermission);

    protected:
        UserManager *userManager;
        SessionManager *sessionManager;
};