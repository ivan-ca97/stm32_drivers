#include "parameter_handler.hpp"

#include <cstring>

namespace
{
    uint64_t readU64le(const uint8_t* p)
    {
        uint64_t value = 0;
        for(unsigned i = 0; i < 8; i++)
            value |= uint64_t(p[i]) << (8 * i);
        return value;
    }

    ParameterHandler::Progress rejected(ControllerSlave::Response& response, uint8_t code)
    {
        response.fail(code);
        return ParameterHandler::Progress::Done;
    }
}

bool ParameterHandler::persist()
{
    // Snapshot of the whole set (flat order), one float32 per param.
    uint8_t buffer[param::ParamCount * param::ValueSize];
    for(uint8_t id = 0; id < param::ParamCount; id++)
    {
        float value = params.get(id);
        std::memcpy(buffer + id * param::ValueSize, &value, param::ValueSize);
    }
    return storage.write(buffer, sizeof(buffer));
}

void ParameterHandler::load()
{
    uint8_t buffer[param::ParamCount * param::ValueSize];
    uint32_t n = storage.read(buffer, sizeof(buffer));
    if(n < sizeof(buffer))
        return;   // nothing persisted yet (or short) -> keep defaults

    for(uint8_t id = 0; id < param::ParamCount; id++)
    {
        float value;
        std::memcpy(&value, buffer + id * param::ValueSize, param::ValueSize);
        if(params.set(id, value))
            (void)publisher.publish(id, value);   // a failed read-back is not this load's to report
    }
}

ParameterHandler::Progress ParameterHandler::handle(ControllerSlave::Request request)
{
    ControllerSlave::Response& response = request.response();

    if(request.cmd() != param::CmdSetParam)
        return rejected(response, param::ErrorMalformed);

    // [token(8)][paramId(1)][value(4)]
    const auto payload = request.payload();
    constexpr uint8_t need = param::TokenSize + 1 + param::ValueSize;
    if(payload.size() < need)
        return rejected(response, param::ErrorMalformed);

    const uint64_t token = readU64le(payload.data());
    const uint8_t paramId = payload[param::TokenSize];
    float value;
    std::memcpy(&value, &payload[param::TokenSize + 1], param::ValueSize);

    if(!validateToken(token))
        return rejected(response, param::ErrorInvalidToken);
    if(paramId >= param::ParamCount)
        return rejected(response, param::ErrorInvalidParamId);
    if(!params.set(paramId, value))   // typed model validates the value
        return rejected(response, param::ErrorInvalidValue);

    const base::Status published = publisher.publish(paramId, value);   // writes + reads back

    if(!persist())
        return rejected(response, param::ErrorInternal);
    if(!published)
        return rejected(response, param::ErrorVerifyFailed);

    return Progress::Done;   // Ready with an empty payload -> success
}
