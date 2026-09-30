#pragma once

#include <stdint.h>
#include <array>
#include <span>

#include "base/result.h"
#include "register_bank_error.hpp"

// RegisterBank — flat register space served for immediate (pre-loaded) reads.
// Written by the main loop, read by the ISR. It can be shared by several
// ControllerSlave instances so they expose the same state.
//
// The bank also owns the layout: writers reserve the span they publish into, and a
// reservation that overlaps an existing one is refused. Without that, two writers
// given bad bases would silently corrupt each other's registers.
//
// Storage is supplied by the caller (see StaticRegisterBank), so each system sizes
// its own register space.
class RegisterBank
{
    public:
        // Fails with one of register_bank's errors.
        using ReserveResult = base::Status;

        struct Reservation
        {
            uint8_t base;
            uint8_t length;
        };

        uint8_t count() const { return static_cast<uint8_t>(registers.size()); }

        uint8_t read(uint16_t addr) const
        {
            return addr < registers.size() ? registers[addr] : 0x00;
        }

        void setRegister(uint8_t addr, uint8_t value);
        void writeRegisters(uint8_t addr, const uint8_t* src, uint8_t length);

        // Claim [base, base + length) for one writer. Reservations are permanent:
        // the layout is decided at start-up and never changes.
        ReserveResult reserve(uint8_t base, uint8_t length);

    protected:
        RegisterBank(std::span<uint8_t> registers, std::span<Reservation> reservations)
            : registers(registers), reservations(reservations) {}

        std::span<uint8_t> registers;
        std::span<Reservation> reservations;
        uint8_t reservationCount = 0;
};

// Register bank with its own storage. `Count` is the register space; `MaxWindows`
// is how many writers may claim a span of it.
template <uint8_t Count, uint8_t MaxWindows>
class StaticRegisterBank : public RegisterBank
{
    public:
        StaticRegisterBank() : RegisterBank(registerStorage, reservationStorage) {}

    protected:
        std::array<uint8_t, Count> registerStorage = {};
        std::array<Reservation, MaxWindows> reservationStorage = {};
};
