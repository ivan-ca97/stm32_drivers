#include <cstdint>
#include <expected>

#include "storage/flash_upkeep.h"

namespace storage {

FlashUpkeep::FlashUpkeep(flash_log::Store& store)
    : store_(store)
{
}

uint32_t FlashUpkeep::freeBytes() const
{
    return store_.freeBytes();
}

base::Status FlashUpkeep::compact()
{
    store_.allowCompaction();
    base::Status compacted = store_.compact();
    store_.forbidCompaction();

    if (!compacted.has_value()) {
        return std::unexpected(compacted.error());
    }
    return base::ok;
}

} // namespace storage
