#include "i2c_bus.hpp"
#include "i2c_bus_builder.hpp"

#include <algorithm>

#include "stm32f4xx_ll_i2c.h"

#define EXPECTED_TIMER_TOLERANCE_PERIOD_US 100
#define I2C_FAST_MODE_CUTOFF_FREQUENCY 100000
#define I2C_EVENT_IRQ_PRIORITY 1
#define I2C_ERROR_IRQ_PRIORITY 1

// Initialize with empty drivers array.
std::array<I2cBus*, I2C_BUS_MAX> I2cBus::drivers = {};

I2C_TypeDef* I2cBus::getInstance(void)
{
    return instance;
}

void I2cBus::timerCallback(void* argument)
{
    I2cBus* bus = static_cast<I2cBus*>(argument);

    bool sent = bus->sendNextTransaction();
    if(!sent)
        bus->scheduleTimer();
}

void I2cBus::scheduleTimer()
{
    auto ticks = verifyTimer();
    timer->setCallback(timerCallback, this);
    timer->setAlarm(ticks, true);
    timer->start();
}

bool I2cBus::verifyPendingTransaction()
{
    return sendNextTransaction();
}

bool I2cBus::sendNextTransaction(void)
{
    currentTransaction = queue->peek();
    if(!currentTransaction)
        return false;

    if(LL_I2C_IsActiveFlag_BUSY(instance))
    {
        if(timer)
            scheduleTimer();
        return false;
    }

    LL_I2C_GenerateStartCondition(instance);
    this->status = I2C_BUS_START_ATTEMPT;
    return true;
}

void I2cBus::setTransaction(I2cTransaction &transaction)
{
    queue->enqueue(transaction);

    if(queue->size() == 1 && status == I2C_BUS_IDLE)
    {
        sendNextTransaction();
    }
}

void I2cBus::eventCallback()
{
    switch(status)
    {
        case I2C_BUS_IDLE:
        case I2C_BUS_SLAVE_TRANSMIT:
        case I2C_BUS_SLAVE_RECEIVE:
            eventSlaveCallback();
            break;

        case I2C_BUS_START_ATTEMPT:
        case I2C_BUS_SEND_SLAVE_ADDRESS:
        case I2C_BUS_SEND_REGISTER:
        case I2C_BUS_SEND_DATA:
        case I2C_BUS_SEND_LAST_DATA_BYTE:
        case I2C_BUS_LAST_REGISTER_BYTE:
        case I2C_BUS_REPEATED_START:
        case I2C_BUS_REPEATED_START_ACK_ADDR:
        case I2C_BUS_RECEIVE_DATA:
            eventMasterCallback();
            break;
    }
}

void I2cBus::handleInterrupt(I2cBusSelection bus, I2cInterruptType type)
{
    I2cBus *driver = I2cBus::drivers[bus];
    if(driver)
    {
        switch(type)
        {
        case I2C_EVENT:
            // HAL_I2C_EV_IRQHandler(&driver->handle);
            driver->eventCallback();
            break;
        case I2C_ERROR:
            driver->errorCallback();
            break;
        }
    }
}

uint32_t I2cBus::verifyTimer()
{
    uint32_t timerPeriodUs = timer->getPeriodUs();

    uint32_t retryIntervalUs = retryIntervalMs * 1000;
    uint32_t expectedTicks = retryIntervalUs / timerPeriodUs;
    uint32_t actualRetryTimeUs  = timerPeriodUs * expectedTicks;

    if (expectedTicks == 0)
        throw I2cException("Retry interval too short for timer resolution");

    if (abs((int32_t)(actualRetryTimeUs  - retryIntervalUs)) > EXPECTED_TIMER_TOLERANCE_PERIOD_US)
        throw I2cException("Misconfigured timer");

    return expectedTicks;
}

I2cBus::I2cBus(const I2cBusConfig& config)
    : queue(config.queue), bus(config.bus), name(config.name), timer(config.timer), slave(config.slave), retryIntervalMs(config.retryIntervalMs)
{
    if(timer)
    {
        verifyTimer();
        timer->setCallback(timerCallback, this);
    }
    registerDriver(this->bus);

    this->fastMode = config.clockSpeed >= I2C_FAST_MODE_CUTOFF_FREQUENCY;

    bool masterOnly = (config.ownAddress1 == 0x0) && (config.ownAddress2 == 0x0);
    if(!masterOnly)
        areAddressesValid(config.ownAddress1, config.ownAddress2, config.addressing7Bit);

    initInstance(config);

    initGpio();
    initNvic();
}

void I2cBus::areAddressesValid(uint16_t ownAddress1, uint16_t ownAddress2, bool addressing7bit)
{
    if(!checkAddressValidity(ownAddress1, addressing7bit))
        throw I2cException("The provided I2C address 1 is not valid");

    // If ownAddress 2 is 0x00, single address is used.
    if(ownAddress2 == 0x00)
        return;

    if(!checkAddressValidity(ownAddress2, addressing7bit))
        throw I2cException("The provided I2C address 2 is not valid");
}

bool I2cBus::checkAddressValidity(uint16_t address, bool addressing7bit)
{
    // Addresses under 0x0F and over 0x78 are reserved in the I2C standard.
    if(addressing7bit && (address <= 0x0F || address >= 0x78))
    {
        return false;
    }

    // Check that the address exceeds the 10 bit range.
    if(!addressing7bit && address > 0x3FF)
    {
        return false;
    }

    return true;
}

