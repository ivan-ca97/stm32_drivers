#include "peer_measurement_reader.hpp"

#include <cstring>

PeerMeasurementReader::StartResult PeerMeasurementReader::startRead()
{
    if(busy())
        return std::unexpected(peer_measurement_reader::busy);

    retries = 0;
    transactionFailed = false;
    dispatchSeqRead();
    phase = Phase::ReadingSeq1;
    return {};
}

PeerMeasurementReader::Step PeerMeasurementReader::finish(Step step)
{
    transactionBusy = false;
    transactionFailed = false;
    phase = Phase::Idle;
    return step;
}

PeerMeasurementReader::Step PeerMeasurementReader::service()
{
    if(phase == Phase::Idle)
        return Step::Idle;

    // The bus' state is the only word that the bus is done with it
    // and the buffer can be used again.
    if(pendingTransaction.getState() == I2cTransaction::CANCELLED)
        return finish(Step::Idle);

    if(transactionBusy)
        return Step::Busy;

    if(transactionFailed)
        return finish(Step::Failed);

    switch(phase)
    {
        case Phase::ReadingSeq1:
            seq1 = seqByte;
            dispatchDataRead();
            phase = Phase::ReadingData;
            return Step::Busy;

        case Phase::ReadingData:
            dispatchSeqRead();
            phase = Phase::ReadingSeq2;
            return Step::Busy;

        case Phase::ReadingSeq2:
            if(seq1 == seqByte && (seq1 & 1) == 0)
            {
                decode();
                return finish(Step::Completed);
            }

            if(++retries >= MaxRetries)
                return finish(Step::Failed);   // never caught a stable snapshot

            dispatchSeqRead();
            phase = Phase::ReadingSeq1;
            return Step::Busy;

        case Phase::Idle:
            break;
    }

    return Step::Idle;
}

void PeerMeasurementReader::dispatchRead(uint8_t offset, uint8_t* buffer, uint16_t length)
{
    transactionBusy = true;

    pendingTransaction = I2cTransaction::Builder()
        .setDirection(I2cTransaction::RX)
        .withData(buffer, length)
        .withRegister(windowBase + offset)
        .withPostCallback(onStepDone, this)
        .withErrorCallback(onStepError, this)
        .build();

    peer << pendingTransaction;
}

void PeerMeasurementReader::dispatchSeqRead()
{
    dispatchRead(measurements::SequenceOffset, &seqByte, 1);
}

void PeerMeasurementReader::dispatchDataRead()
{
    dispatchRead(DataBlockBase, dataBytes, DataBlockSize);
}

void PeerMeasurementReader::readFloat(uint8_t offset, ReactorMeasurements::ValueType& value) const
{
    std::memcpy(&value, dataBytes + (offset - DataBlockBase), measurements::FloatSize);   // little-endian
}

void PeerMeasurementReader::decode()
{
    readFloat(measurements::ReactivityLogarithmicOffset, snapshot.reactivityLogarithmic);
    readFloat(measurements::ReactivityLinearOffset, snapshot.reactivityLinear);
    readFloat(measurements::PowerRateLogarithmicOffset, snapshot.powerRateLogarithmic);
    readFloat(measurements::PowerRateLinearOffset, snapshot.powerRateLinear);
    snapshotValid = true;
}

void PeerMeasurementReader::onStepDone(void* self)
{
    static_cast<PeerMeasurementReader*>(self)->transactionBusy = false;
}

void PeerMeasurementReader::onStepError(void* self)
{
    PeerMeasurementReader* reader = static_cast<PeerMeasurementReader*>(self);
    reader->transactionBusy = false;
    reader->transactionFailed = true;
}

void PeerMeasurementReader::abandon()
{
    pendingTransaction.cancel();
}
