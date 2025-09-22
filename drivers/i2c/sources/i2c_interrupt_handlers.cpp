#include "i2c_bus.hpp"

/*
 *  Interrupt handlers by driver
 */
extern "C" void I2C1_EV_IRQHandler()
{
    I2cBus::handleInterrupt(I2cBus::Selection::Bus1, I2cBus::InterruptType::Event);
}

extern "C" void I2C2_EV_IRQHandler()
{
    I2cBus::handleInterrupt(I2cBus::Selection::Bus2, I2cBus::InterruptType::Event);
}

extern "C" void I2C3_EV_IRQHandler()
{
    I2cBus::handleInterrupt(I2cBus::Selection::Bus3, I2cBus::InterruptType::Event);
}

extern "C" void I2C1_ER_IRQHandler()
{
    I2cBus::handleInterrupt(I2cBus::Selection::Bus1, I2cBus::InterruptType::Error);
}

extern "C" void I2C2_ER_IRQHandler()
{
    I2cBus::handleInterrupt(I2cBus::Selection::Bus2, I2cBus::InterruptType::Error);
}

extern "C" void I2C3_ER_IRQHandler()
{
    I2cBus::handleInterrupt(I2cBus::Selection::Bus3, I2cBus::InterruptType::Error);
}