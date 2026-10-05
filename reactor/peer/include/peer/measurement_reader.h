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
//
// The sequence, the values and the sequence again are queued together, so the bus
// reads them back to back: the window between the two sequence reads is the bus
// time, not the caller's service() period.
class MeasurementReader {
public:
    MeasurementReader(I2cDevice& device, uint8_t windowBase);

    // The bus holds the addresses of the transactions inside.
    MeasurementReader(const MeasurementReader&) = delete;
    MeasurementReader& operator=(const MeasurementReader&) = delete;

    // Fails with busy while a read is under way.
    base::Status start();

    bool busy() const;

    Step service();

    // Gives up on the read. busy() holds until the bus lets the transactions go.
    void abandon();

    // The last snapshot read whole, or noData until one has been.
    base::Result<ReactorMeasurements> latest() const;

private:
    // Torn reads tolerated before the read is reported as failed.
    static constexpr uint8_t maxRetries = 5;

    static constexpr uint8_t valuesBase = measurement_window::reactivityLogarithmicOffset;
    static constexpr uint8_t valuesSize = measurement_window::valuesLength - valuesBase;

    void dispatch();
    I2cTransaction read(uint8_t offset, uint8_t* buffer, uint16_t length) const;
    bool released();
    bool anyIn(I2cTransaction::State state);
    Step finish(Step step);
    void decode();
    void readValue(uint8_t offset, ReactorMeasurements::ValueType& value) const;

    I2cDevice& device_;
    uint8_t windowBase_;

    bool reading_ = false;
    uint8_t retries_ = 0;
    ReactorMeasurements snapshot_ = {};
    bool snapshotValid_ = false;

    I2cTransaction firstSequence_;
    I2cTransaction values_;
    I2cTransaction secondSequence_;
    uint8_t firstSequenceByte_ = 0;
    uint8_t secondSequenceByte_ = 0;
    uint8_t valueBytes_[valuesSize] = {};
};

} // namespace peer
