#pragma once

#include <cstdint>

#include "base/result.h"
#include "i2c_device.hpp"
#include "i2c_transaction.hpp"
#include "peer/error.h"
#include "peer/measurement_window.h"
#include "peer/step.h"
#include "reactor_measurements.hpp"

namespace peer {

// Reads the peer's measurement window, retrying while the seqlock shows a write in
// progress. start() begins one read and service() advances it, without blocking.
class MeasurementReader {
public:
    MeasurementReader(I2cDevice& device, uint8_t windowBase);

    // The bus holds the address of the transaction inside.
    MeasurementReader(const MeasurementReader&) = delete;
    MeasurementReader& operator=(const MeasurementReader&) = delete;

    // Fails with busy while a read is under way.
    base::Status start();

    bool busy() const;

    Step service();

    // Gives up on the read. busy() holds until the bus lets the transaction go.
    void abandon();

    // The last snapshot read whole, or noData until one has been.
    base::Result<ReactorMeasurements> latest() const;

private:
    enum class Phase : uint8_t {
        Idle,
        ReadingFirstSequence,
        ReadingValues,
        ReadingSecondSequence,
    };

    // Torn reads tolerated before the read is reported as failed.
    static constexpr uint8_t maxRetries = 5;

    static constexpr uint8_t valuesBase = measurement_window::reactivityLogarithmicOffset;
    static constexpr uint8_t valuesSize = measurement_window::valuesLength - valuesBase;

    Step finish(Step step);
    void dispatchRead(uint8_t offset, uint8_t* buffer, uint16_t length);
    void decode();
    void readValue(uint8_t offset, ReactorMeasurements::ValueType& value) const;

    static void onStepDone(void* self);
    static void onStepError(void* self);

    I2cDevice& device_;
    uint8_t windowBase_;

    Phase phase_ = Phase::Idle;
    uint8_t retries_ = 0;
    uint8_t firstSequence_ = 0;
    ReactorMeasurements snapshot_ = {};
    bool snapshotValid_ = false;

    volatile bool transactionBusy_ = false;
    volatile bool transactionFailed_ = false;

    I2cTransaction transaction_;
    uint8_t sequenceByte_ = 0;
    uint8_t valueBytes_[valuesSize] = {};
};

} // namespace peer
