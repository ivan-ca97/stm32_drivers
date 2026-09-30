# Code Style & Naming Conventions

Single source of truth for style in this project. `CLAUDE.md` covers workflow,
architecture and build, and defers here for everything below.

C++23, exceptions and RTTI off.

---

## 1. Naming

| Element | Convention | Example |
|---|---|---|
| Files | `snake_case` | `color_math.h`, `adc_unit.cpp` |
| Classes and enums, enum values included | `PascalCase` | `AdcInput`, `Error::OutOfRange` |
| Functions, methods, attributes, variables, constants | `camelCase` | `readRaw()`, `defaultGamma` |
| Private data members | trailing underscore | `coolHpoint_` |
| Namespaces | lowercase, one per component | `analog`, `lamp` |

Identifiers and comments in English.

Names owned by STM32Cube keep their own style and must never be renamed: struct
fields such as `.Pin` or `.Alternate`, types such as `GPIO_InitTypeDef`,
headers such as `stm32f4xx_hal.h`, and the interrupt handlers the startup file
expects (`I2C1_EV_IRQHandler`). A blanket rename across the tree will break
them.

### 1.1 No type information in names

The editor already reports the type on hover, so the name does not repeat it.
No `k` on constants, no `Ptr` or `_p` on pointers, no `m_` on members. The
trailing underscore on private data stays, because that marks visibility rather
than type.

```cpp
// Do this:
using Callback = void (*)(void* context);
inline constexpr float defaultTolerance = 0.05f;

// Not this:
using CallbackPtr = void (*)(void* context);
inline constexpr float kDefaultTolerance = 0.05f;
```

### 1.2 No abbreviations

`repository` not `repo`, `request` not `req`. Accepted because they are
universally understood: `config`, `gpio`, `id`.

### 1.3 Construction

A type that can fail to be built exposes a nested `Builder` whose `build()`
validates and returns `base::Result<T>`, with the object constructed inside it.
Nothing is allocated, and the object never moves, so it can be registered by
address with a bus or an interrupt.

The constructor takes a `Permit` last, declared as
`using Permit = base::Passkey<Builder>;` right after the builder. The
constructor has to be public for `std::expected` to reach it, and the permit is
what keeps anything but the builder from calling it. The type deletes copy and
move.

```cpp
base::Result<Ads1115> Ads1115::Builder::build() const
{
    if (device_ == nullptr) {
        return std::unexpected(missingDevice);
    }
    return base::Result<Ads1115>(std::in_place, *device_, Permit{});
}
```

The object lives where the `Result` is declared: a local, a member initialized in
the constructor's list, or a `static`. Return the concrete type, never the
interface. Whoever declares it owns it, and hands the rest of the system a
reference to the interface:

```cpp
base::Result<Ads1115> adc = Ads1115::Builder()
    .device(adcDevice)
    .build();
if (!adc.has_value()) {
    return std::unexpected(adc.error());
}
adc::Input& input = *adc;
```

No `std::unique_ptr` or any other heap ownership.

---

## 2. Include ordering

Three groups, separated by blank lines, alphabetical within each:

```cpp
#include <cstdint>
#include <memory>

#include "stm32f4xx_hal.h"
#include "stm32f4xx_ll_i2c.h"

#include "adc/input.h"
#include "base/result.h"
```

1. Standard library
2. STM32Cube (HAL, LL, CMSIS)
3. This project

---

## 3. Comments

Keep them to a minimum. If a comment seems necessary, first check whether the
code can be made clear enough on its own.

**Do not document design decisions.** Those belong in the chat or in a separate
document. A comment documents the code, not the reasoning that led to it.

What earns a comment: a contract the signature cannot express, units, a
non-obvious dependency between two API calls, or where a magic number came from.

```cpp
// Reads after a program return stale data until the data cache is flushed.
__HAL_FLASH_DATA_CACHE_RESET();

float tolerance = defaultTolerance; // fraction of full scale, 0.0 - 1.0
```

