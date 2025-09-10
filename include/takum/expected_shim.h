#pragma once

/**
 * @file expected_shim.h
 * @brief Cross-standard compatibility for std::expected functionality.
 *
 * This header provides a consistent interface for expected-like error handling
 * across different C++ standards. For C++23 and later, it aliases std::expected.
 * For older standards, it provides a compatible implementation with the subset
 * of functionality used by the takum library.
 *
 * @note The shim implementation only supports the operations actually used
 *       by takum functions and may not be a complete std::expected replacement
 */

#include "takum/compiler_detection.h"
#if TAKUM_HAS_STD_EXPECTED
#include <expected>
/**
 * @namespace takum::detail
 * @brief Internal implementation details for the takum library.
 */
namespace takum::detail {
    /**
     * @brief Type alias for std::expected specialized for takum error handling.
     * @tparam T The success value type
     */
    template<typename T>
    using expected = std::expected<T, takum_error>;
}
#else
#include <optional>
#include <variant>
namespace takum::detail {
    /**
     * @brief Fallback implementation of expected-like type for pre-C++23.
     *
     * Provides a minimal subset of std::expected functionality for compatibility
     * with older C++ standards. Only implements the operations needed by the
     * takum library.
     *
     * @tparam T The success value type
     */
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