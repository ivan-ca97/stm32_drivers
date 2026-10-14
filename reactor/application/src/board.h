#pragma once

#include <cstdint>

#include "stm32f4xx_hal.h"

#include "i2c_bus.hpp"
#include "stm32f4/flash_pages.h"
#include "timer.hpp"

// How this board is wired: the pins, buses, timer and devices the application sits on.
namespace application::board {

struct Pin {
    GPIO_TypeDef* port;
    uint16_t number;
};

// Driven high while healthy. Low, or unpowered, is SCRAM.
inline const Pin scram = {
    .port = GPIOC,
    .number = GPIO_PIN_13,
};

// Placeholder pin: open on the first board, tied to GND on the second.
inline const Pin addressStrap = {
    .port = GPIOB,
    .number = GPIO_PIN_12,
};

inline constexpr I2cBus::Selection commandBus = I2cBus::Selection::Bus2;

// The ADC, and for now also the reads of the peer's windows.
inline constexpr I2cBus::Selection sensorBus = I2cBus::Selection::Bus3;

inline constexpr uint32_t busSpeedHz = 100000;

// Paces the I2C retries.
inline constexpr TimerSelection retryTimer = TIMER_2;

inline constexpr uint8_t adcAddress = 0x48;   // ADS1115 with ADDR tied to GND

// The two pages of persistent storage: sectors 1 and 2, the STORAGE region of the
// linker script, which keeps the program out of them.
inline constexpr stm32f4::FlashPages::Sector firstStoragePage = {
    .number = 1,
    .address = 0x08004000,
};
inline constexpr stm32f4::FlashPages::Sector secondStoragePage = {
    .number = 2,
    .address = 0x08008000,
};
inline constexpr uint32_t storagePageSize = 16 * 1024;

} // namespace application::board
