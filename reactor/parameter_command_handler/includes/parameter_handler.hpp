#pragma once

#include "controller_slave.hpp"   // ControllerSlave::RequestHandler, Response
#include "register_window.hpp"
#include "parameter_commands.hpp"
#include "reactor_parameters.hpp"   // typed source of truth
#include "authentication.hpp"   // token validation
#include "persistent_storage.hpp"   // persistence

// The model and the wire contract each declare the group count independently, so
// that neither has to include the other. This catches them drifting apart.
static_assert(ReactorParams::Count == param::ParamCount, "param model / wire count mismatch");

// ParameterHandler — the parameter section (SetParam). Validates the token
// (>= Maintenance) and the value, updates the typed ReactorParameters (the source
// of truth), persists the whole set, and mirrors the value into its register
// window for the master's immediate reads.
class ParameterHandler : public ControllerSlave::RequestHandler
{
    public:
        ParameterHandler(RegisterWindow& window, Authentication& authentication,
                         ReactorParams& params, PersistentStorage& storage)
            : window(window), authentication(authentication), params(params), storage(storage)
        {
            load();
        }

        void handle(ControllerSlave::Request request) override;

    protected:
        RegisterWindow& window;
        Authentication& authentication;
        ReactorParams& params;
        PersistentStorage& storage;

        bool validateToken(uint64_t token)
        {
            return authentication.validateWithPermission(token, Permission::Maintenance).has_value();
        }

        void mirror(uint8_t paramId, float value);   // write value bytes into the window
        bool persist();   // serialize all params -> storage
        void load();   // storage -> params + window mirror
};
