# C++ driver for STM32F4

This driver uses a C++ class with builder pattern to use the hardware timers.

To compile, define `STM32_BASE_LIBRARIES` with the library containting the base STM32 dependencies in the main `CMakeLists.txt` as `CACHE INTERNAL`. If the project was created with CubeMX, it should be stm32cubemx. For example:

```cmake
set(STM32_BASE_LIBRARIES stm32cubemx CACHE INTERNAL "STM32 base dependencies")
```

## Interrupts
To allow the use of interrupts handlers as expected, include the source file `sources/timer_interrupt_handlers.cpp` under `target_sources` in the main `CMakeLists.txt`, otherwise they won't be correctly linked.