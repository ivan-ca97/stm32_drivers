#pragma once

#include <span>
#include <array>

#include "user.hpp"
#include "session.hpp"

#include "authentication_errors.hpp"
#include "persistent_storage.hpp"

using ResultUser = Result<const User*>;

class UserManager
{
    public:
        ResultUser createUser(Permission newPermission, std::string_view newUsername, std::string_view newPassword, std::string_view newName = "");
        ResultUser getUser(std::string_view username) const;
        ResultUser getUser(User::IdType id) const;

        // Mutate one field of a stored user (looked up by id), then persist. Each
        // validates and writes the stored user directly (no aliasing copies).
        ResultVoid setUsername(User::IdType id, std::string_view newUsername);
        ResultVoid setPassword(User::IdType id, std::string_view newPassword);
        ResultVoid setName(User::IdType id, std::string_view newName);
        ResultVoid setPermission(User::IdType id, Permission newPermission);

        ResultVoid deleteUser(std::string_view username);
        ResultVoid deleteUser(User::IdType id);
        ResultVoid deleteUser(const User& user);

        // usersStorage: user slots. serializeBuffer: scratch sized for the worst-case
        // image. storage: optional persistence backend (nullptr = in-RAM only).
        UserManager(std::span<User*> usersStorage, std::span<uint8_t> serializeBuffer, PersistentStorage* storage = nullptr);

    protected:
        std::span<User*>    users;
        std::span<uint8_t>  serializeBuffer;
        PersistentStorage*  storage;

        User::IdType loadedUsers = 0;
        User::IdType idCounter = 0;

        // Populate the manager from the backend. Called once after the slots are set.
        void load();

        Result<User*> getUserByUsername(std::string_view username) const;
        Result<User*> getUserById(User::IdType id) const;
        User* getFreeUser() const;
        bool usernameExists(std::string_view username) const;

        // Serialize every valid user into buf (returns byte count) / rebuild from a
        // blob. Record: [id:2][perm:1][userLen:1][user][passLen:1][pass][nameLen:1][name].
        bool persist();
        uint32_t serialize(uint8_t* buf, uint32_t cap) const;
        void deserialize(const uint8_t* buf, uint32_t len);
};

template <size_t UsersAmount, size_t UsernameLength, size_t PasswordLength, size_t NameLength>
class StaticUserManager : public UserManager
{
    protected:
        typedef StaticUser<UsernameLength, PasswordLength, NameLength> UserType;

        // count byte + per user: id(2) + perm(1) + 3 length prefixes + field storage.
        static constexpr size_t SerializeBufferSize =
            1 + UsersAmount * (6 + UsernameLength + PasswordLength + NameLength);

        std::array<UserType, UsersAmount>    usersStorage;
        std::array<User*, UsersAmount>       usersPointers;
        std::array<uint8_t, SerializeBufferSize> serializeStorage;

    public:
        StaticUserManager(PersistentStorage* storage = nullptr)
            : UserManager(usersPointers, serializeStorage, storage)
        {
            for (size_t i = 0; i < UsersAmount; i++)
                usersPointers[i] = &usersStorage[i];

            load();   // slots are ready now; pull any persisted users
        }
};
