#include "i2c_bus_builder.hpp"

I2cBus::Builder& I2cBus::Builder::withQueue(Queue<I2cTransaction> *queue)
{
    config.queue = queue;
    return *this;
}

I2cBus::Builder& I2cBus::Builder::withBusSelection(I2cBusSelection bus)
{
    config.bus = bus;
    return *this;
}

I2cBus::Builder& I2cBus::Builder::setBusSpeed(uint32_t clockSpeed)
{
    config.clockSpeed = clockSpeed;
    return *this;
}

I2cBus::Builder& I2cBus::Builder::setName(std::string name)
{
    config.name = name;
    return *this;
}

I2cBus::Builder& I2cBus::Builder::enableSlave(uint16_t ownAddress)
{
    config.ownAddress1 = ownAddress;
    return *this;
}

I2cBus::Builder& I2cBus::Builder::setOwnAddress2(uint16_t ownAddress2)
{
    config.ownAddress2 = ownAddress2;
    return *this;
}

I2cBus::Builder& I2cBus::Builder::setDutyCycle16_9()
{
    config.dutyCycle = I2C_DUTY_CYCLE_16_9;
    return *this;
}

I2cBus::Builder& I2cBus::Builder::set10BitAddressing()
{
    config.addressing7Bit = false;
    return *this;
}

I2cBus::Builder& I2cBus::Builder::disableClockStretching()
{
    config.clockStretching = false;
    return *this;
}

I2cBus::Builder& I2cBus::Builder::enableSlaveGeneralCall()
{
    config.generalCall = true;
    return *this;
}

I2cBus::Builder& I2cBus::Builder::withTimer(Timer* timer)
{
    config.timer = timer;
    return *this;
}

I2cBus::Builder& I2cBus::Builder::withSlave(I2cSlave* slave)
{
    config.slave = slave;
    return *this;
}

I2cBus::Builder& I2cBus::Builder::setRetryIntervalMs(uint16_t retryIntervalMs)
{
    config.retryIntervalMs = retryIntervalMs;
    return *this;
}

void I2cBus::Builder::buildIn(I2cBus* target)
{
    return target->init(config);
}

I2cBus::Config I2cBus::Builder::buildConfig()
{
    return config;
}