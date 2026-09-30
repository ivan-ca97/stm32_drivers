#pragma once

#include <stdint.h>

#include "i2c_device.hpp"
#include "i2c_transaction.hpp"
#include "base/result.h"
#include "peer_parameter_reader_error.hpp"
#include "parameter_commands.hpp"
#include "reactor_parameters.hpp"

// PeerParameterReader — reads a peer's parameter window over I2C. Non-blocking:
// startRead() begins one read, service() advances it.
//
// No seqlock, unlike the measurements: parameters only change in maintenance, with
// the reactor stopped and the operator waiting, so there is no writer to race.
class PeerParameterReader
{
    public:
        enum class Step : uint8_t
        {
            Idle,
            Busy,
            Completed,
            Failed,
        };

        // Fail with one of peer_parameter_reader's errors.
        using StartResult = base::Status;
        using SnapshotResult = base::Result<ReactorParams>;

        PeerParameterReader(I2cDevice& peer, uint8_t windowBase)
            : peer(peer), windowBase(windowBase) {}

        // The bus holds the address of pendingTransaction below.
        PeerParameterReader(const PeerParameterReader&) = delete;
        PeerParameterReader& operator=(const PeerParameterReader&) = delete;

        StartResult startRead();

        bool busy() const { return phase != Phase::Idle; }

        Step service();

        // What the peer last published, or NoData until a read has completed.
        SnapshotResult latest() const
        {
            if(!snapshotValid)
                return std::unexpected(peer_parameter_reader::noData);

            return snapshot;
        }

    protected:
        enum class Phase : uint8_t { Idle, Reading };

        I2cDevice& peer;
        uint8_t windowBase;

        Phase phase = Phase::Idle;
        ReactorParams snapshot;
        bool snapshotValid = false;

        volatile bool transactionBusy = false;
        volatile bool transactionFailed = false;

        I2cTransaction pendingTransaction;
        uint8_t buffer[param::SerializedSize] = {};

        // Turns the bytes into the typed set. False if any of them is not a value the
        // model accepts.
        bool decode();

        static void onStepDone(void* self);
        static void onStepError(void* self);
};
