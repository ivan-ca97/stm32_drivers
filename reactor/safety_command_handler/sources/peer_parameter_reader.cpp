#include "peer_parameter_reader.hpp"

#include <cstring>

PeerParameterReader::StartResult PeerParameterReader::startRead()
{
    if(busy())
        return lib::Error(StartError::Busy);

    transactionFailed = false;
    transactionBusy = true;

    pendingTransaction = I2cTransaction::Builder()
        .setDirection(I2cTransaction::RX)
        .withData(buffer, param::SerializedSize)
        .withRegister(windowBase)
        .withPostCallback(onStepDone, this)
        .withErrorCallback(onStepError, this)
        .build();

    peer << pendingTransaction;
    phase = Phase::Reading;
    return {};
}

PeerParameterReader::Step PeerParameterReader::service()
{
    if(phase == Phase::Idle)
        return Step::Idle;

    if(transactionBusy)
        return Step::Busy;

    phase = Phase::Idle;

    if(transactionFailed)
        return Step::Failed;

    if(!decode())
        return Step::Failed;   // not a parameter set: leave the old snapshot alone

    snapshotValid = true;
    return Step::Completed;
}

void PeerParameterReader::onStepDone(void* self)
{
    static_cast<PeerParameterReader*>(self)->transactionBusy = false;
}

void PeerParameterReader::onStepError(void* self)
{
    PeerParameterReader* reader = static_cast<PeerParameterReader*>(self);
    reader->transactionBusy = false;
    reader->transactionFailed = true;
}

bool PeerParameterReader::decode()
{
    ReactorParams decoded;

    for(uint8_t id = 0; id < param::ParamCount; id++)
    {
        float value;
        std::memcpy(&value, buffer + id * param::ValueSize, param::ValueSize);   // little-endian

        if(!decoded.set(id, value))
            return false;
    }

    snapshot = decoded;
    return true;
}
