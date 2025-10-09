#include "user_manager.hpp"

#include <stdexcept>
#include <algorithm>

ResultUser UserManager::createUser(Permission newPermission, std::string_view newUsername, std::string_view newPassword, std::string_view newName)
{
    User* newUser = getFreeUser();
    if(!newUser)
        return Error(AuthenticationError::UsersBufferFull);

    if(usernameExists(newUsername))
        return Error(AuthenticationError::UsernameAlreadyExists);


    newUser->setPermission(newPermission);
    newUser->setId(idCounter++);
    auto set = newUser->setBufferedFields(newUsername, newPassword, newName);
    if(!set)
        return Error(set.error());

    newUser->makeValid();

    loadedUsers++;
    return newUser;
}

ResultUser UserManager::getUser(std::string_view username) const
{
    return getUserByUsername(username);
}

ResultUser UserManager::getUser(User::IdType id) const
{
    return getUserById(id);
}

Result<User*> UserManager::getUserByUsername(std::string_view username) const
{
    auto findLambda = [&](const User* user) {return user && user->getUsername() == username;};
    auto it = std::find_if(users.begin(), users.end(), findLambda);

    if(it == users.end())
        return Error(AuthenticationError::UsernameNotFound);

    return *it;
}

Result<User*> UserManager::getUserById(User::IdType id) const
{
    auto findLambda = [&](const User* user) {return user && user->getId() == id;};
    auto it = std::find_if(users.begin(), users.end(), findLambda);

    if(it == users.end())
        return Error(AuthenticationError::UserIdNotFound);

    return *it;
}

ResultVoid UserManager::updateUser(User& updatedUser)
{
    if(updatedUser.getPassword().empty() || updatedUser.getUsername().empty())
        return Error(AuthenticationError::EmptyMandatoryField);

    auto userResult = getUserById(updatedUser.getId());
    if(!userResult)
        return Error(userResult.error());

    auto user = *userResult;

    if(user->getUsername() != updatedUser.getUsername())
    {
        if(getUserByUsername(updatedUser.getUsername()))
            return Error(AuthenticationError::UsernameAlreadyExists);
    }

    user->setPermission(updatedUser.getPermission());
    auto set = updatedUser.setBufferedFields(updatedUser.getUsername(), updatedUser.getPassword(), updatedUser.getName());
    if(!set)
        return Error(set.error());

    return {};
}

ResultVoid UserManager::deleteUser(std::string_view username)
{
    auto user = getUserByUsername(username);
    if(!user)
        return Error(user.error());

    return deleteUser(**user);
}

ResultVoid UserManager::deleteUser(User::IdType id)
{
    auto storedUser = getUserById(id);

    if(!storedUser)
        return Error(storedUser.error());

    (*storedUser)->reset();
    loadedUsers--;

    return {};
}

ResultVoid UserManager::deleteUser(const User& user)
{
    auto storedUser = getUserById(user.getId());

    if(!storedUser || **storedUser != user)
        return Error(AuthenticationError::UserIdNotFound);

    (*storedUser)->reset();
    loadedUsers--;

    return {};
}

User::IdType UserManager::getMaxUsers()
{
    return users.size();
}

User* UserManager::getFreeUser() const
{
    auto findLambda = [&](const User* user) {return user && !user->isValid();};
    auto it = std::find_if(users.begin(), users.end(), findLambda);

    if(it == users.end())
        return nullptr;

    return *it;
}

bool UserManager::usernameExists(std::string_view username) const
{
    return static_cast<bool>(getUserByUsername(username));
}

UserManager::UserManager(std::span<User*> usersStorage)
    : users(usersStorage)
{}