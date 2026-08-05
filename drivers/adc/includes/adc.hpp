#pragma once

#include <stdint.h>

#include "result.hpp"

// ============================================================================
// Adc — multi-channel analog-to-digital converter interface.
//
// Whoever constructs the concrete device is responsible for any upkeep it
// needs (e.g. calling a polling implementation).
//
// Per-device configuration (gain, sample rate, which channels to scan, ...) is
// also not part of this interface — set up on the concrete device by whoever
// constructs it.
// ============================================================================
class Adc
{
    public:
        enum class Error
        {
            InvalidChannel,
            NoData,   // never sampled, or the last sample no longer means anything
        };

        using Counts     = lib::Result<int16_t, Error>;
        using MicroVolts = lib::Result<int32_t, Error>;

        virtual ~Adc() = default;

        // Reinitialize to a known state: any acquisition in progress is abandoned,
        // and no channel reports a reading until it has been sampled again.
        virtual void reset() = 0;

        // Most recent count for `channel`. Never blocks. Fails with InvalidChannel if
        // the index does not exist on this device, or with NoData if there is no
        // reading to report — a device must not pass off a default as a measurement.
        virtual Counts readLatestCounts(uint8_t channel) const = 0;

        // Digital full-scale magnitude: counts range is [-fullScaleCounts, fullScaleCounts-1].
        virtual int32_t fullScaleCounts() const = 0;

        // Physical full-scale magnitude in microvolts, matching fullScaleCounts().
        virtual int32_t fullScaleMicroVolts() const = 0;

        // Reading in microvolts, derived from the above. Integer math only.
        // Truncates toward zero.
        MicroVolts readLatestMicroVolts(uint8_t channel) const
        {
            auto counts = readLatestCounts(channel);
            if(!counts)
                return lib::Error(counts.error());

            return static_cast<int32_t>(
                static_cast<int64_t>(*counts) * fullScaleMicroVolts() / fullScaleCounts());
        }
};
