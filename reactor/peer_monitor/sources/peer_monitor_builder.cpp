#include "peer_monitor_builder.hpp"

#include <cassert>

PeerMonitor::Config PeerMonitor::Builder::buildConfig()
{
    assert(config.peer);
    assert(config.clock);
    return config;
}

PeerMonitor::Builder& PeerMonitor::Builder::withPeer(I2cDevice& peer, uint8_t peerWindowBase)
{
    config.peer = &peer;
    config.peerWindowBase = peerWindowBase;
    return *this;
}

PeerMonitor::Builder& PeerMonitor::Builder::afterConsecutiveFailures(uint8_t maxConsecutiveFailures)
{
    config.maxConsecutiveFailures = maxConsecutiveFailures;
    return *this;
}

PeerMonitor::Builder& PeerMonitor::Builder::withTolerance(ReactorMeasurements::ValueType tolerance)
{
    config.tolerance = tolerance;
    return *this;
}

PeerMonitor::Builder& PeerMonitor::Builder::withClock(const Clock& clock)
{
    config.clock = &clock;
    return *this;
}

PeerMonitor::Builder& PeerMonitor::Builder::withReadTimeout(uint32_t readTimeoutMs)
{
    config.readTimeoutMs = readTimeoutMs;
    return *this;
}
