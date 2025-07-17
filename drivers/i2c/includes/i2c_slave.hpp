#pragma once

#include <stdint.h>

class I2cSlave
{
    public:
        virtual void onWriteByte(const uint8_t data) = 0;

        virtual uint8_t onReadByte() = 0;

        virtual void onStop() = 0;

        virtual void onAddressMatch() = 0;
};