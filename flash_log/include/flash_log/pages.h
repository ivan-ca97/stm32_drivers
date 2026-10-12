#pragma once

#include <cstdint>
#include <span>

#include "base/result.h"

namespace flash_log {

// Two erasable pages of the same size. An erased byte reads 0xFF, and programming can
// only clear bits, so a byte is programmed once after each erase, except to clear more
// of its bits.
class Pages {
public:
    virtual ~Pages() = default;

    virtual uint32_t pageSize() const = 0;

    // Sets the whole page to 0xFF. May block for a long time.
    virtual base::Status erase(uint8_t page) = 0;

    virtual base::Status program(uint8_t page, uint32_t offset, std::span<const uint8_t> data) = 0;

    virtual void read(uint8_t page, uint32_t offset, std::span<uint8_t> out) const = 0;
};

} // namespace flash_log
