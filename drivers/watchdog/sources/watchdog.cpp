#include "watchdog.hpp"

#include <expected>

#include "stm32f4xx.h"

namespace
{
    // Key register commands (RM0368 §21.4.1).
    constexpr uint16_t KeyReload      = 0xAAAA;   // reload the counter
    constexpr uint16_t KeyEnable      = 0xCCCC;   // start the watchdog
    constexpr uint16_t KeyUnlockWrite = 0x5555;   // allow writes to PR / RLR

    constexpr uint32_t LsiHz = 32000;   // nominal; the real LSI varies

    // The prescaler is fixed at the coarsest setting (/256, PR = 6). Its 12-bit
    // reload then spans 8 ms to ~32 s, which covers every sensible watchdog period,
    // so there is nothing to choose at runtime. A finer prescaler would only reach
    // below 8 ms — useless here — and the granularity it would buy is far smaller
    // than the LSI's own spread.
    constexpr uint8_t  PrescalerExponent = 6;
    constexpr uint32_t Divider = 4u << PrescalerExponent;

    bool timeoutInRange(uint32_t timeoutMs)
    {
        return timeoutMs >= Watchdog::MinTimeoutMs && timeoutMs <= Watchdog::MaxTimeoutMs;
    }
}

base::Status Watchdog::start(uint32_t timeoutMs)
{
    if(!timeoutInRange(timeoutMs))
        return std::unexpected(watchdog::timeoutOutOfRange);

    // Starting first is what turns the LSI on, and the wait in configure() needs it
    // running. Unlocking before starting would leave that wait spinning forever.
    IWDG->KR = KeyEnable;
    running = true;
    configure(timeoutMs);
    return base::ok;
}

base::Status Watchdog::setTimeout(uint32_t timeoutMs)
{
    if(!running)
        return std::unexpected(watchdog::notRunning);
    if(!timeoutInRange(timeoutMs))
        return std::unexpected(watchdog::timeoutOutOfRange);

    configure(timeoutMs);
    return base::ok;
}

void Watchdog::configure(uint32_t timeoutMs)
{
    const uint32_t reload = (timeoutMs * LsiHz) / (Divider * 1000u);

    IWDG->KR  = KeyUnlockWrite;

    IWDG->PR  = PrescalerExponent;
    IWDG->RLR = static_cast<uint16_t>(reload);

    // PR/RLR take a few LSI cycles to latch; writing the reload key before they are
    // accepted would be ignored.
    while(IWDG->SR != 0) {}

    IWDG->KR = KeyReload;
}

void Watchdog::kick()
{
    IWDG->KR = KeyReload;
}

bool Watchdog::resetWasCausedByWatchdog()
{
    const bool caused = (RCC->CSR & RCC_CSR_IWDGRSTF) != 0;
    if(caused)
        RCC->CSR |= RCC_CSR_RMVF;   // clear the reset-cause flags
    return caused;
}
