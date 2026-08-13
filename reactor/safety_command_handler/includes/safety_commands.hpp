#pragma once

#include <stdint.h>

// Safety section — wire contract. A change here has to be mirrored on the
// master's side.
//
// Authenticated operations that move the safety state: the maintenance-mode
// transitions and the discrepancy latch reset. Kept out of the parameter section
// because that one only answers in maintenance, and entering it cannot require
// already being in it.
//
// Leaving maintenance is refused unless the peer's calibration matches this
// board's, which is why the failures below include the peer.
//
// Every command here carries the same payload:
//   Request: [token]
//   Response:
//      Ready: no payload (len 0)
//      Error: [ErrorCode]
namespace safety
{
    // Deferred command opcodes (routed to this section).
    enum Cmd : uint8_t
    {
        CmdEnterMaintenance = 0x40,
        CmdLeaveMaintenance = 0x41,
        CmdResetDiscrepancyLatch = 0x42,
    };

    static constexpr uint8_t TokenSize = 8;

    enum ErrorCode : uint8_t
    {
        ErrorNone = 0x00,
        ErrorMalformed = 0x80,
        ErrorInvalidToken = 0x81,
        ErrorNoPermission = 0x82,
        ErrorUnknownCommand = 0x83,
        ErrorPeerParametersDiffer = 0x84,
        ErrorPeerUnreachable = 0x85,
    };
}
