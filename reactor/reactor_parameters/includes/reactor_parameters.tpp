#include "reactor_parameters.hpp"

template <uint16_t NeutronGroupsNumber>
bool ReactorParameters<NeutronGroupsNumber>::set(uint16_t index, float value)
{
    if(index >= Count || !std::isfinite(value))
        return false;

    if(index < NeutronGroupsNumber) { fractions[index] = value; return true; }
    index -= NeutronGroupsNumber;
    if(index < NeutronGroupsNumber) { decayConstants[index] = value; return true; }
    index -= NeutronGroupsNumber;
    switch(index)
    {
        case 0: generationTime = value; return true;
        case 1: logChannel = value; return true;
        case 2: linChannel = value; return true;
        case 3: reactivityAlert = value; return true;
        case 4: powerRateAlert = value; return true;
        case 5: reactivityScram = value; return true;
        case 6: rateScram = value; return true;
    }
    return false;
}

template <uint16_t NeutronGroupsNumber>
float ReactorParameters<NeutronGroupsNumber>::get(uint16_t index) const
{
    if(index >= Count) return 0.0f;

    if(index < NeutronGroupsNumber) return fractions[index];
    index -= NeutronGroupsNumber;
    if(index < NeutronGroupsNumber) return decayConstants[index];
    index -= NeutronGroupsNumber;
    switch(index)
    {
        case 0: return generationTime;
        case 1: return logChannel;
        case 2: return linChannel;
        case 3: return reactivityAlert;
        case 4: return powerRateAlert;
        case 5: return reactivityScram;
        case 6: return rateScram;
    }
    return 0.0f;
}

template <uint16_t NeutronGroupsNumber>
float ReactorParameters<NeutronGroupsNumber>::neutronGroupFraction(uint16_t group) const
{
    return group < NeutronGroupsNumber ?fractions[group] : 0.0f;
}

template <uint16_t NeutronGroupsNumber>
float ReactorParameters<NeutronGroupsNumber>::neutronGroupDecayConstant(uint16_t group) const
{
    return group < NeutronGroupsNumber ?decayConstants[group] : 0.0f;
}
