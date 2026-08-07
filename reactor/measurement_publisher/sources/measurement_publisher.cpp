#include "measurement_publisher.hpp"

#include <cstring>

namespace
{
    // Compiler barrier: order the seqlock phases as seen by the ISR reader.
    inline void barrier() { asm volatile("" ::: "memory"); }
}

void MeasurementPublisher::writeFloat(uint8_t offset, float value)
{
    uint8_t bytes[measurements::FloatSize];
    std::memcpy(bytes, &value, sizeof(value));   // little-endian
    window.writeRegisters(offset, bytes, measurements::FloatSize);
}

void MeasurementPublisher::publish(const ReactorMeasurements& m)
{
    // seqlock: mark the window busy (odd), write the data, then publish the new
    // stable version (even). The master reads seq -> data -> seq and retries if
    // seq changed or is odd.
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
