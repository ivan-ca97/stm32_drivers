#pragma once

#include "peer_monitor.hpp"

struct PeerMonitor::Config
{
    public:
        I2cDevice* peer = nullptr;
        const Clock* clock = nullptr;
        uint8_t peerWindowBase = 0;
        uint8_t maxConsecutiveFailures = 3;
        uint32_t readTimeoutMs = 500;
        ReactorMeasurements::ValueType tolerance = 0;

    private:
        Config() = default;

    friend class PeerMonitor::Builder;
};

class PeerMonitor::Builder
{
    private:
        Config config;

    public:
        Config buildConfig();

        Builder& withPeer(I2cDevice& peer, uint8_t peerWindowBase);

        Builder& withClock(const Clock& clock);

        Builder& withReadTimeout(uint32_t readTimeoutMs);

        Builder& afterConsecutiveFailures(uint8_t maxConsecutiveFailures);

        Builder& withTolerance(ReactorMeasurements::ValueType tolerance);
};
