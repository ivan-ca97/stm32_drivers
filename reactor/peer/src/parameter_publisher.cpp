#include <cstdint>
#include <cstring>
#include <expected>

#include "peer/parameter_publisher.h"
#include "peer/parameter_window.h"

namespace peer {

ParameterPublisher::ParameterPublisher(RegisterWindow& window)
    : window_(window)
{
}

base::Status ParameterPublisher::publish(uint16_t index, float value)
{
    if (index >= ReactorParams::Count) {
        return std::unexpected(invalidIndex);
    }

    uint8_t bytes[parameter_window::valueSize];
    std::memcpy(bytes, &value, parameter_window::valueSize);

    uint8_t offset = parameter_window::offset(index);
    window_.writeRegisters(offset, bytes, parameter_window::valueSize);

    for (uint8_t i = 0; i < parameter_window::valueSize; i++) {
        if (window_.read(offset + i) != bytes[i]) {
            return std::unexpected(verifyFailed);
        }
    }
    return base::ok;
}

} // namespace peer
