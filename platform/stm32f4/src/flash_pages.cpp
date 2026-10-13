#include <cstdint>
#include <expected>
#include <span>
#include <utility>

#include "stm32f4/error.h"
#include "stm32f4/flash_pages.h"

namespace stm32f4 {

FlashPages::Builder& FlashPages::Builder::flash(Flash& flash)
{
    flash_ = &flash;
    return *this;
}

FlashPages::Builder& FlashPages::Builder::sectors(Sector first, Sector second, uint32_t size)
{
    sectors_[0] = first;
    sectors_[1] = second;
    size_ = size;
    return *this;
}

FlashPages::Builder& FlashPages::Builder::stallListener(StallListener& listener)
{
    stallListener_ = &listener;
    return *this;
}

base::Result<FlashPages> FlashPages::Builder::build() const
{
    if (flash_ == nullptr || size_ == 0) {
        return std::unexpected(missingPart);
    }
    if (sectors_[0].number == sectors_[1].number) {
        return std::unexpected(sameSector);
    }
    return base::Result<FlashPages>(std::in_place, *this, Permit{});
}

FlashPages::FlashPages(const Builder& parts, Permit)
    : flash_(*parts.flash_),
      sectors_{parts.sectors_[0], parts.sectors_[1]},
      size_(parts.size_),
      stallListener_(parts.stallListener_)
{
}

uint32_t FlashPages::pageSize() const
{
    return size_;
}

base::Status FlashPages::erase(uint8_t page)
{
    if (stallListener_ != nullptr) {
        stallListener_->stallStarting();
    }
    bool erased = flash_.eraseSector(sectorOf(page).number);
    if (stallListener_ != nullptr) {
        stallListener_->stallEnded();
    }

    if (!erased) {
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
