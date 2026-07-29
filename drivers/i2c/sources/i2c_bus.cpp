#include "i2c_bus.hpp"
#include "i2c_bus_builder.hpp"
#include "i2c_bus_hw.hpp"
#include "i2c_device.hpp"

#include "stm32f4xx_ll_i2c.h"

#define EXPECTED_TIMER_TOLERANCE_PERIOD_US 100
#define I2C_FAST_MODE_CUTOFF_FREQUENCY 100000
#define I2C_EVENT_IRQ_PRIORITY 1
#define I2C_ERROR_IRQ_PRIORITY 1

// Initialize with empty drivers array.
std::array<I2cBus*, I2C_BUS_MAX> I2cBus::drivers = {};

I2C_TypeDef* I2cBus::getInstance()
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

bool I2cBus::sendNextTransaction()
{
    auto newTransaction = queue->peek();
    if(!newTransaction)
        return false;

    currentTransaction = *newTransaction;

    if(LL_I2C_IsActiveFlag_BUSY(instance))
    {
        if(timer)
            scheduleTimer();
        return false;
    }

    LL_I2C_GenerateStartCondition(instance);
    currentTransaction->setState(I2cTransaction::STARTING);
    state = State::StartAttempt;
    return true;
}

void I2cBus::setTransaction(I2cTransaction& transaction)
{
    queue->enqueue(&transaction);

    if(queue->size() == 1 && state == State::Idle)
        sendNextTransaction();
}

void I2cBus::eventCallback()
{
    switch(state)
    {
        case State::Idle:
        case State::SlaveTransmit:
        case State::SlaveReceive:
            eventSlaveCallback();
            break;

        case State::StartAttempt:
        case State::SendSlaveAddress:
        case State::SendRegister:
        case State::SendData:
        case State::SendLastDataByte:
        case State::LastRegisterByte:
        case State::RepeatedStart:
        case State::RepeatedStartAckAddr:
        case State::ReceiveData:
            eventMasterCallback();
            break;
    }
}

