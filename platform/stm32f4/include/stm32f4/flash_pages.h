#pragma once

#include <cstdint>
#include <span>

#include "base/passkey.h"
#include "base/result.h"
#include "flash.hpp"
#include "flash_log/pages.h"
#include "stm32f4/stall_listener.h"

namespace stm32f4 {

// flash_log::Pages over two internal flash sectors of the same size.
// Erasing or programming stalls the CPU, since it fetches from the same flash. Programming
// takes microseconds, an erase takes long enough that a StallListener is told around it.
class FlashPages final : public flash_log::Pages {
public:
    struct Sector {
        uint8_t number;
        uint32_t address;
    };

    class Builder {
    public:
        Builder& flash(Flash& flash);
        Builder& sectors(Sector first, Sector second, uint32_t size);
        // Optional.
        Builder& stallListener(StallListener& listener);

        base::Result<FlashPages> build() const;

    private:
        Flash* flash_ = nullptr;
        Sector sectors_[2] = {};
        uint32_t size_ = 0;
        StallListener* stallListener_ = nullptr;

        friend class FlashPages;
    };

    using Permit = base::Passkey<Builder>;

    FlashPages(const Builder& parts, Permit permit);

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
    StallListener* stallListener_;
};

} // namespace stm32f4