**A comment describes its own file, and nothing else.** Explaining a
neighbouring mechanism duplicates whatever documents it and goes stale on its
own. A register map says where each value sits and how it is encoded; the
protocol that publishes those values is explained where it is implemented.

**Name the role, not the specific counterpart.** Write "the master", not "the
ESP32", unless the code genuinely depends on that particular device.

**No `====` or `----` rules in code comments.** Editors render doc comments as
Markdown on hover, and a line of `=` under text turns it into a heading. Every
rule character has some Markdown meaning, so there is no safe one. Separate
blocks with a blank line.

---

## 4. Errors

Anything that can fail returns `base::Result<T>`, or `base::Status` when there
is no value to carry. Never an out parameter plus a status code, and never a
bare status code the caller may ignore.

Return `base::ok` on success rather than `{}`, which forces the reader back to
the signature to find out what it means.

An error carries its own domain, so a failure crossing a boundary keeps its
original cause instead of being replaced by a local one.

```cpp
// Do this:
base::Result<uint16_t> readRaw();

// Not this:
HAL_StatusTypeDef readRaw(uint16_t* out);
```

Each component defines its errors in its own `error.h`: a `Code` enum, the
`Category` that describes it, one `errorCategory` instance, and one
`base::Error` constant per code. Call sites use the constant, never the code:

```cpp
return std::unexpected(adc::noData);
```

Decoding a code back to the enum is for a boundary that has to translate it,
such as a wire protocol, and it checks `belongsTo(errorCategory)` first.

---

## 5. Separate the action from the error check

Makes "do something, then verify" explicit.

```cpp
// Do this:
base::Result<uint16_t> raw = readRaw();
if (!raw.has_value()) {
    return std::unexpected(raw.error());
}

// Not this:
if (auto raw = readRaw(); !raw.has_value()) {
    return std::unexpected(raw.error());
}
```

---

## 6. Explicit returns

Do not return another fallible call directly. Assign, check, then return, so the
flow stays visible.

```cpp
// Do this:
base::Result<CctLevels> levels = renderCct(state, config);
if (!levels.has_value()) {
    return std::unexpected(levels.error());
}
return *levels;

// Not this:
return renderCct(state, config);
```

**Exceptions**:

- A pure mapping that cannot fail may return inline.
- A `build()` returning a type that cannot move constructs the `Result` in the
  `return` itself (§1.3). A named local would need a move to leave the function,
  and that does not compile.

---

## 7. No trailing error after a multi-line literal

The error value gets buried and is easy to miss. Assign first.

```cpp
// Do this:
CctLevels levels = {
    .warm = linear * (1.0f - effectiveCct),
    .cool = linear * effectiveCct,
};
return levels;

// Not this:
return CctLevels{
    .warm = linear * (1.0f - effectiveCct),
    .cool = linear * effectiveCct,
};
```

---

## 8. Consistent struct initialization

Always multi-line, even with a single field, and designated initializers in
declaration order.

```cpp
// Do this:
State state = {
    .on = true,
};

// Not this:
State state = {.on = true};
```

---

## 9. No column alignment

A single space separates a type from its name, a field from its value, and code
from a trailing comment. Do not pad to line things up. The padding has to be
redone whenever the longest name in the group changes, which turns a one-line
edit into a multi-line diff.

```cpp
// Do this:
const ErrorCategory* category_;
int code_;

GPIO_InitTypeDef pinConfig = {
    .Pin = pin,
    .Mode = GPIO_MODE_AF_OD,
};

float tolerance = defaultTolerance; // fraction of full scale, 0.0 - 1.0

// Not this:
const ErrorCategory* category_;
int                  code_;

GPIO_InitTypeDef pinConfig = {
    .Pin  = pin,
    .Mode = GPIO_MODE_AF_OD,
};

float tolerance = defaultTolerance;   // fraction of full scale, 0.0 - 1.0
```

---

## 10. Pointers and references bind to the type

