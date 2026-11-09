#include "adc.hpp"

Adc::MicroVolts Adc::readLatestMicroVolts(uint8_t channel) const
{
    auto counts = readLatestCounts(channel);
    if(!counts)
        return std::unexpected(counts.error());

    return static_cast<int32_t>(
        static_cast<int64_t>(*counts) * fullScaleMicroVolts() / fullScaleCounts());
}

Adc::MicroVolts Adc::Input::readLatestMicroVolts() const
{
    return adc->readLatestMicroVolts(channel);
}
