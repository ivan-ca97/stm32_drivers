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

bool User::authenticate(std::string_view password) const
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

User::IdType User::getId() const
{
    return id;
}

Permission User::getPermission() const
{
    return permission;
}

bool User::hasPermission(Permission queryPermission) const
{
    switch(permission)
    {
        case Permission::Superuser:
            return true;

        case Permission::Maintenance:
            switch(queryPermission)
            {
                case Permission::None:
                case Permission::Observer:
                case Permission::Maintenance:
                    return true;

                case Permission::Superuser:
                    return false;
            }
            break;

        case Permission::Observer:
            switch(queryPermission)
            {
                case Permission::None:
                case Permission::Observer:
                    return true;

                case Permission::Superuser:
                case Permission::Maintenance:
                    return false;
            }
            break;

        case Permission::None:
            switch(queryPermission)
            {
                case Permission::None:
                    return true;

                case Permission::Superuser:
                case Permission::Maintenance:
                case Permission::Observer:
                    return false;
            }
            break;
    }

    return false;
}

bool User::isValid() const
{
    return valid;
}

ResultVoid User::setUsername(std::string_view newUsername)
{
    auto set = setString(newUsername, username);
    if(!set)
        return Error(AuthenticationError::UsernameBufferOverflow);
}

ResultVoid User::setPassword(std::string_view newPassword)
{
    auto set = setString(newPassword, password);
    if(!set)
        return Error(AuthenticationError::PasswordBufferOverflow);
}

ResultVoid User::setName(std::string_view newName)
{
    auto set = setString(newName, name);
    if(!set)
        return Error(AuthenticationError::NameBufferOverflow);
}

void User::setId(User::IdType newId)
{
    id = newId;
}

void User::setPermission(Permission newPermission)
{
    permission = newPermission;
}

ResultVoid User::setBufferedFields(std::string_view username, std::string_view password, std::string_view name)
{
    auto set = setUsername(username);
    if(!set)
        return Error(set.error());

    set = setPassword(password);
    if(!set)
        return Error(set.error());

    set = setName(name);
    if(!set)
        return Error(set.error());
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

ResultVoid User::setString(std::string_view stringValue, std::span<char> storage)
{
    if(stringValue.length() >= storage.size())
        return Error(AuthenticationError::Overflow);

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