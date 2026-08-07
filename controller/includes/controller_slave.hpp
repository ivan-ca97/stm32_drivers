#pragma once

#include <stdint.h>
#include <span>
#include "i2c_slave.hpp"
#include "register_bank.hpp"

// ============================================================================
// ControllerSlave — slave-side register/mailbox protocol over I2C. Command
// semantics come from an injected RequestHandler; this class only moves bytes.
//
// Responsibility split:
//   - This class (ISR, O(1)): register pointer, request (inbox)/response (outbox)
//     buffers, status FSM. Immediate reads are served from a shared RegisterBank.
//   - RequestHandler (main loop): given (cmd, payload) it produces the response.
//
// Protocol (mailbox registers at the top of the range, out of the data path):
//   0x7E (W): request  [cmd][len][payload]
//   0x7F (R): response [status][len][data][filler]   status: 0 Idle,1 Proc,2 Ready,3 Err
//   any other address (R): immediate registers from the shared RegisterBank.
//
// A read-only instance ignores writes to the request register (it still lets a
// master select a register pointer to read).
// ============================================================================
class ControllerSlave : public I2cSlave
{
    public:
        // Response built by a RequestHandler. Wraps the caller's buffer (no
        // allocation, no ownership); tracks how much was written and whether the
        // command failed. The error code is an opaque byte defined by the section.
        class Response
        {
            public:
                Response(uint8_t* buffer, uint8_t capacity)
                    : buffer(buffer), capacity(capacity) {}

                // Append data (bounds-checked, little-endian; a write that would
                // overflow the buffer is dropped whole).
                void put(uint8_t byte);
                void put(const uint8_t* data, uint8_t length);
                void put(std::span<const uint8_t> data);
                void putU16(uint16_t value);
                void putU64(uint64_t value);

                // Mark the command as failed; the payload becomes the error code byte.
                void fail(uint8_t errorCode);

                bool    isError() const { return error; }
                uint8_t length()  const { return len; }

            private:
                uint8_t* buffer;
                uint8_t  capacity;
                uint8_t  len   = 0;
                bool     error = false;
        };

        // A request handed to a RequestHandler: the command + payload, plus access
        // to the response (a writer over the controller's internal buffer).
        class Request
        {
            public:
                Request(uint8_t cmd, std::span<const uint8_t> payload, Response& response)
                    : cmd_(cmd), payload_(payload), response_(response) {}

                uint8_t                  cmd()      const { return cmd_; }
                std::span<const uint8_t> payload()  const { return payload_; }

                // Response writer, over the controller's internal buffer.
                Response&                response() const { return response_; }

            private:
                uint8_t                  cmd_;
                std::span<const uint8_t> payload_;
                Response&                response_;
        };

        // Command semantics interface, implemented by the application. Always
        // runs in the main loop (via process()), never in the ISR.
        class RequestHandler
        {
            public:
                virtual ~RequestHandler() = default;

                // Processes the request and builds its response.
                virtual void handle(Request request) = 0;
        };

        static constexpr uint8_t MAX_PAYLOAD = 64;   // max request/response payload

        // Registers reserved by the protocol (do not use for immediate data).
        // At the top of the range so they stay out of the data path.
        static constexpr uint8_t REG_REQUEST  = 0x7E;
        static constexpr uint8_t REG_RESPONSE = 0x7F;

        // `readOnly` instances (e.g. a monitoring peer) ignore requests: they can
        // only read the bank.
        ControllerSlave(RequestHandler& handler, RegisterBank& bank, bool readOnly = false)
            : handler(handler), bank(bank), readOnly(readOnly) {}

        // ================= ISR-side (called from the interrupt) =================
        void onAddressMatch(Direction direction) override;
        void onWriteByte(const uint8_t data) override;
        uint8_t onReadByte() override;
        void onEndTransaction() override;
        void onError() override;

        // ========================= Main-loop side ==============================
        // Processes the pending request by delegating to the RequestHandler.
        void process();

    protected:
        enum class Status : uint8_t { Idle = 0, Processing = 1, Ready = 2, Error = 3 };

        RequestHandler& handler;
        RegisterBank&   bank;
        bool            readOnly;

        // --- Mailbox (shared ISR <-> main loop) ---
        volatile Status   cmdStatus = Status::Idle;   // byte 0 of the response
        volatile uint8_t  respLen   = 0;
        uint8_t  respData[MAX_PAYLOAD] = {};
        uint8_t  inbox[2 + MAX_PAYLOAD] = {};   // [cmd][len][payload]
        volatile uint16_t inboxLen = 0;
        volatile bool     requestPending = false;

        // --- Per-transaction state (ISR only) ---
        uint8_t  regPointer = 0;
        bool     pointerSet = false;
        bool     writingRequest = false;
        bool     readingResponse = false;
        uint16_t byteIndex = 0;

        // Compiler barrier: on a single-core Cortex-M it is enough to order the
        // publication of the outbox before the status.
        static void publishBarrier() { asm volatile("" ::: "memory"); }
};
