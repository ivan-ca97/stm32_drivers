#pragma once

#include <stdint.h>

// ============================================================================
// Flash — generic internal-flash driver for the STM32F4 (register-level; ST
// provides no LL flash API, so this drives the FLASH registers directly).
//
// Flash can only clear bits 1->0, so a region must be erased (to 0xFF) before it
// can be programmed. The erase unit is a whole sector. Reads are plain
// memory-mapped accesses.
//
// Notes:
//   - Erase/program stall instruction fetch from flash, so they block the CPU
//     (a large sector erase can take ~1 s). Use during idle/maintenance only.
//   - After erase/program the data cache is reset so later reads see the new
//     contents.
// ============================================================================
class Flash
{
    public:
        // Erase a whole sector (to 0xFF). Returns false on a flash error.
        bool eraseSector(uint8_t sector);

        // Program `length` bytes at absolute `address` (must already be erased).
        // Verifies with a read-back. Returns false on error or mismatch.
        bool write(uint32_t address, const uint8_t* data, uint32_t length);

        // Read `length` bytes from absolute `address` (flash is memory-mapped).
        void read(uint32_t address, uint8_t* out, uint32_t length) const;

    protected:
        static void waitWhileBusy();
        static void unlock();
        static void lock();
        static bool checkAndClearErrors();
        static void flushDataCache();
};
