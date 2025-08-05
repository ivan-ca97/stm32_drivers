#pragma once

#include "i2c_bus.hpp"

class I2cDevice
{
    protected:
        uint16_t address;
        I2cBus *bus;
        std::string name;

    public:
        I2cDevice(uint16_t address, I2cBus* bus = nullptr, std::string name = "");
        ~I2cDevice();

        uint16_t getAddress();

        void attachBus(I2cBus* bus);

        void detachBus();

        void setTransaction(I2cTransaction &transaction);

        I2cDevice& operator<<(I2cTransaction& transaction);
};