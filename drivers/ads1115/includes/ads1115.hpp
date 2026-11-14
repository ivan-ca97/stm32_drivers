#pragma once

#include <stdint.h>
#include <array>

#include "i2c_device.hpp"
#include "i2c_transaction.hpp"
#include "adc.hpp"
#include "ads1115_error.hpp"
#include "base/result.h"
#include "clock.hpp"

// Ads1115 — 16-bit I2C ADC (4 single-ended channels), driven single-shot over an
// I2cDevice.
//
// Non-blocking acquisition is caller-driven: startSample() begins one conversion on
// one channel, and service() advances it. With a scan set, service() also starts the
// next scanned channel whenever it finds the device idle.
//
// Timing: a conversion is expected to take the datasheet's time for the data rate.
// Each conversion adjusts that wait to what the device takes:
// Found ready at the first status read, the wait shrinks by a millisecond.
// Found still converting, it becomes the time the device needed.
// A conversion still not done at ten times the datasheet's makes the driver fail with
// ads1115::conversionTimeout until reset() starts it again.
//
// Each completed conversion lands in a per-channel cache that readLatestCounts()
// serves without touching the bus. The cache is written from the I2C completion
// callback (ISR); a single aligned int16_t store/load is atomic here, so no lock is
// needed. Until a channel has been converted at the gain currently in effect it
// reports NoData.
class Ads1115 : public Adc
{
    public:
        // PGA full-scale range (also sets volts-per-count).
        enum class Gain : uint8_t
        {
            Fsr6_144V,
            Fsr4_096V,
            Fsr2_048V,  // reset default
            Fsr1_024V,
            Fsr0_512V,
            Fsr0_256V,
        };

        // Samples per second.
        enum class DataRate : uint8_t
        {
            Sps8,
            Sps16,
            Sps32,
            Sps64,
            Sps128, // reset default
            Sps250,
            Sps475,
            Sps860,
        };

        // Outcome of one service() call.
        enum class Step : uint8_t
        {
            Idle,
            Busy,       // conversion in progress
            Completed,
            Failed,
        };

        // Single-ended inputs. Typed so a channel argument cannot be out of range.
        enum class Channel : uint8_t { Ain0, Ain1, Ain2, Ain3 };

        // Fails with ads1115::busy or ads1115::conversionTimeout: a Channel is always valid.
        using StartResult = base::Status;

        static constexpr uint8_t MaxChannels = 4;

        Ads1115(I2cDevice& device, const Clock& clock) : device(device), clock(clock) {}

        // Changing the gain changes what a count means, so every cached reading
        // stops being interpretable and is dropped. The data rate does not affect
        // scaling, so it leaves the cache alone.
        void setGain(Gain g)
        {
            gain = g;
            validChannels = 0;
        }

        // Restarts the conversion wait from the datasheet's time for the new rate.
        void setDataRate(DataRate r)
        {
            dataRate = r;
            conversionWaitMs = 0;
        }

        void addToScan(Channel channel);
        void removeFromScan(Channel channel);

        // --- ADC ---
        void reset() override;
        Counts readLatestCounts(uint8_t channel) const override;
        int32_t fullScaleCounts() const override { return 32768; }
        int32_t fullScaleMicroVolts() const override;

        // --- Acquisition ---

        // Typed read, always in range. The Adc override above takes a raw index
        // because the interface cannot know how many channels a device has.
        Counts readLatestCounts(Channel channel) const;

        Input input(Channel channel) const;

        // Begin a single conversion on `channel`.
        StartResult startSample(Channel channel);

        // True while a conversion is in flight.
        bool busy() const { return phase != Phase::Idle && phase != Phase::Stopped; }

        // Advance the in-flight conversion by one non-blocking step.
        Step service();

        // --- Direct primitives (blocking / manually-polled) ---
        bool startConversion(Channel channel);  // single-shot
        bool conversionReady();                 // reads the config OS bit
        bool readResult(int16_t& raw);
        bool read(Channel channel, int16_t& raw);   // start -> spin -> read

        float toVolts(int16_t raw) const;

    protected:
        // --------------------------------------------------------------------
        // Config — the device's configuration register.
        // --------------------------------------------------------------------
        class Config
        {
            public:
                enum class Mode : uint8_t { Continuous, SingleShot };

                // Only the disabled case is used so far; the ALERT/RDY modes go here.
                enum class Comparator : uint8_t { Disabled };

                Config& setChannel(Channel channel);
                Config& setGain(Gain gain);
                Config& setDataRate(DataRate dataRate);
                Config& setMode(Mode mode);
                Config& setComparator(Comparator comparator);

                // Ask the device to begin converting as soon as this is written.
                Config& startConversion();

                std::array<uint8_t, 2> getBytes() const;

            private:
                uint16_t word = 0;
        };

        // Stage of the single-conversion state machine driven by service().
        enum class Phase : uint8_t
        {
            Idle,
            StartingConversion,
            WaitingConversion,
            CheckingConversion,     // status read once the conversion should be done
            RecheckingConversion,   // read again: the device was still converting
            ReadingResult,
            Aborting,
            Stopped,                // a conversion timed out
        };

        I2cDevice& device;
        const Clock& clock;
        Gain gain = Gain::Fsr2_048V;
        DataRate dataRate = DataRate::Sps128;

        Phase phase = Phase::Idle;
        Channel currentChannel = Channel::Ain0;   // channel of the conversion in flight
        Gain sampleGain = Gain::Fsr2_048V;   // gain that conversion was started with
        uint32_t conversionWaitMs = 0;
        uint32_t conversionStartedAt = 0;

        volatile bool transactionDone = false;   // blocking primitives only
        volatile bool transactionError = false;

        volatile bool transactionBusy = false;   // service() path only
        volatile bool transactionFailed = false;
        volatile bool discardInFlight = false;   // drop the result the ISR is about to receive

        I2cTransaction pendingTransaction;   // member: the bus queues its address, must stay stable
        uint8_t configTxBytes[2] = {};
        uint8_t rxBytes[2] = {};

        volatile int16_t cache[MaxChannels] = {};   // per channel; see class comment
        volatile uint8_t validChannels = 0;   // bit per channel: cache[i] holds a real reading
        uint8_t scanChannels = 0;   // bit per channel: sampled in turn by service()

        Config singleShotConfig(Channel channel) const;
        uint32_t conversionTimeMs() const;
        uint32_t conversionTimeoutMs() const;
        bool conversionDone() const;
        Channel nextScanChannel() const;
        bool writeConfig(const Config& config);             // blocking
        bool readRegister16(uint8_t reg, uint16_t& value);  // blocking

        void dispatchConfigWrite(const Config& config);
        void dispatchStatusRead();
        void dispatchResultRead();

        static void onStepDone(void* self);
        static void onStepError(void* self);
        static void onResultReady(void* self);
};