The asterisk and the ampersand go with the type, not with the name. The type of
`describe` below is `const char*`, and grouping the type information in one
place says so.

```cpp
// Do this:
const char* describe(int code) const;
base::Result<void> render(const State& state, const RenderConfig& config);
const ErrorCategory* category_;

// Not this:
const char *describe(int code) const;
base::Result<void> render(const State &state, const RenderConfig &config);
const ErrorCategory *category_;
```

This only reads honestly while a declaration declares one name. `char* a, b;`
makes `b` a `char`, not a pointer, because the asterisk binds to the declarator
and not to the specifier. **Declare one name per declaration** and the trap
never comes up.

---

## 11. Wrapping

Lines stop at 100 columns.

When a parameter or argument list does not fit, put every one of them on its own
line and close the parenthesis on a line of its own, aligned with the statement.
Do not align the continuation to the opening parenthesis: that alignment has to
be redone whenever the name before it changes, so renaming a function becomes a
diff across every line of its signature.

```cpp
// Do this:
virtual base::Result<void> render(
    const State& state,
    const RenderConfig& config
) = 0;

// Not this:
virtual base::Result<void> render(const State& state,
                                  const RenderConfig& config) = 0;
```

Break all of them or none. A list split with two arguments on the first line and
one below hides how many there are.

---

## 12. One builder call per line

A chain that sets anything puts every call on its own line, with the dot
leading. A line that starts with a dot reads as a continuation on sight, and
adding, removing or reordering a step touches only that line.

```cpp
// Do this:
base::Result<Ads1115> adc = Ads1115::Builder()
    .device(adcDevice)
    .gain(Ads1115::Gain::Fsr4_096V)
    .build();

// Not this:
base::Result<Ads1115> adc =
    Ads1115::Builder().device(adcDevice).gain(Ads1115::Gain::Fsr4_096V).build();
```

A chain that sets nothing has nothing to scan, so it stays on one line:

```cpp
base::Result<Watchdog> watchdog = Watchdog::Builder().build();
```

---

## 13. No inline struct literal arguments

Assign to a variable first, then pass it. Array literals are exempt.

```cpp
// Do this:
GPIO_InitTypeDef pinConfig = {
    .Pin = pin,
    .Mode = GPIO_MODE_OUTPUT_PP,
};
HAL_GPIO_Init(port, &pinConfig);

// Not this:
HAL_GPIO_Init(port, &(GPIO_InitTypeDef){...});
```

---

## 14. Type definition order

Nested `Builder` and its `Permit`, the constructor and the deleted copy and move,
destructor, public methods, then private helpers and data. This establishes a
consistent reading order: how to create it, what it does, then how it is made.

A type without a builder keeps its constructor at the top of the public
section.

---

## 15. A constant belongs to what it configures

A constant only its own class reads is a member of that class, not a name in
the namespace. Namespace scope claims "anyone here may need this", and a
builder default nobody outside the header names is not that.

```cpp
// Do this:
class Output {
public:
    class Builder {
    public:
        static constexpr uint32_t defaultFrequencyHz = 3000;
```

```cpp
// Not this:
inline constexpr uint32_t defaultFrequencyHz = 3000;

class Output { ... };
```

Public, so a caller reading the header still learns what happens when the
setter goes uncalled. What changes is attribution, not visibility.

The predicate that judges a default belongs to the same class, as a private
`isValid` declared in the header and defined in the `.cpp`. Private, because
the builder is a nested class and reaches it anyway, and because a `constexpr`
defined in one translation unit cannot be called from another.

That places the `static_assert` checking the default inside `build()`, which
is a member and so has the access a namespace scope assertion would not:

```cpp
base::Result<Blink> Blink::Builder::build() const
{
    static_assert(
        Blink::isValid(defaultPeriodMs, defaultDuty),
        "the default blink has to pass this very check"
    );
```

It still fires at compile time, and it sits where the default is read.
Nothing about a default belongs at namespace scope.
