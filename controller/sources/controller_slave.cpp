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
        // Start buffering a request only if not read-only and no request is
        // already in flight (single slot): a new request while busy is dropped,
        // leaving the in-flight one intact.
        const bool busy = requestPending || cmdStatus == Status::Processing;
        writingRequest = (!readOnly && regPointer == REG_REQUEST && !busy);
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

    if(regPointer == REG_RESPONSE)               // 0x11: [status][len][data][filler]
    {
        readingResponse = true;
        if(i == 0) return static_cast<uint8_t>(cmdStatus);
        if(i == 1) return respLen;
        if(i >= 2 && (i - 2) < respLen) return respData[i - 2];
        return 0x00;                             // filler
    }

    return bank.read(regPointer + i);            // immediate registers
}

void ControllerSlave::onEndTransaction()
{
    if(writingRequest)
    {
        writingRequest = false;
        // Request complete -> ask for processing, unless one is already running.
        if(inboxLen >= 2 && cmdStatus != Status::Processing)
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
    if(!requestPending)
        return;
    requestPending = false;

    const uint8_t cmd         = inbox[0];
    const uint8_t declaredLen = inbox[1];
    const uint8_t availLen    = (inboxLen >= 2) ? (uint8_t)(inboxLen - 2) : 0;
    const uint8_t payloadLen  = (declaredLen <= availLen) ? declaredLen : availLen;

    Response response(respData, MAX_PAYLOAD);

    std::span<const uint8_t> payload(&inbox[2], payloadLen);
    Request request(cmd, payload, response);
    handler.handle(request);

    publishBarrier();           // respData/respLen visible BEFORE publishing the status
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
