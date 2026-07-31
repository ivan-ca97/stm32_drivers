#include "register_bank.hpp"

void RegisterBank::setRegister(uint8_t addr, uint8_t value)
{
    if(addr < COUNT)
        registers[addr] = value;
}

void RegisterBank::writeRegisters(uint8_t addr, const uint8_t* src, uint8_t length)
{
    for(uint8_t i = 0; i < length && (addr + i) < COUNT; i++)
        registers[addr + i] = src[i];
}
