#pragma once

#include "peer_monitor.hpp"

struct PeerMonitor::Config
{
    public:
        I2cDevice* peer = nullptr;
        uint8_t peerWindowBase = 0;
        uint8_t maxConsecutiveFailures = 3;
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
        enum class BuildError : uint8_t { NoPeer };
        using BuildResult = lib::Result<Config, BuildError>;

        BuildResult buildConfig();

        Builder& withPeer(I2cDevice& peer, uint8_t peerWindowBase);

        // Failures in a row before the peer counts as unreachable.
        Builder& afterConsecutiveFailures(uint8_t maxConsecutiveFailures);

        // How far the two boards' measurements may differ.
        Builder& withTolerance(ReactorMeasurements::ValueType tolerance);
};
