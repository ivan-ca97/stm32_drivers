#include <cmath>
#include <cstdint>
#include <expected>
#include <utility>

#include "peer/monitor.h"

namespace peer {

Monitor::Builder& Monitor::Builder::link(Link& link)
{
    link_ = &link;
    return *this;
}

Monitor::Builder& Monitor::Builder::clock(const Clock& clock)
{
    clock_ = &clock;
    return *this;
}

Monitor::Builder& Monitor::Builder::consecutiveFailures(uint8_t count)
{
    consecutiveFailures_ = count;
    return *this;
}

Monitor::Builder& Monitor::Builder::readTimeoutMs(uint32_t milliseconds)
{
    readTimeoutMs_ = milliseconds;
    return *this;
}

Monitor::Builder& Monitor::Builder::tolerance(ReactorMeasurements::ValueType tolerance)
{
    tolerance_ = tolerance;
    return *this;
}

base::Result<Monitor> Monitor::Builder::build() const
{
    if (link_ == nullptr || clock_ == nullptr) {
        return std::unexpected(missingPart);
    }

    return base::Result<Monitor>(std::in_place, *this, Permit{});
}

Monitor::Monitor(const Builder& parts, Permit)
    : reader_(parts.link_->measurements()),
      clock_(*parts.clock_),
      maxConsecutiveFailures_(parts.consecutiveFailures_),
      readTimeoutMs_(parts.readTimeoutMs_),
      tolerance_(parts.tolerance_)
{
}

void Monitor::service(const ReactorMeasurements& local)
{
    switch (reader_.service()) {
    case Step::Completed: {
        consecutiveFailures_ = 0;
        base::Result<ReactorMeasurements> peer = reader_.latest();
        if (peer.has_value() && !withinTolerance(local, *peer)) {
            discrepancy_ = true;
        }
        break;
    }

    case Step::Failed:
        countFailure();
        break;

    case Step::Busy:
        if (readTimedOut()) {
            // abandon() frees the reader only once the bus lets go of the transaction.
            // Restarting the clock makes a stuck transaction count one failure per
            // timeout, as a failed read does, instead of one per pass.
            countFailure();
            readStartedMs_ = clock_.milliseconds();
            reader_.abandon();
        }
        return;

    case Step::Idle:
        break;
    }

    if (!reader_.busy()) {
        readStartedMs_ = clock_.milliseconds();
        // busy() was just checked, the one way start() fails.
        (void)reader_.start();
    }
}

bool Monitor::peerAlive() const
{
    return consecutiveFailures_ < maxConsecutiveFailures_;
}

bool Monitor::discrepancyLatched() const
{
    return discrepancy_;
}

void Monitor::resetDiscrepancyLatch()
{
    discrepancy_ = false;
}

bool Monitor::readTimedOut() const
{
    return clock_.millisecondsSince(readStartedMs_) >= readTimeoutMs_;
}

void Monitor::countFailure()
{
    if (consecutiveFailures_ < UINT8_MAX) {
        consecutiveFailures_++;
    }
}

bool Monitor::withinTolerance(
    const ReactorMeasurements& first,
    const ReactorMeasurements& second
) const
{
    return std::fabs(first.reactivityLogarithmic - second.reactivityLogarithmic) <= tolerance_
        && std::fabs(first.reactivityLinear - second.reactivityLinear) <= tolerance_
        && std::fabs(first.powerRateLogarithmic - second.powerRateLogarithmic) <= tolerance_
        && std::fabs(first.powerRateLinear - second.powerRateLinear) <= tolerance_;
}

} // namespace peer
