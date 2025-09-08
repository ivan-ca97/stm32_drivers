#include "authentication.hpp"

#include <stdexcept>
#include <algorithm>

Authentication::Authentication(UserManager* userManager, SessionManager* sessionManager)
    : userManager(userManager), sessionManager(sessionManager)
{

}

const Session* Authentication::authenticate(std::string_view username, std::string_view password)
{
    const User* user = userManager->getUser(username);
    if(!user)
        throw std::logic_error("User not found.");;

    if(!user->authenticate(password))
        throw std::logic_error("Password incorrect.");;

    const Session* session = sessionManager->createSession(*user);
    if(!session)
        throw std::logic_error("Sessions buffer full.");

    return session;
}

const Session* Authentication::validate(TokenType token)
{
    return sessionManager->validate(token);
}

void Authentication::updateSessions()
{
    return sessionManager->updateSessions();
}

