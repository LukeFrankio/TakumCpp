/**
 * @file math_constants.h  
 * @brief Mathematical constants for takum<N> (Phase 4 deliverable).
 *
 * This header provides mathematical constants in the style of <numbers>
 * (C++20) but specialized for takum types. Constants are computed at
 * compile-time using constexpr where possible.
 *
 * Includes deprecated shims for legacy C-style constants like M_PI
 * with appropriate compile-time warnings.
 */

#pragma once

#include "takum/core.h"
#include "takum/math.h"

#include <type_traits>
#include <numbers>

namespace takum {

/**
 * @brief Mathematical constants namespace for takum types.
 * 
 * Provides mathematical constants specialized for takum<N>.
 * Since takum constructors are not constexpr, these use runtime initialization.
 */
namespace math_constants {

/**
 * @brief π (pi) constant for takum<N>.
 * 
 * @tparam T takum<N> type
 */
template <typename T>
inline T pi_v() { return T(3.1415926535897932384626433832795); }

/**
 * @brief e (Euler's number) constant for takum<N>.
 * 
 * @tparam T takum<N> type  
 */
template <typename T>
inline T e_v() { return T(2.7182818284590452353602874713527); }

/**
 * @brief √2 (square root of 2) constant for takum<N>.
 * 
 * @tparam T takum<N> type
 */
template <typename T>
inline T sqrt2_v() { return T(1.4142135623730950488016887242097); }

/**
 * @brief √3 (square root of 3) constant for takum<N>.
 * 
 * @tparam T takum<N> type
 */
template <typename T>
inline T sqrt3_v() { return T(1.7320508075688772935274463415059); }

/**
 * @brief 1/π constant for takum<N>.
 * 
 * @tparam T takum<N> type
 */
template <typename T>
inline T inv_pi_v() { return T(0.31830988618379067153776752674503); }

/**
 * @brief 1/√π constant for takum<N>.
 * 
 * @tparam T takum<N> type
 */
template <typename T>
inline T inv_sqrtpi_v() { return T(0.56418958354775628694807945156077); }

/**
 * @brief ln(2) (natural logarithm of 2) constant for takum<N>.
 * 
 * @tparam T takum<N> type
 */
template <typename T>
inline T ln2_v() { return T(0.69314718055994530941723212145818); }

/**
 * @brief ln(10) (natural logarithm of 10) constant for takum<N>.
 * 
 * @tparam T takum<N> type
 */
template <typename T>
inline T ln10_v() { return T(2.3025850929940456840179914546844); }

/**
 * @brief √(2π) constant for takum<N>.
 * 
 * @tparam T takum<N> type
 */
template <typename T>
inline T sqrt2pi_v() { return T(2.5066282746310005024157652848110); }

/**
 * @brief 1/√(2π) constant for takum<N>.
 * 
 * @tparam T takum<N> type
 */
template <typename T>
inline T inv_sqrt2pi_v() { return T(0.39894228040143267793994605993439); }

/**
 * @brief Euler-Mascheroni constant γ for takum<N>.
 * 
 * @tparam T takum<N> type
 */
template <typename T>
inline T egamma_v() { return T(0.57721566490153286060651209008240); }

/**
 * @brief φ (golden ratio) constant for takum<N>.
 * 
 * @tparam T takum<N> type
 */
template <typename T>
inline T phi_v() { return T(1.6180339887498948482045868343656); }

} // namespace math_constants

} // namespace takum

// ============================================================================
// DEPRECATED LEGACY CONSTANTS WITH WARNINGS
// ============================================================================

#ifndef TAKUM_NO_LEGACY_MATH_CONSTANTS

/**
 * @deprecated Use takum::math_constants::pi_v<takum<N>> instead.
 * 
 * Legacy M_PI constant mapped to takum<64> for compatibility.
 * This shim will be removed in a future version.
 */
#ifndef M_PI
#define M_PI (takum::math_constants::pi_v<takum::takum<64>>())
#pragma message("Deprecated: M_PI is a legacy C-style constant. Prefer takum::math_constants::pi_v<takum<N>>. Behavior: maps to takum<64> precision.")
#endif

/**
 * @deprecated Use takum::math_constants::e_v<takum<N>> instead.
 * 
 * Legacy M_E constant mapped to takum<64> for compatibility.
 */
#ifndef M_E
#define M_E (takum::math_constants::e_v<takum::takum<64>>())
#pragma message("Deprecated: M_E is a legacy C-style constant. Prefer takum::math_constants::e_v<takum<N>>. Behavior: maps to takum<64> precision.")
#endif

/**
 * @deprecated Use takum::math_constants::ln2_v<takum<N>> instead.
 * 
 * Legacy M_LN2 constant mapped to takum<64> for compatibility.
 */
#ifndef M_LN2
#define M_LN2 (takum::math_constants::ln2_v<takum::takum<64>>())
#pragma message("Deprecated: M_LN2 is a legacy C-style constant. Prefer takum::math_constants::ln2_v<takum<N>>. Behavior: maps to takum<64> precision.")
#endif

/**
 * @deprecated Use takum::math_constants::ln10_v<takum<N>> instead.
 * 
 * Legacy M_LN10 constant mapped to takum<64> for compatibility.
 */
#ifndef M_LN10
#define M_LN10 (takum::math_constants::ln10_v<takum::takum<64>>())
#pragma message("Deprecated: M_LN10 is a legacy C-style constant. Prefer takum::math_constants::ln10_v<takum<N>>. Behavior: maps to takum<64> precision.")
#endif

/**
 * @deprecated Use takum::math_constants::sqrt2_v<takum<N>> instead.
 * 
 * Legacy M_SQRT2 constant mapped to takum<64> for compatibility.
 */
#ifndef M_SQRT2
#define M_SQRT2 (takum::math_constants::sqrt2_v<takum::takum<64>>())
#pragma message("Deprecated: M_SQRT2 is a legacy C-style constant. Prefer takum::math_constants::sqrt2_v<takum<N>>. Behavior: maps to takum<64> precision.")
#endif

#endif // TAKUM_NO_LEGACY_MATH_CONSTANTS