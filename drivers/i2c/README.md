# C++ driver for STM32F4

This driver uses a C++ class with to use the I2C buses for the stm32f401ccu6 MCU.

# Requisites
1. Disable all `-fno-exceptions` flags
2. Change  `--specs=nano.specs` with `--specs=nosys.specs` in `gcc-arm-none-eabi.cmake`. This will make the binary larger but allows exceptions to work as expected. Otherwise, they will direct to the `_kill()` syscall
3. To compile, define `STM32_BASE_LIBRARIES` with the library containting the base STM32 dependencies in the main `CMakeLists.txt` as `CACHE INTERNAL`. If the project was created with CubeMX, it should be stm32cubemx. For example:
```cmake
set(STM32_BASE_LIBRARIES stm32cubemx CACHE INTERNAL "STM32 base dependencies")
```
4. This driver uses the full LL library. Define `USE_FULL_LL_DRIVER` and include the sources `stm32f4xx_ll_i2c.c` and `stm32f4xx_ll_rcc.c` when compiling the library.

## Interrupts
To allow the use of interrupts handlers as expected, include the source file `sources/i2c_interrupt_handlers.cpp` under `target_sources` in the main `CMakeLists.txt`, otherwise they won't be correctly linked.