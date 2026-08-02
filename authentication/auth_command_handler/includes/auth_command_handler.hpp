#pragma once

#include "controller_slave.hpp"   // ControllerSlave::RequestHandler
#include "authentication.hpp"     // Authentication (+ Permission, AuthenticationError)
#include "auth_commands.hpp"      // auth:: wire contract

// ============================================================================
// AuthCommandHandler — maps the auth wire protocol (auth_commands.hpp) onto the
// Authentication core. Plugs into a ControllerSlave as its RequestHandler, so it
// runs in the main loop via ControllerSlave::process(), never in the ISR.
//
// Responsibility split:
//   - decode the request payload into typed fields (token/id/permission/strings),
//   - call the matching Authentication operation,
//   - fill the Response: OK payload (token/id/empty) on success, or a single
//     ErrorCode byte via Response::fail() on failure.
//
// Strings are taken as string_views straight into the request payload (valid for
// the duration of handle(); Authentication copies them into its own storage).
// ============================================================================
class AuthCommandHandler : public ControllerSlave::RequestHandler
{
    public:
        explicit AuthCommandHandler(Authentication& authentication)
            : authentication(authentication) {}

        void handle(ControllerSlave::Request request) override;

    protected:
        Authentication& authentication;
};
