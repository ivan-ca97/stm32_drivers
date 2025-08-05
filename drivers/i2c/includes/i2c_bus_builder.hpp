#pragma once
#include "i2c_bus.hpp"

struct I2cBus::Config
{
    I2cBusSelection bus;
    std::string name;
    uint32_t clockSpeed;
    bool addressing7Bit = true;
    I2cDutyCycle dutyCycle = I2C_DUTY_CYCLE_2;
    uint16_t ownAddress1 = 0;
    uint16_t ownAddress2 = 0;
    bool clockStretching = true;
    bool generalCall = false;
    Queue<I2cTransaction>* queue = nullptr;
    Set<I2cDevice*>* devicesSet = nullptr;
    I2cSlave* slave = nullptr;
    Timer* timer = nullptr;
    uint16_t retryIntervalMs;
};


class I2cBus::Builder
{
    private:
        Config config;

    public:
        void buildIn(I2cBus& target);

        Config buildConfig();

        Builder& withBusSelection(I2cBusSelection bus);

        Builder& setBusSpeed(uint32_t clockSpeed);

        Builder& setName(std::string name);

        Builder& enableSlave(uint16_t ownAddress, I2cSlave& slave);

        Builder& setOwnAddress2(uint16_t ownAddress2);

        Builder& setDutyCycle16_9();

        Builder& set10BitAddressing();

        Builder& disableClockStretching();

        Builder& enableSlaveGeneralCall();

        Builder& withQueue(Queue<I2cTransaction>& queue);

        Builder& withDevicesSet(Set<I2cDevice*>& queue);

        Builder& withTimer(Timer& timer);

        Builder& setRetryIntervalMs(uint16_t retryIntervalMs);
};