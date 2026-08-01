#pragma once

#include <stdint.h>
#include <cstring>

#include "flash.hpp"
#include "persistent_storage.hpp"

// ============================================================================
// FlashStore — single-copy, flash-backed multi-region store. Owns one flash
// sector; each region is a fixed-capacity PersistentStorage.
//
// Writing any region rewrites the WHOLE sector image (read current image -> patch
// the region -> erase -> write): the erase granularity is the physical sector, so
// regions sharing it are coupled. Single copy → there is a power-fail window
// between erase and write. Fine for rare writes.
//
// Sector image: [magic:4] then RegionCount slots of [len:2][data:RegionCapacity].
// ============================================================================
template<uint8_t RegionCount, uint16_t RegionCapacity>
class FlashStore
{
    public:
        FlashStore(Flash& flash, uint32_t address, uint8_t sector)
            : flash(flash), address(address), sector(sector)
        {
            for(uint8_t i = 0; i < RegionCount; i++)
                regions[i].bind(this, i);
        }

        // Region 0..RegionCount-1, each a PersistentStorage to inject into a component.
        PersistentStorage& region(uint8_t index) { return regions[index]; }

    protected:
        static constexpr uint32_t MAGIC      = 0x53544F52;   // marks an initialized image
        static constexpr uint32_t HEADER     = 4;
        static constexpr uint32_t SLOT       = 2 + RegionCapacity;         // [len:2][data]
        static constexpr uint32_t IMAGE_SIZE = HEADER + RegionCount * SLOT;

        class Region : public PersistentStorage
        {
            public:
                void bind(FlashStore* owner, uint8_t index)
                {
                    this->owner = owner;
                    this->index = index;
                }

                bool write(const uint8_t* data, uint32_t length) override
                {
                    return owner->writeRegion(index, data, length);
                }

                uint32_t read(uint8_t* buffer, uint32_t maxLength) override
                {
                    return owner->readRegion(index, buffer, maxLength);
                }

                uint32_t capacity() const override { return RegionCapacity; }

            private:
                FlashStore* owner = nullptr;
                uint8_t     index = 0;
        };

        bool writeRegion(uint8_t index, const uint8_t* data, uint32_t length)
        {
            if(index >= RegionCount || length > RegionCapacity)
                return false;

            // Build the full image: load the current one (or start fresh) and patch
            // this region, so the other regions are preserved through the rewrite.
            uint8_t image[IMAGE_SIZE];
            flash.read(address, image, IMAGE_SIZE);
            if(readU32(image) != MAGIC)
            {
                std::memset(image, 0, IMAGE_SIZE);
                writeU32(image, MAGIC);
            }

            uint32_t off = HEADER + index * SLOT;
            image[off]     = static_cast<uint8_t>(length);
            image[off + 1] = static_cast<uint8_t>(length >> 8);
            std::memcpy(image + off + 2, data, length);

            if(!flash.eraseSector(sector))
                return false;
            return flash.write(address, image, IMAGE_SIZE);
        }

        uint32_t readRegion(uint8_t index, uint8_t* buffer, uint32_t maxLength)
        {
            if(index >= RegionCount)
                return 0;

            uint8_t header[4];
            flash.read(address, header, 4);
            if(readU32(header) != MAGIC)
                return 0;   // uninitialized

            uint32_t off = HEADER + index * SLOT;
            uint8_t lenBytes[2];
            flash.read(address + off, lenBytes, 2);
            uint16_t length = lenBytes[0] | (uint16_t(lenBytes[1]) << 8);
            if(length > RegionCapacity)
                return 0;   // corrupt

            uint32_t n = (length < maxLength) ? length : maxLength;
            flash.read(address + off + 2, buffer, n);
            return length;
        }

        static uint32_t readU32(const uint8_t* p)
        {
            return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
        }
        static void writeU32(uint8_t* p, uint32_t v)
        {
            p[0] = static_cast<uint8_t>(v);
            p[1] = static_cast<uint8_t>(v >> 8);
            p[2] = static_cast<uint8_t>(v >> 16);
            p[3] = static_cast<uint8_t>(v >> 24);
        }

        Flash&   flash;
        uint32_t address;
        uint8_t  sector;
        Region   regions[RegionCount];
};
