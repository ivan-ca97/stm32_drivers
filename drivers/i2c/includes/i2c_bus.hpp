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

typedef enum
{
    I2C_BUS_1,
    I2C_BUS_2,
    I2C_BUS_3
}
I2cBusSelection;

typedef enum
{
    I2C_EVENT,
    I2C_ERROR
}
I2cInterruptType;

typedef enum
{
    I2C_DUTY_CYCLE_2,
    I2C_DUTY_CYCLE_16_9

}
I2cDutyCycle;

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
typedef enum
{
    I2C_BUS_IDLE,

    I2C_BUS_SLAVE_TRANSMIT,
    I2C_BUS_SLAVE_RECEIVE,

    I2C_BUS_START_ATTEMPT,
    I2C_BUS_SEND_SLAVE_ADDRESS,
    I2C_BUS_SEND_REGISTER,

    I2C_BUS_SEND_DATA,
    I2C_BUS_SEND_LAST_DATA_BYTE,

    I2C_BUS_LAST_REGISTER_BYTE,
    I2C_BUS_REPEATED_START,
    I2C_BUS_REPEATED_START_ACK_ADDR,
    I2C_BUS_RECEIVE_DATA,
}
I2cBusStatus;

class I2cBus
{
    public:
        class Builder;

        struct Config;

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

        I2cBusSelection getBusNumber();

        I2cBusStatus getStatus();

        uint32_t getCurrentIndex();

    protected:
        static std::array<I2cBus*, I2C_BUS_MAX> drivers;

        I2cSlave* slave = nullptr;

        Queue<I2cTransaction>* queue;

        Set<I2cDevice*>* attachedDevices;

        I2cTransaction* currentTransaction = nullptr;

        I2C_TypeDef* instance = nullptr;

        I2cBusSelection bus;

        bool fastMode;

        std::string name;

        I2cBusStatus status = I2C_BUS_IDLE;

        Timer* timer;

        uint16_t retryIntervalMs;

        uint32_t currentIndex;

        static void handleInterrupt(I2cBusSelection bus, I2cInterruptType type);

        static void timerCallback(void* argument);

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

        void registerDriver(I2cBusSelection bus);

        void initGpio();

        void deinitGpio();

        void initNvic();

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

        void setTransaction(I2cTransaction &transaction);

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