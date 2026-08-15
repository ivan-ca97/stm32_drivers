#pragma once

#include "persistent_storage.hpp"

// NoStorage — accepts every write and never has anything to give back. A stand-in
// for a backend that is not there yet: consumers keep their defaults.
class NoStorage : public PersistentStorage
{
    public:
        bool write(const uint8_t*, uint32_t) override { return true; }

        uint32_t read(uint8_t*, uint32_t) override { return 0; }

        uint32_t capacity() const override { return 0; }
};
