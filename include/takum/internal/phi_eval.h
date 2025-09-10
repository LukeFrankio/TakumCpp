#pragma once
#include <cmath>
#include <cstdint>
#include "takum/internal/phi_spec.h"
#include "takum/internal/phi_types.h"
#include "takum/internal/phi_lut.h"

// Lightweight internal Gaussian-log (Φ) evaluation helpers.
// Strategy implemented now:
//  - Hybrid polynomial evaluation for takum64+ using generated fixed-point coeffs.
//  - Temporary polynomial-only approximation for all precisions (LUT paths TODO).
//    This keeps interface stable so later we can plug in LUT + interpolation for
//    takum16/32 without changing callers.
//
// The polynomial coefficients in poly_coeffs[][] are stored in Q16 fixed-point
// (see generated header). We evaluate using Horner in long double for precision.
// Interval mapping: incoming t is clamped to [-0.5, 0). Intervals partition that
// range uniformly (NUM_INTERVALS). Each interval i covers [ -0.5 + i*step , -0.5 + (i+1)*step ).
// Step = 0.5 / NUM_INTERVALS.
//
// NOTE: This Φ here models a monotone helper φ(t) ~ Φ(t) used in addition formulas.
// It is a placeholder until full Gaussian-log formulation + LUT scaling constants
// are wired (Prop 11 budget aware). Error budgets from max_errors[] available.

namespace takum::internal::phi {

// PhiEvalResult now in phi_types.h

inline PhiEvalResult phi_poly_eval(long double t) noexcept {
    // Generated coefficients cover the full domain [-0.5, +0.5] uniformly.
    if (t > 0.5L) t = 0.5L;
    if (t < -0.5L) t = -0.5L;
    constexpr long double domain_min = -0.5L;
    constexpr long double domain_max = 0.5L;
    constexpr long double span = domain_max - domain_min; // 1.0
    long double u = (t - domain_min) / span; // in [0,1]
    long double f_index = u * static_cast<long double>(NUM_INTERVALS);
    int idx = static_cast<int>(f_index);
    if (idx >= NUM_INTERVALS) idx = NUM_INTERVALS - 1;
    const int32_t* coeff = poly_coeffs[idx];
    long double scale = 1.0L / static_cast<long double>(1ULL << Q_FRAC_BITS);
    long double acc = 0.0L;
    for (int d = POLY_DEGREE; d >= 0; --d) {
        long double c = static_cast<long double>(coeff[d]) * scale;
        acc = acc * t + c;
    }
    long double eb = static_cast<long double>(max_errors[idx]);
    return { acc, eb, idx };
}

// ---------------- Hybrid (coarse anchor + polynomial residual) --------------
// For N > 32 we approximate using a coarse set of anchor samples (interval
// boundaries) and refine via the existing per-interval polynomial (currently
// still global-in-x). This isolates the residual which will shrink once the
// generator emits re-centered local polynomials. Kept internal & easily swappable.
namespace detail {
struct AnchorsHolder {
    long double data[NUM_INTERVALS + 1] = {};
    constexpr AnchorsHolder() : data{} {
        for (int i = 0; i <= NUM_INTERVALS; ++i) {
            long double t = -0.5L + (static_cast<long double>(i) / static_cast<long double>(NUM_INTERVALS));
            const int32_t* coeff = poly_coeffs[(i == NUM_INTERVALS) ? NUM_INTERVALS - 1 : i == 0 ? 0 : i - 1];
            long double scale = 1.0L / static_cast<long double>(1ULL << Q_FRAC_BITS);
            long double acc = 0.0L;
            for (int d = POLY_DEGREE; d >= 0; --d) {
                long double c = static_cast<long double>(coeff[d]) * scale;
                acc = acc * t + c;
            }
            data[i] = acc;
        }
    }
};
inline const AnchorsHolder& get_anchors() {
    static const AnchorsHolder holder{};
    return holder;
}

inline PhiEvalResult phi_hybrid_eval(long double t) noexcept {
    auto poly_res = phi_poly_eval(t); // already clamps t
    long double t_clamped = (t > 0.5L) ? 0.5L : (t < -0.5L ? -0.5L : t);
    long double u = (t_clamped + 0.5L); // span=1
    long double f_index = u * static_cast<long double>(NUM_INTERVALS);
    int idx = static_cast<int>(f_index);
    if (idx >= NUM_INTERVALS) idx = NUM_INTERVALS - 1;
    long double frac = f_index - static_cast<long double>(idx);
    const auto& A = get_anchors().data;
    long double base0 = A[idx];
    long double base1 = A[idx + 1];
    long double baseline = base0 + (base1 - base0) * frac;
    long double residual = poly_res.value - baseline;
    long double eb = std::min(poly_res.abs_error, std::fabsl(residual) + 0.5L * poly_res.abs_error) + 5e-5L;
    return { baseline + residual, eb, idx };
}
} // namespace detail

// Public internal API used by arithmetic (subject to refinement):
inline long double phi(long double t) noexcept { return phi_poly_eval(t).value; }

// Precision-dispatching evaluator returning PhiEvalResult.
// For small N (<=16, <=32) use LUT; else polynomial (future: hybrid).
template <size_t N>
inline PhiEvalResult phi_eval(long double t) noexcept {
    if constexpr (N <= 16) return phi_lut_1024(t);
    else if constexpr (N <= 32) return phi_lut_4096(t);
    else return detail::phi_hybrid_eval(t);
}

// Convenience value-only accessor
template <size_t N>
inline long double phi_v(long double t) noexcept { return phi_eval<N>(t).value; }

// Feature toggle macro (user can define before including arithmetic)
#ifndef TAKUM_ENABLE_FAST_ADD
#define TAKUM_ENABLE_FAST_ADD 0
#endif

} // namespace takum::internal::phi
