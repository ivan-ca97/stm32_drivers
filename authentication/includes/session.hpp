#pragma once

#include "user.hpp"

typedef uint64_t TokenType;

class Session
{
    public:
        Session() = default;
        Session(User& user, uint32_t validitySeconds, uint32_t currentTime);

        void start(User& user, uint32_t validitySeconds, uint32_t currentTime);

        User* getUser() const;
        TokenType getToken() const;
        bool isExpired() const;
        void update(uint32_t currentTime);
        void expire();

    protected:
        User* user;
        TokenType token;
        uint32_t expireTime;
};