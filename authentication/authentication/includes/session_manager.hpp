#pragma once

#include "session.hpp"

#include "authentication_errors.hpp"
#include "clock.hpp"

// Injected session-token source. The entropy (HW RNG / chip UID + counter / ...) is
// provided by the application; this component only depends on the interface.
class TokenGenerator
{
    public:
        virtual ~TokenGenerator() = default;
        virtual Session::TokenType next() = 0;
};

using ResultSession = Result<const Session*>;

class SessionManager
{
    public:
        SessionManager(std::span<Session*> sessionsStorage, const Clock& clock, TokenGenerator& tokenGenerator);

        ResultSession validate(Session::TokenType token);

        const Session* getSession(const User& user) const;

        ResultSession createSession(const User& user);

        void expireSession(const Session& user);

        void updateSessions();

        bool hasSession(const User& user) const;

    protected:
        std::span<Session*> sessions;

        const Clock* clock;
        TokenGenerator* tokenGenerator;

        uint32_t sessionValiditySeconds = 3600;

        Session* getFreeSession();

        Session* getSessionByUser(const User& user) const;
};

template <size_t SessionAmount>
class StaticSessionManager : public SessionManager
{
    protected:
        std::array<Session, SessionAmount> sessionStorage;
        std::array<Session*, SessionAmount> sessionPointers;

    public:
        StaticSessionManager(const Clock& clock, TokenGenerator& tokenGenerator)
            : SessionManager(sessionPointers, clock, tokenGenerator)
        {
            for (size_t i = 0; i < SessionAmount; i++)
                sessionPointers[i] = &sessionStorage[i];
        };
};
