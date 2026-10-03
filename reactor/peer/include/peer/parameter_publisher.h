#pragma once

#include <cstdint>

#include "base/result.h"
#include "peer/error.h"
#include "register_window.hpp"

namespace peer {

// Writes this board's calibration into its parameter window, one value at a time.
class ParameterPublisher {
public:
    explicit ParameterPublisher(RegisterWindow& window);

    ParameterPublisher(const ParameterPublisher&) = delete;
    ParameterPublisher& operator=(const ParameterPublisher&) = delete;

    // Writes the value at its flat index and reads it back. Fails with
    // invalidIndex or verifyFailed; the register is written either way.
    base::Status publish(uint16_t index, float value);

private:
    RegisterWindow& window_;
};

} // namespace peer
