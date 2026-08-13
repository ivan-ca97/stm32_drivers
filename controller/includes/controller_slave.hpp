#pragma once

#include <stdint.h>
#include <span>
#include "i2c_slave.hpp"
#include "register_bank.hpp"
#include "clock.hpp"

// ControllerSlave — slave-side register/mailbox protocol over I2C. This class only
// moves bytes; the command semantics come from an injected RequestHandler.
//
// Everything here runs in the ISR and stays O(1): the handler is called from the
// main loop instead, so no command can stretch the bus.
//
//   0x7E (W): request [cmd][len][payload]
//   0x7F (R): response [status][len][data][filler]   status: 0 Idle,1 Proc,2 Ready,3 Err
//   any other address (R): immediate registers from the shared RegisterBank.
//
// One command at a time: a request is taken only with the mailbox Idle. A reply
// nobody read yet also counts as busy, so a result is never discarded without the
// master learning how its command went — reading it is the ack that frees the slot.
//
// A read-only instance ignores writes to the request register (it still lets a
// master select a register pointer to read).
class ControllerSlave : public I2cSlave
{
    public:
        // Response built by a RequestHandler over a buffer it does not own. The
        // error code is an opaque byte, defined by each section.
        class Response
        {
            public:
                Response(uint8_t* buffer, uint8_t capacity)
                    : buffer(buffer), capacity(capacity) {}

                // Little-endian. A write that would overflow is dropped whole.
                void put(uint8_t byte);
                void put(const uint8_t* data, uint8_t length);
                void put(std::span<const uint8_t> data);
                void putU16(uint16_t value);
                void putU64(uint64_t value);

                // Replaces the payload with the error code byte.
                void fail(uint8_t errorCode);

                bool isError() const { return error; }
                uint8_t length() const { return len; }

            private:
                uint8_t* buffer;
                uint8_t capacity;
                uint8_t len = 0;
                bool error = false;
        };

        class Request
        {
            public:
                Request(uint8_t cmd, std::span<const uint8_t> payload, Response& response)
                    : cmd_(cmd), payload_(payload), response_(response) {}

                uint8_t cmd() const { return cmd_; }
                std::span<const uint8_t> payload() const { return payload_; }

                Response& response() const { return response_; }

            private:
                uint8_t cmd_;
                std::span<const uint8_t> payload_;
                Response& response_;
        };

        // Implemented by the application. Runs in the main loop, never in the ISR.
        class RequestHandler
        {
            public:
                // Pending keeps the status at Processing and hands the same request
                // back next pass, so waiting on I/O does not block the loop.
                enum class Progress : uint8_t { Done, Pending };

                virtual ~RequestHandler() = default;

                // The response accumulates across calls, so a handler that defers may
                // write it whenever it has something to say.
                virtual Progress handle(Request request) = 0;

                // Called instead of a further handle() once the command runs out of
                // time: release anything in flight, nothing will read its result.
                virtual void abandon() {}
        };

        static constexpr uint8_t MAX_PAYLOAD = 64;

        // Reserved by the protocol; not available for immediate data.
        static constexpr uint8_t REG_REQUEST = 0x7E;
        static constexpr uint8_t REG_RESPONSE = 0x7F;

        // Reserved by the transport: no section may define this value.
        static constexpr uint8_t ErrorTimedOut = 0xFF;

        // Kept under the master's own timeout, so its retry finds the mailbox free
        // instead of being turned away.
        static constexpr uint32_t DefaultTimeoutMs = 1500;

        // A `readOnly` instance (a monitoring peer) can only read the bank.
        ControllerSlave(RequestHandler& handler, RegisterBank& bank, const Clock& clock,
                        bool readOnly = false, uint32_t timeoutMs = DefaultTimeoutMs)
            : handler(handler), bank(bank), clock(clock), readOnly(readOnly),
              timeoutMs(timeoutMs) {}

        // --- ISR side ---
        void onAddressMatch(Direction direction) override;
        void onWriteByte(const uint8_t data) override;
        uint8_t onReadByte() override;
        void onEndTransaction() override;
        void onError() override;

        // --- Main-loop side ---
        // Advances the command in flight, one pass per call.
        void process();

    protected:
        enum class Status : uint8_t { Idle = 0, Processing = 1, Ready = 2, Error = 3 };

        RequestHandler& handler;
        RegisterBank& bank;
        const Clock& clock;
        bool readOnly;
        uint32_t timeoutMs;

        // --- Mailbox (shared ISR <-> main loop) ---
        volatile Status cmdStatus = Status::Idle;   // byte 0 of the response
        volatile uint8_t respLen = 0;
        uint8_t respData[MAX_PAYLOAD] = {};
        uint8_t inbox[2 + MAX_PAYLOAD] = {};   // [cmd][len][payload]
        volatile uint16_t inboxLen = 0;
        volatile bool requestPending = false;

        // --- In-flight command (main loop only) ---
        Response response{respData, MAX_PAYLOAD};   // accumulates across process() calls
        bool inFlight = false;
        uint32_t startedMs = 0;

        // --- Per-transaction state (ISR only) ---
        uint8_t regPointer = 0;
        bool pointerSet = false;
        bool writingRequest = false;
        bool readingResponse = false;
        uint16_t byteIndex = 0;

        // Compiler barrier: on a single-core Cortex-M it is enough to order the
        // publication of the outbox before the status.
        static void publishBarrier() { asm volatile("" ::: "memory"); }
};
