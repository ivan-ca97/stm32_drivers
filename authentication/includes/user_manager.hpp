#pragma once

#include <span>
#include <array>

#include "user.hpp"
#include "session.hpp"

class UserManager
{
    public:
        User* createUser(Permission newPermission, std::string_view newUsername, std::string_view newPassword, std::string_view newName = "");
        const User* getUser(std::string_view username) const;
        const User* getUser(uint16_t id) const;
        void updateUser(uint16_t id, User& updatedUser);
        void deleteUser(std::string_view username);
        void deleteUser(User& user);

        UserManager(std::span<User*> usersStorage);

    protected:
        std::span<User*> users;

        uint16_t loadedUsers = 0;
        uint16_t idCounter = 0;

        User* getUserByUsername(std::string_view username) const;
        User* getUserById(uint16_t id) const;
        User* getFreeUser();
};

template <size_t UsersAmount, size_t UsernameLength, size_t PasswordLength, size_t NameLength>
class StaticUserManager : public UserManager
{
    protected:
        typedef StaticUser<UsernameLength, PasswordLength, NameLength> UserType;
        std::array<UserType, UsersAmount> usersStorage;
        std::array<User*, UsersAmount> usersPointers;

    public:
        StaticUserManager()
            : UserManager(usersPointers)
        {
            for (size_t i = 0; i < UsersAmount; i++)
                usersPointers[i] = &usersStorage[i];
        };
};