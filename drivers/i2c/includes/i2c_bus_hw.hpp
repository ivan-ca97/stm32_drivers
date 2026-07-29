#pragma once

#include "i2c_bus.hpp"   // I2cBus::Selection, and (via stm32f4xx.h) HAL types/macros

// ============================================================================
// Per-bus hardware descriptor for the STM32F401 I2C peripherals.
//
// Single source of truth for the bus -> {peripheral, pins, alternate function,
// IRQs, clocks} mapping.
//
// F401 note: I2C2/I2C3 SDA use AF9 (PB3, PB4), NOT AF4. SCLs and I2C1 use AF4.
// (On our clone boards PB9-AF9 is broken, which is why I2C2 SDA is PB3.)
// ============================================================================
struct I2cBusHw
{
    I2C_TypeDef*  instance;
    IRQn_Type     evIrq;
    IRQn_Type     erIrq;

    GPIO_TypeDef* sclPort;
    uint16_t      sclPin;
    uint8_t       sclAf;

    GPIO_TypeDef* sdaPort;
    uint16_t      sdaPin;
    uint8_t       sdaAf;

    void        (*enableClocks)();   // enables the I2C peripheral + GPIO port clock(s)
};

inline const I2cBusHw& i2cBusHw(I2cBus::Selection bus)
{
    static const I2cBusHw table[] =
    {
        // Bus1 - panel / ESP32 link: SCL PB6 (AF4), SDA PB7 (AF4)
        { I2C1, I2C1_EV_IRQn, I2C1_ER_IRQn,
          GPIOB, GPIO_PIN_6, GPIO_AF4_I2C1,
          GPIOB, GPIO_PIN_7, GPIO_AF4_I2C1,
          []{ __HAL_RCC_I2C1_CLK_ENABLE(); __HAL_RCC_GPIOB_CLK_ENABLE(); } },

        // Bus2 - inter-MCU: SCL PB10 (AF4), SDA PB3 (AF9)   [NOT PB9 on the clone]
        { I2C2, I2C2_EV_IRQn, I2C2_ER_IRQn,
          GPIOB, GPIO_PIN_10, GPIO_AF4_I2C2,
          GPIOB, GPIO_PIN_3,  GPIO_AF9_I2C2,
          []{ __HAL_RCC_I2C2_CLK_ENABLE(); __HAL_RCC_GPIOB_CLK_ENABLE(); } },

        // Bus3 - ADC: SCL PA8 (AF4), SDA PB4 (AF9)
        { I2C3, I2C3_EV_IRQn, I2C3_ER_IRQn,
          GPIOA, GPIO_PIN_8, GPIO_AF4_I2C3,
          GPIOB, GPIO_PIN_4, GPIO_AF9_I2C3,
          []{ __HAL_RCC_I2C3_CLK_ENABLE(); __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_GPIOB_CLK_ENABLE(); } },
    };

    return table[static_cast<int>(bus)];
}
