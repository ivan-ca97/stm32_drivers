#pragma once

#include "authentication_errors.hpp"

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

        ResultSession authenticate(std::string_view username, std::string_view password);

        ResultSession validate(Session::TokenType token);

        // Returns pointer to session if the user has the desired permission, error otherwise.
        ResultSession validateWithPermission(Session::TokenType token, Permission permission);

        void updateSessions();

        UserManager* getUserManager();
        SessionManager* getSessionManager();

        Result<User::IdType> createUser(Session::TokenType token, Permission newPermission, std::string_view newUsername, std::string_view newPassword, std::string_view newName);

        ResultVoid deleteUser(Session::TokenType token, const User& user);
        ResultVoid deleteUser(Session::TokenType token, User::IdType userId);

        ResultVoid logOut(Session::TokenType token);

        ResultVoid modifyOwnUsername(Session::TokenType token, std::string_view newUsername);
        ResultVoid modifyOwnPassword(Session::TokenType token, std::string_view oldPassword, std::string_view newPassword);
        ResultVoid modifyOwnName(Session::TokenType token, std::string_view newName);

        ResultVoid modifyUsername(Session::TokenType token, User::IdType id, std::string_view newUsername);
        ResultVoid modifyPassword(Session::TokenType token, User::IdType id, std::string_view newPassword);
        ResultVoid modifyName(Session::TokenType token, User::IdType id, std::string_view newName);
        ResultVoid modifyPermission(Session::TokenType token, User::IdType id, Permission newPermission);

    protected:
        UserManager *userManager;
        SessionManager *sessionManager;
};