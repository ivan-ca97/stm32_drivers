#pragma once

#include <stdint.h>
#include <functional>

class I2cDevice;
class I2cBus;

class I2cTransaction
{
    public:
        class Builder;

        enum Direction
        {
            RX,
            TX
        };

        enum State
        {
            IDLE,
            STARTING,
            SENDING_REGISTER,
            EXCHANGING_DATA,
            FINISHED,
            ERROR,

            CANCELLING,   // cancelled, but the bus still has it
            CANCELLED,   // cancelled and let go: the caller's again
        };

        uint16_t getAddress();

        uint8_t getByte(uint16_t index);

        void setByte(uint8_t byte, uint16_t index);

        uint8_t* getDataPointer();

        uint16_t getDataLengthBytes();

        uint32_t getRegister();

        bool hasRegister();

        uint8_t getRegisterByte(uint8_t index);

        uint8_t getRegisterLengthBytes();

        State getState();

        void setState(State state);

        bool isTx();

        bool isRx();

        void preCallback();

        void postCallback();

        void errorCallback();

        // Give up on this transaction: its callbacks will not run. Watch the state for
        // when the bus lets go.
        void cancel();

    protected:
        I2cDevice* device = nullptr;   // set when queued
        Direction direction;
        State state = IDLE;

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