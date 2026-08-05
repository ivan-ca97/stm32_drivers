#pragma once

#include <stdint.h>

// ============================================================================
// ADS1115 register map. Private to the driver.
// ============================================================================

// Registers.
constexpr uint8_t RegConversion = 0x00;
constexpr uint8_t RegConfig     = 0x01;

// Config register layout (16-bit, big-endian on the wire).
constexpr uint16_t OsBit = 0x8000; // write 1 = start; read 1 = idle/ready

constexpr uint16_t MuxMask = 0x7000; // bits [14:12]
constexpr uint16_t MuxSingleEnded = 0x4000; // MUX = 1xx selects AINx vs GND
constexpr unsigned MuxShift = 12;

constexpr uint16_t PgaMask = 0x0E00; // bits [11:9]
constexpr unsigned PgaShift = 9;

constexpr uint16_t ModeBit = 0x0100; // bit 8: 1 = single-shot, 0 = continuous

constexpr uint16_t DataRateMask = 0x00E0; // bits [7:5]
constexpr unsigned DataRateShift = 5;

constexpr uint16_t ComparatorQueueMask = 0x0003; // bits [1:0]
constexpr uint16_t ComparatorDisabled = 0x0003; // COMP_QUE = 11
