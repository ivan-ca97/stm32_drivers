#pragma once

#include <stdint.h>

#include "stm32f4xx.h"     // GPIO_TypeDef
#include "digital_io.hpp"

// GpioPin — STM32 GPIO pins. Each object owns one pin: the constructor enables
// the port clock and configures it, so a pin is usable as soon as it is built.
class GpioPin
{
    public:
        enum class Pull  : uint8_t { None, Up, Down };
        enum class Drive : uint8_t { PushPull, OpenDrain };

    protected:
        GpioPin(GPIO_TypeDef* port, uint16_t pin) : port(port), pin(pin) {}

        // Enables the port clock and applies `mode` (plus pull/drive) to this pin.
        void configure(uint32_t mode, Pull pull, Drive drive);

        GPIO_TypeDef* port;
        uint16_t      pin;
};

class GpioOutput : public GpioPin, public DigitalOutput
{
    public:
        // Drives `initialLevel` before switching the pin to output, so the line never
        // glitches through the opposite level on startup.
        GpioOutput(GPIO_TypeDef* port, uint16_t pin, bool initialLevel = false,
                   Drive drive = Drive::PushPull, Pull pull = Pull::None);

        void set(bool high) override;
        bool get() const override;
};

class GpioInput : public GpioPin, public DigitalInput
{
    public:
        GpioInput(GPIO_TypeDef* port, uint16_t pin, Pull pull = Pull::None);

        bool get() const override;
};
