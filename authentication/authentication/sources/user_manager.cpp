#include "user_manager.hpp"

#include <cstring>
#include <algorithm>

ResultUser UserManager::createUser(Permission newPermission, std::string_view newUsername, std::string_view newPassword, std::string_view newName)
{
    User* newUser = getFreeUser();
    if(!newUser)
        return Error(AuthenticationError::UsersBufferFull);

    if(usernameExists(newUsername))
        return Error(AuthenticationError::UsernameAlreadyExists);


    newUser->setPermission(newPermission);
    newUser->setId(idCounter++);
    auto set = newUser->setBufferedFields(newUsername, newPassword, newName);
    if(!set)
        return Error(set.error());

    newUser->makeValid();

    loadedUsers++;
    persist();
    return newUser;
}

ResultUser UserManager::getUser(std::string_view username) const
{
    return getUserByUsername(username);
}

ResultUser UserManager::getUser(User::IdType id) const
{
    return getUserById(id);
}

Result<User*> UserManager::getUserByUsername(std::string_view username) const
{
    auto findLambda = [&](const User* user) {return user && user->getUsername() == username;};
    auto it = std::find_if(users.begin(), users.end(), findLambda);

    if(it == users.end())
        return Error(AuthenticationError::UsernameNotFound);

    return *it;
}

Result<User*> UserManager::getUserById(User::IdType id) const
{
    auto findLambda = [&](const User* user) {return user && user->getId() == id;};
    auto it = std::find_if(users.begin(), users.end(), findLambda);

    if(it == users.end())
        return Error(AuthenticationError::UserIdNotFound);

    return *it;
}

ResultVoid UserManager::setUsername(User::IdType id, std::string_view newUsername)
{
    if(newUsername.empty())
        return Error(AuthenticationError::EmptyMandatoryField);

    auto userResult = getUserById(id);
    if(!userResult)
        return Error(userResult.error());
    User* user = *userResult;

    if(user->getUsername() != newUsername && usernameExists(newUsername))
        return Error(AuthenticationError::UsernameAlreadyExists);

    auto set = user->setUsername(newUsername);
    if(!set)
        return Error(set.error());

    persist();
    return {};
}

ResultVoid UserManager::setPassword(User::IdType id, std::string_view newPassword)
{
    if(newPassword.empty())
        return Error(AuthenticationError::EmptyMandatoryField);

    auto userResult = getUserById(id);
    if(!userResult)
        return Error(userResult.error());

    auto set = (*userResult)->setPassword(newPassword);
    if(!set)
        return Error(set.error());

    persist();
    return {};
}

ResultVoid UserManager::setName(User::IdType id, std::string_view newName)
{
    auto userResult = getUserById(id);
    if(!userResult)
        return Error(userResult.error());

    auto set = (*userResult)->setName(newName);
    if(!set)
        return Error(set.error());

    persist();
    return {};
}

ResultVoid UserManager::setPermission(User::IdType id, Permission newPermission)
{
    auto userResult = getUserById(id);
    if(!userResult)
        return Error(userResult.error());

    (*userResult)->setPermission(newPermission);

    persist();
    return {};
}

ResultVoid UserManager::deleteUser(std::string_view username)
{
    auto user = getUserByUsername(username);
    if(!user)
        return Error(user.error());

    return deleteUser(**user);
}

ResultVoid UserManager::deleteUser(User::IdType id)
{
    auto storedUser = getUserById(id);

    if(!storedUser)
        return Error(storedUser.error());

    (*storedUser)->reset();
    loadedUsers--;

    persist();
    return {};
}

ResultVoid UserManager::deleteUser(const User& user)
{
    auto storedUser = getUserById(user.getId());

    if(!storedUser || **storedUser != user)
        return Error(AuthenticationError::UserIdNotFound);

    (*storedUser)->reset();
    loadedUsers--;

    persist();
    return {};
}

User* UserManager::getFreeUser() const
{
    auto findLambda = [&](const User* user) {return user && !user->isValid();};
    auto it = std::find_if(users.begin(), users.end(), findLambda);

    if(it == users.end())
        return nullptr;

    return *it;
}

bool UserManager::usernameExists(std::string_view username) const
{
    return static_cast<bool>(getUserByUsername(username));
}

bool UserManager::persist()
{
    if(!storage)
        return true;

    uint32_t n = serialize(serializeBuffer.data(), serializeBuffer.size());
    return storage->write(serializeBuffer.data(), n);
}

void UserManager::load()
{
    if(!storage)
        return;

    uint32_t n = storage->read(serializeBuffer.data(), serializeBuffer.size());
    if(n > 0 && n <= serializeBuffer.size())
        deserialize(serializeBuffer.data(), n);
}

uint32_t UserManager::serialize(uint8_t* buf, uint32_t cap) const
{
    if(cap < 1)
        return 0;

    uint32_t n = 0;
    uint32_t countPos = n++;    // reserve the count byte
    uint8_t count = 0;

    for(const User* user : users)
    {
        if(!user || !user->isValid())
            continue;

        auto username = user->getUsername();
        auto password = user->getPassword();
        auto name     = user->getName();

        uint32_t need = 2 + 1 + (1 + username.size()) + (1 + password.size()) + (1 + name.size());
        if(n + need > cap)
            break;   // out of buffer

        User::IdType id = user->getId();
        buf[n++] = static_cast<uint8_t>(id);
        buf[n++] = static_cast<uint8_t>(id >> 8);
        buf[n++] = static_cast<uint8_t>(user->getPermission());

        buf[n++] = static_cast<uint8_t>(username.size());
        std::memcpy(buf + n, username.data(), username.size()); n += username.size();
        buf[n++] = static_cast<uint8_t>(password.size());
        std::memcpy(buf + n, password.data(), password.size()); n += password.size();
        buf[n++] = static_cast<uint8_t>(name.size());
        std::memcpy(buf + n, name.data(), name.size()); n += name.size();

        count++;
    }

    buf[countPos] = count;
    return n;
}

void UserManager::deserialize(const uint8_t* buf, uint32_t len)
{
    if(len < 1)
        return;

    uint32_t off = 0;
    uint8_t count = buf[off++];

    for(uint8_t i = 0; i < count; i++)
    {
        if(off + 3 > len)
            return;   // truncated record

        User::IdType id = uint16_t(buf[off]) | (uint16_t(buf[off + 1]) << 8);
        off += 2;
        Permission perm = static_cast<Permission>(buf[off++]);

        // Three length-prefixed fields; bail out on any overrun.
        std::string_view fields[3];
        bool malformed = false;
        for(auto& field : fields)
        {
            if(off + 1 > len) { malformed = true; break; }
            uint8_t fieldLen = buf[off++];
            if(off + fieldLen > len) { malformed = true; break; }
            field = std::string_view(reinterpret_cast<const char*>(buf + off), fieldLen);
            off += fieldLen;
        }
        if(malformed)
            return;

        User* slot = getFreeUser();
        if(!slot)
            return;   // no room

        slot->setPermission(perm);
        slot->setId(id);
        slot->setBufferedFields(fields[0], fields[1], fields[2]);
        slot->makeValid();

        loadedUsers++;
        if(id >= idCounter)
            idCounter = id + 1;
    }
}

UserManager::UserManager(std::span<User*> usersStorage, std::span<uint8_t> serializeBuffer, PersistentStorage* storage)
    : users(usersStorage), serializeBuffer(serializeBuffer), storage(storage)
{}
