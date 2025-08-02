#include "timer.hpp"
#include "timer_builder.hpp"

#include "stm32f4xx.h"

// Initialize with empty drivers array.
std::array<Timer*, TIMER_MAX> Timer::drivers = {};

void Timer::initializePrescaler(uint32_t prescaler, uint32_t frequency)
{
    if(!prescaler)
    {
        auto baseClock = this->getBaseClockFrequency();
        prescaler = baseClock/frequency - 1;
    }
    this->timerRegister->PSC = prescaler;
    this->forceUpdate();
}

Timer::Timer(const Config& config)
{
    init(config);
}

void Timer::init(const Config& config)
{
    timer = config.timer;
    timerRegister = this->getTimerRegisters(config.timer);
    callback = config.callback;

    this->registerTimer(config.timer);

    this->enableClock(config.timer);

    this->initializePrescaler(config.prescaler, config.frequency);

    if(config.enableInterrupt)
        this->setAlarm(config.count, config.oneShotAlarm);

    if(config.autoStart)
        this->start();
}

void Timer::forceUpdate()
{
    // While TIM_CR1_URS is set to true, only overflow events will trigger an interrupt.
    this->timerRegister->CR1 |= TIM_CR1_URS;
    this->timerRegister->EGR |= TIM_EGR_UG;
    this->timerRegister->CR1 &= ~TIM_CR1_URS;
}

void Timer::setCount(uint32_t count)
{
    this->timerRegister->CNT = count;
}

void Timer::setFrequency(uint32_t frequency)
{
    auto baseClock = this->getBaseClockFrequency();
    this->timerRegister->PSC = baseClock/frequency - 1;
}

void Timer::setPrescaler(uint32_t prescaler)
{
    this->timerRegister->PSC = prescaler;
}

uint32_t Timer::getFrequency()
{
    auto baseClock = this->getBaseClockFrequency();
    return baseClock / (this->timerRegister->PSC + 1);
}

uint32_t Timer::getPeriodUs()
{
    auto baseClock = getBaseClockFrequency();
    return (this->timerRegister->PSC + 1) / (baseClock / 1000000);
}

uint32_t Timer::getPrescaler()
{
    return this->timerRegister->PSC;
}

void Timer::setCallback(std::function<void(void*)> callback, void* argument)
{
    this->callback = callback;
    this->callbackArguments = argument;
}

void Timer::setAlarm(uint32_t count, bool oneShot)
{
    this->resetCount = count;
    this->oneShotAlarm = oneShot;

    if(oneShot)
        this->timerRegister->CR1 |= TIM_CR1_OPM;
    else
        this->timerRegister->CR1 &= ~TIM_CR1_OPM;

    this->timerRegister->ARR = count;

    this->setCount(this->resetCount);
    this->enableInterrupt();
}

void Timer::resetAlarm()
{
    if(this->oneShotAlarm)
        this->pause();
    this->setCount(this->resetCount);
}

void Timer::registerTimer(TimerSelection timer)
{
    if(timer == TIMER_MAX)
        throw std::exception(); // TODO custom exception
    if(this->drivers[timer] != nullptr)
        throw std::exception(); // TODO custom exception

    this->drivers[timer] = this;
}

void Timer::start()
{
    this->timerRegister->CR1 |= TIM_CR1_CEN;
}

void Timer::pause()
{
    this->timerRegister->CR1 &= ~TIM_CR1_CEN;
}

bool Timer::isRunning()
{
    return this->timerRegister->CR1 & TIM_CR1_CEN;
}

uint32_t Timer::getCount()
{
    return this->timerRegister->CNT;
}

void Timer::enableInterrupt()
{
    this->alarmOn = true;
    this->timerRegister->DIER |= TIM_DIER_UIE;
    this->timerRegister->CR1 |= TIM_CR1_DIR;

    // TODO puedo diferenciar entre TIM1, TIM9, TIM10 y TIM11?
    switch(this->timer)
    {
        case TIMER_1:
            NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
            break;
        case TIMER_2:
            NVIC_EnableIRQ(TIM2_IRQn);
            break;
        case TIMER_3:
            NVIC_EnableIRQ(TIM3_IRQn);
            break;
        case TIMER_4:
            NVIC_EnableIRQ(TIM4_IRQn);
            break;
        case TIMER_5:
            NVIC_EnableIRQ(TIM5_IRQn);
            break;
        case TIMER_9:
            NVIC_EnableIRQ(TIM1_BRK_TIM9_IRQn);
            break;
        case TIMER_10:
            NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);
            break;
        case TIMER_11:
            NVIC_EnableIRQ(TIM1_TRG_COM_TIM11_IRQn);
            break;
        default:
            throw std::exception(); // TODO custom exception
    }
}

void Timer::disableInterrupt()
{
    this->timerRegister->DIER &= ~TIM_DIER_UIE;
    this->timerRegister->CR1 &= ~TIM_CR1_DIR;
}

TIM_TypeDef* Timer::getTimerRegisters(TimerSelection timer)
{
    switch(timer)
    {
        case TIMER_1:
            return TIM1;
        case TIMER_2:
            return TIM2;
        case TIMER_3:
            return TIM3;
        case TIMER_4:
            return TIM4;
        case TIMER_5:
            return TIM5;
        case TIMER_9:
            return TIM9;
        case TIMER_10:
            return TIM10;
        case TIMER_11:
            return TIM11;
        default:
            throw std::exception(); // TODO custom exception
    }
}

void Timer::enableClock(TimerSelection timer)
{
    switch(timer)
    {
        case TIMER_1:
            RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
            return;
        case TIMER_2:
            RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
            return;
        case TIMER_3:
            RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
            return;
        case TIMER_4:
            RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
            return;
        case TIMER_5:
            RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;
            return;
        case TIMER_9:
            RCC->APB2ENR |= RCC_APB2ENR_TIM9EN;
            return;
        case TIMER_10:
            RCC->APB2ENR |= RCC_APB2ENR_TIM10EN;
            return;
        case TIMER_11:
            RCC->APB2ENR |= RCC_APB2ENR_TIM11EN;
            return;
        default:
            throw std::exception(); // TODO custom exception
    }
}

Timer* Timer::getDriver(TimerSelection timer)
{
    return Timer::drivers[timer];
}

uint32_t Timer::getBaseClockFrequency()
{
    SystemCoreClockUpdate();
    return SystemCoreClock;
}

bool Timer::isTimerUsed(TimerSelection timer)
{
    return Timer::drivers[timer] != nullptr;
}

void Timer::handleInterrupt()
{
    if (this->timerRegister->SR & TIM_SR_UIF)
    {
        this->timerRegister->SR &= ~TIM_SR_UIF;
        if(this->callback)
            this->callback(callbackArguments);
    }
}