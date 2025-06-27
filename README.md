# STM32 OOP drivers
This library allows the use of C++ classes to control the STM32F401CCU6 hardware.

Each driver can have its particular requirements, but in general:
1. To compile, define `STM32_BASE_LIBRARIES` with the library containting the base STM32 dependencies in the main `CMakeLists.txt` as `CACHE INTERNAL`. If the project was created with CubeMX, it should be stm32cubemx. For example:

```cmake
set(STM32_BASE_LIBRARIES stm32cubemx CACHE INTERNAL "STM32 base dependencies")
```
2. To use exceptions:
    1. Disable all `-fno-exceptions` flags
    2. Change  `--specs=nano.specs` with `--specs=nosys.specs` in `gcc-arm-none-eabi.cmake`. This will make the binary larger but allows exceptions to work as expected. Otherwise, they will direct to the `_kill()` syscall
3. To allow the use of interrupts handlers as expected, include the source files where the interrupts are defined for each driver under `target_sources` in the main `CMakeLists.txt`, otherwise they won't be correctly linked.