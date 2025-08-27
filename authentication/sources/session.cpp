#include "session.hpp"

#include <random>

Session::Session(User& user, uint32_t validitySeconds, uint32_t currentTime)
{
    start(user, validitySeconds, currentTime);
}

void Session::start(User& user, uint32_t validitySeconds, uint32_t currentTime)
{
    this->user = &user;
    expireTime = currentTime + validitySeconds;

    static std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<TokenType> dist;
    token = dist(rng);
}

User* Session::getUser() const
{
    return user;
}

TokenType Session::getToken() const
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