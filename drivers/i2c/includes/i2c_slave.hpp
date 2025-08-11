#pragma once

#include <stdint.h>

class I2cBus;

class I2cSlave
{
    public:
        virtual void onWriteByte(const uint8_t data) = 0;

        virtual uint8_t onReadByte() = 0;

        virtual void onStop() = 0;

        virtual void onAddressMatch() = 0;

    protected:
        virtual void setBus(I2cBus& bus) = 0;

    friend class I2cBus;
};