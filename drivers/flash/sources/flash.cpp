#include "flash.hpp"

#include "stm32f4xx.h"

#include <cstring>

namespace
{
    constexpr uint32_t KEY1 = 0x45670123;
    constexpr uint32_t KEY2 = 0xCDEF89AB;

    constexpr uint32_t ERROR_FLAGS =
        FLASH_SR_WRPERR | FLASH_SR_PGAERR | FLASH_SR_PGPERR | FLASH_SR_PGSERR;
}

void Flash::waitWhileBusy()
{
    while(FLASH->SR & FLASH_SR_BSY) {}
}

void Flash::unlock()
{
    if(FLASH->CR & FLASH_CR_LOCK)
    {
        FLASH->KEYR = KEY1;
        FLASH->KEYR = KEY2;
    }
}

void Flash::lock()
{
    FLASH->CR |= FLASH_CR_LOCK;
}

bool Flash::checkAndClearErrors()
{
    bool ok = (FLASH->SR & ERROR_FLAGS) == 0;
    FLASH->SR = ERROR_FLAGS | FLASH_SR_EOP;   // write-1-to-clear
    return ok;
}

void Flash::flushDataCache()
{
    // Reset the data cache so later reads observe the new flash contents.
    FLASH->ACR &= ~FLASH_ACR_DCEN;
    FLASH->ACR |=  FLASH_ACR_DCRST;
    FLASH->ACR &= ~FLASH_ACR_DCRST;
    FLASH->ACR |=  FLASH_ACR_DCEN;
}

bool Flash::eraseSector(uint8_t sector)
{
    waitWhileBusy();
    unlock();
    checkAndClearErrors();

    FLASH->CR &= ~(FLASH_CR_PSIZE | FLASH_CR_SNB);            // PSIZE = x8, clear sector
    FLASH->CR |= FLASH_CR_SER | (uint32_t(sector) << FLASH_CR_SNB_Pos);
    FLASH->CR |= FLASH_CR_STRT;
    waitWhileBusy();
    FLASH->CR &= ~FLASH_CR_SER;

    bool ok = checkAndClearErrors();
    flushDataCache();
    lock();
    return ok;
}

bool Flash::write(uint32_t address, const uint8_t* data, uint32_t length)
{
    waitWhileBusy();
    unlock();
    checkAndClearErrors();

    FLASH->CR &= ~FLASH_CR_PSIZE;                            // PSIZE = x8 (byte program)

    bool ok = true;
    for(uint32_t i = 0; i < length; i++)
    {
        FLASH->CR |= FLASH_CR_PG;
        *reinterpret_cast<volatile uint8_t*>(address + i) = data[i];
        waitWhileBusy();
        FLASH->CR &= ~FLASH_CR_PG;

        if(!checkAndClearErrors())
        {
            ok = false;
            break;
        }
    }

    flushDataCache();
    lock();

    if(ok)
        ok = std::memcmp(reinterpret_cast<const void*>(address), data, length) == 0;
    return ok;
}

void Flash::read(uint32_t address, uint8_t* out, uint32_t length) const
{
    std::memcpy(out, reinterpret_cast<const void*>(address), length);
}
