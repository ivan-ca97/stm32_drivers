#pragma once

#include <cstdint>

#include "base/passkey.h"
#include "base/result.h"
#include "clock.hpp"
#include "controller_slave.hpp"
#include "digital_io.hpp"
#include "measurement_calculator.hpp"
#include "operating_mode.hpp"
#include "peer/measurement_publisher.h"
#include "peer/monitor.h"
#include "reactor/error.h"
#include "reactor_measurements.hpp"
#include "reactor_parameters.hpp"
#include "register_bank.hpp"
#include "router.hpp"
#include "watchdog.hpp"

namespace reactor {

// The whole cycle, from the acquisition channels to the SCRAM line, one pass per
// service().
//
// SCRAM is asserted in maintenance, with a latched discrepancy, with the peer
// unreachable, or with a local level exceeded. The line is driven high only while
// none of those holds.
//
// Owns both slaves over its register bank: the command slave, which takes the master's
// commands, and the peer slave, which only lets the peer read the bank. Each goes on its
// own bus. The parameter section answers only in maintenance.
class Reactor {
public:
    class Builder {
    public:
        static constexpr uint8_t maxSections = 4;

        Builder& bank(RegisterBank& bank);
        Builder& clock(const Clock& clock);
        Builder& mode(OperatingMode& mode);
        Builder& calibration(const ReactorParams& parameters);
        Builder& measurements(
            MeasurementCalculator& calculator,
            peer::MeasurementPublisher& publisher
        );
        Builder& peerMonitor(peer::Monitor& monitor);
        Builder& scramOutput(DigitalOutput& output);
        Builder& watchdog(Watchdog& watchdog);

        // Opcode ranges are inclusive.
        Builder& section(
            uint8_t opcodeLow,
            uint8_t opcodeHigh,
            ControllerSlave::RequestHandler& handler
        );
        Builder& parameterSection(
            uint8_t opcodeLow,
            uint8_t opcodeHigh,
            ControllerSlave::RequestHandler& handler
        );

        base::Result<Reactor> build() const;

    private:
        struct Section {
            uint8_t low;
            uint8_t high;
            ControllerSlave::RequestHandler* handler;
        };

        static bool overlaps(const Section& first, const Section& second);

        RegisterBank* bank_ = nullptr;
        const Clock* clock_ = nullptr;
        OperatingMode* mode_ = nullptr;
        const ReactorParams* parameters_ = nullptr;
        MeasurementCalculator* calculator_ = nullptr;
        peer::MeasurementPublisher* publisher_ = nullptr;
        peer::Monitor* peerMonitor_ = nullptr;
        DigitalOutput* scramOutput_ = nullptr;
        Watchdog* watchdog_ = nullptr;

        Section sections_[maxSections] = {};
        uint8_t sectionCount_ = 0;
        bool tooManySections_ = false;
        Section parameterSection_ = {};

        friend class Reactor;
    };

    using Permit = base::Passkey<Builder>;

    Reactor(const Builder& parts, Permit permit);

    Reactor(const Reactor&) = delete;
    Reactor& operator=(const Reactor&) = delete;

    // Hand to the I2C bus the master is on.
    ControllerSlave& commandSlave();

    // Hand to the I2C bus the peer reads from. Read-only: commands written to it are ignored.
    ControllerSlave& peerSlave();

    // One pass of the main loop, given the latest reading of each channel.
    void service(int32_t logarithmicMicroVolts, int32_t linearMicroVolts);

    bool scramAsserted() const;

private:
    class ParameterGate final : public ControllerSlave::RequestHandler {
    public:
        ParameterGate(const OperatingMode& mode, ControllerSlave::RequestHandler& section);

        Progress handle(ControllerSlave::Request request) override;
        void abandon() override;

    private:
        const OperatingMode& mode_;
        ControllerSlave::RequestHandler& section_;
    };

    void recalibrateOnLeavingMaintenance(bool inMaintenance);
    void updateScram(bool inMaintenance, const ReactorMeasurements& measurements);
    bool localLimitsExceeded(const ReactorMeasurements& measurements) const;

    OperatingMode& mode_;
    const ReactorParams& parameters_;
    MeasurementCalculator& calculator_;
    peer::MeasurementPublisher& publisher_;
    peer::Monitor& peerMonitor_;
    DigitalOutput& scramOutput_;
    Watchdog& watchdog_;

    ParameterGate parameterGate_;
    Router router_;
    ControllerSlave commandSlave_;
    ControllerSlave peerSlave_;

    bool scram_ = true;
    bool wasInMaintenance_;
};

} // namespace reactor
