#pragma once
#include "timer.hpp"

class Timer;

struct Timer::Config
{
    uint32_t count = 0;
    uint32_t frequency = 0;
    uint32_t prescaler = 0;
    bool autoStart = false;
    bool enableInterrupt = false;
    bool oneShotAlarm = false;
    std::function<void(void*)> callback = nullptr;
    void* callbackArguments = nullptr;
    TimerSelection timer = TIMER_MAX;
};

class Timer::Builder
{
    private:
        Config config;

    public:
        void buildIn(Timer& target);

        Config buildConfig();

        Builder& setCount(uint32_t count);

        Builder& setPrescaler(uint32_t prescaler);

        Builder& setFrequency(uint32_t frequency);

        Builder& setCallback(void (*callback)(void*));

        Builder& setCallbackArguments(void* callbackArguments);

        Builder& timerSelection(TimerSelection timer);

        Builder& autoStart();

        Builder& enableInterrupt();

        Builder& setAlarm(uint32_t count);

        Builder& oneShot();

        Builder& periodic();
};