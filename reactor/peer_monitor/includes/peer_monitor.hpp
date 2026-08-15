#pragma once

#include <stdint.h>

#include "clock.hpp"
#include "peer_measurement_reader.hpp"
#include "reactor_measurements.hpp"

// PeerMonitor — liveness and coherence of one peer controller.
//
// Liveness recovers on its own once the peer answers again. A coherence
// discrepancy does not: it latches until resetDiscrepancyLatch() is called, so an
// intermittent mismatch cannot clear itself unnoticed.
class PeerMonitor
{
    public:
        class Builder;

        struct Config;

        explicit PeerMonitor(const Config& config);

        // Advance the peer read and update liveness/coherence against `local`.
        void service(const ReactorMeasurements& local);

        bool peerAlive() const { return consecutiveFailures < maxConsecutiveFailures; }
        bool discrepancyLatched() const { return discrepancy; }

        void resetDiscrepancyLatch() { discrepancy = false; }

    protected:
        PeerMeasurementReader reader;
        const Clock& clock;
        uint8_t maxConsecutiveFailures;
        uint32_t readTimeoutMs;
        ReactorMeasurements::ValueType tolerance;

        uint8_t consecutiveFailures = 0;
        bool discrepancy = false;
        uint32_t readStartedMs = 0;

        void countFailure();
        bool withinTolerance(const ReactorMeasurements& a, const ReactorMeasurements& b) const;
};
