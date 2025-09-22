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
#include "math.h"

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

// ============================================================================
// DEPRECATED C-STYLE MATH FUNCTION SHIMS
// ============================================================================

#ifndef TAKUM_NO_LEGACY_MATH_FUNCTIONS

/**
 * @brief Legacy C-style math function shims for takum types.
 * 
 * These functions provide compatibility for legacy C-style function calls
 * but map to takum<64> by default. They emit deprecation warnings and should
 * be replaced with explicit takum::function calls.
 * 
 * @deprecated All functions in this section are deprecated. Use takum::math
 * functions with explicit type parameters instead.
 */

// Trigonometric functions
#define sinf(x) (takum::sin(takum::takum<32>(x)))
#define sin(x) (takum::sin(takum::takum<64>(x)))  
#define sinl(x) (takum::sin(takum::takum<128>(x)))

#pragma message("Deprecated: sinf/sin/sinl are legacy C-style functions. Prefer takum::sin<N>. Behavior: maps to takum<32/64/128> respectively.")

#define cosf(x) (takum::cos(takum::takum<32>(x)))
#define cos(x) (takum::cos(takum::takum<64>(x)))
#define cosl(x) (takum::cos(takum::takum<128>(x)))

#pragma message("Deprecated: cosf/cos/cosl are legacy C-style functions. Prefer takum::cos<N>. Behavior: maps to takum<32/64/128> respectively.")

#define tanf(x) (takum::tan(takum::takum<32>(x)))
#define tan(x) (takum::tan(takum::takum<64>(x)))
#define tanl(x) (takum::tan(takum::takum<128>(x)))

#pragma message("Deprecated: tanf/tan/tanl are legacy C-style functions. Prefer takum::tan<N>. Behavior: maps to takum<32/64/128> respectively.")

#define asinf(x) (takum::asin(takum::takum<32>(x)))
#define asin(x) (takum::asin(takum::takum<64>(x)))
#define asinl(x) (takum::asin(takum::takum<128>(x)))

#pragma message("Deprecated: asinf/asin/asinl are legacy C-style functions. Prefer takum::asin<N>. Behavior: maps to takum<32/64/128> respectively.")

#define acosf(x) (takum::acos(takum::takum<32>(x)))
#define acos(x) (takum::acos(takum::takum<64>(x)))
#define acosl(x) (takum::acos(takum::takum<128>(x)))

#pragma message("Deprecated: acosf/acos/acosl are legacy C-style functions. Prefer takum::acos<N>. Behavior: maps to takum<32/64/128> respectively.")

#define atanf(x) (takum::atan(takum::takum<32>(x)))
#define atan(x) (takum::atan(takum::takum<64>(x)))
#define atanl(x) (takum::atan(takum::takum<128>(x)))

#pragma message("Deprecated: atanf/atan/atanl are legacy C-style functions. Prefer takum::atan<N>. Behavior: maps to takum<32/64/128> respectively.")

#define atan2f(y,x) (takum::atan2(takum::takum<32>(y), takum::takum<32>(x)))
#define atan2(y,x) (takum::atan2(takum::takum<64>(y), takum::takum<64>(x)))
#define atan2l(y,x) (takum::atan2(takum::takum<128>(y), takum::takum<128>(x)))

#pragma message("Deprecated: atan2f/atan2/atan2l are legacy C-style functions. Prefer takum::atan2<N>. Behavior: maps to takum<32/64/128> respectively.")

// Hyperbolic functions
#define sinhf(x) (takum::sinh(takum::takum<32>(x)))
#define sinh(x) (takum::sinh(takum::takum<64>(x)))
#define sinhl(x) (takum::sinh(takum::takum<128>(x)))

#pragma message("Deprecated: sinhf/sinh/sinhl are legacy C-style functions. Prefer takum::sinh<N>. Behavior: maps to takum<32/64/128> respectively.")

#define coshf(x) (takum::cosh(takum::takum<32>(x)))
#define cosh(x) (takum::cosh(takum::takum<64>(x)))
#define coshl(x) (takum::cosh(takum::takum<128>(x)))

#pragma message("Deprecated: coshf/cosh/coshl are legacy C-style functions. Prefer takum::cosh<N>. Behavior: maps to takum<32/64/128> respectively.")

#define tanhf(x) (takum::tanh(takum::takum<32>(x)))
#define tanh(x) (takum::tanh(takum::takum<64>(x)))
#define tanhl(x) (takum::tanh(takum::takum<128>(x)))

