#include <cstdint>
#include <expected>
#include <span>

#include "stm32f4/error.h"
#include "stm32f4/flash_pages.h"

namespace stm32f4 {

FlashPages::FlashPages(Flash& flash, Sector first, Sector second, uint32_t size)
    : flash_(flash), sectors_{first, second}, size_(size)
{
}

uint32_t FlashPages::pageSize() const
{
    return size_;
}

base::Status FlashPages::erase(uint8_t page)
{
    if (!flash_.eraseSector(sectorOf(page).number)) {
        return std::unexpected(eraseFailed);
    }
    return base::ok;
}

base::Status FlashPages::program(uint8_t page, uint32_t offset, std::span<const uint8_t> data)
{
    if (!flash_.write(sectorOf(page).address + offset, data.data(), data.size())) {
        return std::unexpected(programFailed);
    }
    return base::ok;
}

void FlashPages::read(uint8_t page, uint32_t offset, std::span<uint8_t> out) const
{
    flash_.read(sectorOf(page).address + offset, out.data(), out.size());
}

// The log only ever asks for pages 0 and 1.
const FlashPages::Sector& FlashPages::sectorOf(uint8_t page) const
{
    return sectors_[page == 0 ? 0 : 1];
}

} // namespace stm32f4
