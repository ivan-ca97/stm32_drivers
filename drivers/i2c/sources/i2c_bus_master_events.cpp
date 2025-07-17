#include "i2c_bus.hpp"

#include "stm32f4xx_ll_i2c.h"

#define READ false
#define WRITE true

bool I2cBus::sendSlaveAddress(bool readBit)
{
    // The address should only be sent right after the start condition. Do not send if the start condition isn't set.
    if(!LL_I2C_IsActiveFlag_SB(instance))
        return false;

    uint16_t address = this->currentTransaction->getAddress() << 1;

    address |= readBit;

    this->currentTransaction->preCallback();
    LL_I2C_TransmitData8(instance, address);

    return true;
}

void I2cBus::prepareMasterRx(uint8_t remainingBytes)
{
    if(remainingBytes == 1)
    {
        LL_I2C_AcknowledgeNextData(instance, LL_I2C_NACK);
        LL_I2C_GenerateStopCondition(instance);
    }
    else if(remainingBytes == 2)
    {
        LL_I2C_EnableBitPOS(instance);
        LL_I2C_AcknowledgeNextData(instance, LL_I2C_NACK);
    }
    else
    {
        LL_I2C_AcknowledgeNextData(instance, LL_I2C_ACK);
    }
}

void I2cBus::finishCurrentTransaction(bool postCallback)
{
    if(postCallback)
        currentTransaction->postCallback();
    queue->dequeue();
    status = I2C_BUS_IDLE;
    sendNextTransaction();
}

void I2cBus::masterStateStartAttemp()
{
    bool readBit = currentTransaction->isRead() && !currentTransaction->hasRegister();
    bool sentAddress = sendSlaveAddress(readBit);
    if(sentAddress)
        status = I2C_BUS_SEND_SLAVE_ADDRESS;
    return;
}

void I2cBus::masterStateSendSlaveAddress()
{
    if(!LL_I2C_IsActiveFlag_ADDR(instance))
        return;

    if(currentTransaction->hasRegister())
        status = I2C_BUS_SEND_REGISTER;
    else if(currentTransaction->isRead())
        status = I2C_BUS_SEND_DATA;
    else
    {
        prepareMasterRx(currentTransaction->getDataLengthBytes());
        status = I2C_BUS_RECEIVE_DATA;
    }

    LL_I2C_ClearFlag_ADDR(instance);
    LL_I2C_EnableIT_BUF(instance);
    currentIndex = 0;
    return;
}

void I2cBus::masterStateSendRegister()
{
    if(!LL_I2C_IsActiveFlag_TXE(instance))
        return;

    LL_I2C_TransmitData8(instance, currentTransaction->getRegisterByte(currentIndex++));

    if(currentIndex < currentTransaction->getRegisterLengthBytes())
        return;

    if(currentTransaction->isRead())
    {
        LL_I2C_DisableIT_BUF(instance);
        LL_I2C_GenerateStartCondition(instance);
        status = I2C_BUS_LAST_REGISTER_BYTE;
    }

    if(currentTransaction->isWrite())
        status = I2C_BUS_SEND_DATA;

    currentIndex = 0;
}

void I2cBus::masterStateSendData()
{
    if(!LL_I2C_IsActiveFlag_TXE(instance))
        return;

    LL_I2C_TransmitData8(instance, currentTransaction->getByte(currentIndex++));

    if(currentIndex >= currentTransaction->getDataLengthBytes())
    {
        LL_I2C_DisableIT_BUF(instance);
        this->status = I2C_BUS_SEND_LAST_DATA_BYTE;
    }
}

void I2cBus::masterStateSendLastDataByte()
{
    if(!LL_I2C_IsActiveFlag_BTF(instance))
        return;

    LL_I2C_GenerateStopCondition(instance);
    finishCurrentTransaction(true);
}

void I2cBus::masterStateSendLastRegisterByte()
{
    if(!LL_I2C_IsActiveFlag_BTF(instance))
        return;

    LL_I2C_GenerateStartCondition(instance);
    status = I2C_BUS_REPEATED_START;
}

void I2cBus::masterStateRepeatedStart()
{
    bool readBit = currentTransaction->isRead();
    bool sentAddress = sendSlaveAddress(readBit);
    if(sentAddress)
        this->status = I2C_BUS_REPEATED_START_ACK_ADDR;
}

