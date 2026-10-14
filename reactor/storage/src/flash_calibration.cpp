#include <cstdint>
#include <cstring>
#include <expected>

#include "calibration_error.hpp"
#include "storage/flash_calibration.h"

namespace storage {

FlashCalibration::FlashCalibration(flash_log::Store& store, uint8_t key)
    : store_(store), key_(key)
{
}

base::Status FlashCalibration::save(const ReactorParams& parameters)
{
    uint8_t bytes[setSize];
    for (uint16_t index = 0; index < ReactorParams::Count; index++) {
        float value = parameters.get(index);
        std::memcpy(bytes + index * sizeof(float), &value, sizeof(float));
    }

    base::Status written = store_.write(key_, bytes);
    if (!written.has_value()) {
        return std::unexpected(written.error());
    }
    return base::ok;
}

base::Result<ReactorParams> FlashCalibration::load()
{
    uint8_t bytes[setSize];
    base::Result<uint32_t> copied = store_.read(key_, bytes);
    if (!copied.has_value()) {
        return std::unexpected(copied.error());
    }
    if (*copied == 0) {
        return std::unexpected(calibration::noneStored);
    }
    if (*copied != setSize) {
        return std::unexpected(calibration::malformed);
    }

    ReactorParams parameters;
    for (uint16_t index = 0; index < ReactorParams::Count; index++) {
        float value;
        std::memcpy(&value, bytes + index * sizeof(float), sizeof(float));
        if (!parameters.set(index, value)) {
            return std::unexpected(calibration::malformed);
        }
    }
    return parameters;
}

} // namespace storage
