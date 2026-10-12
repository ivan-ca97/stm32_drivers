#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "base/passkey.h"
#include "base/result.h"
#include "flash_log/error.h"
#include "flash_log/pages.h"

namespace flash_log {

// Values under up to maxKeys keys, kept as a log over two Pages. A write appends a
// record, and the last whole record of a key is its value, so a write never erases.
// Where each key's value sits is kept in RAM, rebuilt from the log on start-up.
//
// Compaction copies the latest record of every key to the other page, erasing it first.
// That is the only erase, and it needs allowCompaction() in effect: then it runs when
// compact() is called, or from a write that does not fit. Under forbidCompaction(), the
// default, compact() fails with compactionForbidden and such a write with full.
//
// A record or a page counts only once its last byte is written, so losing power at any
// point leaves the previous values readable.
class Store {
public:
    class Builder {
    public:
        Builder& pages(Pages& pages);

        // Formats the pages if neither page holds a log. May erase.
        base::Result<Store> build() const;

    private:
        Pages* pages_ = nullptr;

        friend class Store;
    };

    using Permit = base::Passkey<Builder>;

    static constexpr uint8_t maxKeys = 64;

    // Rebuilds the index from the given page.
    Store(Pages& pages, uint8_t activePage, uint32_t generation, Permit permit);

    Store(const Store&) = delete;
    Store& operator=(const Store&) = delete;

    // Fails with keyOutOfRange, tooLarge, full, or the pages' own error.
    base::Status write(uint8_t key, std::span<const uint8_t> data);

    // Copies the key's value into `out` and returns its size. A key never written reads
    // as empty. Fails with keyOutOfRange, or bufferTooSmall, copying nothing.
    base::Result<uint32_t> read(uint8_t key, std::span<uint8_t> out) const;

    // Whether compaction may run, from compact() or from a write that does not fit.
    void allowCompaction();
    void forbidCompaction();

    // Erases the other page and moves the latest value of every key there. Blocks for as
    // long as the erase takes. Fails with compactionForbidden, or the pages' own error.
    base::Status compact();

    // Room left in the page for new records, headers included.
    uint32_t freeBytes() const;

    // Most data a single record can carry.
    uint32_t maxRecordData() const;

private:
    struct Record {
        uint32_t offset;   // 0: none, since the page header sits there
        uint16_t length;
    };

    using Index = std::array<Record, maxKeys>;

    uint32_t scan(uint8_t page, Index& index) const;
    base::Status append(uint8_t key, std::span<const uint8_t> data);

    Pages& pages_;
    uint8_t activePage_;
    uint32_t generation_;
    uint32_t writeOffset_ = 0;
    bool compactionAllowed_ = false;
    Index index_ = {};
};

} // namespace flash_log
