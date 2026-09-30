#include "peer_monitor_builder.hpp"   // Config

#include <cmath>

PeerMonitor::PeerMonitor(const Config& config)
    : reader(*config.peer, config.peerWindowBase), clock(*config.clock),
      maxConsecutiveFailures(config.maxConsecutiveFailures),
      readTimeoutMs(config.readTimeoutMs),
      tolerance(config.tolerance) {}

void PeerMonitor::countFailure()
{
    if(consecutiveFailures < UINT8_MAX)
        consecutiveFailures++;
}

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
            countFailure();
            break;

        case PeerMeasurementReader::Step::Busy:
            if(clock.millisecondsSince(readStartedMs) < readTimeoutMs)
                return;

            // Abandoning only frees the reader once the bus lets go, so the clock
            // restarts either way and one still held keeps failing at its own pace.
            countFailure();
            readStartedMs = clock.milliseconds();
            reader.abandon();
            break;

        default:
            break;
    }

    if(!reader.busy())
    {
        readStartedMs = clock.milliseconds();
        (void)reader.startRead();   // busy() just checked: cannot fail with peer_measurement_reader::busy
    }
}

bool PeerMonitor::withinTolerance(const ReactorMeasurements& a, const ReactorMeasurements& b) const
{
    return std::fabs(a.reactivityLogarithmic - b.reactivityLogarithmic) <= tolerance
        && std::fabs(a.reactivityLinear - b.reactivityLinear) <= tolerance
        && std::fabs(a.powerRateLogarithmic - b.powerRateLogarithmic) <= tolerance
        && std::fabs(a.powerRateLinear - b.powerRateLinear) <= tolerance;
}
