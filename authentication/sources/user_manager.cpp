#include "user_manager.hpp"

#include <stdexcept>
#include <algorithm>

User* UserManager::createUser(Permission newPermission, std::string_view newUsername, std::string_view newPassword, std::string_view newName)
{
    User* newUser = getFreeUser();
    if(!newUser)
        throw std::logic_error("Users buffer full.");

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

const User* UserManager::getUser(uint16_t id) const
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

User* UserManager::getUserById(uint16_t id) const
{
    auto findLambda = [&](const User* user) {return user && user->getId() == id;};
    auto it = std::find_if(users.begin(), users.end(), findLambda);

    if(it == users.end())
        return nullptr;

    return *it;
}

void UserManager::updateUser(uint16_t id, User& updatedUser)
{
    if(updatedUser.getPassword().empty() || updatedUser.getUsername().empty())
        throw std::logic_error("Empty password or username trying to update user.");

    User* user = getUserById(id);
    user->setName(updatedUser.getName());
    user->setPassword(updatedUser.getPassword());
    user->setUsername(updatedUser.getUsername());
    user->setPermission(updatedUser.getPermission());
}

void UserManager::deleteUser(std::string_view username)
{
    User* user = getUserByUsername(username);
    if(!user)
        throw std::logic_error("User not found.");

    deleteUser(*user);
}

void UserManager::deleteUser(User& user)
{
    user.reset();
    loadedUsers--;
}


User* UserManager::getFreeUser()
{
    auto findLambda = [&](const User* user) {return user && !user->isValid();};
    auto it = std::find_if(users.begin(), users.end(), findLambda);

    if(it == users.end())
        return nullptr;

    return *it;
}

UserManager::UserManager(std::span<User*> usersStorage)
    : users(usersStorage)
{}