#pragma once

#include <stdint.h>

// Parameter section — wire contract. A change here has to be mirrored on the
// master's side.
//
// Reads:  calibration parameters are immediate bank reads, float32 little-endian,
//         each at a fixed offset (see paramOffset). No command involved.
// Writes: deferred command SetParam, routed to the parameter section.
//
//   SetParam request: [cmd=0x80][token(8)][paramId(1)][value(4 = float32 LE)]
//   Ready:  (empty)               -> token OK + written + verified
//   Error:  [ErrorCode(1)]        -> bad token / permission / paramId / verify
namespace param
{
    // Deferred command opcodes (routed to this section).
    enum Cmd : uint8_t
    {
        CmdSetParam = 0x80,
    };

    // Opcode range this section claims in the router.
    static constexpr uint8_t CmdRangeLow = 0x80;
    static constexpr uint8_t CmdRangeHigh = 0xEF;

    static constexpr uint8_t TokenSize = 8;
    static constexpr uint8_t ValueSize = 4;   // float32, little-endian

    // Flat paramId ordinal. Group-indexed params (beta, lambda) occupy
    // consecutive ids. Fixed 6 neutron groups for now.
    static constexpr uint8_t NeutronGroups = 6;
    enum ParamId : uint8_t
    {
        ParamNeutronFractionBase = 0,   // 0..5   beta_i
        ParamNeutronDecayBase = ParamNeutronFractionBase + NeutronGroups,   // 6..11  lambda_i
        ParamNeutronGenerationTime = ParamNeutronDecayBase + NeutronGroups,   // 12     Lambda
        ParamLogarithmicChannelConstant,   // 13
        ParamLinearChannelConstant,   // 14
        ParamReactivityAlert,   // 15
        ParamPowerRateAlert,   // 16
        ParamReactivityScram,   // 17
        ParamRateScram,   // 18
        ParamCount   // 19
    };

    // Bank layout: each param is a float32 at WindowBase + paramId*ValueSize.
    // paramOffset is the global address the master reads.
    static constexpr uint8_t WindowBase = 0x20;
    static constexpr uint8_t WindowLength = ParamCount * ValueSize;

    // Global read address for the master (window base + window-local offset).
    constexpr uint8_t paramOffset(uint8_t paramId)
    {
        return WindowBase + paramId * ValueSize;
    }

    // Error codes returned in the response payload on failure.
    enum ErrorCode : uint8_t
    {
        ErrorNone = 0x00,
        ErrorMalformed = 0x80,   // payload too short
        ErrorInvalidToken = 0x81,
        ErrorNoPermission = 0x82,
        ErrorInvalidParamId = 0x83,
        ErrorVerifyFailed = 0x84,   // read-back did not match
        ErrorInternal = 0x85,   // persistence backend error
        ErrorInvalidValue = 0x86,   // non-finite / rejected by the typed model
    };
}
