#include "measurement_calculator.hpp"

namespace
{
    ReactorMeasurements::ValueType toVolts(int32_t microVolts)
    {
        return static_cast<ReactorMeasurements::ValueType>(microVolts) / 1.0e6f;
    }
}

void MeasurementCalculator::reset()
{
    started = false;
}

void MeasurementCalculator::recalibrate(const ReactorParams& calibration)
{
    params = calibration;
    reset();
}

ReactorMeasurements MeasurementCalculator::update(int32_t logChannelMicroVolts,
                                                  int32_t linChannelMicroVolts)
{
    started = true;
    lastSampleMs = clock.milliseconds();

    // Placeholder.
    ReactorMeasurements measurements;
    measurements.reactivityLogarithmic = toVolts(logChannelMicroVolts);
    measurements.reactivityLinear = toVolts(linChannelMicroVolts);
    measurements.powerRateLogarithmic = toVolts(logChannelMicroVolts);
    measurements.powerRateLinear = toVolts(linChannelMicroVolts);
    return measurements;
}
