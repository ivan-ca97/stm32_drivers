#include "ads1115.hpp"

#include "ads1115_registers.hpp"

int32_t Ads1115::fullScaleMicroVolts() const
{
    switch(gain)
    {
        case Gain::Fsr6_144V: return 6144000;
        case Gain::Fsr4_096V: return 4096000;
        case Gain::Fsr2_048V: return 2048000;
        case Gain::Fsr1_024V: return 1024000;
        case Gain::Fsr0_512V: return 512000;
        case Gain::Fsr0_256V: return 256000;
    }
    return 2048000;
}

float Ads1115::toVolts(int16_t raw) const
{
    return float(raw) * (float(fullScaleMicroVolts()) / float(fullScaleCounts())) / 1.0e6f;
}

Adc::Counts Ads1115::readLatestCounts(uint8_t channel) const
{
    if(channel >= MaxChannels)
        return std::unexpected(adc::invalidChannel);

    return readLatestCounts(Channel(channel));
}

Adc::Counts Ads1115::readLatestCounts(Channel channel) const
{
    const uint8_t index = uint8_t(channel);

    if((validChannels & (1u << index)) == 0)
        return std::unexpected(adc::noData);

    return static_cast<int16_t>(cache[index]);   // single aligned load; atomic w.r.t. the ISR writer
}

void Ads1115::reset()
{
    // Cached counts are dropped: after a reset nothing should still be reported as a
    // current reading.
    validChannels = 0;

    if(!transactionBusy)
    {
        phase = Phase::Idle;
        return;
    }

    // A transaction is already on the bus and cannot be recalled, but its result can
    // be thrown away. Aborting keeps busy() true until it drains, so nothing is
    // dispatched on top of it.
    discardInFlight = true;
    phase = Phase::Aborting;
}

Ads1115::StartResult Ads1115::startSample(Channel channel)
{
    if(busy())
        return std::unexpected(ads1115::busy);

    currentChannel = channel;
    sampleGain = gain;   // remembered so the result can be rejected if the gain changes mid-flight
    transactionFailed = false;
    dispatchConfigWrite(singleShotConfig(channel));
    phase = Phase::StartingConversion;
    return {};
}

Ads1115::Step Ads1115::service()
{
    switch(phase)
    {
        case Phase::Idle:
            return Step::Idle;   // nothing in flight: the caller decides what to sample

        case Phase::StartingConversion:
            if(transactionBusy) return Step::Busy;
            if(transactionFailed)
            {
                phase = Phase::Idle;
                return Step::Failed;
            }
            dispatchStatusRead();
            phase = Phase::PollingStatus;
            return Step::Busy;

        case Phase::PollingStatus:
            if(transactionBusy) return Step::Busy;
            if(transactionFailed)
            {
                phase = Phase::Idle;
                return Step::Failed;
            }
            if((uint16_t(rxBytes[0]) << 8 | rxBytes[1]) & OsBit)
            {
                dispatchResultRead();
                phase = Phase::ReadingResult;
            }
            else
            {
                dispatchStatusRead();   // not ready yet -> poll again
            }
            return Step::Busy;

        case Phase::ReadingResult:
            if(transactionBusy) return Step::Busy;
            phase = Phase::Idle;
            if(transactionFailed) return Step::Failed;
            return Step::Completed;   // the callback already stored the count

        case Phase::Aborting:
            if(transactionBusy) return Step::Busy;   // wait for the bus to drain
            discardInFlight = false;
            transactionFailed = false;
            phase = Phase::Idle;
            return Step::Idle;
    }
    return Step::Idle;
}

void Ads1115::dispatchConfigWrite(const Config& config)
{
    const auto bytes = config.getBytes();
    configTxBytes[0] = bytes[0];
    configTxBytes[1] = bytes[1];
    transactionBusy = true;

    pendingTransaction = I2cTransaction::Builder()
        .setDirection(I2cTransaction::TX)
        .withData(configTxBytes, 2)
        .withRegister(RegConfig, 1)
        .withPostCallback(onStepDone, this)
        .withErrorCallback(onStepError, this)
        .build();

    device << pendingTransaction;
}

