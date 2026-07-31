#pragma once

#include <stdint.h>
#include "register_bank.hpp"

// ============================================================================
// RegisterWindow — a bounded, write-protected view over a RegisterBank. A section
// gets a window [base, base+length) and publishes with window-local offsets; a
// write outside the window is dropped, so a section cannot clobber another
// section's registers. The master still reads the whole bank; the window only
// scopes what a section writes (and, since a section holds only its window, what
// it can address at all).
// ============================================================================
class RegisterWindow
{
    public:
        RegisterWindow(RegisterBank& bank, uint8_t base, uint8_t length)
            : bank(bank), base(base), length(length) {}

        // Read within the window (offset local to the window).
        uint8_t read(uint8_t offset) const
        {
            return offset < length ? bank.read(base + offset) : 0x00;
        }

        // Write within the window; out-of-window writes are dropped.
        void setRegister(uint8_t offset, uint8_t value);
        void writeRegisters(uint8_t offset, const uint8_t* src, uint8_t len);

        uint8_t size() const { return length; }

    private:
        RegisterBank& bank;
        uint8_t       base;
        uint8_t       length;
};
