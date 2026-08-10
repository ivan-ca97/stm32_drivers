#include "measurement_publisher.hpp"

#include <cstring>

namespace
{
    // Compiler barrier: order the seqlock phases as seen by the ISR reader.
    inline void barrier() { asm volatile("" ::: "memory"); }
}

void MeasurementPublisher::writeFloat(uint8_t offset, ReactorMeasurements::ValueType value)
{
    uint8_t bytes[measurements::FloatSize];
    std::memcpy(bytes, &value, sizeof(value));   // little-endian
    window.writeRegisters(offset, bytes, measurements::FloatSize);
}

void MeasurementPublisher::publish(const ReactorMeasurements& m)
{
    // The master reads seq -> data -> seq and retries if seq changed or is odd.
    sequence++;   // -> odd
    window.setRegister(measurements::SequenceOffset, sequence);
    barrier();

    writeFloat(measurements::ReactivityLogarithmicOffset, m.reactivityLogarithmic);
    writeFloat(measurements::ReactivityLinearOffset, m.reactivityLinear);
    writeFloat(measurements::PowerRateLogarithmicOffset, m.powerRateLogarithmic);
    writeFloat(measurements::PowerRateLinearOffset, m.powerRateLinear);
    barrier();

    sequence++;   // -> even
    window.setRegister(measurements::SequenceOffset, sequence);
}
