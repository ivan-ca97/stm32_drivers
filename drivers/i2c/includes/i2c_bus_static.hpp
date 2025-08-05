#pragma once
#include "i2c_bus.hpp"
#include "i2c_bus_builder.hpp"

template <size_t TransactionsBufferSize, size_t DevicesBufferSize>
class I2cBusStatic : public I2cBus
{
    protected:
        StaticQueue<I2cTransaction, TransactionsBufferSize> queue;
        StaticSet<I2cDevice*, DevicesBufferSize> devicesSet;

    public:
        I2cBusStatic() = default;
        I2cBusStatic(const Config& config)
        {
            Config modifiableConfig = config;
            init(modifiableConfig);
        }

        void init(Config& config)
        {
            if(config.queue)
                throw std::logic_error("Pre-Configured queue for static I2C Bus.");

            config.queue = &queue;
            config.devicesSet = &devicesSet;

            I2cBus::init(config);
        }
};