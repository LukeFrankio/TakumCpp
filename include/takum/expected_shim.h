#pragma once

#if __cplusplus >= 202302L
#include <expected>
namespace takum::detail {
    template<typename T>
    using expected = std::expected<T, takum_error>;
}
#else
#include <optional>
#include <variant>
namespace takum::detail {
    template<typename T>
    class expected {
        std::optional<T> value_;
        bool has_value_ = false;
        takum_error error_;

    public:
        expected() noexcept : has_value_(false) {}
        expected(const T& value) noexcept : value_(value), has_value_(true) {}
        expected(takum_error error) noexcept : has_value_(false), error_(error) {}

        bool has_value() const noexcept { return has_value_; }
        const T& value() const noexcept { return value_; }
        takum_error error() const noexcept { return error_; }
        explicit operator bool() const noexcept { return has_value_; }
        const T& value_or(const T& dflt) const noexcept { return has_value_ ? *value_ : dflt; }
    };
}
#endif