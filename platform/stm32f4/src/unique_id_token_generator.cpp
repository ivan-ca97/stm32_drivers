#include <cstdint>

#include "stm32f4/unique_id_token_generator.h"

namespace stm32f4 {
namespace {

// Factory-programmed 96-bit unique ID, read-only (RM0368, device electronic signature).
constexpr uint32_t uniqueIdAddress = 0x1FFF7A10;

} // namespace

Session::TokenType UniqueIdTokenGenerator::next()
{
    const volatile uint32_t* id = reinterpret_cast<const volatile uint32_t*>(uniqueIdAddress);
    uint64_t base = (uint64_t(id[0]) << 32) ^ (uint64_t(id[1]) << 16) ^ uint64_t(id[2]);

    counter_++;
    uint64_t token = base + counter_;
    if (token == 0) {
        return 1;
    }
    return token;
}

} // namespace stm32f4
