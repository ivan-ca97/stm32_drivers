#include "authentication.hpp"

#include <stdexcept>
#include <algorithm>

Authentication::Authentication(UserManager& userManager, SessionManager& sessionManager)
    : userManager(&userManager), sessionManager(&sessionManager)
{

}

const Session* Authentication::authenticate(std::string_view username, std::string_view password)
{
    const User* user = userManager->getUser(username);
    if(!user)
        throw std::logic_error("User not found.");

    if(!user->authenticate(password))
        throw std::logic_error("Password incorrect.");

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

UserManager* Authentication::getUserManager()
{
    return userManager;
}

SessionManager* Authentication::getSessionManager()
{
    return sessionManager;
}

const Session* Authentication::validateWithPermission(TokenType token, Permission permission)
{
    const Session* session = sessionManager->validate(token);
    if(!session)
        return nullptr;

    if(!session->getUser()->hasPermission(permission))
        return nullptr;

    return session;
}

void Authentication::createUser(TokenType token, Permission newPermission, std::string_view newUsername, std::string_view newPassword, std::string_view newName)
{
    if(!validateWithPermission(token, Permission::Superuser))
        throw std::logic_error("Invalid token or user doesn't have necessary permissions.");

    userManager->createUser(newPermission, newUsername, newPassword, newName);
}

void Authentication::deleteUser(TokenType token, const User& user)
{
    if(!validateWithPermission(token, Permission::Superuser))
        throw std::logic_error("Invalid token or user doesn't have necessary permissions.");

    userManager->deleteUser(user);
}

void Authentication::deleteUser(TokenType token, uint16_t userId)
{
    if(!validateWithPermission(token, Permission::Superuser))
        throw std::logic_error("Invalid token or user doesn't have necessary permissions.");

    userManager->deleteUser(userId);
}

void Authentication::logOut(TokenType token)
{
    const Session* session = sessionManager->validate(token);
    if(!session)
        throw std::logic_error("Invalid token.");

    sessionManager->expireSession(*session);
}

void Authentication::modifyOwnUsername(TokenType token, std::string_view newUsername)
{
    const Session* session = sessionManager->validate(token);
    if(!session)
        throw std::logic_error("Invalid token.");

    User updatedUser = *session->getUser();

    updatedUser.setUsername(newUsername);
    userManager->updateUser(updatedUser);
}

void Authentication::modifyOwnPassword(TokenType token, std::string_view oldPassword, std::string_view newPassword)
{
    const Session* session = sessionManager->validate(token);
    if(!session)
        throw std::logic_error("Invalid token.");

    User updatedUser = *session->getUser();

    if(!updatedUser.authenticate(oldPassword))
        throw std::logic_error("Wrong password.");

    updatedUser.setPassword(newPassword);
    userManager->updateUser(updatedUser);
}

void Authentication::modifyOwnName(TokenType token, std::string_view newName)
{
    const Session* session = sessionManager->validate(token);
    if(!session)
        throw std::logic_error("Invalid token.");

    const User* user = session->getUser();
    if(!user)
        throw std::logic_error("Error getting session user.");

    User updatedUser = *user;
    updatedUser.setName(newName);
    userManager->updateUser(updatedUser);
}


void Authentication::modifyUsername(TokenType token, uint16_t id, std::string_view newUsername)
{
    if(!validateWithPermission(token, Permission::Superuser))
        throw std::logic_error("Invalid token or user doesn't have necessary permissions");

    const User* user = userManager->getUser(id);
    if(!user)
        throw std::logic_error("Error getting session user.");

    User updatedUser = *user;
    updatedUser.setUsername(newUsername);
    userManager->updateUser(updatedUser);
}

void Authentication::modifyPassword(TokenType token, uint16_t id, std::string_view newPassword)
{
    if(!validateWithPermission(token, Permission::Superuser))
        throw std::logic_error("Invalid token or user doesn't have necessary permissions");

    const User* user = userManager->getUser(id);
    if(!user)
        throw std::logic_error("Error getting session user.");

    User updatedUser = *user;
    updatedUser.setPassword(newPassword);
    userManager->updateUser(updatedUser);
}

void Authentication::modifyName(TokenType token, uint16_t id, std::string_view newName)
{
    if(!validateWithPermission(token, Permission::Superuser))
        throw std::logic_error("Invalid token or user doesn't have necessary permissions");

    const User* user = userManager->getUser(id);
    if(!user)
        throw std::logic_error("Error getting session user.");

    User updatedUser = *user;
    updatedUser.setName(newName);
    userManager->updateUser(updatedUser);
}

void Authentication::modifyPermission(TokenType token, uint16_t id, Permission newPermission)
{
    if(!validateWithPermission(token, Permission::Superuser))
        throw std::logic_error("Invalid token or user doesn't have necessary permissions");

    const User* user = userManager->getUser(id);
    if(!user)
        throw std::logic_error("Error getting session user.");

    User updatedUser = *user;
    updatedUser.setPermission(newPermission);
    userManager->updateUser(updatedUser);
}
