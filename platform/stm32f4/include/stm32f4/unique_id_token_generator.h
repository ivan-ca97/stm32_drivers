#pragma once

#include <cstdint>

#include "session_manager.hpp"

namespace stm32f4 {

// Session tokens from the chip's 96-bit unique ID mixed with a running counter: unique per
// session, and different between chips. Never 0, which sessions take for "no token".
class UniqueIdTokenGenerator final : public TokenGenerator {
public:
    Session::TokenType next() override;

private:
    uint64_t counter_ = 0;
};

} // namespace stm32f4
