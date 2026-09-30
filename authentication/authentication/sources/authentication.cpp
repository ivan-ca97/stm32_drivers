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
        return std::unexpected(authentication::usernameNotFound);

    if(!(*user)->authenticate(password))
        return std::unexpected(authentication::incorrectPassword);

    auto session = sessionManager->createSession(**user);
    if(!session)
        return std::unexpected(authentication::sessionBufferFull);

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
        return std::unexpected(session.error());

    if(!(*session)->getUser()->hasPermission(permission))
        return std::unexpected(authentication::insufficientPermissions);

    return session;
}

Result<User::IdType> Authentication::createUser(Session::TokenType token, Permission newPermission, std::string_view newUsername, std::string_view newPassword, std::string_view newName)
{
    auto session = validateWithPermission(token, Permission::Superuser);
    if(!session)
        return std::unexpected(session.error());

    auto newUser = userManager->createUser(newPermission, newUsername, newPassword, newName);
    if(!newUser)
        return std::unexpected(newUser.error());

    return (*newUser)->getId();
}

ResultVoid Authentication::deleteUser(Session::TokenType token, const User& user)
{
    auto session = validateWithPermission(token, Permission::Superuser);
    if(!session)
        return std::unexpected(session.error());

    return userManager->deleteUser(user);
}

ResultVoid Authentication::deleteUser(Session::TokenType token, User::IdType userId)
{
    auto session = validateWithPermission(token, Permission::Superuser);
    if(!session)
        return std::unexpected(session.error());

    return userManager->deleteUser(userId);
}

ResultVoid Authentication::logOut(Session::TokenType token)
{
    auto session = sessionManager->validate(token);
    if(!session)
        return std::unexpected(session.error());

    sessionManager->expireSession(**session);
    return {};
}

ResultVoid Authentication::modifyOwnUsername(Session::TokenType token, std::string_view newUsername)
{
    auto session = sessionManager->validate(token);
    if(!session)
        return std::unexpected(session.error());

    const User* user = (*session)->getUser();
    if(!user)
        return std::unexpected(authentication::integrityFailure);

    return userManager->setUsername(user->getId(), newUsername);
}

ResultVoid Authentication::modifyOwnPassword(Session::TokenType token, std::string_view oldPassword, std::string_view newPassword)
{
    auto session = sessionManager->validate(token);
    if(!session)
        return std::unexpected(session.error());

    const User* user = (*session)->getUser();
    if(!user)
        return std::unexpected(authentication::integrityFailure);

    if(!user->authenticate(oldPassword))
        return std::unexpected(authentication::incorrectPassword);

    return userManager->setPassword(user->getId(), newPassword);
}

ResultVoid Authentication::modifyOwnName(Session::TokenType token, std::string_view newName)
{
    auto session = sessionManager->validate(token);
    if(!session)
        return std::unexpected(session.error());

    const User* user = (*session)->getUser();
    if(!user)
        return std::unexpected(authentication::integrityFailure);

    return userManager->setName(user->getId(), newName);
}


ResultVoid Authentication::modifyUsername(Session::TokenType token, User::IdType id, std::string_view newUsername)
{
    auto session = validateWithPermission(token, Permission::Superuser);
    if(!session)
        return std::unexpected(session.error());

    return userManager->setUsername(id, newUsername);
}

ResultVoid Authentication::modifyPassword(Session::TokenType token, User::IdType id, std::string_view newPassword)
{
    auto session = validateWithPermission(token, Permission::Superuser);
    if(!session)
        return std::unexpected(session.error());

    return userManager->setPassword(id, newPassword);
}

ResultVoid Authentication::modifyName(Session::TokenType token, User::IdType id, std::string_view newName)
{
    auto session = validateWithPermission(token, Permission::Superuser);
    if(!session)
        return std::unexpected(session.error());

    return userManager->setName(id, newName);
}

ResultVoid Authentication::modifyPermission(Session::TokenType token, User::IdType id, Permission newPermission)
{
    auto session = validateWithPermission(token, Permission::Superuser);
    if(!session)
        return std::unexpected(session.error());

    return userManager->setPermission(id, newPermission);
}
