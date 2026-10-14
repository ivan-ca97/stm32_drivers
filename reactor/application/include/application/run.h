#pragma once

namespace application {

// Builds the whole system on this board and runs it. Returns only by not returning: a
// configuration that cannot be built stops here, with SCRAM never released.
[[noreturn]] void run();

} // namespace application
