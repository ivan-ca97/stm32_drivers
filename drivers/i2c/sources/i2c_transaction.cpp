#include "i2c_transaction.hpp"
#include "i2c_device.hpp"



I2cTransaction::I2cTransaction()
    : data(nullptr), dataBytes(0), device(nullptr)
{

}

I2cTransaction I2cTransaction::I2cTxTransaction(I2cDevice *device, uint8_t* data, uint16_t dataBytes, uint32_t deviceRegister, RegisterLength deviceRegisterBytes)
{
    return I2cTransaction(TRANSACTION_TX, data, dataBytes, device, deviceRegister, deviceRegisterBytes);
}

I2cTransaction I2cTransaction::I2cRxTransaction(I2cDevice *device, uint8_t* data, uint16_t dataBytes, uint32_t deviceRegister, RegisterLength deviceRegisterBytes)
{
    return I2cTransaction(TRANSACTION_RX, data, dataBytes, device, deviceRegister, deviceRegisterBytes);
}

I2cTransaction::I2cTransaction(TransactionDirection direction, uint8_t* data, uint16_t dataBytes, I2cDevice *device, uint16_t address, uint32_t deviceRegister, RegisterLength deviceRegisterBytes)
    : direction(direction), address(address), data(data), dataBytes(dataBytes), device(device), deviceRegister(deviceRegister), deviceRegisterBytes(deviceRegisterBytes)
{
    if(deviceRegisterBytes == REGISTER_NULL && deviceRegister != 0)
        throw I2cException("Configured register but not register length");
}

I2cTransaction::I2cTransaction(TransactionDirection direction, uint8_t* data, uint16_t dataBytes, uint16_t address, uint32_t deviceRegister, RegisterLength deviceRegisterBytes)
    : I2cTransaction(direction, data, dataBytes, nullptr, address, deviceRegister, deviceRegisterBytes)
{

}

I2cTransaction::I2cTransaction(TransactionDirection direction, uint8_t* data, uint16_t dataBytes, I2cDevice *device, uint32_t deviceRegister, RegisterLength deviceRegisterBytes)
    : I2cTransaction(direction, data, dataBytes, device, device->getAddress(), deviceRegister, deviceRegisterBytes)
{

}

void I2cTransaction::setPreCallback(Callback callback, void* parameters)
{
    preCallbackFunction = callback;
    preCallbackParameters = parameters;
}

void I2cTransaction::setPostCallback(Callback callback, void* parameters)
{
    postCallbackFunction = callback;
    postCallbackParameters = parameters;
}

void I2cTransaction::setErrorCallback(Callback callback, void* parameters)
{
    errorCallbackFunction = callback;
    errorCallbackParameters = parameters;
}

void I2cTransaction::attachBus(I2cBus* bus)
{
    bus = bus;
}

uint16_t I2cTransaction::getAddress(void)
{
    return address;
}

uint8_t* I2cTransaction::getDataPointer(void)
{
    return data;
}

uint16_t I2cTransaction::getDataLenthBytes(void)
{
    return dataBytes;
}

uint16_t I2cTransaction::getCurrentIndex(void)
{
    return bus->getCurrentIndex();
}

bool I2cTransaction::hasPendingBytes(void)
{
    // TODO
    return true;
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

uint16_t I2cTransaction::getRegister(void)
{
    return deviceRegister;
}

bool I2cTransaction::hasRegister(void)
{
    return deviceRegisterBytes != REGISTER_NULL;
}

uint8_t I2cTransaction::getRegisterByte(uint8_t index)
{
    uint8_t registerLengthBytes = getRegisterLengthBytes();
    if (index >= registerLengthBytes)
        throw std::out_of_range("Index out of register length");

    uint8_t shift = 8 * (registerLengthBytes - index - 1);
    return static_cast<uint8_t>((deviceRegister >> shift) & 0xFF);
}

uint8_t I2cTransaction::getRegisterLengthBytes(void)
{
    switch(deviceRegisterBytes)
    {
        case REGISTER_NULL:
            return 0;
        case REGISTER_8_BITS:
            return 1;
        case REGISTER_16_BITS:
            return 2;
        case REGISTER_24_BITS:
            return 3;
        case REGISTER_32_BITS:
            return 4;
    }
}

TransactionDirection I2cTransaction::getDirection(void)
{
    return direction;
}

bool I2cTransaction::isRead(void)
{
    return direction == TRANSACTION_RX;
}

bool I2cTransaction::isWrite(void)
{
    return direction == TRANSACTION_TX;
}

I2cTransactionStatus I2cTransaction::getStatus(void)
{
    return status;
}

void I2cTransaction::setStatus(I2cTransactionStatus newStatus)
{
    status = newStatus;
}

bool I2cTransaction::isDone(void)
{
    return status == TRANSACTION_SENT || status == TRANSACTION_ERROR;
}

void I2cTransaction::send(void)
{
    if(!device)
        throw I2cException("Device for the I2cTransaction not set");

    device->setTransaction(*this);
}

void I2cTransaction::preCallback()
{
    if(preCallbackFunction)
        preCallbackFunction(preCallbackParameters);
    this->setStatus(TRANSACTION_IN_PROGRESS);
}

void I2cTransaction::postCallback()
{
    this->setStatus(TRANSACTION_SENT);
    if(postCallbackFunction)
        postCallbackFunction(postCallbackParameters);
}

void I2cTransaction::errorCallback()
{
    if(errorCallbackFunction)
        errorCallbackFunction(errorCallbackParameters);
}