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
    {
        currentTransaction->postCallback();
        currentTransaction->setState(I2cTransaction::FINISHED);
    }
    queue->dequeue();
    state = State::Idle;
    sendNextTransaction();
}

void I2cBus::masterStateStartAttemp()
{
    bool readBit = currentTransaction->isRx() && !currentTransaction->hasRegister();
    bool sentAddress = sendSlaveAddress(readBit);
    if(sentAddress)
        state = State::SendSlaveAddress;
    return;
}

void I2cBus::masterStateSendSlaveAddress()
{
    if(!LL_I2C_IsActiveFlag_ADDR(instance))
        return;

    if(currentTransaction->hasRegister())
    {
        state = State::SendRegister;
        currentTransaction->setState(I2cTransaction::SENDING_REGISTER);
    }
    else if(currentTransaction->isRx())
    {
        state = State::SendData;
        currentTransaction->setState(I2cTransaction::EXCHANGING_DATA);
    }
    else
    {
        prepareMasterRx(currentTransaction->getDataLengthBytes());
        state = State::ReceiveData;
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

    currentTransaction->setState(I2cTransaction::EXCHANGING_DATA);
    if(currentTransaction->isRx())
    {
        LL_I2C_DisableIT_BUF(instance);
        LL_I2C_GenerateStartCondition(instance);
        state = State::LastRegisterByte;
    }

    if(currentTransaction->isTx())
        state = State::SendData;

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
        state = State::SendLastDataByte;
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
    state = State::RepeatedStart;
}

void I2cBus::masterStateRepeatedStart()
{
    bool readBit = currentTransaction->isRx();
    bool sentAddress = sendSlaveAddress(readBit);
    if(sentAddress)
        state = State::RepeatedStartAckAddr;
}

void I2cBus::masterStateRepeatedStartAckAddr()
{
    if(!LL_I2C_IsActiveFlag_ADDR(instance))
        return;

    prepareMasterRx(currentTransaction->getDataLengthBytes());
    LL_I2C_ClearFlag_ADDR(instance);
    LL_I2C_EnableIT_BUF(instance);
    currentIndex = 0;

    state = State::ReceiveData;
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
            state = State::SlaveTransmit;
        else
            state = State::SlaveReceive;

        LL_I2C_EnableIT_BUF(instance);
        slave->onAddressMatch(sr2 & I2C_SR2_TRA ? I2cSlave::Direction::TX : I2cSlave::Direction::RX);
    }

    if(state == State::SlaveTransmit && LL_I2C_IsActiveFlag_TXE(instance))
        LL_I2C_TransmitData8(instance, slave->onReadByte());

    while(state == State::SlaveReceive && LL_I2C_IsActiveFlag_RXNE(instance))
        slave->onWriteByte(LL_I2C_ReceiveData8(instance));

    if(LL_I2C_IsActiveFlag_STOP(instance))
    {
        LL_I2C_ClearFlag_STOP(instance);
        LL_I2C_DisableIT_BUF(instance);
        slave->onEndTransaction();
        state = State::Idle;
    }
}

void I2cBus::eventMasterCallback()
{
    switch(state)
    {
        case State::Idle:
            // Slave behaviour only
            break;

        case State::StartAttempt:
            masterStateStartAttemp();
            break;

        case State::SendSlaveAddress:
            masterStateSendSlaveAddress();
            break;

        case State::SendRegister:
            masterStateSendRegister();
            break;

        case State::SendData:
            masterStateSendData();
            break;

        case State::SendLastDataByte:
            masterStateSendLastDataByte();
            break;

        case State::LastRegisterByte:
            masterStateSendLastRegisterByte();
            break;

        case State::RepeatedStart:
            masterStateRepeatedStart();
            break;

        case State::RepeatedStartAckAddr:
            masterStateRepeatedStartAckAddr();
            break;

        case State::ReceiveData:
            masterStateReceiveData();
            break;

        default:
            break;
    }
}

void I2cBus::errorCallback()
{
    bool slaveError = true;
    if (LL_I2C_IsActiveFlag_AF(instance))
    {
        LL_I2C_ClearFlag_AF(instance);

        // When sending data as a slave, transactions end with AF.
        if(state == State::SlaveTransmit)
        {
            slaveError = false;
            LL_I2C_DisableIT_BUF(instance);
            slave->onEndTransaction();
            state = State::Idle;
        }
    }

    if (LL_I2C_IsActiveFlag_ARLO(instance))
        LL_I2C_ClearFlag_ARLO(instance);

    if (LL_I2C_IsActiveFlag_BERR(instance))
        LL_I2C_ClearFlag_BERR(instance);

    if (LL_I2C_IsActiveFlag_OVR(instance))
    {
        LL_I2C_ReceiveData8(instance);
        LL_I2C_ClearFlag_OVR(instance);
    }

    if(slaveError)
        slave->onError();

    if(!currentTransaction)
        return;

    currentTransaction->setState(I2cTransaction::ERROR);
    currentTransaction->errorCallback();
    finishCurrentTransaction(false);
    sendNextTransaction();

}