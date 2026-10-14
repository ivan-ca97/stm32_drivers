#pragma once

#include <cstdint>

#include "base/result.h"
#include "flash_log/store.h"
#include "safety/storage_upkeep.h"

namespace storage {

// safety::StorageUpkeep over a flash_log::Store.
class FlashUpkeep final : public safety::StorageUpkeep {
public:
    explicit FlashUpkeep(flash_log::Store& store);

    FlashUpkeep(const FlashUpkeep&) = delete;
    FlashUpkeep& operator=(const FlashUpkeep&) = delete;

    uint32_t freeBytes() const override;
    base::Status compact() override;

private:
    flash_log::Store& store_;
};

} // namespace storage