void I2cBus::handleInterrupt(Selection bus, InterruptType type)
{
    I2cBus *driver = I2cBus::drivers[getBusDriverNumber(bus)];
    if(driver)
    {
        switch(type)
        {
        case InterruptType::Event:
            driver->eventCallback();
            break;
        case InterruptType::Error:
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

void I2cBus::init(const Config& config)
{
    bus = config.bus;
    name = config.name;
    slave = config.slave;
    queue = config.queue;
    attachedDevices = config.devicesSet;
    timer = config.timer;
    retryIntervalMs = config.retryIntervalMs;

    // Save init parameters so resetBus() can reconfigure the peripheral.
    clockSpeed      = config.clockSpeed;
    ownAddress1     = config.ownAddress1;
    ownAddress2     = config.ownAddress2;
    addressing7Bit  = config.addressing7Bit;
    dutyCycle       = config.dutyCycle;
    clockStretching = config.clockStretching;
    generalCall     = config.generalCall;

    if(slave)
        slave->setBus(*this);

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

    // Free the bus in case it got stuck (a slave holding SDA, or the peripheral
    // with BUSY latched) BEFORE configuring the pins as I2C.
    recoverBus();

    // GPIO must be configured BEFORE enabling the peripheral: if the I2C is enabled
    // while SDA/SCL are low, the BUSY flag latches and won't clear without a peripheral reset.
    initGpio();
    initInstance();
    enableInterrupts();
}

I2cBus::I2cBus(const Config& config)
{
    init(config);
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
        return false;

    // Check that the address exceeds the 10 bit range.
    if(!addressing7bit && address > 0x3FF)
        return false;

    return true;
}

I2cBus::Selection I2cBus::getBusNumber()
{
    return bus;
}

I2cBus::State I2cBus::getState()
{
    return state;
}

uint32_t I2cBus::getCurrentIndex()
{
    return currentIndex;
}

void I2cBus::registerDriver(Selection bus)
{
    uint16_t i = getBusDriverNumber(bus);

    if(drivers[i] != nullptr)
        throw I2cException("Bus already in use");

    drivers[i] = this;
}

void I2cBus::initInstance()
{
    instance = i2cBusHw(bus).instance;

    LL_I2C_Disable(instance);
    LL_I2C_DeInit(instance);

    uint32_t llDutyCycle = LL_I2C_DUTYCYCLE_2;
    if(dutyCycle == DutyCycle::Dc_16_9)
        llDutyCycle = LL_I2C_DUTYCYCLE_16_9;

    LL_I2C_InitTypeDef i2cInit;
    LL_I2C_StructInit(&i2cInit);
    i2cInit.PeripheralMode  = LL_I2C_MODE_I2C;
    i2cInit.ClockSpeed      = clockSpeed;
    i2cInit.DutyCycle       = llDutyCycle;
    i2cInit.OwnAddress1     = ownAddress1 << 1;
    i2cInit.TypeAcknowledge = LL_I2C_ACK;
    i2cInit.OwnAddrSize     = addressing7Bit ? LL_I2C_OWNADDRESS1_7BIT : LL_I2C_OWNADDRESS1_10BIT;

    auto initStatus = LL_I2C_Init(instance, &i2cInit);
    if(initStatus != SUCCESS)
        throw I2cException("Error initializing I2C.");

    // Dual address
    LL_I2C_SetOwnAddress2(instance, ownAddress2);
    if(ownAddress2 != 0x00)
        LL_I2C_EnableOwnAddress2(instance);
    else
        LL_I2C_DisableOwnAddress2(instance);

    // Clock stretching
    if(clockStretching)
        LL_I2C_EnableClockStretching(instance);
    else
        LL_I2C_DisableClockStretching(instance);

    // General call
    if(generalCall)
        LL_I2C_EnableGeneralCall(instance);
    else
        LL_I2C_DisableGeneralCall(instance);

    LL_I2C_Enable(instance);
    LL_I2C_AcknowledgeNextData(instance, LL_I2C_ACK);
}

namespace
{
    // Rough delay of tens of microseconds for the recovery bit-bang.
    // It doesn't need to be precise; slower is safe.
    inline void i2cBusDelay()
    {
        for(volatile uint32_t i = 0; i < 200; i++)
            __NOP();
    }
}

void I2cBus::recoverBus()
{
    const I2cBusHw& hw = i2cBusHw(bus);
    hw.enableClocks();

    // SCL and SDA as open-drain GPIO with pull-up, released (high).
    GPIO_InitTypeDef gpio = {};
    gpio.Mode  = GPIO_MODE_OUTPUT_OD;
    gpio.Pull  = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    gpio.Pin = hw.sclPin;
    HAL_GPIO_Init(hw.sclPort, &gpio);
    gpio.Pin = hw.sdaPin;
    HAL_GPIO_Init(hw.sdaPort, &gpio);

    HAL_GPIO_WritePin(hw.sclPort, hw.sclPin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(hw.sdaPort, hw.sdaPin, GPIO_PIN_SET);
    i2cBusDelay();

    // If a slave holds SDA low, clock SCL up to 9 times so it releases it.
    for(uint8_t i = 0; i < 9; i++)
    {
        if(HAL_GPIO_ReadPin(hw.sdaPort, hw.sdaPin) == GPIO_PIN_SET)
            break;

        HAL_GPIO_WritePin(hw.sclPort, hw.sclPin, GPIO_PIN_RESET);
        i2cBusDelay();
        HAL_GPIO_WritePin(hw.sclPort, hw.sclPin, GPIO_PIN_SET);
        i2cBusDelay();
    }

    // Manual STOP condition: SDA low-to-high while SCL is high.
    HAL_GPIO_WritePin(hw.sdaPort, hw.sdaPin, GPIO_PIN_RESET);
    i2cBusDelay();
    HAL_GPIO_WritePin(hw.sclPort, hw.sclPin, GPIO_PIN_SET);
    i2cBusDelay();
    HAL_GPIO_WritePin(hw.sdaPort, hw.sdaPin, GPIO_PIN_SET);
    i2cBusDelay();
}

void I2cBus::resetBus()
{
    // Full bus recovery WITHOUT an MCU reset. Used when the peripheral gets stuck
    // (BUSY/BERR latched) or a slave holds SDA.
    disableInterrupts();
    LL_I2C_Disable(instance);

    recoverBus();      // free the lines via bit-bang (SCL + STOP)
    initGpio();        // pins back to I2C alternate-function
    initInstance();    // LL_I2C_DeInit (RCC reset, clears BUSY) + reconfigure + enable
    enableInterrupts();

    currentIndex = 0;
    currentTransaction = nullptr;
    state = State::Idle;
}

void I2cBus::initGpio()
{
    const I2cBusHw& hw = i2cBusHw(bus);
    hw.enableClocks();

    // Per-pin alternate function (SCL and SDA may differ, see i2c_bus_hw.hpp).
    // Internal pull-up as a bring-up safety net (~40k, weak): for reliable
    // operation use external ~4.7k pull-ups to 3V3.
    GPIO_InitTypeDef gpio = {};
    gpio.Mode  = GPIO_MODE_AF_OD;
    gpio.Pull  = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    gpio.Pin = hw.sclPin;
    gpio.Alternate = hw.sclAf;
    HAL_GPIO_Init(hw.sclPort, &gpio);

    gpio.Pin = hw.sdaPin;
    gpio.Alternate = hw.sdaAf;
    HAL_GPIO_Init(hw.sdaPort, &gpio);
}

void I2cBus::deinitGpio()
{
    const I2cBusHw& hw = i2cBusHw(bus);
    HAL_GPIO_DeInit(hw.sclPort, hw.sclPin);
    HAL_GPIO_DeInit(hw.sdaPort, hw.sdaPin);
}

void I2cBus::enableInterrupts()
{
    const I2cBusHw& hw = i2cBusHw(bus);

    LL_I2C_EnableIT_EVT(this->instance);
    LL_I2C_EnableIT_ERR(this->instance);

    NVIC_SetPriority(hw.evIrq, I2C_EVENT_IRQ_PRIORITY);
    NVIC_SetPriority(hw.erIrq, I2C_ERROR_IRQ_PRIORITY);
    NVIC_EnableIRQ(hw.evIrq);
    NVIC_EnableIRQ(hw.erIrq);
}

void I2cBus::disableInterrupts()
{
    const I2cBusHw& hw = i2cBusHw(bus);
    NVIC_DisableIRQ(hw.evIrq);
    NVIC_DisableIRQ(hw.erIrq);
}

void I2cBus::attachDevice(I2cDevice& device)
{
    attachedDevices->add(&device);
}

void I2cBus::detachDevice(I2cDevice& device)
{
    int length = static_cast<int>(queue->size());
    for(auto i = length - 1; i >= 0; i--)
    {
        auto transaction = *queue->peek(i);
        if(transaction->getAddress() == device.getAddress())
        {
            // If the transaction to remove is the current one, stop it.
            if(i == 0 && state != State::Idle)
                finishCurrentTransaction(false);
            else
                queue->dequeue(i);
        }
    }

    attachedDevices->remove(&device);
}

uint16_t I2cBus::getBusDriverNumber(Selection bus)
{
    // Selection enum values are the driver-array indices (Bus1=0, Bus2=1, Bus3=2).
    return static_cast<uint16_t>(bus);
}

I2cBus::~I2cBus()
{
    drivers[getBusDriverNumber(bus)] = nullptr;
    deinitGpio();
    disableInterrupts();
    LL_I2C_Disable(this->instance);

    auto length = attachedDevices->getLength();
    for(uint16_t i = 0; i < length; i++)
        attachedDevices->pop()->detachBus();
}