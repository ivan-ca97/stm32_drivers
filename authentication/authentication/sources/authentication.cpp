#include "authentication.hpp"

#include <expected>
#include <algorithm>

Authentication::Authentication(UserManager& userManager, SessionManager& sessionManager)
    : userManager(&userManager), sessionManager(&sessionManager)
{

}

ResultSession Authentication::authenticate(std::string_view username, std::string_view password)
{
    auto user = userManager->getUser(username);
    if(!user)
        return Error(AuthenticationError::UsernameNotFound);

    if(!(*user)->authenticate(password))
        return Error(AuthenticationError::IncorrectPassword);

    auto session = sessionManager->createSession(**user);
    if(!session)
        return Error(AuthenticationError::SessionBufferFull);

    return session;
}

ResultSession Authentication::validate(Session::TokenType token)
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

ResultSession Authentication::validateWithPermission(Session::TokenType token, Permission permission)
{
    auto session = sessionManager->validate(token);
    if(!session)
        return Error(session.error());

    if(!(*session)->getUser()->hasPermission(permission))
        return Error(AuthenticationError::InsufficientPermissions);

    return session;
}

Result<User::IdType> Authentication::createUser(Session::TokenType token, Permission newPermission, std::string_view newUsername, std::string_view newPassword, std::string_view newName)
{
    auto session = validateWithPermission(token, Permission::Superuser);
    if(!session)
        return Error(session.error());

    auto newUser = userManager->createUser(newPermission, newUsername, newPassword, newName);
    if(!newUser)
        return Error(newUser.error());

    return (*newUser)->getId();
}

ResultVoid Authentication::deleteUser(Session::TokenType token, const User& user)
{
    auto session = validateWithPermission(token, Permission::Superuser);
    if(!session)
        return Error(session.error());

    return userManager->deleteUser(user);
}

ResultVoid Authentication::deleteUser(Session::TokenType token, User::IdType userId)
{
    auto session = validateWithPermission(token, Permission::Superuser);
    if(!session)
        return Error(session.error());

    return userManager->deleteUser(userId);
}

ResultVoid Authentication::logOut(Session::TokenType token)
{
    auto session = sessionManager->validate(token);
    if(!session)
        return Error(session.error());

    sessionManager->expireSession(**session);
    return {};
}

ResultVoid Authentication::modifyOwnUsername(Session::TokenType token, std::string_view newUsername)
{
    auto session = sessionManager->validate(token);
    if(!session)
        return Error(session.error());

    User updatedUser = *(*session)->getUser();

    auto updated = updatedUser.setUsername(newUsername);
    if(!updated)
        return Error(updated.error());

    return userManager->updateUser(updatedUser);
}

ResultVoid Authentication::modifyOwnPassword(Session::TokenType token, std::string_view oldPassword, std::string_view newPassword)
{
    auto session = sessionManager->validate(token);
    if(!session)
        return Error(session.error());

    User updatedUser = *(*session)->getUser();

    if(!updatedUser.authenticate(oldPassword))
        return Error(AuthenticationError::IncorrectPassword);

    auto updated = updatedUser.setPassword(newPassword);
    if(!updated)
        return Error(updated.error());

    return userManager->updateUser(updatedUser);
}

ResultVoid Authentication::modifyOwnName(Session::TokenType token, std::string_view newName)
{
    auto session = sessionManager->validate(token);
    if(!session)
        return Error(session.error());

    const User* user = (*session)->getUser();
    if(!user)
        return Error(AuthenticationError::IntegrityFailure);

    User updatedUser = *user;

    auto updated = updatedUser.setName(newName);
    if(!updated)
        return Error(updated.error());

    return userManager->updateUser(updatedUser);
}


ResultVoid Authentication::modifyUsername(Session::TokenType token, User::IdType id, std::string_view newUsername)
{
    auto session = validateWithPermission(token, Permission::Superuser);
    if(!session)
        return Error(session.error());

    auto user = userManager->getUser(id);
    if(!user)
        return Error(AuthenticationError::IntegrityFailure);

    User updatedUser = **user;

    auto updated = updatedUser.setUsername(newUsername);
    if(!updated)
        return Error(updated.error());

    return userManager->updateUser(updatedUser);
}

ResultVoid Authentication::modifyPassword(Session::TokenType token, User::IdType id, std::string_view newPassword)
{
    auto session = validateWithPermission(token, Permission::Superuser);
    if(!session)
        return Error(session.error());

    auto user = userManager->getUser(id);
    if(!user)
        return Error(AuthenticationError::IntegrityFailure);

    User updatedUser = **user;

    auto updated = updatedUser.setPassword(newPassword);
    if(!updated)
        return Error(updated.error());

    return userManager->updateUser(updatedUser);
}

ResultVoid Authentication::modifyName(Session::TokenType token, User::IdType id, std::string_view newName)
{
    auto session = validateWithPermission(token, Permission::Superuser);
    if(!session)
        return Error(session.error());

    auto user = userManager->getUser(id);
    if(!user)
        return Error(AuthenticationError::IntegrityFailure);

    User updatedUser = **user;

    auto updated = updatedUser.setName(newName);
    if(!updated)
        return Error(updated.error());

    return userManager->updateUser(updatedUser);
}

ResultVoid Authentication::modifyPermission(Session::TokenType token, User::IdType id, Permission newPermission)
{
    auto session = validateWithPermission(token, Permission::Superuser);
    if(!session)
        return Error(session.error());

    auto user = userManager->getUser(id);
    if(!user)
        return Error(AuthenticationError::IntegrityFailure);

    User updatedUser = **user;

    updatedUser.setPermission(newPermission);

    return userManager->updateUser(updatedUser);
}
