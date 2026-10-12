#pragma once

#include <cstdint>

#include "base/result.h"

namespace safety {

// The persistent storage, as far as keeping room in it goes. Writes only ever add, and
// compaction is what reclaims the room taken by superseded ones.
class StorageUpkeep {
public:
    virtual ~StorageUpkeep() = default;

    // Room left for new writes, in bytes.
    virtual uint32_t freeBytes() const = 0;

    // Reclaims the room taken by superseded writes. Blocks for as long as it takes.
    virtual base::Status compact() = 0;
};

} // namespace safety
