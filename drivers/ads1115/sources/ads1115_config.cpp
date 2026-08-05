#include "ads1115.hpp"

#include "ads1115_registers.hpp"

// ============================================================================
// Ads1115::Config — turning named fields into the device's config word.
//
// Every setter clears its own field before writing it, so they are idempotent
// and order-independent.
// ============================================================================

Ads1115::Config& Ads1115::Config::setChannel(Channel channel)
{
    word = uint16_t((word & ~MuxMask) | MuxSingleEnded | (uint16_t(channel) << MuxShift));
    return *this;
}

Ads1115::Config& Ads1115::Config::setGain(Gain gain)
{
    word = uint16_t((word & ~PgaMask) | (uint16_t(gain) << PgaShift));
    return *this;
}

Ads1115::Config& Ads1115::Config::setDataRate(DataRate dataRate)
{
    word = uint16_t((word & ~DataRateMask) | (uint16_t(dataRate) << DataRateShift));
    return *this;
}

Ads1115::Config& Ads1115::Config::setMode(Mode mode)
{
    word = (mode == Mode::SingleShot) ? uint16_t(word | ModeBit)
                                      : uint16_t(word & ~ModeBit);
    return *this;
}

Ads1115::Config& Ads1115::Config::setComparator(Comparator comparator)
{
    (void)comparator;   // only Disabled exists so far
    word = uint16_t((word & ~ComparatorQueueMask) | ComparatorDisabled);
    return *this;
}

Ads1115::Config& Ads1115::Config::startConversion()
{
    word |= OsBit;
    return *this;
}

std::array<uint8_t, 2> Ads1115::Config::getBytes() const
{
    return { uint8_t(word >> 8), uint8_t(word & 0xFF) };
}

Ads1115::Config Ads1115::singleShotConfig(Channel channel) const
{
    return Config()
        .setChannel(channel)
        .setGain(gain)
        .setDataRate(dataRate)
        .setMode(Config::Mode::SingleShot)
        .setComparator(Config::Comparator::Disabled)
        .startConversion();
}
