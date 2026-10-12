#pragma once

#include <cstdint>
#include <span>

#include "base/result.h"
#include "flash.hpp"
#include "flash_log/pages.h"

namespace stm32f4 {

// The two pages of a flash_log::Pages, as two internal flash sectors of the same size.
// Erasing or programming stalls the CPU, since it fetches from the same flash.
class FlashPages final : public flash_log::Pages {
public:
    struct Sector {
        uint8_t number;
        uint32_t address;
    };

    FlashPages(Flash& flash, Sector first, Sector second, uint32_t size);

    FlashPages(const FlashPages&) = delete;
    FlashPages& operator=(const FlashPages&) = delete;

    uint32_t pageSize() const override;
    base::Status erase(uint8_t page) override;
    base::Status program(uint8_t page, uint32_t offset, std::span<const uint8_t> data) override;
    void read(uint8_t page, uint32_t offset, std::span<uint8_t> out) const override;

private:
    const Sector& sectorOf(uint8_t page) const;

    Flash& flash_;
    Sector sectors_[2];
    uint32_t size_;
};

} // namespace stm32f4
