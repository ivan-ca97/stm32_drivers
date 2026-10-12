#pragma once

#include <cstdint>

// Where everything sits in a page. Multi-byte fields are little-endian.
//
//   Page:   [magic:4][generation:4][committed:1] records...
//   Record: [key:1][length:2][crc:2][committed:1][data:length]
//
// A committed byte is programmed last, so anything cut short by a power loss reads as
// uncommitted and is ignored. The CRC covers key, length and data.
namespace flash_log::layout {

inline constexpr uint32_t magic = 0x474F4C46;   // "FLOG"

inline constexpr uint32_t magicOffset = 0;
inline constexpr uint32_t generationOffset = 4;
inline constexpr uint32_t pageCommittedOffset = 8;
inline constexpr uint32_t pageHeaderSize = 9;

inline constexpr uint32_t keyOffset = 0;
inline constexpr uint32_t lengthOffset = 1;
inline constexpr uint32_t crcOffset = 3;
inline constexpr uint32_t recordCommittedOffset = 5;
inline constexpr uint32_t recordHeaderSize = 6;

inline constexpr uint8_t erased = 0xFF;
inline constexpr uint8_t committed = 0x00;

// A length still erased: the header was cut short, and nothing after it can be trusted.
inline constexpr uint16_t tornLength = 0xFFFF;

} // namespace flash_log::layout
