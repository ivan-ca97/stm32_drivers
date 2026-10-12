#pragma once

#include <cstdint>

// Wire contract of the safety section. A change here has to be mirrored on the
// master's side.
//
// Every command carries the same payload:
//   Request: [token(8)]
//   Ready:   no payload
//   Error:   [WireError(1)]
namespace safety {

enum class Command : uint8_t {
    EnterMaintenance = 0x40,
    LeaveMaintenance = 0x41,
    ResetDiscrepancyLatch = 0x42,
    CompactStorage = 0x43,
};

inline constexpr uint8_t tokenSize = 8;

enum class WireError : uint8_t {
    Malformed = 0x80,
    InvalidToken = 0x81,
    NoPermission = 0x82,
    UnknownCommand = 0x83,
    PeerParametersDiffer = 0x84,
    PeerUnreachable = 0x85,
    AlreadyInMaintenance = 0x86,
    NotInMaintenance = 0x87,
    PeerParametersCorrupted = 0x88,
    CalibrationNotSaved = 0x89,
    CompactionRequired = 0x8A,
    CompactionFailed = 0x8B,
};

} // namespace safety