void Ads1115::dispatchStatusRead()
{
    transactionBusy = true;

    pendingTransaction = I2cTransaction::Builder()
        .setDirection(I2cTransaction::RX)
        .withData(rxBytes, 2)
        .withRegister(RegConfig, 1)
        .withPostCallback(onStepDone, this)
        .withErrorCallback(onStepError, this)
        .build();

    device << pendingTransaction;
}

void Ads1115::dispatchResultRead()
{
    transactionBusy = true;

    pendingTransaction = I2cTransaction::Builder()
        .setDirection(I2cTransaction::RX)
        .withData(rxBytes, 2)
        .withRegister(RegConversion, 1)
        .withPostCallback(onResultReady, this)
        .withErrorCallback(onStepError, this)
        .build();

    device << pendingTransaction;
}

void Ads1115::onStepDone(void* self)
{
    static_cast<Ads1115*>(self)->transactionBusy = false;
}

void Ads1115::onStepError(void* self)
{
    Ads1115* ads = static_cast<Ads1115*>(self);
    ads->transactionBusy = false;
    ads->transactionFailed = true;
}

void Ads1115::onResultReady(void* self)
{
    Ads1115* ads = static_cast<Ads1115*>(self);
    const uint8_t index = uint8_t(ads->currentChannel);

    // Publish only if this count still means something: a reset() abandoned the
    // conversion, or a setGain() changed the scale it was taken at.
    const bool usable = !ads->discardInFlight && ads->sampleGain == ads->gain;

    if(usable)
    {
        ads->cache[index] = int16_t((uint16_t(ads->rxBytes[0]) << 8) | ads->rxBytes[1]);
        ads->validChannels |= uint8_t(1u << index);
    }

    ads->transactionBusy = false;
}

// ============================================================================
// Blocking primitives — NOT part of the acquisition path above.
//
// Each of these spins on the bus until its transaction finishes, so they must
// not be called from a loop that has to stay responsive. They exist for tests
// and bring-up tools.
// ============================================================================

namespace
{
    // These callbacks just flip a bool the caller is spinning on.
    void flag(void* p) { *static_cast<volatile bool*>(p) = true; }
}

bool Ads1115::writeConfig(const Config& config)
{
    auto bytes = config.getBytes();
    transactionDone = false;
    transactionError = false;

    I2cTransaction transaction = I2cTransaction::Builder()
        .setDirection(I2cTransaction::TX)
        .withData(bytes.data(), 2)
        .withRegister(RegConfig, 1)
        .withPostCallback(flag, (void*)&transactionDone)
        .withErrorCallback(flag, (void*)&transactionError)
        .build();

    device << transaction;
    while(!transactionDone && !transactionError) {}
    return !transactionError;
}

bool Ads1115::readRegister16(uint8_t reg, uint16_t& value)
{
    uint8_t bytes[2] = { 0, 0 };
    transactionDone = false;
    transactionError = false;

    I2cTransaction transaction = I2cTransaction::Builder()
        .setDirection(I2cTransaction::RX)
        .withData(bytes, 2)
        .withRegister(reg, 1)
        .withPostCallback(flag, (void*)&transactionDone)
        .withErrorCallback(flag, (void*)&transactionError)
        .build();

    device << transaction;
    while(!transactionDone && !transactionError) {}
    if(transactionError)
        return false;

    value = (uint16_t(bytes[0]) << 8) | bytes[1];
    return true;
}

bool Ads1115::startConversion(Channel channel)
{
    return writeConfig(singleShotConfig(channel));
}

bool Ads1115::conversionReady()
{
    uint16_t config;
    if(!readRegister16(RegConfig, config))
        return false;
    return (config & OsBit) != 0;   // OS = 1 -> idle (conversion done)
}

bool Ads1115::readResult(int16_t& raw)
{
    uint16_t value;
    if(!readRegister16(RegConversion, value))
        return false;
    raw = int16_t(value);
    return true;
}

bool Ads1115::read(Channel channel, int16_t& raw)
{
    if(!startConversion(channel))
        return false;

    for(uint32_t attempt = 0; attempt < 1000; attempt++)
        if(conversionReady())
            return readResult(raw);

    return false;   // timed out waiting for the conversion
}
