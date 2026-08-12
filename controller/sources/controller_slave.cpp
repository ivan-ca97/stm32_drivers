#include "controller_slave.hpp"

// -------------------------------- ISR side ----------------------------------

void ControllerSlave::onAddressMatch(Direction direction)
{
    byteIndex = 0;
    readingResponse = false;
    if(direction == Direction::RX)
        pointerSet = false;   // write phase: the register pointer arrives first
}

void ControllerSlave::onWriteByte(const uint8_t data)
{
    if(!pointerSet)
    {
        regPointer = data;
        pointerSet = true;
        byteIndex = 0;
        writingRequest = (!readOnly && regPointer == REG_REQUEST
                          && !requestPending && cmdStatus == Status::Idle);
        if(writingRequest)
            inboxLen = 0;
        return;
    }

    if(writingRequest && inboxLen < sizeof(inbox))
        inbox[inboxLen++] = data;
    // writes to read-only registers are ignored
}

uint8_t ControllerSlave::onReadByte()
{
    const uint16_t i = byteIndex++;

    if(regPointer == REG_RESPONSE)   // [status][len][data][filler]
    {
        readingResponse = true;
        if(i == 0) return static_cast<uint8_t>(cmdStatus);
        if(i == 1) return respLen;
        if(i >= 2 && (i - 2) < respLen) return respData[i - 2];
        return 0x00;   // filler
    }

    return bank.read(regPointer + i);   // immediate registers
}

void ControllerSlave::onEndTransaction()
{
    if(writingRequest)
    {
        writingRequest = false;
        // Request complete -> ask for processing. Re-checked here because the
        // mailbox may have been taken while the frame was still arriving.
        if(inboxLen >= 2 && cmdStatus == Status::Idle)
        {
            requestPending = true;
            cmdStatus = Status::Processing;
        }
    }
    else if(readingResponse && (cmdStatus == Status::Ready || cmdStatus == Status::Error))
    {
        // The read is the ack: any read of a Ready/Error response returns to Idle,
        // so the master must read the whole response in a single transaction.
        cmdStatus = Status::Idle;
    }

    pointerSet = false;
    writingRequest = false;
    readingResponse = false;
    byteIndex = 0;
}

void ControllerSlave::onError()
{
    if(writingRequest && cmdStatus != Status::Processing)
        inboxLen = 0;   // drop the partial request
    pointerSet = false;
    writingRequest = false;
    readingResponse = false;
    byteIndex = 0;
}

// ------------------------------ Main-loop side -------------------------------

void ControllerSlave::process()
{
    if(requestPending)
    {
        requestPending = false;
        inFlight = true;
        startedMs = clock.milliseconds();
        response = Response(respData, MAX_PAYLOAD);
    }

    if(!inFlight)
        return;

    // The inbox stays put while a command is in flight: the ISR only takes a new
    // request when the mailbox is Idle, so re-reading it every pass is safe.
    const uint8_t cmd         = inbox[0];
    const uint8_t declaredLen = inbox[1];
    const uint8_t availLen    = (inboxLen >= 2) ? (uint8_t)(inboxLen - 2) : 0;
    const uint8_t payloadLen  = (declaredLen <= availLen) ? declaredLen : availLen;

    std::span<const uint8_t> payload(&inbox[2], payloadLen);
    Request request(cmd, payload, response);

    if(handler.handle(request) == RequestHandler::Progress::Pending)
    {
        if(clock.millisecondsSince(startedMs) < timeoutMs)
            return;   // still working, still within its budget

        handler.abandon();
        response = Response(respData, MAX_PAYLOAD);
        response.fail(ErrorTimedOut);
    }

    inFlight = false;

    publishBarrier();   // respData/respLen visible BEFORE publishing the status
    respLen   = response.length();
    cmdStatus = response.isError() ? Status::Error : Status::Ready;
}

// -------------------------------- Response ----------------------------------

void ControllerSlave::Response::put(uint8_t byte)
{
    if(len + 1 > capacity) return;
    buffer[len++] = byte;
}

void ControllerSlave::Response::put(const uint8_t* data, uint8_t length)
{
    if(len + length > capacity) return;
    for(uint8_t i = 0; i < length; i++)
        buffer[len++] = data[i];
}

void ControllerSlave::Response::put(std::span<const uint8_t> data)
{
    put(data.data(), static_cast<uint8_t>(data.size()));
}

void ControllerSlave::Response::putU16(uint16_t value)
{
    if(len + 2 > capacity) return;
    buffer[len++] = uint8_t(value);
    buffer[len++] = uint8_t(value >> 8);
}

void ControllerSlave::Response::putU64(uint64_t value)
{
    if(len + 8 > capacity) return;
    for(unsigned i = 0; i < 8; i++)
        buffer[len++] = uint8_t(value >> (8 * i));
}

void ControllerSlave::Response::fail(uint8_t errorCode)
{
    error = true;
    len   = 0;
    put(errorCode);
}
