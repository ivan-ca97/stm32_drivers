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
    reading_ = true;
    dispatch();
    return base::ok;
}

bool MeasurementReader::busy() const
{
    return reading_;
}

Step MeasurementReader::service()
{
    if (!reading_) {
        return Step::Idle;
    }
    if (!released()) {
        return Step::Busy;
    }
    if (anyIn(I2cTransaction::CANCELLED)) {
        return finish(Step::Idle);
    }
    if (anyIn(I2cTransaction::ERROR)) {
        return finish(Step::Failed);
    }

    if (firstSequenceByte_ == secondSequenceByte_ && (firstSequenceByte_ & 1) == 0) {
        decode();
        return finish(Step::Completed);
    }

    retries_++;
    if (retries_ >= maxRetries) {
        return finish(Step::Failed);
    }
    dispatch();
    return Step::Busy;
}

void MeasurementReader::abandon()
{
    firstSequence_.cancel();
    values_.cancel();
    secondSequence_.cancel();
}

base::Result<ReactorMeasurements> MeasurementReader::latest() const
{
    if (!snapshotValid_) {
        return std::unexpected(noData);
    }
    return snapshot_;
}

void MeasurementReader::dispatch()
{
    firstSequence_ = read(measurement_window::sequenceOffset, &firstSequenceByte_, 1);
    values_ = read(valuesBase, valueBytes_, valuesSize);
    secondSequence_ = read(measurement_window::sequenceOffset, &secondSequenceByte_, 1);

    device_ << firstSequence_;
    device_ << values_;
    device_ << secondSequence_;
}

I2cTransaction MeasurementReader::read(uint8_t offset, uint8_t* buffer, uint16_t length) const
{
    I2cTransaction transaction = I2cTransaction::Builder()
        .setDirection(I2cTransaction::RX)
        .withData(buffer, length)
        .withRegister(windowBase_ + offset)
        .build();
    return transaction;
}

bool MeasurementReader::released()
{
    return firstSequence_.isDone() && values_.isDone() && secondSequence_.isDone();
}

bool MeasurementReader::anyIn(I2cTransaction::State state)
{
    return firstSequence_.getState() == state
        || values_.getState() == state
        || secondSequence_.getState() == state;
}

Step MeasurementReader::finish(Step step)
{
    reading_ = false;
    return step;
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

} // namespace peer