void I2cBus::masterStateRepeatedStartAckAddr()
{
    if(!LL_I2C_IsActiveFlag_ADDR(instance))
        return;

    prepareMasterRx(currentTransaction->getDataLengthBytes());
    LL_I2C_ClearFlag_ADDR(instance);
    LL_I2C_EnableIT_BUF(instance);
    this->currentIndex = 0;

    this->status = I2C_BUS_RECEIVE_DATA;
}

void I2cBus::masterStateReceiveData()
{
    if(!LL_I2C_IsActiveFlag_RXNE(instance))
        return;

    uint8_t readByte = LL_I2C_ReceiveData8(instance);
    currentTransaction->setByte(readByte, currentIndex++);

    uint8_t remainingBytes = currentTransaction->getDataLengthBytes() - currentIndex;
    prepareMasterRx(remainingBytes);
    if(remainingBytes == 0)
        finishCurrentTransaction(true);
}

void I2cBus::eventSlaveCallback()
{
    if(LL_I2C_IsActiveFlag_ADDR(instance))
    {
        LL_I2C_ReadReg(instance, SR1);
        uint32_t sr2 = LL_I2C_ReadReg(instance, SR2);
        if(sr2 & I2C_SR2_TRA)
            status = I2C_BUS_SLAVE_TRANSMIT;
        else
            status = I2C_BUS_SLAVE_RECEIVE;

        LL_I2C_EnableIT_BUF(instance);
        slave->onAddressMatch();
    }

    if(status == I2C_BUS_SLAVE_TRANSMIT && LL_I2C_IsActiveFlag_TXE(instance))
        LL_I2C_TransmitData8(instance, slave->onReadByte());

    while(status == I2C_BUS_SLAVE_RECEIVE && LL_I2C_IsActiveFlag_RXNE(instance))
        slave->onWriteByte(LL_I2C_ReceiveData8(instance));

    if(LL_I2C_IsActiveFlag_STOP(instance))
    {
        LL_I2C_DisableIT_BUF(instance);
        LL_I2C_ClearFlag_STOP(instance);
        slave->onStop();
        status = I2C_BUS_IDLE;
    }
}

void I2cBus::eventMasterCallback()
{
    switch(this->status)
    {
        case I2C_BUS_IDLE:
            // Slave behaviour only
            break;

        case I2C_BUS_START_ATTEMPT:
            masterStateStartAttemp();
            break;

        case I2C_BUS_SEND_SLAVE_ADDRESS:
            masterStateSendSlaveAddress();
            break;

        case I2C_BUS_SEND_REGISTER:
            masterStateSendRegister();
            break;

        case I2C_BUS_SEND_DATA:
            masterStateSendData();
            break;

        case I2C_BUS_SEND_LAST_DATA_BYTE:
            masterStateSendLastDataByte();
            break;

        case I2C_BUS_LAST_REGISTER_BYTE:
            masterStateSendLastRegisterByte();
            break;

        case I2C_BUS_REPEATED_START:
            masterStateRepeatedStart();
            break;

        case I2C_BUS_REPEATED_START_ACK_ADDR:
            masterStateRepeatedStartAckAddr();
            break;

        case I2C_BUS_RECEIVE_DATA:
            masterStateReceiveData();
            break;

        default:
            break;
    }
}

void I2cBus::errorCallback()
{
    if (LL_I2C_IsActiveFlag_AF(instance))
        LL_I2C_ClearFlag_AF(instance);

    if (LL_I2C_IsActiveFlag_ARLO(instance))
        LL_I2C_ClearFlag_ARLO(instance);

    if (LL_I2C_IsActiveFlag_BERR(instance))
        LL_I2C_ClearFlag_BERR(instance);

    if (LL_I2C_IsActiveFlag_OVR(instance))
    {
        LL_I2C_ReceiveData8(instance);
        LL_I2C_ClearFlag_OVR(instance);
    }

    if(!currentTransaction)
        return;

    currentTransaction->setStatus(TRANSACTION_ERROR);
    currentTransaction->errorCallback();
    finishCurrentTransaction(false);
    sendNextTransaction();

}