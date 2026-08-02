#include "session.hpp"

void Session::start(const User& user, uint32_t validitySeconds, uint32_t currentTime, TokenType token)
{
    this->user = &user;
    expireTime = currentTime + validitySeconds;
    this->token = token;
}

const User* Session::getUser() const
{
    return user;
}

Session::TokenType Session::getToken() const
{
    return token;
}

bool Session::isExpired(uint32_t currentTime) const
{
    // A slot is expired if it was never started/was cleared (user/token/expireTime
    // zeroed) or its validity has elapsed. expireTime == 0 falls into currentTime
    // >= expireTime, so a cleared slot always reads as expired.
    return !user || !token || currentTime >= expireTime;
}

void Session::update(uint32_t currentTime)
{
    if(isExpired(currentTime))
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
