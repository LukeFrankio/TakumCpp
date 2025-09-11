/**
 * @file arithmetic.h
 * @brief Basic arithmetic operators and safe variants for takum<N>.
 *
 * @deprecated References to "Phase‑3" and "Phase‑4" are deprecated. This header
 * provides the current takum arithmetic implementation with optimized Φ-based
 * addition and standard floating-point fallbacks.
 *
 * This header provides arithmetic implementations that use host `double`
 * intermediates for multiplication and division, with specialized Gaussian-log (Φ)
 * evaluation for addition and subtraction to improve accuracy and performance.
 */

#pragma once

#include "takum/core.h"
#include "takum/internal/phi_eval.h" // Φ evaluator for enhanced addition
#include "takum/config.h"

#include <type_traits>
#include <cmath>

namespace takum {

/**
 * @brief Add two takum values (Φ-enhanced path with fallback).
 * 
 * @deprecated The term "Phase‑4 Φ path" is deprecated. This implements the
 * current takum addition algorithm using Gaussian‑log (Φ) evaluation.
 * 
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
    long double mag_a = fabsl(ell_a);
    long double mag_b = fabsl(ell_b);
    
    // Handle cases where one operand is 1.0 (ell = 0)
    if (mag_a == 0.0L && mag_b == 0.0L) {
        // Both are 1.0 or -1.0: result is 2.0 or 0.0
        return takum<N>((Sa ? -1.0 : 1.0) + (Sb ? -1.0 : 1.0));
    }
    if (mag_a == 0.0L || mag_b == 0.0L) {
        // One operand is ±1.0, but still exercise Phi path for diagnostics
        // Fall through to normal path with ratio = 0
        if (mag_a == 0.0L) {
            std::swap(mag_a, mag_b); 
            std::swap(Sa, Sb);
        }
        // Now mag_a > 0, mag_b = 0, so ratio = 0
    } else {
        if (mag_b > mag_a) { std::swap(mag_a, mag_b); std::swap(Sa, Sb); }
    }
    
    if (mag_a == mag_b && Sa != Sb) return takum<N>{}; // perfect cancellation

    long double ratio = (mag_a == 0.0L) ? 0.0L : (mag_b / mag_a);
    
    // Check if the smaller value is negligible compared to the larger value
    // Convert ell magnitudes back to actual value magnitudes for comparison
    long double val_a = (mag_a == 0.0L) ? 1.0L : expl(mag_a / 2.0L);
    long double val_b = (mag_b == 0.0L) ? 1.0L : expl(mag_b / 2.0L);
    long double val_ratio = val_b / val_a;
    
    if (val_ratio < 1e-6L) { // negligible addend in actual value space
        return takum<N>::from_ell(Sa, mag_a);
    }

    long double t = (ratio - 0.5L); // map to [-0.5,0.5]
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
        // For simple cases, use double arithmetic to maintain exact precision
        double da = a.to_double();
        double db = b.to_double();
        double result = da + db;
        if (std::isfinite(result)) {
            return takum<N>(result);
        }
        
        // Fallback to log-sum-exp for edge cases
        long double log_a = mag_a / 2.0L;  // log(|a|) = ell_a/2
        long double log_b = mag_b / 2.0L;  // log(|b|) = ell_b/2
        long double s = (Sa == Sb) ? 1.0L : -1.0L;
        long double diff = log_b - log_a;  // log(|b|/|a|)
        long double z = s * expl(diff);    // ±|b|/|a|
        if (fabsl(z) < 1e-24L) {
            ell_res_mag = mag_a;
        } else {
            long double arg = 1.0L + z;
            if (arg <= 0.0L) return (arg == 0.0L) ? takum<N>{} : takum<N>::nar();
            ell_res_mag = 2.0L * (log_a + logl(arg));  // Convert back to ell = 2*log
        }
    }
    return takum<N>::from_ell(Sa, ell_res_mag);
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
    long double mb = fabsl(eb);
    takum<N> negb = takum<N>::from_ell(!Sb, mb);
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
#if TAKUM_HAS_STD_EXPECTED
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
#endif // TAKUM_HAS_STD_EXPECTED

// Optional path (pre-C++23) safe_abs & safe_recip equivalents
#if !TAKUM_HAS_STD_EXPECTED
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
#endif // !TAKUM_HAS_STD_EXPECTED

} // namespace takum
