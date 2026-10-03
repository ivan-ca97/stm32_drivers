#include <cstdint>
#include <cstring>
#include <expected>

#include "peer/parameter_reader.h"

namespace peer {

ParameterReader::ParameterReader(I2cDevice& device, uint8_t windowBase)
    : device_(device), windowBase_(windowBase)
{
}

base::Status ParameterReader::start()
{
    if (busy()) {
        return std::unexpected(peer::busy);
    }

    transactionFailed_ = false;
    transactionBusy_ = true;

    transaction_ = I2cTransaction::Builder()
        .setDirection(I2cTransaction::RX)
        .withData(buffer_, parameter_window::length)
        .withRegister(windowBase_)
        .withPostCallback(onStepDone, this)
        .withErrorCallback(onStepError, this)
        .build();

    device_ << transaction_;
    phase_ = Phase::Reading;
    return base::ok;
}

bool ParameterReader::busy() const
{
    return phase_ != Phase::Idle;
}

Step ParameterReader::service()
{
    if (phase_ == Phase::Idle) {
        return Step::Idle;
    }
    if (transactionBusy_) {
        return Step::Busy;
    }

    phase_ = Phase::Idle;

    if (transactionFailed_) {
        return Step::Failed;
    }
    if (!decode()) {
        return Step::Failed;
    }

    snapshotValid_ = true;
    return Step::Completed;
}

base::Result<ReactorParams> ParameterReader::latest() const
{
    if (!snapshotValid_) {
        return std::unexpected(noData);
    }
    return snapshot_;
}

bool ParameterReader::decode()
{
    ReactorParams decoded;

    for (uint16_t index = 0; index < ReactorParams::Count; index++) {
        float value;
        std::memcpy(&value, buffer_ + parameter_window::offset(index), parameter_window::valueSize);
        if (!decoded.set(index, value)) {
            return false;
        }
    }

    snapshot_ = decoded;
    return true;
}

void ParameterReader::onStepDone(void* self)
{
    static_cast<ParameterReader*>(self)->transactionBusy_ = false;
}

void ParameterReader::onStepError(void* self)
{
    ParameterReader* reader = static_cast<ParameterReader*>(self);
    reader->transactionBusy_ = false;
    reader->transactionFailed_ = true;
}

} // namespace peer
