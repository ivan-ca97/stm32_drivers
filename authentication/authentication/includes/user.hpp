#pragma once

#include <stdint.h>
#include <string>
#include <span>
#include <array>

#include "authentication_errors.hpp"

enum class Permission
{
    Superuser,
    Maintenance,
    Observer,
    None,
};

class User
{
    public:
        using IdType = uint16_t;

        User(std::span<char> usernameStorage, std::span<char> passwordStorage, std::span<char> nameStorage);

        bool authenticate(std::string_view password) const;

        const std::string_view getUsername() const;
        const std::string_view getPassword() const;
        const std::string_view getName() const;
        User::IdType getId() const;
        Permission getPermission() const;
        bool hasPermission(Permission permission) const;
        bool isValid() const;

        ResultVoid setUsername(std::string_view newUsername);
        ResultVoid setPassword(std::string_view newPassword);
        ResultVoid setName(std::string_view newName);
        void setId(User::IdType newId);
        void setPermission(Permission newPermission);
        void makeValid();

        // Method used to receive a single error code for all fields set.
        ResultVoid setBufferedFields(std::string_view username, std::string_view password, std::string_view name);

        void reset();

        bool operator==(const User& other) const;

    protected:
        std::span<char> username;
        std::span<char> password;
        std::span<char> name;

        IdType id;
        Permission permission;
        bool valid;

        static ResultVoid setString(std::string_view stringValue, std::span<char> storage);
        static std::string_view getString(std::span<char> storage);
};

template <size_t UsernameMaxLength, size_t PasswordMaxLength, size_t NameMaxLength>
class StaticUser : public User
{
    protected:
        std::array<char, UsernameMaxLength> usernameStorage;
        std::array<char, PasswordMaxLength> passwordStorage;
        std::array<char, NameMaxLength> nameStorage;

    public:
        StaticUser() : User(usernameStorage, passwordStorage, nameStorage) {};
};