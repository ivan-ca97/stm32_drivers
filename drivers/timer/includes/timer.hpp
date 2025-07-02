#pragma once

// TODO diferenciar entre timers 16 y 32 bits
// TODO Auto elegir timer cuando no se elige uno en el constuctor
// TODO Destructor
// TODO custom exception
// TODO Interrupciones
// TODO Crear atributo para tener el registro en vez de usar TimerSelection?
// TODO Lanzar excepciones en caso de timer mal configurado (ejemplo prescaler muy grande)
// TODO Si el timer esta configurado por frecuencia, auto ajustar prescaler en caso de cambio de clock

// - TIM1 → Avanzado, 16 bits
// - TIM2 → General, 32 bits
// - TIM3 → General, 16 bits
// - TIM4 → General, 16 bits
// - TIM5 → General, 32 bits
// - TIM9 → General, 16 bits
// - TIM10 → Básico, 16 bits
// - TIM11 → Básico, 16 bits
#ifdef __cplusplus
extern "C" {
#endif
void TIM1_UP_TIM10_IRQHandler(void);
void TIM1_BRK_TIM9_IRQHandler(void);
void TIM1_TRG_COM_TIM11_IRQHandler(void);
void TIM1_CC_IRQHandler(void);
void TIM2_IRQHandler(void);
void TIM3_IRQHandler(void);
void TIM4_IRQHandler(void);
#ifdef __cplusplus
}
#endif

#include <array>
#include "stm32f401xc.h"

typedef enum
{
    TIMER_1,
    TIMER_2,
    TIMER_3,
    TIMER_4,
    TIMER_5,
    TIMER_9,
    TIMER_10,
    TIMER_11,
    TIMER_MAX
}
TimerSelection;

struct TimerConfig
{
    uint32_t count = 0;
    uint32_t frequency = 0;
    uint32_t prescaler = 0;
    bool autoStart = false;
    bool enableInterrupt = false;
    bool oneShotAlarm = false;
    void (*callback)(void*) = nullptr;
    void* callbackArguments = nullptr;
    TimerSelection timer = TIMER_MAX;
};

class Timer
{
    protected:
        TimerSelection timer;
        TIM_TypeDef* timerRegister;
        uint32_t resetCount = 0;
        bool alarmOn = false;
        bool oneShotAlarm = false;

        static std::array<Timer*, TIMER_MAX> drivers;

        void registerTimer(TimerSelection timer);

        void* callbackArguments;
        void (*callback)(void*);

        void initializePrescaler(uint32_t prescaler, uint32_t frequency);

        TIM_TypeDef* getTimerRegisters(TimerSelection timer);
        void enableClock(TimerSelection timer);

        void handleInterrupt();

        void forceUpdate();

    public:
        class Builder;

        explicit Timer(const TimerConfig& config);

        void start();
        void pause();
        void setAlarm(uint32_t count, bool oneShot = false);
        void resetAlarm();
        bool isRunning();
        uint32_t getCount();

        void setCount(uint32_t count);
        void setFrequency(uint32_t frequency);
        void setPrescaler(uint32_t prescaler);

        uint32_t getPrescaler();
        uint32_t getFrequency();

        void enableInterrupt();
        void disableInterrupt();

        static Timer* getDriver(TimerSelection timer);

        static uint32_t getBaseClockFrequency();

        static bool isTimerUsed(TimerSelection timer);

        // Interrupt handlers declared as friends
        friend void TIM1_UP_TIM10_IRQHandler(void);
        friend void TIM1_BRK_TIM9_IRQHandler(void);
        friend void TIM1_TRG_COM_TIM11_IRQHandler(void);
        friend void TIM1_CC_IRQHandler(void);
        friend void TIM2_IRQHandler(void);
        friend void TIM3_IRQHandler(void);
        friend void TIM4_IRQHandler(void);
};

class Timer::Builder
{
    private:
        TimerConfig config;

    public:
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

        Timer build();
};