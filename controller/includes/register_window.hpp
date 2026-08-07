#pragma once

#include <stdint.h>

#include "register_bank.hpp"

// RegisterWindow — a bounded, write-protected view over a RegisterBank. A writer
// gets a window [base, base+length) and publishes with window-local offsets; a
// write outside the window is dropped, so one writer cannot clobber another's
// registers. The master still reads the whole bank; the window only scopes what a
// writer publishes into (and, since it holds only its window, what it can address).
//
// A window is obtained by claiming its span from the bank.
class RegisterWindow
{
    public:
        using ClaimResult = lib::Result<RegisterWindow, RegisterBank::ReserveError>;

        // Reserve [base, base+length) in the bank and return the view over it.
        static ClaimResult claim(RegisterBank& bank, uint8_t base, uint8_t length);

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
        RegisterWindow(RegisterBank& bank, uint8_t base, uint8_t length)
            : bank(bank), base(base), length(length) {}

        RegisterBank& bank;
        uint8_t base;
        uint8_t length;
};
