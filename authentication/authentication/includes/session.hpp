#pragma once

#include "user.hpp"

class Session
{
    public:
        using TokenType = uint64_t;

        Session() = default;

        void start(const User& user, uint32_t validitySeconds, uint32_t currentTime, TokenType token);

        const User* getUser() const;
        TokenType getToken() const;
        bool isExpired(uint32_t currentTime) const;
        void update(uint32_t currentTime);
        void expire();

        bool operator==(const Session& other) const;

    protected:
        const User* user = nullptr;
        TokenType token = 0;
        uint32_t expireTime = 0;
};