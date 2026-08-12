#include "router.hpp"

bool Router::addSection(uint8_t low, uint8_t high, RequestHandler& handler)
{
    if(count >= MAX_SECTIONS || low > high)
        return false;

    // Reject a range that overlaps an already-registered section (interval
    // intersection). Opcodes map to exactly one section, so overlap is a config error.
    for(uint8_t i = 0; i < count; i++)
        if(low <= sections[i].high && sections[i].low <= high)
            return false;

    sections[count++] = { low, high, &handler };
    return true;
}

Router::Progress Router::handle(ControllerSlave::Request request)
{
    const uint8_t cmd = request.cmd();
    for(uint8_t i = 0; i < count; i++)
    {
        if(cmd < sections[i].low || cmd > sections[i].high)
            continue;

        const Progress progress = sections[i].handler->handle(request);
        deferredSection = (progress == Progress::Pending) ? sections[i].handler : nullptr;
        return progress;
    }

    request.response().fail(unknownCommandCode);
    return Progress::Done;
}

void Router::abandon()
{
    if(deferredSection == nullptr)
        return;

    deferredSection->abandon();
    deferredSection = nullptr;
}
