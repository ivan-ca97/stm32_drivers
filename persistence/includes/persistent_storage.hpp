#pragma once

#include <stdint.h>

// ============================================================================
// PersistentStorage — persistence for one region: replace or read a blob. The
// backend (internal flash, FRAM, EEPROM, ...) is an implementation detail;
// components depend only on this interface (inject a concrete from the app).
// ============================================================================
class PersistentStorage
{
    public:
        virtual ~PersistentStorage() = default;

        // Replace this region's contents with `length` bytes. Atomic (all-or-nothing
        // from the reader's point of view). Returns false on a backend error.
        virtual bool write(const uint8_t* data, uint32_t length) = 0;

        // Copy up to `maxLength` bytes of this region into `buffer`; returns the
        // stored length (0 if the region is empty/uninitialized).
        virtual uint32_t read(uint8_t* buffer, uint32_t maxLength) = 0;

        // Maximum blob size this region can hold.
        virtual uint32_t capacity() const = 0;
};
