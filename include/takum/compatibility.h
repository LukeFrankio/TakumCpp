#pragma once

/**
 * @file compatibility.h
 * @brief Backwards-compatibility helpers and shims for legacy APIs.
 *
 * This header contains short-lived aliases and compatibility shims that help
 * older consumers migrate to the core `takum<N>` types. It includes deprecated
 * type aliases and fallback implementations for older C++ standards.
 *
 * @deprecated Items in this header are deprecated. Use core takum<N> types directly.
 *
 * **Namespace Usage Guidance:**
 * Some test files use `using namespace takum::types;` while others use explicit 
 * qualification like `takum::types::takum32`. Both patterns are acceptable:
 * - Use `using namespace` for brevity when no ambiguity exists
 * - Use explicit qualification to avoid naming conflicts or improve clarity
 * - Choose consistently within each translation unit
 *
 * @note Prefer using the core `takum<N>` types directly in new code
 * @note This header may be removed in future versions after deprecation periods
 */

#include "core.h"
#include "types.h"

/**
 * @typedef float8_t
 * @brief Deprecated shim for non-standard 8-bit floating point using takum<8>.
 *
 * This type alias provides compatibility for legacy code expecting a float8_t
 * type. It maps to takum<8> with appropriate zero-padding for the 8-bit format.
 *
 * @deprecated Use takum<8> directly instead of float8_t
 * @note Uses ghost bits (zero-padding) in the packed representation
 */
using float8_t = takum::takum<8>;

#pragma message("Deprecated: float8_t is a shim for non-standard float8 using takum<8> with ghost bits. Prefer takum<8>.")

#if !TAKUM_HAS_STD_EXPECTED
/**
 * @brief Compatibility shim for std::expected when C++23 is not available.
 *
 * @deprecated This compatibility shim will be removed when C++23 becomes
 * the minimum supported standard. Use std::expected directly when available.
 *
 * Provides a minimal expected-like interface for use with takum types on
 * older C++ standards. Only implements the subset of functionality needed
 * by the takum library.
 */
namespace takum {
/**
 * @brief Lightweight std::expected-like type for pre-C++23 compatibility.
 *
 * @deprecated This compatibility type will be removed when C++23 becomes
 * the minimum supported standard. Use std::expected directly when available.
 *
 * Provides basic expected/error handling functionality when std::expected
 * is not available. Supports the core operations needed by takum library.
 *
 * @tparam T The success value type
 * @tparam E The error type
 */
template <typename T, typename E>
struct expected_shim {
    std::optional<T> value;
    E error;

    bool has_value() const { return value.has_value(); }
    T value_or(const T& default_val) const { return value.value_or(default_val); }
    const T& operator*() const { return *value; }
};

} // namespace takum
#endif