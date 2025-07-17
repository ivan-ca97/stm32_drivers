#include "i2c_bus.hpp"

struct I2cBusConfig
{
    Queue<I2cTransaction>* queue;
    I2cBusSelection bus;
    std::string name;
    uint32_t clockSpeed;
    bool addressing7Bit = true;
    I2cDutyCycle dutyCycle = I2C_DUTY_CYCLE_2;
    uint16_t ownAddress1 = 0;
    uint16_t ownAddress2 = 0;
    bool clockStretching = true;
    bool generalCall = false;
    Timer* timer = nullptr;
    I2cSlave* slave = nullptr;
    uint16_t retryIntervalMs;
};

class I2cBus::Builder
{
    private:
        I2cBusConfig config;

    public:
        I2cBus build();

        Builder& withQueue(Queue<I2cTransaction> *queue);

        Builder& withBusSelection(I2cBusSelection bus);

        Builder& setBusSpeed(uint32_t clockSpeed);

        Builder& setName(std::string name);

        Builder& enableSlave(uint16_t ownAddress);

        Builder& setOwnAddress2(uint16_t ownAddress2);

        Builder& setDutyCycle16_9();

        Builder& set10BitAddressing();

        Builder& disableClockStretching();

        Builder& enableSlaveGeneralCall();

        Builder& withTimer(Timer* timer);

        Builder& withSlave(I2cSlave* slave);

        Builder& setRetryIntervalMs(uint16_t retryIntervalMs);
};