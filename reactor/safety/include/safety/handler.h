#pragma once

#include <cstdint>
#include <optional>
#include <span>

#include "authentication.hpp"
#include "base/passkey.h"
#include "base/result.h"
#include "controller_slave.hpp"
#include "operating_mode.hpp"
#include "peer/link.h"
#include "peer/monitor.h"
#include "reactor_parameters.hpp"
#include "safety/commands.h"
#include "safety/error.h"

namespace safety {

// The master's interface to the operating mode: it enters and leaves maintenance and
// resets the discrepancy latch. Every command needs a token with Maintenance permission.
//
// Entering only changes the mode. Leaving reads the peer's parameter window first and
// succeeds only if both boards hold the same calibration, answering Pending while the
// read is under way. That is the only moment the two calibrations are compared.
//
// A command for the mode already in place is refused, so the master learns it was out
// of step.
//
// A peer window holding a value the model rejects is read again, up to
// maxParameterReads times, before it is reported as corrupted, one bad read is taken
// for a transmission error.
class Handler final : public ControllerSlave::RequestHandler {
public:
    static constexpr uint8_t maxParameterReads = 3;

    class Builder {
    public:
        Builder& authentication(Authentication& authentication);
        Builder& mode(OperatingMode& mode);
        Builder& calibration(const ReactorParams& parameters);
        Builder& peer(peer::Link& link, peer::Monitor& monitor);

        base::Result<Handler> build() const;

    private:
        Authentication* authentication_ = nullptr;
        OperatingMode* mode_ = nullptr;
        const ReactorParams* parameters_ = nullptr;
        peer::Link* link_ = nullptr;
        peer::Monitor* monitor_ = nullptr;

        friend class Handler;
    };

    using Permit = base::Passkey<Builder>;

    Handler(const Builder& parts, Permit permit);

    Handler(const Handler&) = delete;
    Handler& operator=(const Handler&) = delete;

    Progress handle(ControllerSlave::Request request) override;
    void abandon() override;

private:
    // Why a request is turned away before its command is looked at, if it is.
    std::optional<WireError> refusal(std::span<const uint8_t> payload) const;

    Progress leaveMaintenance(ControllerSlave::Response& response);
    void startPeerRead();
    bool rereadIfCorrupted(peer::Step step);
    bool isReadCorrupted(peer::Step step) const;
    bool peerAgreesOnParameters() const;

    Authentication& authentication_;
    OperatingMode& mode_;
    const ReactorParams& localParameters_;
    peer::ParameterReader& peerParameters_;
    peer::Monitor& peerMonitor_;
    bool awaitingPeerRead_ = false;
    uint8_t parameterReads_ = 0;
};

} // namespace safety
