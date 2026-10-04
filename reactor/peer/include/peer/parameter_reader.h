#pragma once

#include <cstdint>

#include "base/result.h"
#include "i2c_device.hpp"
#include "i2c_transaction.hpp"
#include "peer/error.h"
#include "peer/parameter_window.h"
#include "peer/step.h"
#include "reactor_parameters.hpp"

namespace peer {

// Reads the peer's parameter window in one transaction. start() begins the read and
// service() advances it, without blocking.
//
// Reads without a seqlock: the window only changes in maintenance.
class ParameterReader {
public:
    ParameterReader(I2cDevice& device, uint8_t windowBase);

    // The bus holds the address of the transaction inside.
    ParameterReader(const ParameterReader&) = delete;
    ParameterReader& operator=(const ParameterReader&) = delete;

    // Fails with busy while a read is under way.
    base::Status start();

    bool busy() const;

    // Failed also covers a window holding a value the model rejects; the previous
    // snapshot is kept then, and lastRead() tells the two apart.
    Step service();

    // How the last finished read went: ok, readFailed or malformed.
    base::Status lastRead() const;

    // The last set read whole, or noData until one has been.
    base::Result<ReactorParams> latest() const;

private:
    enum class Phase : uint8_t {
        Idle,
        Reading,
    };

    bool decode();

    static void onStepDone(void* self);
    static void onStepError(void* self);

    I2cDevice& device_;
    uint8_t windowBase_;

    Phase phase_ = Phase::Idle;
    ReactorParams snapshot_;
    bool snapshotValid_ = false;
    base::Status lastRead_ = base::ok;

    volatile bool transactionBusy_ = false;
    volatile bool transactionFailed_ = false;

    I2cTransaction transaction_;
    uint8_t buffer_[parameter_window::length] = {};
};

} // namespace peer
