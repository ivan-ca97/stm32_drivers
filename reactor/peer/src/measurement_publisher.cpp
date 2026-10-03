#include <cstdint>
#include <cstring>

#include "peer/measurement_publisher.h"
#include "peer/measurement_window.h"

namespace peer {
namespace {

// Orders the seqlock phases as the ISR reading the window sees them.
void compilerBarrier()
{
    asm volatile("" ::: "memory");
}

} // namespace

MeasurementPublisher::MeasurementPublisher(RegisterWindow& window)
    : window_(window)
{
}

void MeasurementPublisher::publish(const ReactorMeasurements& measurements)
{
    sequence_++;
    window_.setRegister(measurement_window::sequenceOffset, sequence_);
    compilerBarrier();

    writeValue(measurement_window::reactivityLogarithmicOffset, measurements.reactivityLogarithmic);
    writeValue(measurement_window::reactivityLinearOffset, measurements.reactivityLinear);
    writeValue(measurement_window::powerRateLogarithmicOffset, measurements.powerRateLogarithmic);
    writeValue(measurement_window::powerRateLinearOffset, measurements.powerRateLinear);
    compilerBarrier();

    sequence_++;
    window_.setRegister(measurement_window::sequenceOffset, sequence_);
}

void MeasurementPublisher::writeValue(uint8_t offset, ReactorMeasurements::ValueType value)
{
    uint8_t bytes[measurement_window::valueSize];
    std::memcpy(bytes, &value, measurement_window::valueSize);
    window_.writeRegisters(offset, bytes, measurement_window::valueSize);
}

} // namespace peer
