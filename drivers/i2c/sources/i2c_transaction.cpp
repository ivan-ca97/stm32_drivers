#include "i2c_transaction.hpp"
#include "i2c_device.hpp"
#include <stdexcept>

void I2cTransaction::preCallback()
{
    if(preCallbackFunction)
        preCallbackFunction(preCallbackParameters);
}

void I2cTransaction::postCallback()
{
    if(postCallbackFunction)
        postCallbackFunction(postCallbackParameters);
}

void I2cTransaction::errorCallback()
{
    if(errorCallbackFunction)
        errorCallbackFunction(errorCallbackParameters);
}

uint16_t I2cTransaction::getAddress()
{
    return device->getAddress();
}

uint8_t I2cTransaction::getByte(uint16_t index)
{
    if(index >= this->dataBytes)
        throw I2cException("No more data in transaction");
    return data[index];
}

void I2cTransaction::setByte(uint8_t byte, uint16_t index)
{
    if(index >= this->dataBytes)
        throw I2cException("Out of bounds");
    data[index] = byte;
}

uint8_t* I2cTransaction::getDataPointer()
{
    return data;
}

uint16_t I2cTransaction::getDataLengthBytes()
{
    return dataBytes;
}

uint32_t I2cTransaction::getRegister()
{
    return deviceRegister;
}

bool I2cTransaction::hasRegister()
{
    return deviceRegisterBytes > 0;
}

uint8_t I2cTransaction::getRegisterByte(uint8_t index)
{
    uint8_t registerLengthBytes = getRegisterLengthBytes();
    if (index >= registerLengthBytes)
        throw std::out_of_range("Index out of register length");

    uint8_t shift = 8 * (registerLengthBytes - index - 1);
    return static_cast<uint8_t>((deviceRegister >> shift) & 0xFF);
}

uint8_t I2cTransaction::getRegisterLengthBytes()
{
    return deviceRegisterBytes;
}

bool I2cTransaction::isTx()
{
    return direction == TX;
}

bool I2cTransaction::isRx()
{
    return direction == RX;
}

I2cTransaction::Builder& I2cTransaction::Builder::setDirection(Direction direction)
{
    transaction.direction = direction;
    return *this;
}

I2cTransaction::Builder& I2cTransaction::Builder::withData(uint8_t* data, uint16_t sizeBytes)
{
    transaction.data = data;
    transaction.dataBytes = sizeBytes;
    return *this;
}

I2cTransaction::Builder& I2cTransaction::Builder::withRegister(uint32_t deviceRegister, uint8_t length)
{
    transaction.deviceRegister = deviceRegister;
    transaction.deviceRegisterBytes = length;
    return *this;
}

I2cTransaction::Builder& I2cTransaction::Builder::withPreCallback(std::function<void(void*)> function, void* parameters)
{
    transaction.preCallbackParameters = parameters;
    transaction.preCallbackFunction = function;
    return *this;
}

I2cTransaction::Builder& I2cTransaction::Builder::withPostCallback(std::function<void(void*)> function, void* parameters)
{
    transaction.postCallbackParameters = parameters;
    transaction.postCallbackFunction = function;
    return *this;
}

I2cTransaction::Builder& I2cTransaction::Builder::withErrorCallback(std::function<void(void*)> function, void* parameters)
{
    transaction.errorCallbackParameters = parameters;
    transaction.errorCallbackFunction = function;
    return *this;
}

I2cTransaction I2cTransaction::Builder::build()
{
    return transaction;
}