#include <cstdint>
#include <expected>
#include <span>
#include <utility>

#include "flash_log/store.h"
#include "layout.h"

namespace flash_log {
namespace {

// Bytes moved per read/program while checking or copying a record.
constexpr uint32_t chunkSize = 32;

struct PageHeader {
    bool valid;
    uint32_t generation;
};

uint32_t readLittleEndian32(std::span<const uint8_t> bytes)
{
    return uint32_t(bytes[0])
        | (uint32_t(bytes[1]) << 8)
        | (uint32_t(bytes[2]) << 16)
        | (uint32_t(bytes[3]) << 24);
}

void writeLittleEndian32(std::span<uint8_t> bytes, uint32_t value)
{
    for (uint32_t i = 0; i < 4; i++) {
        bytes[i] = uint8_t(value >> (8 * i));
    }
}

// CRC-16/CCITT-FALSE, continued from `crc`.
uint16_t crc16(std::span<const uint8_t> bytes, uint16_t crc)
{
    for (uint8_t byte : bytes) {
        crc ^= uint16_t(byte) << 8;
        for (int bit = 0; bit < 8; bit++) {
            bool carry = (crc & 0x8000) != 0;
            crc = uint16_t(crc << 1);
            if (carry) {
                crc ^= 0x1021;
            }
        }
    }
    return crc;
}

constexpr uint16_t crcSeed = 0xFFFF;

PageHeader readPageHeader(const Pages& pages, uint8_t page)
{
    uint8_t bytes[layout::pageHeaderSize];
    pages.read(page, 0, bytes);

    std::span<const uint8_t> header(bytes);
    PageHeader result = {
        .valid = readLittleEndian32(header.subspan(layout::magicOffset, 4)) == layout::magic
            && bytes[layout::pageCommittedOffset] == layout::committed,
        .generation = readLittleEndian32(header.subspan(layout::generationOffset, 4)),
    };
    return result;
}

// Visits every committed record of a page, as (key, offset, length, crc). Returns the
// first free offset, or the page size when a torn header leaves the rest untrustworthy.
template <typename Visit>
uint32_t walk(const Pages& pages, uint8_t page, Visit visit)
{
    uint32_t pageSize = pages.pageSize();
    uint32_t offset = layout::pageHeaderSize;

    while (offset + layout::recordHeaderSize <= pageSize) {
        uint8_t header[layout::recordHeaderSize];
        pages.read(page, offset, header);

        if (header[layout::keyOffset] == layout::erased) {
            return offset;
        }

        uint16_t length = uint16_t(header[layout::lengthOffset])
            | uint16_t(header[layout::lengthOffset + 1] << 8);
        if (length == layout::tornLength
            || offset + layout::recordHeaderSize + length > pageSize) {
            return pageSize;
        }

        if (header[layout::recordCommittedOffset] == layout::committed) {
            uint16_t crc = uint16_t(header[layout::crcOffset])
                | uint16_t(header[layout::crcOffset + 1] << 8);
            visit(header[layout::keyOffset], offset, length, crc);
        }
        offset += layout::recordHeaderSize + length;
    }
    return pageSize;
}

bool crcMatches(const Pages& pages, uint8_t page, uint32_t offset, uint16_t length, uint16_t crc)
{
    uint8_t header[layout::crcOffset];
    pages.read(page, offset, header);
    uint16_t computed = crc16(header, crcSeed);

    uint32_t dataOffset = offset + layout::recordHeaderSize;
    for (uint32_t done = 0; done < length; done += chunkSize) {
        uint8_t chunk[chunkSize];
        uint32_t size = length - done < chunkSize ? length - done : chunkSize;
        std::span<uint8_t> part(chunk, size);
        pages.read(page, dataOffset + done, part);
        computed = crc16(part, computed);
    }
    return computed == crc;
}

base::Status writePageHeader(Pages& pages, uint8_t page, uint32_t generation)
{
    uint8_t header[layout::pageCommittedOffset];
    writeLittleEndian32(std::span<uint8_t>(header).subspan(layout::magicOffset, 4), layout::magic);
    writeLittleEndian32(
        std::span<uint8_t>(header).subspan(layout::generationOffset, 4),
        generation
    );

    base::Status written = pages.program(page, 0, header);
    if (!written.has_value()) {
        return std::unexpected(written.error());
    }
    return base::ok;
}

base::Status commitPage(Pages& pages, uint8_t page)
{
    const uint8_t mark[] = {layout::committed};
    base::Status committed = pages.program(page, layout::pageCommittedOffset, mark);
    if (!committed.has_value()) {
        return std::unexpected(committed.error());
    }
    return base::ok;
}

base::Status format(Pages& pages, uint8_t page, uint32_t generation)
{
    base::Status erased = pages.erase(page);
    if (!erased.has_value()) {
        return std::unexpected(erased.error());
    }
    base::Status written = writePageHeader(pages, page, generation);
    if (!written.has_value()) {
        return std::unexpected(written.error());
    }
    base::Status committed = commitPage(pages, page);
    if (!committed.has_value()) {
        return std::unexpected(committed.error());
    }
    return base::ok;
}

} // namespace

Store::Builder& Store::Builder::pages(Pages& pages)
{
    pages_ = &pages;
    return *this;
}

base::Result<Store> Store::Builder::build() const
{
    if (pages_ == nullptr) {
        return std::unexpected(missingPart);
    }
    if (pages_->pageSize() <= layout::pageHeaderSize + layout::recordHeaderSize) {
        return std::unexpected(pageTooSmall);
    }

    PageHeader first = readPageHeader(*pages_, 0);
    PageHeader second = readPageHeader(*pages_, 1);

    uint8_t activePage = 0;
    uint32_t generation = 1;
    if (first.valid && (!second.valid || first.generation >= second.generation)) {
        generation = first.generation;
    } else if (second.valid) {
        activePage = 1;
        generation = second.generation;
    } else {
        base::Status formatted = format(*pages_, activePage, generation);
        if (!formatted.has_value()) {
            return std::unexpected(formatted.error());
        }
    }

    return base::Result<Store>(std::in_place, *pages_, activePage, generation, Permit{});
}

Store::Store(Pages& pages, uint8_t activePage, uint32_t generation, Permit)
    : pages_(pages), activePage_(activePage), generation_(generation)
{
    writeOffset_ = scan(activePage_, index_);
}

base::Status Store::write(uint8_t key, std::span<const uint8_t> data)
{
    if (key >= maxKeys) {
        return std::unexpected(keyOutOfRange);
    }
    if (data.size() > maxRecordData()) {
        return std::unexpected(tooLarge);
    }

    uint32_t needed = layout::recordHeaderSize + data.size();
    if (writeOffset_ + needed > pages_.pageSize()) {
        if (!compactionAllowed_) {
            return std::unexpected(full);
        }
        base::Status compacted = compact();
        if (!compacted.has_value()) {
            return std::unexpected(compacted.error());
        }
        if (writeOffset_ + needed > pages_.pageSize()) {
            return std::unexpected(full);
        }
    }

    base::Status appended = append(key, data);
    if (!appended.has_value()) {
        return std::unexpected(appended.error());
    }
    return base::ok;
}

base::Result<uint32_t> Store::read(uint8_t key, std::span<uint8_t> out) const
{
    if (key >= maxKeys) {
        return std::unexpected(keyOutOfRange);
    }

    const Record& record = index_[key];
    if (record.offset == 0) {
        return 0u;
    }

    if (out.size() < record.length) {
        return std::unexpected(bufferTooSmall);
    }

    pages_.read(activePage_, record.offset + layout::recordHeaderSize, out.first(record.length));
    return uint32_t(record.length);
}

void Store::allowCompaction()
{
    compactionAllowed_ = true;
}

void Store::forbidCompaction()
{
    compactionAllowed_ = false;
}

uint32_t Store::freeBytes() const
{
    return pages_.pageSize() - writeOffset_;
}

uint32_t Store::maxRecordData() const
{
    return pages_.pageSize() - layout::pageHeaderSize - layout::recordHeaderSize;
}

// Fills `index` with the latest whole record of every key on the page, and returns the
// first free offset.
uint32_t Store::scan(uint8_t page, Index& index) const
{
    index = {};
    return walk(pages_, page, [&](uint8_t key, uint32_t offset, uint16_t length, uint16_t crc) {
        if (key < maxKeys && crcMatches(pages_, page, offset, length, crc)) {
            index[key] = Record{
                .offset = offset,
                .length = length,
            };
        }
    });
}

base::Status Store::append(uint8_t key, std::span<const uint8_t> data)
{
    uint8_t header[layout::recordHeaderSize];
    header[layout::keyOffset] = key;
    header[layout::lengthOffset] = uint8_t(data.size());
    header[layout::lengthOffset + 1] = uint8_t(data.size() >> 8);
    uint16_t crc = crc16(data, crc16(std::span<const uint8_t>(header, layout::crcOffset), crcSeed));
    header[layout::crcOffset] = uint8_t(crc);
    header[layout::crcOffset + 1] = uint8_t(crc >> 8);

    uint32_t offset = writeOffset_;

    // Whatever happens below, the space is used: a failed record is skipped, not reused.
    writeOffset_ += layout::recordHeaderSize + data.size();

    std::span<const uint8_t> fields(header, layout::recordCommittedOffset);
    base::Status headerWritten = pages_.program(activePage_, offset, fields);
    if (!headerWritten.has_value()) {
        return std::unexpected(headerWritten.error());
    }
    base::Status dataWritten = pages_.program(activePage_, offset + layout::recordHeaderSize, data);
    if (!dataWritten.has_value()) {
        return std::unexpected(dataWritten.error());
    }

    const uint8_t mark[] = {layout::committed};
    base::Status committed = pages_.program(
        activePage_,
        offset + layout::recordCommittedOffset,
        mark
    );
    if (!committed.has_value()) {
        return std::unexpected(committed.error());
    }

    index_[key] = Record{
        .offset = offset,
        .length = uint16_t(data.size()),
    };
    return base::ok;
}

base::Status Store::compact()
{
    if (!compactionAllowed_) {
        return std::unexpected(compactionForbidden);
    }

    uint8_t target = activePage_ == 0 ? 1 : 0;

    base::Status erased = pages_.erase(target);
    if (!erased.has_value()) {
        return std::unexpected(erased.error());
    }
    base::Status headerWritten = writePageHeader(pages_, target, generation_ + 1);
    if (!headerWritten.has_value()) {
        return std::unexpected(headerWritten.error());
    }

    Index moved = {};
    uint32_t targetOffset = layout::pageHeaderSize;
    for (uint8_t key = 0; key < maxKeys; key++) {
        const Record& record = index_[key];
        if (record.offset == 0) {
            continue;
        }

        uint32_t size = layout::recordHeaderSize + record.length;
        for (uint32_t done = 0; done < size; done += chunkSize) {
            uint8_t chunk[chunkSize];
            uint32_t part = size - done < chunkSize ? size - done : chunkSize;
            std::span<uint8_t> bytes(chunk, part);
            pages_.read(activePage_, record.offset + done, bytes);

            base::Status copied = pages_.program(target, targetOffset + done, bytes);
            if (!copied.has_value()) {
                return std::unexpected(copied.error());
            }
        }

        moved[key] = Record{
            .offset = targetOffset,
            .length = record.length,
        };
        targetOffset += size;
    }

    // Only now does the new page outrank the old one.
    base::Status committed = commitPage(pages_, target);
    if (!committed.has_value()) {
        return std::unexpected(committed.error());
    }

    activePage_ = target;
    generation_++;
    writeOffset_ = targetOffset;
    index_ = moved;
    return base::ok;
}

} // namespace flash_log
