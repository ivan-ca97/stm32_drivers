#include "timer_builder.hpp"


Timer::Builder& Timer::Builder::setCount(uint32_t count)
{
    config.count = count;
    return *this;
}

Timer::Builder& Timer::Builder::setPrescaler(uint32_t prescaler)
{
    config.prescaler = prescaler;
    return *this;
}

Timer::Builder& Timer::Builder::setFrequency(uint32_t frequency)
{
    config.frequency = frequency;
    return *this;
}

Timer::Builder& Timer::Builder::autoStart()
{
    config.autoStart = true;
    return *this;
}

Timer::Builder& Timer::Builder::enableInterrupt()
{
    config.enableInterrupt = true;
    return *this;
}

Timer::Builder& Timer::Builder::setAlarm(uint32_t count)
{
    config.enableInterrupt = true;
    config.count = count;
    return *this;
}

Timer::Builder& Timer::Builder::oneShot()
{
    config.oneShotAlarm = true;
    return *this;
}

Timer::Builder& Timer::Builder::periodic()
{
    config.oneShotAlarm = false;
    return *this;
}

Timer::Builder& Timer::Builder::setCallback(void (*callback)(void*))
{
    config.callback = callback;
    return *this;
}

Timer::Builder& Timer::Builder::setCallbackArguments(void* callbackArguments)
{
    config.callbackArguments = callbackArguments;
    return *this;
}

Timer::Builder& Timer::Builder::timerSelection(TimerSelection timer)
{
    config.timer = timer;
    return *this;
}

void Timer::Builder::buildIn(Timer& target)
{
    return target.init(config);
}

Timer::Config Timer::Builder::buildConfig()
{
    return config;
}