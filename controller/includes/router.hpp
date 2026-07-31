#pragma once

#include "controller_slave.hpp"   // ControllerSlave::RequestHandler, Response

// ============================================================================
// Router — a RequestHandler that dispatches each command to the section that
// registered the opcode range covering it. Sections stay independent; the router
// is the only piece that knows the opcode partition.
// ============================================================================
class Router : public ControllerSlave::RequestHandler
{
    public:
        static constexpr uint8_t MAX_SECTIONS = 8;

        // `unknownCommandCode` is the error byte returned when no section owns the
        // command (the integrator aligns it with the wire protocol).
        explicit Router(uint8_t unknownCommandCode = 0xFF)
            : unknownCommandCode(unknownCommandCode) {}

        // Register `handler` for the inclusive opcode range [low, high].
        // Returns false if the table is full or the range is invalid.
        bool addSection(uint8_t low, uint8_t high, RequestHandler& handler);

        void handle(ControllerSlave::Request request) override;

    private:
        struct Section
        {
            uint8_t                         low;
            uint8_t                         high;
            ControllerSlave::RequestHandler* handler;
        };

        Section  sections[MAX_SECTIONS];
        uint8_t  count = 0;
        uint8_t  unknownCommandCode;
};
