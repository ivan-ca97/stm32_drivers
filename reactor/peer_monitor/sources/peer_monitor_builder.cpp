#include "peer_monitor_builder.hpp"

PeerMonitor::Builder::BuildResult PeerMonitor::Builder::buildConfig()
{
    if(config.peer == nullptr)
        return lib::Error(BuildError::NoPeer);

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
