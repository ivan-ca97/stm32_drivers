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
        Authentication(UserManager* userManager, SessionManager* sessionManager);

        const Session* authenticate(std::string_view username, std::string_view password);

        const Session* validate(TokenType token);

        void updateSessions();

    protected:
        UserManager *userManager;
        SessionManager *sessionManager;
};