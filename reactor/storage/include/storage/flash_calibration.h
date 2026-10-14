#pragma once

#include <cstdint>

#include "base/result.h"
#include "calibration_storage.hpp"
#include "flash_log/store.h"
#include "reactor_parameters.hpp"

namespace storage {

// Persists a set of reactor parameters in a flash_log::Store, as one record under the given
// key, so a set is always read back whole. Each value is a float32 in the MCU's byte order,
// in ReactorParams' flat order.
class FlashCalibration final : public CalibrationStorage {
public:
    FlashCalibration(flash_log::Store& store, uint8_t key);

    FlashCalibration(const FlashCalibration&) = delete;
    FlashCalibration& operator=(const FlashCalibration&) = delete;

    base::Status save(const ReactorParams& parameters) override;
    base::Result<ReactorParams> load() override;

private:
    static constexpr uint32_t setSize = ReactorParams::Count * sizeof(float);

    flash_log::Store& store_;
    uint8_t key_;
};

} // namespace storage
