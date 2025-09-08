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
    // Phase-4 Gaussian-log (Φ) addition.
    if (a.is_nar() || b.is_nar()) return takum<N>::nar();

    // Prefer quantized-double intermediate (matches Phase‑3 behavior/tests):
    double da = a.to_double();
    double db = b.to_double();
    if (std::isfinite(da) && std::isfinite(db)) {
        // This constructs via encode_from_double and ensures bitwise
        // equality against reference takum(qa+qb) used by tests.
        // Also handles zero cases.
        return takum<N>(da + db);
    }

    // Fallback to Phase-4 Gaussian-log when double intermediates are non-finite
    // (e.g., overflow) or unavailable.
    // Extract signs and logits ℓ = 2*ln(|x|) in long double precision
    long double ell_a = a.get_exact_ell();
    long double ell_b = b.get_exact_ell();
    if (!std::isfinite((double)ell_a) || !std::isfinite((double)ell_b)) return takum<N>::nar();

    // Signs
    bool Sa = (ell_a < 0.0L);
    bool Sb = (ell_b < 0.0L);
    long double abs_a = std::fabsl(ell_a);
    long double abs_b = std::fabsl(ell_b);

    // Order so that abs_a >= abs_b
    if (abs_b > abs_a) {
        std::swap(abs_a, abs_b);
        std::swap(Sa, Sb);
    }

    // If magnitudes are identical and signs cancel -> zero
    if (abs_a == abs_b && Sa != Sb) return takum<N>{};

    // Compute ell_result = sign * (max + log(1 + sign*exp(min-max)))
    long double maxv = abs_a;
    long double minv = abs_b;
    long double s = (Sa == Sb) ? 1.0L : -1.0L; // sign factor for addition
    long double diff = minv - maxv; // <= 0

    // Compute log1p in high precision: log1p(s*exp(diff)) but ensure argument positive
    long double z = s * std::expl(diff);
    // If |z| is tiny, result ~ maxv
    long double addterm;
    if (std::fabsl(z) < 1e-18L) addterm = 0.0L;
    else {
        long double arg = 1.0L + z;
        if (arg <= 0.0L) {
            // Numerical cancellation -> results in zero or NaR
            if (arg == 0.0L) return takum<N>{};
            return takum<N>::nar();
        }
        addterm = std::logl(arg);
    }
    long double ell_res_mag = maxv + addterm;
    long double ell_res = (Sa ? -ell_res_mag : ell_res_mag);

    // Use from_ell to encode result with proper rounding and canonicalization
    return takum<N>::from_ell(Sa, ell_res);
}

/**
 * Subtract two takum values.
 */
template <size_t N>
inline takum<N> operator-(const takum<N>& a, const takum<N>& b) noexcept {
    // Prefer double-intermediate subtraction when possible
    if (a.is_nar() || b.is_nar()) return takum<N>::nar();
    double da = a.to_double();
    double db = b.to_double();
    if (std::isfinite(da) && std::isfinite(db)) {
        return takum<N>(da - db);
    }

    // Fallback: treat as addition with flipped sign via from_ell
    takum<N> nb = b;
    long double ell_b = b.get_exact_ell();
    if (!std::isfinite((double)ell_b)) return takum<N>::nar();
    bool Sb = (ell_b < 0.0L);
    long double mag_b = std::fabsl(ell_b);
    bool Sb_flipped = !Sb;
    nb = takum<N>::from_ell(Sb_flipped, (Sb_flipped ? -mag_b : mag_b));
    return a + nb;
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
