#pragma once

#include "user.hpp"

typedef uint64_t TokenType;

class Session
{
    public:
        Session() = default;
        Session(const User& user, uint32_t validitySeconds, uint32_t currentTime);

        void start(const User& user, uint32_t validitySeconds, uint32_t currentTime);

        const User* getUser() const;
        TokenType getToken() const;
        bool isExpired() const;
        void update(uint32_t currentTime);
        void expire();

        bool operator==(const Session& other) const;

    protected:
        const User* user = nullptr;
        TokenType token = 0;
        uint32_t expireTime = 0;
};