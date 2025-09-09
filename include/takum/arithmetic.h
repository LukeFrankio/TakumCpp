/**
 * @file arithmetic.h
 * @brief Basic arithmetic operators and safe variants for takum<N>.
 *
 * This header provides simple, correct Phase‑3 arithmetic implementations
 * that use host `double` intermediates to compute results. Phase‑4 will
 * replace addition/subtraction with a Takum-native Gaussian-log (Φ)
 * implementation for better accuracy and performance.
 */

#pragma once

#include "takum/core.h"

#include <type_traits>
#include <cmath>

namespace takum {

/**
 * Add two takum values.
 * @note Uses host double intermediate; NaR propagates.
 */
template <size_t N>
inline takum<N> operator+(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return takum<N>::nar();
    double da = a.to_double();
    double db = b.to_double();
    if (!std::isfinite(da) || !std::isfinite(db)) return takum<N>::nar();
    return takum<N>(da + db);
}

/**
 * Subtract two takum values.
 */
template <size_t N>
inline takum<N> operator-(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return takum<N>::nar();
    double da = a.to_double();
    double db = b.to_double();
    if (!std::isfinite(da) || !std::isfinite(db)) return takum<N>::nar();
    return takum<N>(da - db);
}

/**
 * Multiply two takum values.
 */
template <size_t N>
inline takum<N> operator*(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return takum<N>::nar();
    double da = a.to_double();
    double db = b.to_double();
    if (!std::isfinite(da) || !std::isfinite(db)) return takum<N>::nar();
    return takum<N>(da * db);
}

/**
 * Divide two takum values. Division by zero yields NaR.
 */
template <size_t N>
inline takum<N> operator/(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return takum<N>::nar();
    double da = a.to_double();
    double db = b.to_double();
    if (!std::isfinite(da) || !std::isfinite(db) || db == 0.0) return takum<N>::nar();
    return takum<N>(da / db);
}

/**
 * Absolute value for takum.
 */
template <size_t N>
inline takum<N> abs(const takum<N>& a) noexcept {
    if (a.is_nar()) return takum<N>::nar();
    double da = a.to_double();
    if (!std::isfinite(da)) return takum<N>::nar();
    return takum<N>(std::fabs(da));
}

/**
 * @name Safe arithmetic variants
 * Functions that provide error handling for arithmetic operations instead of
 * propagating NaR values. These return `std::expected` when available (C++23+)
 * or `std::optional` as a fallback for older standards.
 * 
 * @note These functions return an error when:
 *       - Any operand is NaR
 *       - The result cannot be represented (overflow/underflow to NaR)
 * @note Prefer these functions when explicit error handling is required
 */
//@{
#if __cplusplus >= 202302L
/**
 * @brief Safe addition with explicit error handling.
 * 
 * Performs addition of two takum values with explicit error reporting
 * instead of NaR propagation. Returns std::unexpected on error conditions.
 *
 * @tparam N Bit width of the takum format
 * @param a First operand
 * @param b Second operand
 * @return std::expected containing result or error information
 */
template <size_t N>
inline std::expected<takum<N>, takum_error> safe_add(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR operand"});
    takum<N> r = a + b;
    if (r.is_nar()) return std::unexpected(takum_error{takum_error::Kind::Overflow, "result NaR/overflow"});
    return r;
}

/**
 * @brief Safe multiplication with explicit error handling.
 * 
 * Performs multiplication of two takum values with explicit error reporting
 * instead of NaR propagation. Returns std::unexpected on error conditions.
 *
 * @tparam N Bit width of the takum format
 * @param a First operand
 * @param b Second operand
 * @return std::expected containing result or error information
 */
template <size_t N>
inline std::expected<takum<N>, takum_error> safe_mul(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR operand"});
    takum<N> r = a * b;
    if (r.is_nar()) return std::unexpected(takum_error{takum_error::Kind::Overflow, "result NaR/overflow"});
    return r;
}
#else
/**
 * @brief Safe addition for older C++ standards (returns optional).
 * 
 * Performs addition of two takum values with error checking, returning
 * std::nullopt on error conditions instead of propagating NaR.
 *
 * @tparam N Bit width of the takum format
 * @param a First operand
 * @param b Second operand
 * @return std::optional containing result or nullopt on error
 */
template <size_t N>
inline std::optional<takum<N>> safe_add(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::nullopt;
    takum<N> r = a + b;
    if (r.is_nar()) return std::nullopt;
    return r;
}

/**
 * @brief Safe multiplication for older C++ standards (returns optional).
 * 
 * Performs multiplication of two takum values with error checking, returning
 * std::nullopt on error conditions instead of propagating NaR.
 *
 * @tparam N Bit width of the takum format
 * @param a First operand
 * @param b Second operand
 * @return std::optional containing result or nullopt on error
 */
template <size_t N>
inline std::optional<takum<N>> safe_mul(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::nullopt;
    takum<N> r = a * b;
    if (r.is_nar()) return std::nullopt;
    return r;
}
#endif

// Safe subtraction and division variants
#if __cplusplus >= 202302L
/**
 * @brief Safe subtraction with explicit error handling.
 * 
 * Performs subtraction of two takum values with explicit error reporting
 * instead of NaR propagation. Returns std::unexpected on error conditions.
 *
 * @tparam N Bit width of the takum format
 * @param a Minuend
 * @param b Subtrahend
 * @return std::expected containing result or error information
 */
template <size_t N>
inline std::expected<takum<N>, takum_error> safe_sub(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR operand"});
    takum<N> r = a - b;
    if (r.is_nar()) return std::unexpected(takum_error{takum_error::Kind::Overflow, "result NaR/overflow"});
    return r;
}

/**
 * @brief Safe division with explicit error handling.
 * 
 * Performs division of two takum values with explicit error reporting
 * instead of NaR propagation. Returns std::unexpected on error conditions
 * including division by zero.
 *
 * @tparam N Bit width of the takum format
 * @param a Dividend
 * @param b Divisor
 * @return std::expected containing result or error information
 */
template <size_t N>
inline std::expected<takum<N>, takum_error> safe_div(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR operand"});
    takum<N> r = a / b;
    if (r.is_nar()) return std::unexpected(takum_error{takum_error::Kind::Overflow, "result NaR/overflow"});
    return r;
}
#else
/**
 * @brief Safe subtraction for older C++ standards (returns optional).
 * 
 * Performs subtraction of two takum values with error checking, returning
 * std::nullopt on error conditions instead of propagating NaR.
 *
 * @tparam N Bit width of the takum format
 * @param a Minuend
 * @param b Subtrahend
 * @return std::optional containing result or nullopt on error
 */
template <size_t N>
inline std::optional<takum<N>> safe_sub(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::nullopt;
    takum<N> r = a - b;
    if (r.is_nar()) return std::nullopt;
    return r;
}

/**
 * @brief Safe division for older C++ standards (returns optional).
 * 
 * Performs division of two takum values with error checking, returning
 * std::nullopt on error conditions including division by zero.
 *
 * @tparam N Bit width of the takum format
 * @param a Dividend
 * @param b Divisor
 * @return std::optional containing result or nullopt on error
 */
template <size_t N>
inline std::optional<takum<N>> safe_div(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::nullopt;
    takum<N> r = a / b;
    if (r.is_nar()) return std::nullopt;
    return r;
}
#endif

} // namespace takum
