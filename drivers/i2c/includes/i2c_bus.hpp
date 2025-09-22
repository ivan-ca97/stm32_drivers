#pragma once

#include <stdint.h>
#include <array>
#include "stm32f4xx.h"

#include "i2c_driver_exceptions.hpp"
#include "i2c_transaction.hpp"
#include "i2c_slave.hpp"

#include "timer.hpp"
#include "queue.hpp"
#include "set.hpp"

#define I2C_BUS_MAX 3

#ifdef __cplusplus
extern "C" {
#endif
void I2C1_ER_IRQHandler();
void I2C1_EV_IRQHandler();
void I2C2_ER_IRQHandler();
void I2C2_EV_IRQHandler();
void I2C3_ER_IRQHandler();
void I2C3_EV_IRQHandler();
#ifdef __cplusplus
}
#endif

class I2cDevice;

class I2cBus
{
    public:
        class Builder;

        struct Config;

        enum class State
        {
            Idle,

            SlaveTransmit,
            SlaveReceive,

            StartAttempt,
            SendSlaveAddress,
            SendRegister,

            SendData,
            SendLastDataByte,

            LastRegisterByte,
            RepeatedStart,
            RepeatedStartAckAddr,
            ReceiveData,
        };

        enum class Selection
        {
            Bus1 = 0,
            Bus2 = 1,
            Bus3 = 2
        };

        enum class InterruptType
        {
            Event,
            Error
        };

        enum class DutyCycle
        {
            Dc_2,
            Dc_16_9

        };

        I2C_TypeDef* getInstance();

        void init(const Config& config);

        I2cBus() = default;
        I2cBus(const Config& config);
        ~I2cBus();

        bool verifyPendingTransaction();

        /*
         *  @brief Checks whether the address is valid, taking into account the addressing mode
         *  (7 bit or 10 bit)
         *
         *	@param address Address to be checked
         *	@param addressing7bit Addressing mode to consider (false: 10 bit- true: 7 bit)
         */
        bool checkAddressValidity(uint16_t address, bool addressing7bit);

        Selection getBusNumber();

        State getState();

        uint32_t getCurrentIndex();

        void enableInterrupts();
        void disableInterrupts();

    protected:
        static std::array<I2cBus*, I2C_BUS_MAX> drivers;

        I2cSlave* slave = nullptr;

        Queue<I2cTransaction*>* queue;

        Set<I2cDevice*>* attachedDevices;

        I2cTransaction* currentTransaction = nullptr;

        I2C_TypeDef* instance = nullptr;

        Selection bus;

        bool fastMode;

        std::string name;

        State state = State::Idle;

        Timer* timer;

        uint16_t retryIntervalMs;

        uint32_t currentIndex;

        static void handleInterrupt(Selection bus, InterruptType type);

        static void timerCallback(void* argument);

        static uint16_t getBusDriverNumber(Selection bus);

        uint32_t verifyTimer();

        void scheduleTimer();

        /*
         *  @brief Initializes the I2C instance with the given parameters
         *
         *	@param config All the configuration parameters passed in the constructor.
         *
         *  @throws I2cException: If there's a HAL error.
         */
        void initInstance(const Config& config);

        void registerDriver(Selection bus);

        void initGpio();
        void deinitGpio();

        /*
         *  @brief Checks whether the addresses are valid, taking into account the addressing mode
         *  (7 bit or 10 bit).
         *
         *	@param ownAddress1 Address 1 to be checked
         *	@param ownAddress2 Address 2 to be checked
         *	@param addressing7bit Addressing mode to consider (false: 10 bit- true: 7 bit)
         *
         *  @throws I2cException: When the provided parameters are not in a valid state.
         */
        void areAddressesValid(uint16_t ownAddress1, uint16_t ownAddress2, bool addressing7bit);

        void attachDevice(I2cDevice& device);

        void detachDevice(I2cDevice& device);

        bool sendNextTransaction();

        void setTransaction(I2cTransaction& transaction);

        void eventCallback();

        void errorCallback();

        bool sendSlaveAddress(bool readBit);
        void prepareMasterRx(uint8_t remainingBytes);
        void finishCurrentTransaction(bool postCallback);

        void masterStateStartAttemp();
        void masterStateSendSlaveAddress();
        void masterStateSendRegister();
        void masterStateSendData();
        void masterStateSendLastDataByte();
        void masterStateSendLastRegisterByte();
        void masterStateRepeatedStart();
        void masterStateRepeatedStartAckAddr();
        void masterStateReceiveData();

        void eventSlaveCallback();
        void eventMasterCallback();

    friend class I2cDevice;

    // Interrupt handlers declared as friends
    friend void I2C1_EV_IRQHandler();

    friend void I2C2_EV_IRQHandler();

    friend void I2C3_EV_IRQHandler();

    friend void I2C1_ER_IRQHandler();

    friend void I2C2_ER_IRQHandler();

    friend void I2C3_ER_IRQHandler();
};