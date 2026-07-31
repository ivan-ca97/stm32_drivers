#include "register_window.hpp"

void RegisterWindow::setRegister(uint8_t offset, uint8_t value)
{
    if(offset < length)
        bank.setRegister(base + offset, value);
}

void RegisterWindow::writeRegisters(uint8_t offset, const uint8_t* src, uint8_t len)
{
    for(uint8_t i = 0; i < len && (offset + i) < length; i++)
        bank.setRegister(base + offset + i, src[i]);
}
