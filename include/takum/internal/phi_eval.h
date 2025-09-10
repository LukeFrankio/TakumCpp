#pragma once
#include <cmath>
#include <cstdint>
#include "takum/internal/phi_spec.h"

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

struct PhiEvalResult {
    long double value;      // approximated Φ(t)
    long double abs_error;  // conservative bound (interval max error)
    int interval;           // interval index used
};

inline PhiEvalResult phi_poly_eval(long double t) noexcept {
    // Generated coefficients cover the full domain [-0.5, +0.5] uniformly.
    // Clamp to that range.
    if (t > 0.5L) t = 0.5L;
    if (t < -0.5L) t = -0.5L;
    constexpr long double domain_min = -0.5L;
    constexpr long double domain_max = 0.5L;
    constexpr long double span = domain_max - domain_min; // 1.0
    // Map to interval index
    long double u = (t - domain_min) / span; // in [0,1]
    long double f_index = u * static_cast<long double>(NUM_INTERVALS);
    int idx = static_cast<int>(f_index);
    if (idx >= NUM_INTERVALS) idx = NUM_INTERVALS - 1;

    // Horner evaluate polynomial P(t) with global t (coeffs were generated in global x, not shifted).
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

// Public internal API used by arithmetic (subject to refinement):
inline long double phi(long double t) noexcept {
    return phi_poly_eval(t).value;
}

} // namespace takum::internal::phi
