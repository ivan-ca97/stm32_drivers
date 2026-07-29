#include "session_manager.hpp"

#include <stdexcept>
#include <algorithm>

ResultSession SessionManager::validate(Session::TokenType token)
{
    auto findLambda = [&](const Session* session) {return session->getToken() == token;};
    auto it = std::find_if(sessions.begin(), sessions.end(), findLambda);

    if(it == sessions.end())
        return Error(AuthenticationError::InvalidToken);

    return *it;
}

const Session* SessionManager::getSession(const User& user) const
{
    return getSessionByUser(user);
}

ResultSession SessionManager::createSession(const User& user)
{
    // Invalidate currently active session if there is one for the user.
    Session* session = getSessionByUser(user);
    if(session)
        session->expire();

    session = getFreeSession();
    if(!session)
        return Error(AuthenticationError::SessionBufferFull);

    session->start(user, sessionValiditySeconds, clock->getTime());
    return session;
}

Session* SessionManager::getFreeSession()
{
    auto findLambda = [&](const Session* session) {return session->isExpired();};
    auto it = std::find_if(sessions.begin(), sessions.end(), findLambda);

    if(it == sessions.end())
        return nullptr;

    return *it;
}

Session* SessionManager::getSessionByUser(const User& user) const
{
    auto findLambda = [&](const Session* session) {return session->getUser() == &user;};
    auto it = std::find_if(sessions.begin(), sessions.end(), findLambda);

    if(it == sessions.end())
        return nullptr;

    return *it;
}

void SessionManager::expireSession(const Session& sessionToExpire)
{
    auto findLambda = [&](const Session* session) {return *session == sessionToExpire;};
    auto it = std::find_if(sessions.begin(), sessions.end(), findLambda);

    if(it == sessions.end())
        return;

    (*it)->expire();
}

void SessionManager::updateSessions()
{
    auto time = clock->getTime();
    for(auto it = sessions.begin(); it != sessions.end(); it++)
        (*it)->update(time);
}

bool SessionManager::hasSession(const User& user) const
{
    return getSession(user) != nullptr;
}

SessionManager::SessionManager(std::span<Session*> sessionsStorage, const Clock& clock)
    : sessions(sessionsStorage), clock(&clock)
{}