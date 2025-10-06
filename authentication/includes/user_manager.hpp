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
        const User* getUser(User::IdType id) const;
        void updateUser(User& updatedUser);
        void deleteUser(std::string_view username);
        void deleteUser(User::IdType id);
        void deleteUser(const User& user);

        User::IdType getMaxUsers();

        UserManager(std::span<User*> usersStorage);

    protected:
        std::span<User*> users;

        User::IdType loadedUsers = 0;
        User::IdType idCounter = 0;

        User* getUserByUsername(std::string_view username) const;
        User* getUserById(User::IdType id) const;
        User* getFreeUser();
        void checkRepeatedUsername(std::string_view username);
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