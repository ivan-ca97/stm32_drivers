#pragma once

#include <stdint.h>

#include "i2c_device.hpp"
#include "i2c_transaction.hpp"
#include "result.hpp"
#include "reactor_measurements.hpp"
#include "measurement_map.hpp"

static_assert(sizeof(ReactorMeasurements::ValueType) == measurements::FloatSize,
              "measurement value type does not match the size on the wire");

// PeerMeasurementReader — reads a peer's measurement window over I2C with the
// seqlock retry (SEQ -> data -> SEQ).
class PeerMeasurementReader
{
    public:
        enum class Step : uint8_t
        {
            Idle,
            Busy,   // read in progress
            Completed,
            Failed,
        };

        enum class StartError : uint8_t { Busy };
        using StartResult = lib::Result<void, StartError>;

        enum class SnapshotError : uint8_t { NoData };   // no read has completed yet
        using SnapshotResult = lib::Result<ReactorMeasurements, SnapshotError>;

        PeerMeasurementReader(I2cDevice& peer, uint8_t windowBase)
            : peer(peer), windowBase(windowBase) {}

        StartResult startRead();

        bool busy() const { return phase != Phase::Idle; }

        // Non-blocking step.
        Step service();

        // The last snapshot read whole, or NoData until one has been: a peer that has
        // never answered must not read as a peer measuring zero.
        SnapshotResult latest() const
        {
            if(!snapshotValid)
                return lib::Error(SnapshotError::NoData);
            return snapshot;
        }

    protected:
        // Stage of the seqlock read driven by service().
        enum class Phase : uint8_t { Idle, ReadingSeq1, ReadingData, ReadingSeq2 };

        // One read covers every value field; the sequence byte is read on its own.
        static constexpr uint8_t DataBlockBase = measurements::ReactivityLogarithmicOffset;
        static constexpr uint8_t DataBlockSize = measurements::MeasurementsLength - DataBlockBase;

        static constexpr uint8_t MaxRetries = 5;   // retries on a torn (mid-update) read

        I2cDevice& peer;
        uint8_t windowBase;

        Phase phase = Phase::Idle;
        uint8_t retries = 0;
        uint8_t seq1 = 0;   // sequence seen before the data read, compared against the one after
        ReactorMeasurements snapshot{};
        bool snapshotValid = false;

        volatile bool transactionBusy = false;
        volatile bool transactionFailed = false;

        I2cTransaction pendingTransaction;
        uint8_t seqByte = 0;
        uint8_t dataBytes[DataBlockSize] = {};

        void dispatchSeqRead();
        void dispatchDataRead();
        void readFloat(uint8_t offset, ReactorMeasurements::ValueType& value) const;
        void decode();

        static void onStepDone(void* self);
        static void onStepError(void* self);
};
