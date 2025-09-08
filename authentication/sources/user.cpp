#include "user.hpp"

#include <cstring>
#include <algorithm>
#include <stdexcept>

User::User(std::span<char> usernameStorage, std::span<char> passwordStorage, std::span<char> nameStorage)
    : username(usernameStorage), password(passwordStorage), name(nameStorage), id(0), permission(Permission::None), valid(false)
{
    std::fill(username.begin(), username.end(), '\0');
    std::fill(password.begin(), password.end(), '\0');
    std::fill(name.begin(), name.end(), '\0');
}

const bool User::authenticate(std::string_view password) const
{
    return password == this->password.data();
}

const std::string_view User::getUsername() const
{
    return getString(username);
}

const std::string_view User::getPassword() const
{
    return getString(password);
}

const std::string_view User::getName() const
{
    return getString(name);
}

uint16_t User::getId() const
{
    return id;
}

Permission User::getPermission() const
{
    return permission;
}

bool User::isValid() const
{
    return valid;
}

void User::setUsername(std::string_view newUsername)
{
    setString(newUsername, username, "Username too long");
}

void User::setPassword(std::string_view newPassword)
{
    setString(newPassword, password, "Password too long");
}

void User::setName(std::string_view newName)
{
    setString(newName, name, "Name too long");
}

void User::setId(uint16_t newId)
{
    id = newId;
}

void User::setPermission(Permission newPermission)
{
    permission = newPermission;
}

void User::makeValid()
{
    valid = true;
}


void User::reset()
{
    id = 0;
    permission = Permission::None;
    valid = false;

    username[0] = '\0';
    name[0] = '\0';
    password[0] = '\0';
}

void User::setString(std::string_view stringValue, std::span<char> storage, std::string_view errorMessage)
{
    if(stringValue.length() >= storage.size())
        throw std::logic_error(std::string(errorMessage));

    std::copy(stringValue.begin(), stringValue.end(), storage.begin());
    storage[stringValue.length()] = '\0';
}

std::string_view User::getString(std::span<char> storage)
{
    auto stringLength = std::strlen(storage.data());
    auto maxSize = storage.size();
    auto length = std::min(stringLength, maxSize);
    return std::string_view(storage.data(), length);
}

bool User::operator==(const User& other) const {
    return
        id == other.id &&
        std::string_view(username) == std::string_view(other.username) &&
        std::string_view(password) == std::string_view(other.password) &&
        std::string_view(name) == std::string_view(other.name) &&
        permission == other.permission;
}