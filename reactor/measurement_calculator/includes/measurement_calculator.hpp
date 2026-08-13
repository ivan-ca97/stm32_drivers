#pragma once

#include <stdint.h>

#include "clock.hpp"
#include "reactor_parameters.hpp"
#include "reactor_measurements.hpp"

// MeasurementCalculator — turns the two acquisition channels into the published
// measurements, using the calibration.
//
// The computation is stateful.
//
// NOT IMPLEMENTED YET
class MeasurementCalculator
{
    public:
        MeasurementCalculator(const ReactorParams& calibration, const Clock& clock)
            : params(calibration), clock(clock) {}

        ReactorMeasurements update(int32_t logChannelMicroVolts, int32_t linChannelMicroVolts);

        // Adopt a calibration, dropping whatever was accumulated under the previous
        // one: it no longer describes what was integrated.
        void recalibrate(const ReactorParams& calibration);

    protected:
        ReactorParams params;
        const Clock& clock;

        bool started = false;
        uint32_t lastSampleMs = 0;

        void reset();
};