#pragma message("Deprecated: tanhf/tanh/tanhl are legacy C-style functions. Prefer takum::tanh<N>. Behavior: maps to takum<32/64/128> respectively.")

// Exponential and logarithmic functions
#define expf(x) (takum::exp(takum::takum<32>(x)))
#define exp(x) (takum::exp(takum::takum<64>(x)))
#define expl(x) (takum::exp(takum::takum<128>(x)))

#pragma message("Deprecated: expf/exp/expl are legacy C-style functions. Prefer takum::exp<N>. Behavior: maps to takum<32/64/128> respectively.")

#define logf(x) (takum::log(takum::takum<32>(x)))
#define log(x) (takum::log(takum::takum<64>(x)))
#define logl(x) (takum::log(takum::takum<128>(x)))

#pragma message("Deprecated: logf/log/logl are legacy C-style functions. Prefer takum::log<N>. Behavior: maps to takum<32/64/128> respectively, NaR on non-positive.")

#define log10f(x) (takum::log10(takum::takum<32>(x)))
#define log10(x) (takum::log10(takum::takum<64>(x)))
#define log10l(x) (takum::log10(takum::takum<128>(x)))

#pragma message("Deprecated: log10f/log10/log10l are legacy C-style functions. Prefer takum::log10<N>. Behavior: maps to takum<32/64/128> respectively.")

// Power and root functions  
#define powf(x,y) (takum::pow(takum::takum<32>(x), takum::takum<32>(y)))
#define pow(x,y) (takum::pow(takum::takum<64>(x), takum::takum<64>(y)))
#define powl(x,y) (takum::pow(takum::takum<128>(x), takum::takum<128>(y)))

#pragma message("Deprecated: powf/pow/powl are legacy C-style functions. Prefer takum::pow<N>. Behavior: maps to takum<32/64/128> respectively. Note: pow(NaN,0) -> NaR (differs from C++26).")

#define sqrtf(x) (takum::sqrt(takum::takum<32>(x)))
#define sqrt(x) (takum::sqrt(takum::takum<64>(x)))
#define sqrtl(x) (takum::sqrt(takum::takum<128>(x)))

#pragma message("Deprecated: sqrtf/sqrt/sqrtl are legacy C-style functions. Prefer takum::sqrt<N>. Behavior: maps to takum<32/64/128> respectively.")

// Rounding functions
#define floorf(x) (takum::floor(takum::takum<32>(x)))
#define floor(x) (takum::floor(takum::takum<64>(x)))
#define floorl(x) (takum::floor(takum::takum<128>(x)))

#pragma message("Deprecated: floorf/floor/floorl are legacy C-style functions. Prefer takum::floor<N>. Behavior: maps to takum<32/64/128> respectively.")

#define ceilf(x) (takum::ceil(takum::takum<32>(x)))
#define ceil(x) (takum::ceil(takum::takum<64>(x)))
#define ceill(x) (takum::ceil(takum::takum<128>(x)))

#pragma message("Deprecated: ceilf/ceil/ceill are legacy C-style functions. Prefer takum::ceil<N>. Behavior: maps to takum<32/64/128> respectively.")

#define roundf(x) (takum::round(takum::takum<32>(x)))
#define round(x) (takum::round(takum::takum<64>(x)))
#define roundl(x) (takum::round(takum::takum<128>(x)))

#pragma message("Deprecated: roundf/round/roundl are legacy C-style functions. Prefer takum::round<N>. Behavior: maps to takum<32/64/128> respectively.")

#define truncf(x) (takum::trunc(takum::takum<32>(x)))
#define trunc(x) (takum::trunc(takum::takum<64>(x)))
#define truncl(x) (takum::trunc(takum::takum<128>(x)))

#pragma message("Deprecated: truncf/trunc/truncl are legacy C-style functions. Prefer takum::trunc<N>. Behavior: maps to takum<32/64/128> respectively.")

// Note: Classification function shims are not provided as macros due to conflicts
// with std::isnan, std::isinf, std::isfinite. Use takum::isnan, takum::isinf, 
// takum::isfinite directly for takum types.

#pragma message("Note: Classification functions (isnan, isinf, isfinite) are not shimmed due to std:: conflicts. Use takum::isnan, takum::isinf, takum::isfinite directly.")

#endif // TAKUM_NO_LEGACY_MATH_FUNCTIONS