#pragma once

#include <stdint.h>

// ============================================================================
// Auth command protocol — wire contract shared with the ESP32 (controller_link).
//
// Transported over the ControllerSlave mailbox (reg 0x10 request / 0x11 response):
//     request  = [cmd][len][payload]
//     response = [status][len][data]   (status: 2 Ready = OK, 3 Error)
//
// This handler runs in the main loop (deferred), never in the ISR.
//
// Payload layouts (fields concatenated in order; strings are NUL-terminated,
// fixed-width integers are little-endian raw bytes — both MCUs are LE):
//
//   LogIn             : user\0 pass\0                    -> OK: token(8)
//   LogOut            : token(8)                         -> OK: (empty)
//   CreateUser        : token(8) user\0 pass\0 perm(1)   -> OK: id(2)
//   DeleteUser        : token(8) id(2)                   -> OK: (empty)
//   ModifyOwnUsername : token(8) user\0                  -> OK: (empty)
//   ModifyOwnPassword : token(8) oldPass\0 newPass\0     -> OK: (empty)
//   ModifyOwnName     : token(8) name\0                  -> OK: (empty)
//   ModifyUsername    : token(8) id(2) user\0            -> OK: (empty)
//   ModifyPassword    : token(8) id(2) pass\0            -> OK: (empty)
//   ModifyName        : token(8) id(2) name\0            -> OK: (empty)
//   ModifyPermission  : token(8) id(2) perm(1)           -> OK: (empty)
//
// On success the ControllerSlave status becomes Ready(2) and the response
// payload is the OK payload above (possibly empty). On error the status becomes
// Error(3) and the response payload is a single ErrorCode byte (see below).
//
// Keep this header in sync with the ESP32 side.
// ============================================================================

namespace auth
{
    // Command opcodes (byte 0 of the request).
    // PROVISIONAL grouping — must be agreed with the ESP32 controller_link:
    //   0x0x session, 0x1x user CRUD, 0x2x modify-own, 0x3x modify-others.
    enum Cmd : uint8_t
    {
        CmdLogIn             = 0x01,
        CmdLogOut            = 0x02,

        CmdCreateUser        = 0x10,
        CmdDeleteUser        = 0x11,

        CmdModifyOwnUsername = 0x20,
        CmdModifyOwnPassword = 0x21,
        CmdModifyOwnName     = 0x22,

        CmdModifyUsername    = 0x30,
        CmdModifyPassword    = 0x31,
        CmdModifyName        = 0x32,
        CmdModifyPermission  = 0x33,
    };

    // Fixed field sizes on the wire.
    static constexpr uint8_t TokenSize = 8;   // Session::TokenType (uint64_t), little-endian
    static constexpr uint8_t IdSize    = 2;   // User::IdType (uint16_t), little-endian
    static constexpr uint8_t PermSize  = 1;

    // Permission id on the wire: ascending privilege.
    // WARNING: this is NOT the Permission enum's underlying value (that enum is
    // Superuser=0 .. None=3). The handler maps between the two explicitly.
    enum WirePermission : uint8_t
    {
        PermNone        = 0,
        PermObserver    = 1,
        PermMaintenance = 2,
        PermSuperuser   = 3,
    };

    // Error code carried in the response payload when status == Error.
    // Explicit, stable values — do not reorder; append new codes at the end.
    // 0x01..0x0E mirror AuthenticationError; 0x80+ are handler/framing level.
    enum ErrorCode : uint8_t
    {
        ErrNone              = 0x00,
        ErrUserIdNotFound    = 0x01,
        ErrUsernameNotFound  = 0x02,
        ErrUsernameExists    = 0x03,
        ErrInvalidToken      = 0x04,
        ErrNoPermission      = 0x05,
        ErrIncorrectPassword = 0x06,
        ErrUsersBufferFull   = 0x07,
        ErrSessionBufferFull = 0x08,
        ErrOverflow          = 0x09,
        ErrUsernameOverflow  = 0x0A,
        ErrNameOverflow      = 0x0B,
        ErrPasswordOverflow  = 0x0C,
        ErrEmptyField        = 0x0D,
        ErrIntegrityFailure  = 0x0E,

        ErrMalformedRequest  = 0x80,   // payload too short / missing NUL terminator
        ErrUnknownCommand    = 0x81,
        ErrInternal          = 0x82,
    };
}
