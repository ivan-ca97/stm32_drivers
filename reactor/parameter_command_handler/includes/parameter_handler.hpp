#pragma once

#include "controller_slave.hpp"   // ControllerSlave::RequestHandler, Response
#include "peer/parameter_publisher.h"
#include "parameter_commands.hpp"
#include "reactor_parameters.hpp"   // typed source of truth
#include "authentication.hpp"   // token validation
#include "persistent_storage.hpp"   // persistence

// The wire's ParamId list is built from the model's NeutronGroups, but laid out by
// hand; this catches the two shapes drifting apart if a param is added to one and
// not the other.
static_assert(ReactorParams::Count == param::ParamCount, "param model / wire count mismatch");

// TokenSize is a wire constant (auth_commands.hpp declares the same value
// independently), not a derived one.
static_assert(param::TokenSize == sizeof(Session::TokenType), "wire token size does not match Session::TokenType");

// ParameterHandler — the parameter section (SetParam). Validates the token
// (>= Maintenance) and the value, updates the typed ReactorParameters (the source
// of truth), persists the whole set, and publishes the value into the parameter
// window for the masters' immediate reads.
class ParameterHandler : public ControllerSlave::RequestHandler
{
    public:
        ParameterHandler(peer::ParameterPublisher& publisher, Authentication& authentication,
                         ReactorParams& params, PersistentStorage& storage)
            : publisher(publisher), authentication(authentication), params(params), storage(storage)
        {
            load();
        }

        Progress handle(ControllerSlave::Request request) override;

    protected:
        peer::ParameterPublisher& publisher;
        Authentication& authentication;
        ReactorParams& params;
        PersistentStorage& storage;

        bool validateToken(uint64_t token)
        {
            return authentication.validateWithPermission(token, Permission::Maintenance).has_value();
        }

        bool persist();   // serialize all params -> storage
        void load();   // storage -> params + published window
};
