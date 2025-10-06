#include "serial_authentication.hpp"

uint8_t SerialAuthentication::logIn()
{
    uint8_t byte = 0;
    switch(state)
    {
        case State::SendingToken:
            if(!getTokenByte(&byte))
                break;
            state = State::SendingErrorCode;
            break;

        default:
            break;
    }

    return byte;
}

uint8_t SerialAuthentication::createUser()
{
    uint8_t byte = 0;
    switch(state)
    {
        case State::SendingId:
            if(!getIdByte(&byte))
                break;
            state = State::SendingErrorCode;
            break;

        default:
            break;
    }

    return byte;
}