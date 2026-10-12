#include <cstdint>
#include <span>

#include "flash_log/region.h"

namespace flash_log {

Region::Region(Store& store, uint8_t key, uint32_t capacity)
    : store_(store),
      key_(key),
      capacity_(capacity < store.maxRecordData() ? capacity : store.maxRecordData())
{
}

bool Region::write(const uint8_t* data, uint32_t length)
{
    if (length > capacity_) {
        return false;
    }
    base::Status written = store_.write(key_, std::span<const uint8_t>(data, length));
    return written.has_value();
}

uint32_t Region::read(uint8_t* buffer, uint32_t maxLength)
{
    base::Result<uint32_t> copied = store_.read(key_, std::span<uint8_t>(buffer, maxLength));
    return copied.value_or(0);
}

uint32_t Region::capacity() const
{
    return capacity_;
}

} // namespace flash_log
