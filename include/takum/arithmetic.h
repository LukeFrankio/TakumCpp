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
 * @name Safe variants
 * Functions returning `std::expected` when available or `std::optional` as a
 * fallback. These return an error when operands are NaR or the result cannot
 * be represented.
 */
//@{
#if __cplusplus >= 202302L
/**
 * Safe addition returning std::expected<takum, takum_error>.
 */
template <size_t N>
inline std::expected<takum<N>, takum_error> safe_add(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR operand"});
    takum<N> r = a + b;
    if (r.is_nar()) return std::unexpected(takum_error{takum_error::Kind::Overflow, "result NaR/overflow"});
    return r;
}

/**
 * Safe multiplication returning std::expected<takum, takum_error>.
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
 * Safe addition returning std::optional<takum> on older standards.
 */
template <size_t N>
inline std::optional<takum<N>> safe_add(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::nullopt;
    takum<N> r = a + b;
    if (r.is_nar()) return std::nullopt;
    return r;
}

/**
 * Safe multiplication returning std::optional<takum> on older standards.
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
template <size_t N>
inline std::expected<takum<N>, takum_error> safe_sub(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR operand"});
    takum<N> r = a - b;
    if (r.is_nar()) return std::unexpected(takum_error{takum_error::Kind::Overflow, "result NaR/overflow"});
    return r;
}

template <size_t N>
inline std::expected<takum<N>, takum_error> safe_div(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR operand"});
    takum<N> r = a / b;
    if (r.is_nar()) return std::unexpected(takum_error{takum_error::Kind::Overflow, "result NaR/overflow"});
    return r;
}
#else
template <size_t N>
inline std::optional<takum<N>> safe_sub(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::nullopt;
    takum<N> r = a - b;
    if (r.is_nar()) return std::nullopt;
    return r;
}

template <size_t N>
inline std::optional<takum<N>> safe_div(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::nullopt;
    takum<N> r = a / b;
    if (r.is_nar()) return std::nullopt;
    return r;
}
#endif

} // namespace takum
