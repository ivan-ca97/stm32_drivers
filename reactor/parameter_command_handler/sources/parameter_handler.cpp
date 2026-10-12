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

void ParameterHandler::load()
{
    // Without a stored set, or one that cannot be read, the model keeps its defaults.
    const base::Result<ReactorParams> stored = storage.load();
    if(stored)
        params = *stored;

    for(uint8_t id = 0; id < param::ParamCount; id++)
        (void)publisher.publish(id, params.get(id));   // a failed read-back is not this load's to report
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
    if(!published)
        return rejected(response, param::ErrorVerifyFailed);

    return Progress::Done;   // Ready with an empty payload -> success
}
