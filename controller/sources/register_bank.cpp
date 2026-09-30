#include "register_bank.hpp"

void RegisterBank::setRegister(uint8_t addr, uint8_t value)
{
    if(addr < registers.size())
        registers[addr] = value;
}

void RegisterBank::writeRegisters(uint8_t addr, const uint8_t* src, uint8_t length)
{
    for(uint8_t i = 0; i < length && (addr + i) < registers.size(); i++)
        registers[addr + i] = src[i];
}

RegisterBank::ReserveResult RegisterBank::reserve(uint8_t base, uint8_t length)
{
    if(length == 0)
        return std::unexpected(register_bank::emptySpan);
    if(uint16_t(base) + length > registers.size())
        return std::unexpected(register_bank::outOfBank);
    if(reservationCount >= reservations.size())
        return std::unexpected(register_bank::tooManyWindows);

    // Interval intersection against every span already handed out.
    const uint16_t end = uint16_t(base) + length;
    for(uint8_t i = 0; i < reservationCount; i++)
    {
        const uint16_t otherEnd = uint16_t(reservations[i].base) + reservations[i].length;
        if(base < otherEnd && reservations[i].base < end)
            return std::unexpected(register_bank::overlap);
    }

    reservations[reservationCount++] = { base, length };
    return {};
}
