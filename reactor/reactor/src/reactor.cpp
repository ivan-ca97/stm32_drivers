#include <cmath>
#include <cstdint>
#include <utility>

#include "parameter_commands.hpp"
#include "reactor/reactor.h"

namespace reactor {

Reactor::Builder& Reactor::Builder::bank(RegisterBank& bank)
{
    bank_ = &bank;
    return *this;
}

Reactor::Builder& Reactor::Builder::clock(const Clock& clock)
{
    clock_ = &clock;
    return *this;
}

Reactor::Builder& Reactor::Builder::mode(OperatingMode& mode)
{
    mode_ = &mode;
    return *this;
}

Reactor::Builder& Reactor::Builder::calibration(const ReactorParams& parameters)
{
    parameters_ = &parameters;
    return *this;
}

Reactor::Builder& Reactor::Builder::acquisition(Adc::Input logarithmic, Adc::Input linear)
{
    logarithmic_ = logarithmic;
    linear_ = linear;
    return *this;
}

Reactor::Builder& Reactor::Builder::passPeriod(uint32_t milliseconds)
{
    passPeriodMs_ = milliseconds;
    return *this;
}

Reactor::Builder& Reactor::Builder::measurements(
    MeasurementCalculator& calculator,
    peer::MeasurementPublisher& publisher
)
{
    calculator_ = &calculator;
    publisher_ = &publisher;
    return *this;
}

Reactor::Builder& Reactor::Builder::peerMonitor(peer::Monitor& monitor)
{
    peerMonitor_ = &monitor;
    return *this;
}

Reactor::Builder& Reactor::Builder::scramOutput(DigitalOutput& output)
{
    scramOutput_ = &output;
    return *this;
}

Reactor::Builder& Reactor::Builder::watchdog(Watchdog& watchdog)
{
    watchdog_ = &watchdog;
    return *this;
}

Reactor::Builder& Reactor::Builder::section(
    uint8_t opcodeLow,
    uint8_t opcodeHigh,
    ControllerSlave::RequestHandler& handler
)
{
    if (sectionCount_ == maxSections) {
        tooManySections_ = true;
        return *this;
    }

    Section section = {
        .low = opcodeLow,
        .high = opcodeHigh,
        .handler = &handler,
    };
    sections_[sectionCount_] = section;
    sectionCount_++;
    return *this;
}

Reactor::Builder& Reactor::Builder::parameterSection(
    uint8_t opcodeLow,
    uint8_t opcodeHigh,
    ControllerSlave::RequestHandler& handler
)
{
    Section section = {
        .low = opcodeLow,
        .high = opcodeHigh,
        .handler = &handler,
    };
    parameterSection_ = section;
    return *this;
}

bool Reactor::Builder::overlaps(const Section& first, const Section& second)
{
    return first.low <= second.high && second.low <= first.high;
}

base::Result<Reactor> Reactor::Builder::build() const
{
    static_assert(
        maxSections + 1 <= Router::MAX_SECTIONS,
        "the router has to hold every section plus the parameter gate"
    );

    bool complete = bank_ != nullptr
        && clock_ != nullptr
        && mode_ != nullptr
        && parameters_ != nullptr
        && logarithmic_.has_value()
        && linear_.has_value()
        && passPeriodMs_ != 0
        && calculator_ != nullptr
        && publisher_ != nullptr
        && peerMonitor_ != nullptr
        && scramOutput_ != nullptr
        && watchdog_ != nullptr
        && parameterSection_.handler != nullptr;
    if (!complete) {
        return std::unexpected(missingPart);
    }
    if (tooManySections_) {
        return std::unexpected(tooManySections);
    }
    if (parameterSection_.low > parameterSection_.high) {
        return std::unexpected(invalidOpcodeRange);
    }

    for (uint8_t i = 0; i < sectionCount_; i++) {
        if (sections_[i].low > sections_[i].high) {
            return std::unexpected(invalidOpcodeRange);
        }
        if (overlaps(sections_[i], parameterSection_)) {
            return std::unexpected(overlappingSections);
        }
        for (uint8_t j = i + 1; j < sectionCount_; j++) {
            if (overlaps(sections_[i], sections_[j])) {
                return std::unexpected(overlappingSections);
            }
        }
    }

    return base::Result<Reactor>(std::in_place, *this, Permit{});
}

Reactor::ParameterGate::ParameterGate(
    const OperatingMode& mode,
    ControllerSlave::RequestHandler& section
)
    : mode_(mode), section_(section)
{
}

Reactor::ParameterGate::Progress Reactor::ParameterGate::handle(ControllerSlave::Request request)
{
    if (!mode_.inMaintenance()) {
        request.response().fail(param::ErrorNotInMaintenance);
        return Progress::Done;
    }

    return section_.handle(request);
}

void Reactor::ParameterGate::abandon()
{
    section_.abandon();
}

// The builder validated every range against the others and the router's capacity,
// so addSection() cannot refuse any of them.
Reactor::Reactor(const Builder& parts, Permit)
    : clock_(*parts.clock_),
      mode_(*parts.mode_),
      parameters_(*parts.parameters_),
      logarithmic_(*parts.logarithmic_),
      linear_(*parts.linear_),
      passPeriodMs_(parts.passPeriodMs_),
      calculator_(*parts.calculator_),
      publisher_(*parts.publisher_),
      peerMonitor_(*parts.peerMonitor_),
      scramOutput_(*parts.scramOutput_),
      watchdog_(*parts.watchdog_),
      parameterGate_(*parts.mode_, *parts.parameterSection_.handler),
      commandSlave_(router_, *parts.bank_, *parts.clock_),
      peerSlave_(router_, *parts.bank_, *parts.clock_, true),
      wasInMaintenance_(parts.mode_->inMaintenance())
{
    for (uint8_t i = 0; i < parts.sectionCount_; i++) {
        const Builder::Section& section = parts.sections_[i];
        router_.addSection(section.low, section.high, *section.handler);
    }
    router_.addSection(parts.parameterSection_.low, parts.parameterSection_.high, parameterGate_);
}

ControllerSlave& Reactor::commandSlave()
{
    return commandSlave_;
}

ControllerSlave& Reactor::peerSlave()
{
    return peerSlave_;
}

bool Reactor::scramAsserted() const
{
    return scram_;
}

void Reactor::service()
{
    // Kicked on entry as well, so a call that runs long in commandSlave_.process() still
    // gets a whole watchdog period.
    watchdog_.kick();

    commandSlave_.process();

    if (clock_.millisecondsSince(lastPass_) >= passPeriodMs_) {
        lastPass_ = clock_.milliseconds();
        pass();
    }

    watchdog_.kick();
}

void Reactor::pass()
{
    bool inMaintenance = mode_.inMaintenance();
    recalibrateOnLeavingMaintenance(inMaintenance);

    // FIXME: a channel without a reading enters the pass as 0 µV, and one whose ADC stopped
    // answering keeps its last reading. Either should be a fault that asserts SCRAM.
    Adc::MicroVolts logarithmic = logarithmic_.readLatestMicroVolts();
    Adc::MicroVolts linear = linear_.readLatestMicroVolts();
    ReactorMeasurements measurements = calculator_.update(
        logarithmic.value_or(0),
        linear.value_or(0)
    );
    publisher_.publish(measurements);
    peerMonitor_.service(measurements);

    updateScram(inMaintenance, measurements);
}

void Reactor::recalibrateOnLeavingMaintenance(bool inMaintenance)
{
    if (wasInMaintenance_ && !inMaintenance) {
        calculator_.recalibrate(parameters_);
    }
    wasInMaintenance_ = inMaintenance;
}

void Reactor::updateScram(bool inMaintenance, const ReactorMeasurements& measurements)
{
    scram_ = inMaintenance
        || peerMonitor_.discrepancyLatched()
        || !peerMonitor_.peerAlive()
        || localLimitsExceeded(measurements);

    scramOutput_.set(!scram_);
}

// An uncalibrated set leaves every level at zero, which trips here.
bool Reactor::localLimitsExceeded(const ReactorMeasurements& measurements) const
{
    return std::fabs(measurements.reactivityLogarithmic) >= parameters_.reactivityScramLevel()
        || std::fabs(measurements.reactivityLinear) >= parameters_.reactivityScramLevel()
        || std::fabs(measurements.powerRateLogarithmic) >= parameters_.rateScramLevel()
        || std::fabs(measurements.powerRateLinear) >= parameters_.rateScramLevel();
}

} // namespace reactor
