#pragma once
/**
 * @file precision_traits.h
 * @brief Basic precision / error budget helpers for takum<N> (Phase‑4 support).
 *
 * Provides constexpr helpers exposing a conservative λ(p) bound (Proposition 11
 * analogue) used to budget approximation + rounding error for Φ based addition.
 * This is intentionally lightweight; a fuller traits table can extend this.
 */

#include <cstddef>
#include <cmath>

namespace takum::precision {

/// Return an approximate effective precision p (mantissa bits) for width N.
/// Very conservative: reserve 5 structural bits (S D R) + at most 7 regime bits.
template <size_t N>
constexpr size_t effective_p() noexcept {
  return (N > 12) ? (N - 12) : (N > 5 ? N - 5 : 1); // fallback floor
}

/// λ(p) bound (~ 2/3 ulp) – coarse model: λ(p) ≈ 0.66 * 2^{-p}
template <size_t N>
constexpr long double lambda_p() noexcept {
  constexpr size_t p = effective_p<N>();
  return 0.66L * ldexpl(1.0L, -static_cast<int>(p));
}

/// Helper combining two absolute error sources and a rounding step (ulp ~ 2^{-p}).
template <size_t N>
constexpr long double combined_error(long double a, long double b, long double rounding = 0.0L) noexcept {
  long double total = a + b + rounding;
  long double budget = lambda_p<N>();
  return (total > budget) ? total : total; // caller compares vs budget explicitly
}

} // namespace takum::precision
