#pragma once
#include <cmath>
#include <cstdint>
#include <array>
#include "takum/internal/phi_spec.h"
#include "takum/internal/phi_types.h"
#include "takum/internal/phi_lut.h"
#include "takum/precision_traits.h"
#include "takum/config.h"

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

// Coarse LUT size for hybrid (independent of NUM_INTERVALS polynomial partition)
constexpr int HYBRID_LUT_SIZE = TAKUM_COARSE_LUT_SIZE; // macro-configurable
static_assert(HYBRID_LUT_SIZE > 0 && HYBRID_LUT_SIZE <= 4096,
    "HYBRID_LUT_SIZE must be in (0, 4096] for phi_hybrid_eval");
struct HybridCoarseLUT {
    std::array<long double, HYBRID_LUT_SIZE + 1> v{}; // endpoints inclusive
    constexpr HybridCoarseLUT() : v{} {
        for (int i = 0; i <= HYBRID_LUT_SIZE; ++i) {
            long double t = -0.5L + (static_cast<long double>(i) / HYBRID_LUT_SIZE);
            // Use polynomial path to seed; later could be high-precision offline values
            auto poly = phi_poly_eval(t);
            v[i] = poly.value;
        }
    }
};
inline const HybridCoarseLUT& coarse_hybrid_table() {
    static const HybridCoarseLUT tbl{}; // lazily constructed at first use
    return tbl;
}

inline PhiEvalResult phi_hybrid_eval(long double t) noexcept {
    // Clamp
    if (t > 0.5L) t = 0.5L; else if (t < -0.5L) t = -0.5L;
    // Coarse interval
    long double u = (t + 0.5L); // [0,1]
    long double coarse_f = u * HYBRID_LUT_SIZE;
    int ci = static_cast<int>(coarse_f);
    if (ci >= HYBRID_LUT_SIZE) ci = HYBRID_LUT_SIZE - 1;
    long double cfrac = coarse_f - static_cast<long double>(ci);
    const auto& cv = coarse_hybrid_table().v;
    long double base0 = cv[ci];
    long double base1 = cv[ci + 1];
    long double coarse_interp = base0 + (base1 - base0) * cfrac;

    // Fine polynomial interval refinement
    auto poly_res = phi_poly_eval(t);
    long double residual = poly_res.value - coarse_interp;
    // Conservative combined error: poly_res.abs_error + interpolation diff scaling
    long double eb = poly_res.abs_error + std::fabsl(residual) * 0.25L + 5e-6L;
    return { coarse_interp + residual, eb, ci };
}
} // namespace detail

// Public internal API used by arithmetic (subject to refinement):
inline long double phi(long double t) noexcept { return phi_poly_eval(t).value; }

// Precision-dispatching evaluator returning PhiEvalResult.
// For small N (<=16, <=32) use LUT; else polynomial (future: hybrid).
template <size_t N>
inline PhiEvalResult phi_eval(long double t) noexcept {
    if constexpr (N <= 16) {
        return phi_lut_1024(t);
    } else if constexpr (N <= 32) {
        return phi_lut_4096(t);
    } else {
        return detail::phi_hybrid_eval(t); // adaptive degree hook could go here
    }
}

// Convenience value-only accessor
template <size_t N>
inline long double phi_v(long double t) noexcept { return phi_eval<N>(t).value; }

// Feature toggle macro (user can define before including arithmetic)
#ifndef TAKUM_ENABLE_FAST_ADD
#define TAKUM_ENABLE_FAST_ADD 0
#endif

// Helper: check whether accumulated Φ error stays within λ(p) budget (informational for now)
template <size_t N>
inline bool within_phi_budget(const PhiEvalResult& r) noexcept {
    return r.abs_error <= precision::lambda_p<N>();
}

// Diagnostics counters (non-atomic to keep header-only & constexpr-friendly).
struct PhiDiagCounters {
    unsigned long eval_calls = 0;
    unsigned long budget_ok = 0;
    unsigned long budget_fail = 0;
    long double worst_error = 0.0L;
};

template <size_t N>
inline PhiDiagCounters& phi_diag() {
    static PhiDiagCounters c{};
    return c;
}

template <size_t N>
inline void record_phi(const PhiEvalResult& r, bool ok) {
#if TAKUM_ENABLE_PHI_DIAGNOSTICS
    auto& d = phi_diag<N>();
    ++d.eval_calls;
    if (ok) ++d.budget_ok; else ++d.budget_fail;
    if (r.abs_error > d.worst_error) d.worst_error = r.abs_error;
#else
    (void)r; (void)ok;
#endif
}

} // namespace takum::internal::phi
