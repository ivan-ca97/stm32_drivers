#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <utility>

#include "authentication_errors.hpp"
#include "safety/commands.h"
#include "safety/handler.h"

namespace safety {
namespace {

static_assert(
    tokenSize == sizeof(Session::TokenType),
    "the wire token size has to match Session::TokenType"
);

uint64_t readLittleEndian(std::span<const uint8_t> bytes)
{
    uint64_t value = 0;
    for (size_t i = 0; i < sizeof(uint64_t); i++) {
        value |= uint64_t(bytes[i]) << (8 * i);
    }
    return value;
}

Handler::Progress reject(ControllerSlave::Response& response, WireError error)
{
    response.fail(static_cast<uint8_t>(error));
    return Handler::Progress::Done;
}

} // namespace

Handler::Builder& Handler::Builder::authentication(Authentication& authentication)
{
    authentication_ = &authentication;
    return *this;
}

Handler::Builder& Handler::Builder::mode(OperatingMode& mode)
{
    mode_ = &mode;
    return *this;
}

Handler::Builder& Handler::Builder::calibration(
    const ReactorParams& parameters,
    CalibrationStorage& storage
)
{
    parameters_ = &parameters;
    calibrationStorage_ = &storage;
    return *this;
}

Handler::Builder& Handler::Builder::peer(peer::Link& link, peer::Monitor& monitor)
{
    link_ = &link;
    monitor_ = &monitor;
    return *this;
}

Handler::Builder& Handler::Builder::storage(StorageUpkeep& upkeep, uint32_t minimumFreeBytes)
{
    storage_ = &upkeep;
    minimumFreeBytes_ = minimumFreeBytes;
    return *this;
}

base::Result<Handler> Handler::Builder::build() const
{
    bool complete = authentication_ != nullptr
        && mode_ != nullptr
        && parameters_ != nullptr
        && calibrationStorage_ != nullptr
        && link_ != nullptr
        && monitor_ != nullptr
        && storage_ != nullptr;
    if (!complete) {
        return std::unexpected(missingPart);
    }

    return base::Result<Handler>(std::in_place, *this, Permit{});
}

Handler::Handler(const Builder& parts, Permit)
    : authentication_(*parts.authentication_),
      mode_(*parts.mode_),
      localParameters_(*parts.parameters_),
      calibrationStorage_(*parts.calibrationStorage_),
      peerParameters_(parts.link_->parameters()),
      peerMonitor_(*parts.monitor_),
      storage_(*parts.storage_),
      minimumFreeBytes_(parts.minimumFreeBytes_)
{
}

Handler::Progress Handler::handle(ControllerSlave::Request request)
{
    ControllerSlave::Response& response = request.response();

    if (awaitingPeerRead_) {
        return leaveMaintenance(response);
    }

    std::optional<WireError> refused = refusal(request.payload());
    if (refused.has_value()) {
        return reject(response, *refused);
    }

    switch (static_cast<Command>(request.cmd())) {
    case Command::EnterMaintenance:
        if (mode_.inMaintenance()) {
            return reject(response, WireError::AlreadyInMaintenance);
        }
        mode_.enterMaintenance();
        return Progress::Done;
    case Command::LeaveMaintenance:
        if (!mode_.inMaintenance()) {
            return reject(response, WireError::NotInMaintenance);
        }
        if (storage_.freeBytes() < minimumFreeBytes_) {
            return reject(response, WireError::CompactionRequired);
        }
        return leaveMaintenance(response);
    case Command::ResetDiscrepancyLatch:
        peerMonitor_.resetDiscrepancyLatch();
        return Progress::Done;
    case Command::CompactStorage:
        return compactStorage(response);
    }
    return reject(response, WireError::UnknownCommand);
}

void Handler::abandon()
{
    awaitingPeerRead_ = false;
}

std::optional<WireError> Handler::refusal(std::span<const uint8_t> payload) const
{
    if (payload.size() != tokenSize) {
        return WireError::Malformed;
    }

    uint64_t token = readLittleEndian(payload);
    ResultSession session = authentication_.validateWithPermission(token, Permission::Maintenance);
    if (session.has_value()) {
        return std::nullopt;
    }

    bool lacksPermission = session.error() == authentication::insufficientPermissions;
    return lacksPermission ? WireError::NoPermission : WireError::InvalidToken;
}

bool Handler::peerAgreesOnParameters() const
{
    base::Result<ReactorParams> peerParameters = peerParameters_.latest();
    if (!peerParameters.has_value()) {
        return false;
    }
    return *peerParameters == localParameters_;
}

Handler::Progress Handler::leaveMaintenance(ControllerSlave::Response& response)
{
    if (!awaitingPeerRead_) {
        startPeerRead();
        return Progress::Pending;
    }

    peer::Step step = peerParameters_.service();
    if (step == peer::Step::Busy || rereadIfCorrupted(step)) {
        return Progress::Pending;
    }

    awaitingPeerRead_ = false;

    if (isReadCorrupted(step)) {
        return reject(response, WireError::PeerParametersCorrupted);
    }
    if (step != peer::Step::Completed) {
        return reject(response, WireError::PeerUnreachable);
    }
    if (!peerAgreesOnParameters()) {
        return reject(response, WireError::PeerParametersDiffer);
    }
    // Still in maintenance, so a save that has to compact may do so.
    base::Status saved = calibrationStorage_.save(localParameters_);
    if (!saved.has_value()) {
        return reject(response, WireError::CalibrationNotSaved);
    }

    peerMonitor_.resetDiscrepancyLatch();
    mode_.leaveMaintenance();
    return Progress::Done;
}

void Handler::startPeerRead()
{
    // Drains a read left behind by an abandoned command, which would otherwise keep
    // start() refusing.
    (void)peerParameters_.service();

    base::Status started = peerParameters_.start();
    if (started.has_value()) {
        awaitingPeerRead_ = true;
        parameterReads_ = 1;
    }
}

bool Handler::rereadIfCorrupted(peer::Step step)
{
    if (!isReadCorrupted(step) || parameterReads_ >= maxParameterReads) {
        return false;
    }

    // The read has just finished, so the reader is free and start() cannot refuse.
    (void)peerParameters_.start();
    parameterReads_++;
    return true;
}

bool Handler::isReadCorrupted(peer::Step step) const
{
    return step == peer::Step::Failed && peerParameters_.lastRead().error() == peer::malformed;
}

Handler::Progress Handler::compactStorage(ControllerSlave::Response& response)
{
    if (!mode_.inMaintenance()) {
        return reject(response, WireError::NotInMaintenance);
    }

    base::Status compacted = storage_.compact();
    if (!compacted.has_value()) {
        return reject(response, WireError::CompactionFailed);
    }
    return Progress::Done;
}

} // namespace safety
