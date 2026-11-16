#include "adc.hpp"

Adc::Reading Adc::readLatestMicroVolts(uint8_t channel) const
{
    auto counts = readLatestCounts(channel);
    if(!counts)
        return std::unexpected(counts.error());

    const int32_t microVolts = static_cast<int32_t>(
        static_cast<int64_t>(counts->value) * fullScaleMicroVolts() / fullScaleCounts());
    return Sample{
        microVolts,
        counts->startedAtMs
    };
}

Adc::Reading Adc::Input::readLatestMicroVolts() const
{
    return adc->readLatestMicroVolts(channel);
}
