#pragma once

#include <cstdint>
#include <optional>

#include "base/passkey.h"
#include "base/result.h"
#include "i2c_device.hpp"
#include "peer/error.h"
#include "peer/measurement_reader.h"
#include "peer/parameter_reader.h"

namespace peer {

// The other board, as seen from this one: the device that reaches it and a reader
// for each of its windows. The readers share the device, and the bus queues their
// transactions.
class Link {
public:
    class Builder {
    public:
        Builder& device(I2cDevice& device);

        // Bases of the peer's windows in its register bank.
        Builder& measurementWindow(uint8_t base);
        Builder& parameterWindow(uint8_t base);

        base::Result<Link> build() const;

    private:
        I2cDevice* device_ = nullptr;
        std::optional<uint8_t> measurementBase_;
        std::optional<uint8_t> parameterBase_;

        friend class Link;
    };

    using Permit = base::Passkey<Builder>;

    Link(const Builder& parts, Permit permit);

    Link(const Link&) = delete;
    Link& operator=(const Link&) = delete;

    MeasurementReader& measurements();
    ParameterReader& parameters();

private:
    MeasurementReader measurements_;
    ParameterReader parameters_;
};

} // namespace peer