I2cBusSelection I2cBus::getBusNumber(void)
{
    return bus;
}

I2cBusStatus I2cBus::getStatus()
{
    return status;
}

uint32_t I2cBus::getCurrentIndex()
{
    return currentIndex;
}

void I2cBus::registerDriver(I2cBusSelection bus)
{
    uint8_t i;

    switch(bus)
    {
        case I2C_BUS_1:
            i = 0;
            break;
        case I2C_BUS_2:
            i = 1;
            break;
        case I2C_BUS_3:
            i = 2;
            break;
        default:
            throw I2cException();
    }

    if(drivers[i] != nullptr)
        throw I2cException("Bus already in use");

    drivers[i] = this;
}

void I2cBus::initInstance(const I2cBusConfig& config)
{
    switch (this->bus)
    {
        case I2C_BUS_1:
            instance = I2C1;
            break;
        case I2C_BUS_2:
            instance = I2C2;
            break;
        case I2C_BUS_3:
            instance = I2C3;
            break;
        default:
            throw I2cException();
    }

    LL_I2C_Disable(instance);
    LL_I2C_DeInit(instance);

    uint32_t dutyCycle;
    if(config.dutyCycle == I2C_DUTY_CYCLE_2)
        dutyCycle = LL_I2C_DUTYCYCLE_2;
    if(config.dutyCycle == I2C_DUTY_CYCLE_16_9)
        dutyCycle = LL_I2C_DUTYCYCLE_16_9;

    LL_I2C_InitTypeDef i2cInit;
    LL_I2C_StructInit(&i2cInit);
    i2cInit.PeripheralMode  = LL_I2C_MODE_I2C;
    i2cInit.ClockSpeed      = config.clockSpeed;
    i2cInit.DutyCycle       = dutyCycle;
    i2cInit.OwnAddress1     = config.ownAddress1 << 1;
    i2cInit.TypeAcknowledge = LL_I2C_ACK;
    i2cInit.OwnAddrSize     = config.addressing7Bit ? LL_I2C_OWNADDRESS1_7BIT : LL_I2C_OWNADDRESS1_10BIT;

    auto initStatus = LL_I2C_Init(instance, &i2cInit);
    if(initStatus != SUCCESS)
        throw I2cException("Error initializing I2C.");

    // LL_I2C_Enable(instance);

    // Dual address
    LL_I2C_SetOwnAddress2(instance, config.ownAddress2);
    if(config.ownAddress2 != 0x00)
        LL_I2C_EnableOwnAddress2(instance);
    else
        LL_I2C_DisableOwnAddress2(instance);

    // Clock stretching
    if(config.clockStretching)
        LL_I2C_EnableClockStretching(instance);
    else
        LL_I2C_DisableClockStretching(instance);

    // General call
    if(config.generalCall)
        LL_I2C_EnableGeneralCall(instance);
    else
        LL_I2C_DisableGeneralCall(instance);

    LL_I2C_Enable(instance);
    LL_I2C_AcknowledgeNextData(instance, LL_I2C_ACK);
}

void I2cBus::initGpio(void)
{
    uint32_t pinMaskA = 0;
    uint32_t pinMaskB = 0;

    uint32_t alternateFunction = 0;

    switch(bus)
    {
        case I2C_BUS_1:
            __HAL_RCC_I2C1_CLK_ENABLE();

            pinMaskB = GPIO_PIN_6 | GPIO_PIN_7;
            alternateFunction = GPIO_AF4_I2C1;
            break;
        case I2C_BUS_2:
            __HAL_RCC_I2C2_CLK_ENABLE();

            pinMaskB = GPIO_PIN_3 | GPIO_PIN_10;
            alternateFunction = GPIO_AF4_I2C2;
            break;
        case I2C_BUS_3:
            __HAL_RCC_I2C3_CLK_ENABLE();

            pinMaskA = GPIO_PIN_8;
            pinMaskB = GPIO_PIN_4;
            alternateFunction = GPIO_AF4_I2C3;
            break;
    }

    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {
        .Pin = pinMaskB,
        .Mode = GPIO_MODE_AF_OD,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .Alternate = alternateFunction
    };

    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    if(pinMaskA)
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();

        GPIO_InitStruct.Pin = pinMaskA;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

void I2cBus::initNvic(void)
{
    IRQn_Type eventInterrupt;
    IRQn_Type errorInterrupt;

    switch(this->bus)
    {
        case I2C_BUS_1:
            eventInterrupt = I2C1_EV_IRQn;
            errorInterrupt = I2C1_ER_IRQn;
            break;
        case I2C_BUS_2:
            eventInterrupt = I2C2_EV_IRQn;
            errorInterrupt = I2C2_ER_IRQn;
            break;
        case I2C_BUS_3:
            eventInterrupt = I2C3_EV_IRQn;
            errorInterrupt = I2C3_ER_IRQn;
            break;
    }

    LL_I2C_EnableIT_EVT(this->instance);
    LL_I2C_EnableIT_ERR(this->instance);

    NVIC_SetPriority(eventInterrupt, I2C_EVENT_IRQ_PRIORITY);
    NVIC_SetPriority(errorInterrupt, I2C_ERROR_IRQ_PRIORITY);
    NVIC_EnableIRQ(eventInterrupt);
    NVIC_EnableIRQ(errorInterrupt);
}