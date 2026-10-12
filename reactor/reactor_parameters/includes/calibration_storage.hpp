#pragma once

#include "base/result.h"
#include "calibration_error.hpp"
#include "reactor_parameters.hpp"

// CalibrationStorage — where the approved calibration persists, as one whole set, so what
// is stored is always a complete calibration.
class CalibrationStorage
{
    public:
        virtual ~CalibrationStorage() = default;

        // Replace the stored set. Fails with the backend's own error.
        virtual base::Status save(const ReactorParams& parameters) = 0;

        // The stored set. Fails with calibration::noneStored, calibration::malformed, or the
        // backend's own error.
        virtual base::Result<ReactorParams> load() = 0;
};
