#pragma once

#include <stdint.h>

// ============================================================================
// RegisterBank — flat register space served for immediate (pre-loaded) reads.
// Written by the main loop, read by the ISR. It can be shared by several
// ControllerSlave instances so they expose the same state.
// ============================================================================
class RegisterBank
{
    public:
        static constexpr uint8_t COUNT = 128;   // register space 0x00..0x7F

        uint8_t read(uint16_t addr) const { return addr < COUNT ? registers[addr] : 0x00; }

        void setRegister(uint8_t addr, uint8_t value);
        void writeRegisters(uint8_t addr, const uint8_t* src, uint8_t length);

    private:
        uint8_t registers[COUNT] = {};
};
