#pragma once

#include <cstdint>

#include "base/passkey.h"
#include "base/result.h"
#include "clock.hpp"
#include "peer/error.h"
#include "peer/link.h"
#include "reactor_measurements.hpp"

namespace peer {

// Liveness and coherence of the peer, from its measurement window.
//
// Liveness recovers on its own once the peer answers again. A discrepancy latches
// until resetDiscrepancyLatch().
class Monitor {
public:
    class Builder {
    public:
        static constexpr uint8_t defaultConsecutiveFailures = 3;
        static constexpr uint32_t defaultReadTimeoutMs = 500;
        static constexpr ReactorMeasurements::ValueType defaultTolerance = 0;

        Builder& link(Link& link);
        Builder& clock(const Clock& clock);

        // Failed reads in a row before the peer counts as unreachable.
        Builder& consecutiveFailures(uint8_t count);

        // A read still running after this long counts as failed.
        Builder& readTimeoutMs(uint32_t milliseconds);

        // Largest difference between the two boards' values that is still coherent.
        Builder& tolerance(ReactorMeasurements::ValueType tolerance);

        base::Result<Monitor> build() const;

    private:
        Link* link_ = nullptr;
        const Clock* clock_ = nullptr;
        uint8_t consecutiveFailures_ = defaultConsecutiveFailures;
        uint32_t readTimeoutMs_ = defaultReadTimeoutMs;
        ReactorMeasurements::ValueType tolerance_ = defaultTolerance;

        friend class Monitor;
    };

    using Permit = base::Passkey<Builder>;

    Monitor(const Builder& parts, Permit permit);

    Monitor(const Monitor&) = delete;
    Monitor& operator=(const Monitor&) = delete;

    // Advances the read and compares what arrived against `local`.
    void service(const ReactorMeasurements& local);

    bool peerAlive() const;
    bool discrepancyLatched() const;
    void resetDiscrepancyLatch();

private:
    bool readTimedOut() const;
    void countFailure();
    bool withinTolerance(const ReactorMeasurements& first, const ReactorMeasurements& second) const;

    MeasurementReader& reader_;
    const Clock& clock_;
    uint8_t maxConsecutiveFailures_;
    uint32_t readTimeoutMs_;
    ReactorMeasurements::ValueType tolerance_;

    uint8_t consecutiveFailures_ = 0;
    bool discrepancy_ = false;
    uint32_t readStartedMs_ = 0;
};

} // namespace peer
