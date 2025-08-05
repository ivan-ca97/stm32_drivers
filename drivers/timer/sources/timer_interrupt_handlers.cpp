#include "timer.hpp"
#include "stm32f4xx.h"

extern "C" {
    void TIM1_UP_TIM10_IRQHandler()
    {
        auto timer = Timer::getDriver(TIMER_1);
        timer->handleInterrupt();

        // int count = TIM1->CNT;
        // if (TIM1->SR & TIM_SR_UIF)
        // {
        //     TIM1->SR &= ~TIM_SR_UIF;
        // }
    }

    void TIM1_BRK_TIM9_IRQHandler()
    {
        // TODO
        auto timer = Timer::getDriver(TIMER_9);
        timer->handleInterrupt();
    }

    void TIM1_TRG_COM_TIM11_IRQHandler()
    {
        // TODO
        auto timer = Timer::getDriver(TIMER_11);
        timer->handleInterrupt();
    }

    void TIM1_CC_IRQHandler()
    {
        // TODO
    }

    void TIM2_IRQHandler()
    {
        auto timer = Timer::getDriver(TIMER_2);
        timer->handleInterrupt();
    }

    void TIM3_IRQHandler()
    {
        auto timer = Timer::getDriver(TIMER_3);
        timer->handleInterrupt();
    }

    void TIM4_IRQHandler()
    {
        auto timer = Timer::getDriver(TIMER_4);
        timer->handleInterrupt();

    }
}