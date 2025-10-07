#include "authentication.hpp"
#include "authentication_exceptions.hpp"

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
        throw UserNotFoundError("User not found.");

    if(!user->authenticate(password))
        throw AuthenticationError("Password incorrect.");

    const Session* session = sessionManager->createSession(*user);
    if(!session)
        throw BufferFullError("Sessions buffer full.");

    return session;
}

const Session* Authentication::validate(Session::TokenType token)
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

const Session* Authentication::validateWithPermission(Session::TokenType token, Permission permission)
{
    const Session* session = sessionManager->validate(token);
    if(!session)
        throw InvalidTokenError("Invalid Token.");

    if(!session->getUser()->hasPermission(permission))
        throw InsuficientPermissionsError("User does not have sufficient permissions.");

    return session;
}

User::IdType Authentication::createUser(Session::TokenType token, Permission newPermission, std::string_view newUsername, std::string_view newPassword, std::string_view newName)
{
    validateWithPermission(token, Permission::Superuser);

    auto newUser = userManager->createUser(newPermission, newUsername, newPassword, newName);
    return newUser->getId();
}

void Authentication::deleteUser(Session::TokenType token, const User& user)
{
    validateWithPermission(token, Permission::Superuser);

    userManager->deleteUser(user);
}

void Authentication::deleteUser(Session::TokenType token, User::IdType userId)
{
    validateWithPermission(token, Permission::Superuser);

    userManager->deleteUser(userId);
}

void Authentication::logOut(Session::TokenType token)
{
    const Session* session = sessionManager->validate(token);
    if(!session)
        throw InvalidTokenError("Invalid token.");

    sessionManager->expireSession(*session);
}

void Authentication::modifyOwnUsername(Session::TokenType token, std::string_view newUsername)
{
    const Session* session = sessionManager->validate(token);
    if(!session)
        throw InvalidTokenError("Invalid token.");

    User updatedUser = *session->getUser();

    updatedUser.setUsername(newUsername);
    userManager->updateUser(updatedUser);
}

void Authentication::modifyOwnPassword(Session::TokenType token, std::string_view oldPassword, std::string_view newPassword)
{
    const Session* session = sessionManager->validate(token);
    if(!session)
        throw InvalidTokenError("Invalid token.");

    User updatedUser = *session->getUser();

    if(!updatedUser.authenticate(oldPassword))
        throw AuthenticationError("Wrong password.");

    updatedUser.setPassword(newPassword);
    userManager->updateUser(updatedUser);
}

void Authentication::modifyOwnName(Session::TokenType token, std::string_view newName)
{
    const Session* session = sessionManager->validate(token);
    if(!session)
        throw InvalidTokenError("Invalid token.");

    const User* user = session->getUser();
    if(!user)
        throw IntegrityError("Error getting session user.");

    User updatedUser = *user;
    updatedUser.setName(newName);
    userManager->updateUser(updatedUser);
}


void Authentication::modifyUsername(Session::TokenType token, User::IdType id, std::string_view newUsername)
{
    validateWithPermission(token, Permission::Superuser);

    const User* user = userManager->getUser(id);
    if(!user)
        throw IntegrityError("Error getting session user.");

    User updatedUser = *user;
    updatedUser.setUsername(newUsername);
    userManager->updateUser(updatedUser);
}

void Authentication::modifyPassword(Session::TokenType token, User::IdType id, std::string_view newPassword)
{
    validateWithPermission(token, Permission::Superuser);

    const User* user = userManager->getUser(id);
    if(!user)
        throw IntegrityError("Error getting session user.");

    User updatedUser = *user;
    updatedUser.setPassword(newPassword);
    userManager->updateUser(updatedUser);
}

void Authentication::modifyName(Session::TokenType token, User::IdType id, std::string_view newName)
{
    validateWithPermission(token, Permission::Superuser);

    const User* user = userManager->getUser(id);
    if(!user)
        throw IntegrityError("Error getting session user.");

    User updatedUser = *user;
    updatedUser.setName(newName);
    userManager->updateUser(updatedUser);
}

void Authentication::modifyPermission(Session::TokenType token, User::IdType id, Permission newPermission)
{
    validateWithPermission(token, Permission::Superuser);

    const User* user = userManager->getUser(id);
    if(!user)
        throw IntegrityError("Error getting session user.");

    User updatedUser = *user;
    updatedUser.setPermission(newPermission);
    userManager->updateUser(updatedUser);
}
