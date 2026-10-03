#include <cstdint>
#include <expected>
#include <utility>

#include "peer/link.h"

namespace peer {

Link::Builder& Link::Builder::device(I2cDevice& device)
{
    device_ = &device;
    return *this;
}

Link::Builder& Link::Builder::measurementWindow(uint8_t base)
{
    measurementBase_ = base;
    return *this;
}

Link::Builder& Link::Builder::parameterWindow(uint8_t base)
{
    parameterBase_ = base;
    return *this;
}

base::Result<Link> Link::Builder::build() const
{
    bool complete = device_ != nullptr
        && measurementBase_.has_value()
        && parameterBase_.has_value();
    if (!complete) {
        return std::unexpected(missingPart);
    }

    return base::Result<Link>(std::in_place, *this, Permit{});
}

Link::Link(const Builder& parts, Permit)
    : measurements_(*parts.device_, *parts.measurementBase_),
      parameters_(*parts.device_, *parts.parameterBase_)
{
}

MeasurementReader& Link::measurements()
{
    return measurements_;
}

ParameterReader& Link::parameters()
{
    return parameters_;
}

} // namespace peer
