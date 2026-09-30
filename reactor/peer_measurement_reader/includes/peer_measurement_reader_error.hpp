#pragma once

#include "base/error.h"

// Errors of PeerMeasurementReader.
namespace peer_measurement_reader
{
    enum class Code
    {
        Busy,
        NoData,
    };

    class Category final : public base::ErrorCategory
    {
        public:
            const char* name() const override { return "peer_measurement_reader"; }

            const char* describe(int code) const override
            {
                switch(static_cast<Code>(code))
                {
                    case Code::Busy: return "a read is already running";
                    case Code::NoData: return "no read has completed yet";
                }
                return "unknown error";
            }
    };

    inline constexpr Category errorCategory{};

    inline constexpr base::Error busy{errorCategory, static_cast<int>(Code::Busy)};
    inline constexpr base::Error noData{errorCategory, static_cast<int>(Code::NoData)};
}
