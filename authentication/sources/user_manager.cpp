#include "user_manager.hpp"

#include "authentication_exceptions.hpp"

#include <stdexcept>
#include <algorithm>

User* UserManager::createUser(Permission newPermission, std::string_view newUsername, std::string_view newPassword, std::string_view newName)
{
    User* newUser = getFreeUser();
    if(!newUser)
        throw BufferFullError("Users buffer full.");

    checkRepeatedUsername(newUsername);

    newUser->setUsername(newUsername);
    newUser->setPassword(newPassword);
    newUser->setName(newName);
    newUser->setPermission(newPermission);
    newUser->setId(idCounter++);
    newUser->makeValid();

    loadedUsers++;
    return newUser;
}

const User* UserManager::getUser(std::string_view username) const
{
    return getUserByUsername(username);
}

const User* UserManager::getUser(User::IdType id) const
{
    return getUserById(id);
}

User* UserManager::getUserByUsername(std::string_view username) const
{
    auto findLambda = [&](const User* user) {return user && user->getUsername() == username;};
    auto it = std::find_if(users.begin(), users.end(), findLambda);

    if(it == users.end())
        return nullptr;

    return *it;
}

User* UserManager::getUserById(User::IdType id) const
{
    auto findLambda = [&](const User* user) {return user && user->getId() == id;};
    auto it = std::find_if(users.begin(), users.end(), findLambda);

    if(it == users.end())
        return nullptr;

    return *it;
}

void UserManager::updateUser(User& updatedUser)
{
    if(updatedUser.getPassword().empty() || updatedUser.getUsername().empty())
        throw InsuficientDataError("Empty password or username trying to update user.");

    User* user = getUserById(updatedUser.getId());

    if(user->getUsername() != updatedUser.getUsername())
        checkRepeatedUsername(updatedUser.getUsername());

    user->setName(updatedUser.getName());
    user->setPassword(updatedUser.getPassword());
    user->setUsername(updatedUser.getUsername());
    user->setPermission(updatedUser.getPermission());
}

void UserManager::deleteUser(std::string_view username)
{
    User* user = getUserByUsername(username);
    if(!user)
        throw UserNotFoundError("User not found by username.");

    deleteUser(*user);
}

void UserManager::deleteUser(User::IdType id)
{
    auto storedUser = getUserById(id);

    if(!storedUser)
        throw UserNotFoundError("User not found by ID.");

    storedUser->reset();
    loadedUsers--;
}

void UserManager::deleteUser(const User& user)
{
    auto storedUser = getUserById(user.getId());

    if(!storedUser || *storedUser != user)
        throw UserNotFoundError("User not found by ID.");

    storedUser->reset();
    loadedUsers--;
}

User::IdType UserManager::getMaxUsers()
{
    return users.size();
}

User* UserManager::getFreeUser()
{
    auto findLambda = [&](const User* user) {return user && !user->isValid();};
    auto it = std::find_if(users.begin(), users.end(), findLambda);

    if(it == users.end())
        return nullptr;

    return *it;
}

void UserManager::checkRepeatedUsername(std::string_view username)
{
    if(getUserByUsername(username))
        throw UsernameAlreadyExistsError("Username already exists.");
}

UserManager::UserManager(std::span<User*> usersStorage)
    : users(usersStorage)
{}