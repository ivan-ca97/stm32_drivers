#include "peer_measurement_reader.hpp"

#include <cstring>

PeerMeasurementReader::StartResult PeerMeasurementReader::startRead()
{
    if(busy())
        return lib::Error(StartError::Busy);

    retries = 0;
    transactionFailed = false;
    dispatchSeqRead();
    phase = Phase::ReadingSeq1;
    return {};
}

PeerMeasurementReader::Step PeerMeasurementReader::service()
{
    switch(phase)
    {
        case Phase::Idle:
            return Step::Idle;   // nothing in flight: the caller decides when to read

        case Phase::ReadingSeq1:
            if(transactionBusy) return Step::Busy;
            if(transactionFailed)
            {
                phase = Phase::Idle;
                return Step::Failed;
            }
            seq1 = seqByte;
            dispatchDataRead();
            phase = Phase::ReadingData;
            return Step::Busy;

        case Phase::ReadingData:
            if(transactionBusy) return Step::Busy;
            if(transactionFailed)
            {
                phase = Phase::Idle;
                return Step::Failed;
            }
            dispatchSeqRead();
            phase = Phase::ReadingSeq2;
            return Step::Busy;

        case Phase::ReadingSeq2:
            if(transactionBusy) return Step::Busy;
            if(transactionFailed)
            {
                phase = Phase::Idle;
                return Step::Failed;
            }
            if(seq1 == seqByte && (seq1 & 1) == 0)
            {
                decode();
                phase = Phase::Idle;
                return Step::Completed;
            }
            if(++retries >= MaxRetries)
            {
                phase = Phase::Idle;
                return Step::Failed;   // never caught a stable snapshot
            }
            dispatchSeqRead();
            phase = Phase::ReadingSeq1;
            return Step::Busy;
    }
    return Step::Idle;
}

void PeerMeasurementReader::dispatchSeqRead()
{
    transactionBusy = true;

    pendingTransaction = I2cTransaction::Builder()
        .setDirection(I2cTransaction::RX)
        .withData(&seqByte, 1)
        .withRegister(windowBase + measurements::SequenceOffset)
        .withPostCallback(onStepDone, this)
        .withErrorCallback(onStepError, this)
        .build();

    peer << pendingTransaction;
}

void PeerMeasurementReader::dispatchDataRead()
{
    transactionBusy = true;

    pendingTransaction = I2cTransaction::Builder()
        .setDirection(I2cTransaction::RX)
        .withData(dataBytes, DataBlockSize)
        .withRegister(windowBase + DataBlockBase)
        .withPostCallback(onStepDone, this)
        .withErrorCallback(onStepError, this)
        .build();

    peer << pendingTransaction;
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
