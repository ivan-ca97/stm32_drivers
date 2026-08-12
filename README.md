# STM32 OOP drivers
C++ classes to control the STM32F401CCU6 hardware, plus reusable components built on top
of them.

| Path | Contents |
|---|---|
| `drivers/` | Hardware drivers: `i2c`, `timer`, `flash`, `gpio`, `watchdog`, `ads1115`, and the generic `adc` interface |
| `lib/` | Building blocks with no hardware dependency: `queue`, `set`, `result`, `custom_exception` |
| `persistence/` | `PersistentStorage` interface + `FlashStore` (flash-backed, multi-region) |
| `controller/` | Register + request/response slave protocol over I2C (see its `DESIGN.md`) |
| `authentication/` | Users, sessions and permissions, with an optional wire-protocol handler |
| `reactor/` | **Application layer** (reactor-specific) — not a generic component |

Everything except `reactor/` is meant to stay agnostic to the surrounding system.

Each driver can have its particular requirements, but in general:
1. To compile, define `STM32_BASE_LIBRARIES` with the library containting the base STM32 dependencies in the main `CMakeLists.txt` as `CACHE INTERNAL`. If the project was created with CubeMX, it should be stm32cubemx. For example:

```cmake
set(STM32_BASE_LIBRARIES stm32cubemx CACHE INTERNAL "STM32 base dependencies")
```
2. To use exceptions (currently required by `i2c` and `timer` — see
   `docs/exceptions-removal.md` for the plan to drop them):
    1. Disable all `-fno-exceptions` flags
    2. Change  `--specs=nano.specs` with `--specs=nosys.specs` in `gcc-arm-none-eabi.cmake`. This will make the binary larger but allows exceptions to work as expected. Otherwise, they will direct to the `_kill()` syscall
3. To allow the use of interrupts handlers as expected, include the source files where the interrupts are defined for each driver under `target_sources` in the main `CMakeLists.txt`, otherwise they won't be correctly linked.

## Specific requirements
### I2C
This driver uses the full LL library. Define `USE_FULL_LL_DRIVER` (for example adding `add_compile_definitions(USE_FULL_LL_DRIVER)` in the `CMakeFile.txt`) and include the sources `stm32f4xx_ll_i2c.c` and `stm32f4xx_ll_rcc.c` when compiling the library.