#pragma once

namespace base {

// A permit only `Holder` can issue.
//
// It leaves a function reachable without leaving it callable: the signature
// stays public, so a container that builds in place can find the constructor,
// and nothing outside `Holder` can produce the argument it asks for.
template <typename Holder>
class Passkey {
private:
    Passkey() = default;

    friend Holder;
};

} // namespace base
