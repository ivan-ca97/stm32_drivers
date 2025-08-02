#pragma once

#include <stdint.h>
#include <functional>

class I2cDevice;
class I2cBus;

class I2cTransaction
{
    public:
        class Builder;

        typedef enum
        {
            RX,
            TX
        }
        Direction;

        uint16_t getAddress(void);

        uint8_t getByte(uint16_t index);

        void setByte(uint8_t byte, uint16_t index);

        uint8_t* getDataPointer(void);

        uint16_t getDataLengthBytes(void);

        uint32_t getRegister(void);

        bool hasRegister(void);

        uint8_t getRegisterByte(uint8_t index);

        uint8_t getRegisterLengthBytes(void);

        bool isTx(void);

        bool isRx(void);

        void preCallback();

        void postCallback();

        void errorCallback();

    protected:
        Direction direction;
        I2cDevice* device;

        uint8_t* data;
        uint16_t dataBytes;
        uint32_t deviceRegister;
        uint8_t deviceRegisterBytes;

        void* preCallbackParameters = nullptr;
        void* postCallbackParameters = nullptr;
        void* errorCallbackParameters = nullptr;
        std::function<void(void*)> preCallbackFunction = nullptr;
        std::function<void(void*)> postCallbackFunction = nullptr;
        std::function<void(void*)> errorCallbackFunction = nullptr;

    friend class I2cDevice;
};

class I2cTransaction::Builder
{
    public:
        Builder& setDirection(Direction direction);

        Builder& withData(uint8_t* data, uint16_t sizeBytes);

        Builder& withRegister(uint32_t deviceRegister, uint8_t length = 1);

        Builder& withPreCallback(std::function<void(void*)> function, void* parameters = nullptr);

        Builder& withPostCallback(std::function<void(void*)> function, void* parameters = nullptr);

        Builder& withErrorCallback(std::function<void(void*)> function, void* parameters = nullptr);

        I2cTransaction build();

    protected:
        I2cTransaction transaction;
};