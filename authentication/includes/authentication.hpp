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

        const Session* validate(Session::TokenType token);

        // Returns pointer to session if the user has the desired permission, nullptr otherwise.
        const Session* validateWithPermission(Session::TokenType token, Permission permission);

        void updateSessions();

        UserManager* getUserManager();
        SessionManager* getSessionManager();

        User::IdType createUser(Session::TokenType token, Permission newPermission, std::string_view newUsername, std::string_view newPassword, std::string_view newName);

        void deleteUser(Session::TokenType token, const User& user);
        void deleteUser(Session::TokenType token, User::IdType userId);

        void logOut(Session::TokenType token);

        void modifyOwnUsername(Session::TokenType token, std::string_view newUsername);
        void modifyOwnPassword(Session::TokenType token, std::string_view oldPassword, std::string_view newPassword);
        void modifyOwnName(Session::TokenType token, std::string_view newName);

        void modifyUsername(Session::TokenType token, User::IdType id, std::string_view newUsername);
        void modifyPassword(Session::TokenType token, User::IdType id, std::string_view newPassword);
        void modifyName(Session::TokenType token, User::IdType id, std::string_view newName);
        void modifyPermission(Session::TokenType token, User::IdType id, Permission newPermission);

    protected:
        UserManager *userManager;
        SessionManager *sessionManager;
};