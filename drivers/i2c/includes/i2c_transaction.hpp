#pragma once

#include <stdint.h>

class I2cDevice;
class I2cBus;

typedef void (*Callback)(void*);

typedef enum
{
    TRANSACTION_RX,
    TRANSACTION_TX
}
TransactionDirection;

typedef enum
{
    REGISTER_NULL,
    REGISTER_8_BITS,
    REGISTER_16_BITS,
    REGISTER_24_BITS,
    REGISTER_32_BITS
}
RegisterLength;

typedef enum
{
    TRANSACTION_PENDING,
    TRANSACTION_IN_PROGRESS,
    TRANSACTION_SENT,
    TRANSACTION_ERROR,
}
I2cTransactionStatus;

class I2cTransaction
{
    protected:
        TransactionDirection direction;
        uint16_t address;
        uint8_t* data;
        uint16_t dataBytes;
        I2cDevice* device;
        I2cBus* bus;
        uint32_t deviceRegister;
        RegisterLength deviceRegisterBytes;
        I2cTransactionStatus status = TRANSACTION_PENDING;

        void* preCallbackParameters = nullptr;
        void* postCallbackParameters = nullptr;
        void* errorCallbackParameters = nullptr;
        Callback preCallbackFunction = nullptr;
        Callback postCallbackFunction = nullptr;
        Callback errorCallbackFunction = nullptr;

    public:
        I2cTransaction();

        static I2cTransaction I2cTxTransaction(I2cDevice *device, uint8_t* data, uint16_t dataBytes, uint32_t deviceRegister = 0, RegisterLength deviceRegisterBytes = REGISTER_NULL);

        static I2cTransaction I2cRxTransaction(I2cDevice *device, uint8_t* data, uint16_t dataBytes, uint32_t deviceRegister = 0, RegisterLength deviceRegisterBytes = REGISTER_NULL);

        I2cTransaction(TransactionDirection direction, uint8_t* data, uint16_t dataBytes, I2cDevice *device, uint16_t address, uint32_t deviceRegister = 0, RegisterLength deviceRegisterBytes = REGISTER_NULL);

        I2cTransaction(TransactionDirection direction, uint8_t* data, uint16_t dataBytes, uint16_t address, uint32_t deviceRegister = 0, RegisterLength deviceRegisterBytes = REGISTER_NULL);

        I2cTransaction(TransactionDirection direction, uint8_t* data, uint16_t dataBytes, I2cDevice *device, uint32_t deviceRegister = 0, RegisterLength deviceRegisterBytes = REGISTER_NULL);

        void setPreCallback(Callback callback, void* parameters);

        void setPostCallback(Callback callback, void* parameters);

        void setErrorCallback(Callback callback, void* parameters);

        void attachBus(I2cBus* bus);

        uint16_t getAddress(void);

        uint8_t* getDataPointer(void);

        uint16_t getDataLengthBytes(void);

        uint16_t getCurrentIndex(void);

        uint8_t getByte(uint16_t index);

        void setByte(uint8_t byte, uint16_t index);

        uint16_t getRegister(void);

        bool hasRegister(void);

        uint8_t getRegisterByte(uint8_t index);

        uint8_t getRegisterLengthBytes(void);

        TransactionDirection getDirection(void);

        bool isRead(void);

        bool isWrite(void);

        I2cTransactionStatus getStatus(void);

        void setStatus(I2cTransactionStatus newStatus);

        bool isDone(void);

        /*
         *  @brief Calls the pre-transaction callback before the transaction is set, with the configured parameters.
         */
        void preCallback(void);

        /*
         *  @brief Calls the post-transaction callback after the transaction is finished, with the configured parameters.
         */
        void postCallback(void);

        /*
         *  @brief Calls the transaction error callback after the transaction fails, with the configured parameters.
         */
        void errorCallback(void);

        void send(void);
};