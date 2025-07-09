#include "i2c_bus.hpp"

#include "stm32f4xx_ll_i2c.h"

void I2cBus::sendSlaveAddress()
{
    if(LL_I2C_IsActiveFlag_SB(this->instance))
    {
        uint16_t address = this->currentTransaction->getAddress() << 1;
        if(currentTransaction->isRead())
            address |= 0x01;
        this->currentTransaction->preCallback();

        LL_I2C_TransmitData8(this->instance, address << 1);

        this->status = I2C_BUS_SEND_SLAVE_ADDRESS;
    }
}

void I2cBus::eventMasterCallback()
{
    I2C_TypeDef* i2cBusInstance = this->instance;

    switch(this->status)
    {
        case I2C_BUS_IDLE:
            // Slave behaviour only
            break;

        case I2C_BUS_START_ATTEMPT:
            if(LL_I2C_IsActiveFlag_SB(i2cBusInstance))
            {
                uint16_t address = this->currentTransaction->getAddress() << 1;
                if(currentTransaction->isRead() && !currentTransaction->hasRegister())
                    address |= 0x01;
                this->currentTransaction->preCallback();

                LL_I2C_TransmitData8(i2cBusInstance, address);

                this->status = I2C_BUS_SEND_SLAVE_ADDRESS;
            }
            break;

        case I2C_BUS_SEND_SLAVE_ADDRESS:
            if(LL_I2C_IsActiveFlag_ADDR(i2cBusInstance))
            {
                this->currentIndex = 0;

                LL_I2C_EnableIT_BUF(i2cBusInstance);
                if(currentTransaction->hasRegister())
                {
                    LL_I2C_ClearFlag_ADDR(i2cBusInstance);
                    this->status = I2C_BUS_SEND_REGISTER;
                    break;
                }

                if(this->currentTransaction->getDirection() == TRANSACTION_TX)
                {
                    LL_I2C_ClearFlag_ADDR(i2cBusInstance);
                    this->status = I2C_BUS_SEND_DATA;
                    break;
                }

                uint16_t size = currentTransaction->getDataLenthBytes();
                if (size == 1)
                {
                    LL_I2C_AcknowledgeNextData(i2cBusInstance, LL_I2C_NACK);
                    LL_I2C_GenerateStopCondition(i2cBusInstance);
                }
                else if (size == 2)
                {
                    LL_I2C_EnableBitPOS(i2cBusInstance);
                    LL_I2C_AcknowledgeNextData(i2cBusInstance, LL_I2C_NACK);
                }
                else
                {
                    LL_I2C_AcknowledgeNextData(i2cBusInstance, LL_I2C_ACK);
                }
                LL_I2C_ClearFlag_ADDR(i2cBusInstance);
                this->status = I2C_BUS_RECEIVE_DATA;
            }
            break;

        case I2C_BUS_SEND_REGISTER:
            if(LL_I2C_IsActiveFlag_TXE(i2cBusInstance))
            {
                uint8_t byte = this->currentTransaction->getRegisterByte(this->currentIndex++);
                LL_I2C_TransmitData8(i2cBusInstance, byte);

                if(this->currentIndex >= this->currentTransaction->getRegisterLengthBytes())
                {
                    this->currentIndex = 0;

                    if(this->currentTransaction->getDirection() == TRANSACTION_RX)
                    {
                        LL_I2C_DisableIT_BUF(i2cBusInstance);
                        LL_I2C_GenerateStartCondition(instance);
                        this->status = I2C_BUS_LAST_REGISTER_BYTE;
                        break;
                    }

                    if(this->currentTransaction->getDirection() == TRANSACTION_TX)
                    {
                        LL_I2C_EnableIT_BUF(instance);
                        this->status = I2C_BUS_SEND_DATA;
                        break;
                    }
                }
            }
            break;

        case I2C_BUS_SEND_DATA:
            if(LL_I2C_IsActiveFlag_TXE(i2cBusInstance))
            {
                uint8_t byte = this->currentTransaction->getByte(this->currentIndex++);
                LL_I2C_TransmitData8(i2cBusInstance, byte);
            }

            if(this->currentIndex >= this->currentTransaction->getDataLenthBytes())
            {
                LL_I2C_DisableIT_BUF(i2cBusInstance);
                this->status = I2C_BUS_LAST_DATA_BYTE;
                this->currentIndex = 0;
            }
            break;

        case I2C_BUS_LAST_REGISTER_BYTE:
            if(LL_I2C_IsActiveFlag_BTF(i2cBusInstance))
            {
                LL_I2C_GenerateStartCondition(i2cBusInstance);
                this->status = I2C_BUS_REPEATED_START;
            }
            break;

        case I2C_BUS_REPEATED_START:
            if(LL_I2C_IsActiveFlag_SB(i2cBusInstance))
            {
                uint16_t address = this->currentTransaction->getAddress() << 1;
                if(currentTransaction->isRead())
                    address |= 0x01;

                LL_I2C_TransmitData8(i2cBusInstance, address);

                this->status = I2C_BUS_REPEATED_START_ACK_ADDR;
            }
            break;

        case I2C_BUS_REPEATED_START_ACK_ADDR:
            if(LL_I2C_IsActiveFlag_ADDR(i2cBusInstance))
            {
                LL_I2C_EnableIT_BUF(i2cBusInstance);
                uint16_t size = currentTransaction->getDataLenthBytes();
                if (size == 1)
                {
                    LL_I2C_AcknowledgeNextData(i2cBusInstance, LL_I2C_NACK);
                    LL_I2C_GenerateStopCondition(i2cBusInstance);
                }
                else if (size == 2)
                {
                    LL_I2C_EnableBitPOS(i2cBusInstance);
                    LL_I2C_AcknowledgeNextData(i2cBusInstance, LL_I2C_NACK);
                }
                else
                {
                    LL_I2C_AcknowledgeNextData(i2cBusInstance, LL_I2C_ACK);
                }
                LL_I2C_ClearFlag_ADDR(i2cBusInstance);

                this->status = I2C_BUS_RECEIVE_DATA;
            }
            break;

        case I2C_BUS_RECEIVE_DATA:
            if(LL_I2C_IsActiveFlag_RXNE(i2cBusInstance))
            {
                uint8_t byte = LL_I2C_ReceiveData8(i2cBusInstance);

                currentTransaction->setByte(byte, currentIndex);
                currentIndex++;

                uint8_t remainingBytes = currentTransaction->getDataLenthBytes() - currentIndex;
                if(remainingBytes == 1)
                {
                    LL_I2C_AcknowledgeNextData(i2cBusInstance, LL_I2C_NACK);
                    LL_I2C_GenerateStopCondition(i2cBusInstance);
                }
                if(remainingBytes == 0)
                {
                    LL_I2C_DisableIT_BUF(i2cBusInstance);
                    this->currentTransaction->postCallback();
                    this->queue->dequeue();
                    this->status = I2C_BUS_IDLE;
                    this->sendNextTransaction();
                }
            }
            break;

        case I2C_BUS_LAST_DATA_BYTE:
            if(LL_I2C_IsActiveFlag_BTF(i2cBusInstance))
            {
                LL_I2C_GenerateStopCondition(i2cBusInstance);
                this->currentTransaction->postCallback();
                this->queue->dequeue();
                this->status = I2C_BUS_IDLE;
                this->sendNextTransaction();
            }
            break;
    }
}