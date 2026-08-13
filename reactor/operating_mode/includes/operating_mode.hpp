#pragma once

#include <stdint.h>

// OperatingMode — whether the system currently accepts a calibration change.
// Starts in Maintenance: powering up is not a clearance to operate.
class OperatingMode
{
    public:
        enum class Mode : uint8_t { Normal, Maintenance };

        Mode mode() const { return current; }
        bool inMaintenance() const { return current == Mode::Maintenance; }

        void enterMaintenance() { current = Mode::Maintenance; }
        void leaveMaintenance() { current = Mode::Normal; }

    protected:
        Mode current = Mode::Maintenance;
};
