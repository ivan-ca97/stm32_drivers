#include <cstdint>
#include <cstring>
#include <expected>

#include "peer/measurement_reader.h"

namespace peer {

MeasurementReader::MeasurementReader(I2cDevice& device, uint8_t windowBase)
    : device_(device), windowBase_(windowBase)
{
}

base::Status MeasurementReader::start()
{
    if (busy()) {
        return std::unexpected(peer::busy);
    }

    retries_ = 0;
    transactionFailed_ = false;
    dispatchRead(measurement_window::sequenceOffset, &sequenceByte_, 1);
    phase_ = Phase::ReadingFirstSequence;
    return base::ok;
}

bool MeasurementReader::busy() const
{
    return phase_ != Phase::Idle;
}

Step MeasurementReader::service()
{
    if (phase_ == Phase::Idle) {
        return Step::Idle;
    }

    // Only the transaction's own state says the bus is done with the buffer.
    if (transaction_.getState() == I2cTransaction::CANCELLED) {
        return finish(Step::Idle);
    }
    if (transactionBusy_) {
        return Step::Busy;
    }
    if (transactionFailed_) {
        return finish(Step::Failed);
    }

    switch (phase_) {
    case Phase::ReadingFirstSequence:
        firstSequence_ = sequenceByte_;
        dispatchRead(valuesBase, valueBytes_, valuesSize);
        phase_ = Phase::ReadingValues;
        return Step::Busy;

    case Phase::ReadingValues:
        dispatchRead(measurement_window::sequenceOffset, &sequenceByte_, 1);
        phase_ = Phase::ReadingSecondSequence;
        return Step::Busy;

    case Phase::ReadingSecondSequence:
        if (firstSequence_ == sequenceByte_ && (firstSequence_ & 1) == 0) {
            decode();
            return finish(Step::Completed);
        }
        retries_++;
        if (retries_ >= maxRetries) {
            return finish(Step::Failed);
        }
        dispatchRead(measurement_window::sequenceOffset, &sequenceByte_, 1);
        phase_ = Phase::ReadingFirstSequence;
        return Step::Busy;

    case Phase::Idle:
        break;
    }
    return Step::Idle;
}

void MeasurementReader::abandon()
{
    transaction_.cancel();
}

base::Result<ReactorMeasurements> MeasurementReader::latest() const
{
    if (!snapshotValid_) {
        return std::unexpected(noData);
    }
    return snapshot_;
}

Step MeasurementReader::finish(Step step)
{
    transactionBusy_ = false;
    transactionFailed_ = false;
    phase_ = Phase::Idle;
    return step;
}

void MeasurementReader::dispatchRead(uint8_t offset, uint8_t* buffer, uint16_t length)
{
    transactionBusy_ = true;

    transaction_ = I2cTransaction::Builder()
        .setDirection(I2cTransaction::RX)
        .withData(buffer, length)
        .withRegister(windowBase_ + offset)
        .withPostCallback(onStepDone, this)
        .withErrorCallback(onStepError, this)
        .build();

    device_ << transaction_;
}

void MeasurementReader::decode()
{
    readValue(measurement_window::reactivityLogarithmicOffset, snapshot_.reactivityLogarithmic);
    readValue(measurement_window::reactivityLinearOffset, snapshot_.reactivityLinear);
    readValue(measurement_window::powerRateLogarithmicOffset, snapshot_.powerRateLogarithmic);
    readValue(measurement_window::powerRateLinearOffset, snapshot_.powerRateLinear);
    snapshotValid_ = true;
}

void MeasurementReader::readValue(uint8_t offset, ReactorMeasurements::ValueType& value) const
{
    std::memcpy(&value, valueBytes_ + (offset - valuesBase), measurement_window::valueSize);
}

void MeasurementReader::onStepDone(void* self)
{
    static_cast<MeasurementReader*>(self)->transactionBusy_ = false;
}

void MeasurementReader::onStepError(void* self)
{
    MeasurementReader* reader = static_cast<MeasurementReader*>(self);
    reader->transactionBusy_ = false;
    reader->transactionFailed_ = true;
}

} // namespace peer
