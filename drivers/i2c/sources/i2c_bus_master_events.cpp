#include "i2c_bus.hpp"

#include "stm32f4xx_ll_i2c.h"

#define READ false
#define WRITE true

// ============================================================================
// I2C master state machine (STM32 I2Cv1) — theory of operation
//
// The master is a byte-at-a-time state machine driven by the EV/ER interrupts.
// Each masterState*() handler runs on an event and advances `state`.
//
// Transfer sequences on the wire:
//   Write:          START addr+W  data...                       STOP
//   Read:           START addr+R  data...                       STOP
//   Register read:  START addr+W  reg  REPEATED-START addr+R  data...  STOP
//   Register write: START addr+W  reg  data...                  STOP
//
// Key I2Cv1 flags and the timing rules the FSM relies on:
//   SB    - Start Bit sent -> send the slave address now.
//   ADDR  - address ACKed  -> clear it (read SR1 then SR2) to proceed.
//   TXE   - DR empty (byte moved to the shift register) -> load next byte.
//           The previous byte is STILL shifting out on the wire.
//   BTF   - Byte Transfer Finished: the byte is fully on the wire and the clock
//           is stretched. Only now is it safe to issue a STOP or a
//           REPEATED-START. Acting on TXE instead of BTF would cut the byte.
//   RXNE  - receive DR holds a byte to read.
//   BUF IT (ITBUFEN) enables the TXE/RXNE interrupts; it is disabled while
//           waiting for BTF so we don't get spurious TXE-driven callbacks.
//
// Master receive tail (ST procedure): for the last 1-2 bytes, ACK/POS must be
// set before reading so the last byte is NACKed and a STOP is issued. See
// prepareMasterRx().
//
// STOP is a MASTER-only action: generating a STOP while addressed as a slave latches
// the STOP bit and breaks the slave. So error handling must deal with the slave/idle
// cases and return BEFORE the master recovery path (which issues a STOP to release the
// bus). See errorCallback().
// ============================================================================

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
    currentTransaction = nullptr;
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
    else if(currentTransaction->isTx())
    {
        state = State::SendData;
        currentTransaction->setState(I2cTransaction::EXCHANGING_DATA);
    }
    else
    {
        prepareMasterRx(currentTransaction->getDataLengthBytes());
        currentTransaction->setState(I2cTransaction::EXCHANGING_DATA);
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
        // Register sent. For a read, wait for BTF before the repeated-START (see the
        // file header) — masterStateSendLastRegisterByte issues it. Don't START here.
        LL_I2C_DisableIT_BUF(instance);
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
    if(!slave)
        return;

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
    // Read and clear all error flags.
    bool af   = LL_I2C_IsActiveFlag_AF(instance);
    bool arlo = LL_I2C_IsActiveFlag_ARLO(instance);
    bool berr = LL_I2C_IsActiveFlag_BERR(instance);
    bool ovr  = LL_I2C_IsActiveFlag_OVR(instance);

    if(af)   LL_I2C_ClearFlag_AF(instance);
    if(arlo) LL_I2C_ClearFlag_ARLO(instance);
    if(berr) LL_I2C_ClearFlag_BERR(instance);
    if(ovr) { LL_I2C_ReceiveData8(instance); LL_I2C_ClearFlag_OVR(instance); }

    // Handle slave-side and idle errors first, then return: the master recovery below
    // issues a STOP, which is master-only (see file header).

    // Slave-side error
    if(state == State::SlaveTransmit || state == State::SlaveReceive)
    {
        LL_I2C_DisableIT_BUF(instance);

        // AF during SlaveTransmit is the NORMAL end: the master NACKs the last byte.
        if(af && state == State::SlaveTransmit)
        {
            if(slave) slave->onEndTransaction();
        }
        else
        {
            if(slave) slave->onError();
        }
        state = State::Idle;
        LL_I2C_AcknowledgeNextData(instance, LL_I2C_ACK);   // re-arm ACK for the next transaction
        return;
    }

    bool hadMasterTransaction = (currentTransaction != nullptr);

    // No master transaction in progress: spurious error while idle / addressed as a slave.
    if(!hadMasterTransaction)
    {
        LL_I2C_AcknowledgeNextData(instance, LL_I2C_ACK);
        if(berr && LL_I2C_IsActiveFlag_BUSY(instance))
            resetBus();
        return;
    }

    // Master-side error with a transaction in progress
    LL_I2C_DisableIT_BUF(instance);

    currentTransaction->setState(I2cTransaction::ERROR);
    currentTransaction->errorCallback();
    if(queue && queue->hasData())
        queue->dequeue();
    currentTransaction = nullptr;
    state = State::Idle;

    // Release the bus with a STOP (required after a NACK as master).
    LL_I2C_GenerateStopCondition(instance);

    // Full recovery (no MCU reset) ONLY on a real bus error.
    if(berr)
        resetBus();

    // Try to make progress with whatever is left in the queue.
    sendNextTransaction();
}