#include "peer_monitor_builder.hpp"   // Config

#include <cmath>

PeerMonitor::PeerMonitor(const Config& config)
    : reader(*config.peer, config.peerWindowBase),
      maxConsecutiveFailures(config.maxConsecutiveFailures),
      tolerance(config.tolerance) {}

void PeerMonitor::service(const ReactorMeasurements& local)
{
    switch(reader.service())
    {
        case PeerMeasurementReader::Step::Completed:
        {
            consecutiveFailures = 0;
            auto peer = reader.latest();
            if(peer && !withinTolerance(local, *peer))
                discrepancy = true;
            break;
        }

        case PeerMeasurementReader::Step::Failed:
            if(consecutiveFailures < UINT8_MAX)
                consecutiveFailures++;
            break;

        default:
            break;
    }

    if(!reader.busy())
        (void)reader.startRead();   // busy() just checked: cannot fail with StartError::Busy
}

bool PeerMonitor::withinTolerance(const ReactorMeasurements& a, const ReactorMeasurements& b) const
{
    return std::fabs(a.reactivityLogarithmic - b.reactivityLogarithmic) <= tolerance
        && std::fabs(a.reactivityLinear - b.reactivityLinear) <= tolerance
        && std::fabs(a.powerRateLogarithmic - b.powerRateLogarithmic) <= tolerance
        && std::fabs(a.powerRateLinear - b.powerRateLinear) <= tolerance;
}
