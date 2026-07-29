#pragma once

#include <span>
#include <array>

#include "user.hpp"
#include "session.hpp"

#include "authentication_errors.hpp"

using ResultUser = Result<const User*>;

class UserManager
{
    public:
        ResultUser createUser(Permission newPermission, std::string_view newUsername, std::string_view newPassword, std::string_view newName = "");
        ResultUser getUser(std::string_view username) const;
        ResultUser getUser(User::IdType id) const;
        ResultVoid updateUser(User& updatedUser);
        ResultVoid deleteUser(std::string_view username);
        ResultVoid deleteUser(User::IdType id);
        ResultVoid deleteUser(const User& user);

        User::IdType getMaxUsers();

        UserManager(std::span<User*> usersStorage);

    protected:
        std::span<User*> users;

        User::IdType loadedUsers = 0;
        User::IdType idCounter = 0;

        Result<User*> getUserByUsername(std::string_view username) const;
        Result<User*> getUserById(User::IdType id) const;
        User* getFreeUser() const;
        bool usernameExists(std::string_view username) const;
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