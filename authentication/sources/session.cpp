#include "session.hpp"

#include <random>

Session::Session(const User& user, uint32_t validitySeconds, uint32_t currentTime)
{
    start(user, validitySeconds, currentTime);
}

void Session::start(const User& user, uint32_t validitySeconds, uint32_t currentTime)
{
    this->user = &user;
    expireTime = currentTime + validitySeconds;

    static std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<TokenType> dist;
    token = dist(rng);
}

const User* Session::getUser() const
{
    return user;
}

Session::TokenType Session::getToken() const
{
    return token;
}

bool Session::isExpired() const
{
    return !expireTime || !user || !token;
}

void Session::update(uint32_t currentTime)
{
    if(currentTime < expireTime)
        expire();
}

void Session::expire()
{
    user = nullptr;
    token = 0;
    expireTime = 0;
}

bool Session::operator==(const Session& other) const
{
    return
        user == other.user &&
        token == other.token &&
        expireTime == other.expireTime;
}