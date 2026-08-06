#pragma once

// DigitalOutput / DigitalInput — single-line digital I/O, free of any hardware
// dependency. Consumers depend on these and stay testable; the concrete pin is
// wired by whoever constructs it.
class DigitalOutput
{
    public:
        virtual ~DigitalOutput() = default;

        virtual void set(bool high) = 0;

        // Level currently being driven.
        virtual bool get() const = 0;

        void high() { set(true); }
        void low()  { set(false); }
};

class DigitalInput
{
    public:
        virtual ~DigitalInput() = default;

        virtual bool get() const = 0;
};
