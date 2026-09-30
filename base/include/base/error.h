#pragma once

namespace base {

// One static instance per error domain.
class ErrorCategory {
public:
    virtual ~ErrorCategory() = default;

    virtual const char* name() const = 0;

    // Returns a static string for `code`, or a fallback for unknown values.
    virtual const char* describe(int code) const = 0;
};

class Error {
public:
    constexpr Error(const ErrorCategory& category, int code)
        : category_(&category), code_(code)
    {
    }

    const char* domain() const { return category_->name(); }
    const char* message() const { return category_->describe(code_); }

    constexpr int code() const { return code_; }

    // Whether code() is one of `category`'s, and so safe to cast to its enum.
    constexpr bool belongsTo(const ErrorCategory& category) const
    {
        return category_ == &category;
    }

    constexpr bool operator==(const Error&) const = default;

private:
    const ErrorCategory* category_;
    int code_;
};

} // namespace base
