#pragma once

#include <array>
#include <cmath>
#include <stdint.h>

// ReactorParameters — typed reactor calibration.
//
// Flat ordinal access (set/get by index) is the primary interface; whoever stores or
// transmits the set relies on that order, so it is part of the type's contract:
//   [0 .. G-1]    neutron group fractions       (beta_i)
//   [G .. 2G-1]   neutron group decay constants (lambda_i)
//   [2G+0]        neutron generation time
//   [2G+1]        log channel constant
//   [2G+2]        lin channel constant
//   [2G+3]        reactivity alert level
//   [2G+4]        power-rate alert level
//   [2G+5]        reactivity scram level
//   [2G+6]        power-rate scram level
// Named getters expose the same values for computation.
template <uint16_t NeutronGroupsNumber>
class ReactorParameters
{
    public:
        static constexpr uint16_t Count = 2 * NeutronGroupsNumber + 7;

        uint16_t count() const { return Count; }
        uint16_t neutronGroups() const { return NeutronGroupsNumber; }

        // Flat ordinal access. set() rejects an out-of-range index or a non-finite
        // value (NaN/inf) and returns false; get() returns 0 for a bad index.
        bool set(uint16_t index, float value);
        float get(uint16_t index) const;

        // Named accessors for internal computation (group getters clamp range).
        float neutronGroupFraction(uint16_t group) const;
        float neutronGroupDecayConstant(uint16_t group) const;
        float neutronGenerationTime() const { return generationTime; }
        float logChannelConstant() const { return logChannel; }
        float linChannelConstant() const { return linChannel; }
        float reactivityAlertLevel() const { return reactivityAlert; }
        float powerRateAlertLevel() const { return powerRateAlert; }
        float reactivityScramLevel() const { return reactivityScram; }
        float rateScramLevel() const { return rateScram; }

    protected:
        std::array<float, NeutronGroupsNumber> fractions{};
        std::array<float, NeutronGroupsNumber> decayConstants{};
        float generationTime = 0.0f;
        float logChannel = 0.0f;
        float linChannel = 0.0f;
        float reactivityAlert = 0.0f;
        float powerRateAlert = 0.0f;
        float reactivityScram = 0.0f;
        float rateScram = 0.0f;
};

#include "reactor_parameters.tpp"

// Delayed-neutron groups this reactor is modelled with, and the resulting concrete
// type. Anything that consumes the calibration uses this — computation included, so
// it must not require the command protocol.
static constexpr uint16_t NeutronGroups = 6;
using ReactorParams = ReactorParameters<NeutronGroups>;
