#pragma once

#include <cstdint>

#include "flash_log/store.h"
#include "persistent_storage.hpp"

namespace flash_log {

// A blob kept under one key of a Store, behind the PersistentStorage interface.
class Region final : public PersistentStorage {
public:
    // `capacity` is capped at what one record can carry.
    Region(Store& store, uint8_t key, uint32_t capacity);

    Region(const Region&) = delete;
    Region& operator=(const Region&) = delete;

    bool write(const uint8_t* data, uint32_t length) override;
    uint32_t read(uint8_t* buffer, uint32_t maxLength) override;
    uint32_t capacity() const override;

private:
    Store& store_;
    uint8_t key_;
    uint32_t capacity_;
};

} // namespace flash_log
