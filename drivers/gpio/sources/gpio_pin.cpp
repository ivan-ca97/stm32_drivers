#include "gpio_pin.hpp"

#include "stm32f4xx_hal.h"   // HAL_GPIO_*, GPIO_InitTypeDef, RCC clock macros

namespace
{
    // The port clock must be running before the pin can be configured.
    void enablePortClock(GPIO_TypeDef* port)
    {
        if(port == GPIOA)      __HAL_RCC_GPIOA_CLK_ENABLE();
        else if(port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
        else if(port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
        else if(port == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
        else if(port == GPIOE) __HAL_RCC_GPIOE_CLK_ENABLE();
        else if(port == GPIOH) __HAL_RCC_GPIOH_CLK_ENABLE();
    }

    uint32_t toHalPull(GpioPin::Pull pull)
    {
        switch(pull)
        {
            case GpioPin::Pull::Up:   return GPIO_PULLUP;
            case GpioPin::Pull::Down: return GPIO_PULLDOWN;
            case GpioPin::Pull::None: break;
        }
        return GPIO_NOPULL;
    }
}

void GpioPin::configure(uint32_t mode, Pull pull, Drive drive)
{
    enablePortClock(port);

    GPIO_InitTypeDef gpio = {};
    gpio.Pin   = pin;
    gpio.Mode  = (mode == GPIO_MODE_OUTPUT_PP && drive == Drive::OpenDrain)
                 ? GPIO_MODE_OUTPUT_OD : mode;
    gpio.Pull  = toHalPull(pull);
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port, &gpio);
}

GpioOutput::GpioOutput(GPIO_TypeDef* port, uint16_t pin, bool initialLevel,
                       Drive drive, Pull pull)
    : GpioPin(port, pin)
{
    enablePortClock(port);
    // Drive the level first: the pin only starts driving once configured as an
    // output, so this avoids a glitch through the opposite level.
    HAL_GPIO_WritePin(port, pin, initialLevel ? GPIO_PIN_SET : GPIO_PIN_RESET);
    configure(GPIO_MODE_OUTPUT_PP, pull, drive);
}

void GpioOutput::set(bool high)
{
    HAL_GPIO_WritePin(port, pin, high ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

bool GpioOutput::get() const
{
    return (port->ODR & pin) != 0;   // driven level, not the sampled input
}

GpioInput::GpioInput(GPIO_TypeDef* port, uint16_t pin, Pull pull)
    : GpioPin(port, pin)
{
    configure(GPIO_MODE_INPUT, pull, Drive::PushPull);
}

bool GpioInput::get() const
{
    return HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET;
}
