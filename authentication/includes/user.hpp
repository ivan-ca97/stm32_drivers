#pragma once

#include <stdint.h>
#include <string>
#include <span>
#include <array>

enum class Permission
{
    Superuser,
    Maintenance,
    Observer,
    None
};

class User
{
    public:
        User(std::span<char> usernameStorage, std::span<char> passwordStorage, std::span<char> nameStorage);

        const bool authenticate(std::string_view password) const;

        const std::string_view getUsername() const;
        const std::string_view getPassword() const;
        const std::string_view getName() const;
        uint16_t getId() const;
        Permission getPermission() const;
        bool isValid() const;

        void setUsername(std::string_view newUsername);
        void setPassword(std::string_view newPassword);
        void setName(std::string_view newName);
        void setId(uint16_t newId);
        void setPermission(Permission newPermission);
        void makeValid();

        void reset();

        bool operator==(const User& other) const;

    protected:
        std::span<char> username;
        std::span<char> password;
        std::span<char> name;

        uint16_t id;
        Permission permission;
        bool valid;

        static void setString(std::string_view stringValue, std::span<char> storage, std::string_view errorMessage = "String too long");
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