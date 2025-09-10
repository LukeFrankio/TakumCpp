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
#include "takum/internal/phi_eval.h" // Phase-4 Φ evaluator
#include "takum/config.h"

#include <type_traits>
#include <cmath>

namespace takum {

/**
 * @brief Add two takum values (Phase‑4 Φ path with fallback).
 * Primary path: Gaussian‑log helper Φ to reduce double rounding.
 * Fallback: exact log-sum-exp in ℓ space when budget exceeded.
 */
template <size_t N>
inline takum<N> operator+(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return takum<N>::nar();

    long double ell_a = a.get_exact_ell();
    long double ell_b = b.get_exact_ell();
    if (!std::isfinite((double)ell_a) || !std::isfinite((double)ell_b)) {
        double da = a.to_double();
        double db = b.to_double();
        if (!std::isfinite(da) || !std::isfinite(db)) return takum<N>::nar();
        return takum<N>(da + db);
    }

    bool Sa = (ell_a < 0.0L);
    bool Sb = (ell_b < 0.0L);
    long double mag_a = std::fabsl(ell_a);
    long double mag_b = std::fabsl(ell_b);
    if (mag_b > mag_a) { std::swap(mag_a, mag_b); std::swap(Sa, Sb); }
    if (mag_a == mag_b && Sa != Sb) return takum<N>{}; // perfect cancellation

    long double ratio = (mag_a == 0.0L) ? 0.0L : (mag_b / mag_a);
    if (ratio < 1e-6L) { // negligible addend
        long double final_ell = (Sa ? -mag_a : mag_a);
        return takum<N>::from_ell(Sa, final_ell);
    }

    long double t = (ratio - 0.5L); // map to [-0.5,0.5]
    // NOTE: inside namespace takum, qualifying again with takum:: would
    // select the class template name (ambiguous with namespace). Use the
    // relative qualification to reach internal::phi helpers.
    auto phi_res = internal::phi::phi_eval<N>(t);
    bool ok = internal::phi::within_phi_budget<N>(phi_res);
    internal::phi::record_phi<N>(phi_res, ok);

    long double ell_res_mag;
    bool force_fallback = false;
    if (ok) {
        long double s = (Sa == Sb) ? 1.0L : -1.0L;
        long double adj = s * ratio - 0.5L * ratio * ratio; // 2nd-order
        long double blended = adj * phi_res.value;
        ell_res_mag = mag_a + blended;
        if (ell_res_mag < 0.0L) force_fallback = true;
    }
    if (!ok || force_fallback) {
        long double maxv = mag_a;
        long double minv = mag_b;
        long double s = (Sa == Sb) ? 1.0L : -1.0L;
        long double diff = minv - maxv;
        long double z = s * std::expl(diff);
        if (std::fabsl(z) < 1e-24L) ell_res_mag = maxv; else {
            long double arg = 1.0L + z;
            if (arg <= 0.0L) return (arg == 0.0L) ? takum<N>{} : takum<N>::nar();
            ell_res_mag = maxv + std::logl(arg);
        }
    }
    long double final_ell = (Sa ? -ell_res_mag : ell_res_mag);
    return takum<N>::from_ell(Sa, final_ell);
}

/**
 * @brief Subtract two takum values (a - b) via addition with sign flip.
 */
template <size_t N>
inline takum<N> operator-(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return takum<N>::nar();
    long double eb = b.get_exact_ell();
    if (!std::isfinite((double)eb)) {
        double da = a.to_double();
        double db = b.to_double();
        if (!std::isfinite(da) || !std::isfinite(db)) return takum<N>::nar();
        return takum<N>(da - db);
    }
    bool Sb = (eb < 0.0L);
    long double mb = std::fabsl(eb);
    takum<N> negb = takum<N>::from_ell(!Sb, (Sb ? mb : -mb));
    return a + negb;
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

template <size_t N>
inline std::expected<takum<N>, takum_error> safe_sub(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR operand"});
    takum<N> r = a - b;
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

template <size_t N>
inline std::expected<takum<N>, takum_error> safe_div(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR operand"});
    if (b.to_double() == 0.0) return std::unexpected(takum_error{takum_error::Kind::DomainError, "division by zero"});
    takum<N> r = a / b;
    if (r.is_nar()) return std::unexpected(takum_error{takum_error::Kind::Overflow, "result NaR/overflow"});
    return r;
}

/**
 * @brief Safe absolute value.
 */
template <size_t N>
inline std::expected<takum<N>, takum_error> safe_abs(const takum<N>& a) noexcept {
    if (a.is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR operand"});
    return takum<N>(std::fabs(a.to_double()));
}

/**
 * @brief Safe reciprocal (reports domain error on zero or NaR input).
 */
template <size_t N>
inline std::expected<takum<N>, takum_error> safe_recip(const takum<N>& a) noexcept {
    if (a.is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR operand"});
    if (a.to_double() == 0.0) return std::unexpected(takum_error{takum_error::Kind::DomainError, "reciprocal of zero"});
    takum<N> r = a.reciprocal();
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

template <size_t N>
inline std::optional<takum<N>> safe_sub(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::nullopt;
    takum<N> r = a - b;
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

template <size_t N>
inline std::optional<takum<N>> safe_div(const takum<N>& a, const takum<N>& b) noexcept {
    if (a.is_nar() || b.is_nar()) return std::nullopt;
    if (b.to_double() == 0.0) return std::nullopt;
    takum<N> r = a / b;
    if (r.is_nar()) return std::nullopt;
    return r;
}
#endif // __cplusplus >= 202302L

// Optional path (pre-C++23) safe_abs & safe_recip equivalents
#if __cplusplus < 202302L
template <size_t N>
inline std::optional<takum<N>> safe_abs(const takum<N>& a) noexcept {
    if (a.is_nar()) return std::nullopt;
    return takum<N>(std::fabs(a.to_double()));
}

template <size_t N>
inline std::optional<takum<N>> safe_recip(const takum<N>& a) noexcept {
    if (a.is_nar()) return std::nullopt;
    if (a.to_double() == 0.0) return std::nullopt;
    takum<N> r = a.reciprocal();
    if (r.is_nar()) return std::nullopt;
    return r;
}
#endif // __cplusplus >= 202302L

} // namespace takum
