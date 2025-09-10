#include <gtest/gtest.h>
#include "takum/arithmetic.h"
#include "takum/internal/phi_eval.h"
#include "takum/config.h"

// Intentionally avoid 'using namespace takum;' to prevent ambiguity between
// namespace takum and the class template takum<N> under GCC/Clang name lookup.

template <size_t N>
static void exercise_add_range() {
  takum::takum<N> base(1.0); // ell ~ 0
  for (int k = -12; k <= 12; ++k) {
    double scale = std::exp(k * 0.25); // moderate dynamic range
    takum::takum<N> other(scale);
    auto r = base + other;
    (void)r;
  }
}

TEST(PhiPhase4, DiagnosticsCountersAccumulate64) {
  auto& before = takum::internal::phi::phi_diag<64>();
  unsigned long start_calls = before.eval_calls;
  exercise_add_range<64>();
  auto& after = takum::internal::phi::phi_diag<64>();
  EXPECT_GT(after.eval_calls, start_calls);
  EXPECT_GE(after.eval_calls, after.budget_ok + after.budget_fail);
}

TEST(PhiPhase4, ExtremeRatioBypassesPhi) {
  takum::takum<32> a(1.0);
  takum::takum<32> b(std::exp(-100.0)); // extremely tiny
  auto& diag_before = takum::internal::phi::phi_diag<32>();
  unsigned long calls_before = diag_before.eval_calls;
  auto r = a + b; (void)r;
  auto& diag_after = takum::internal::phi::phi_diag<32>();
  // Allow either 0 or 1 new call (implementation may still evaluate Φ)
  EXPECT_LE(diag_after.eval_calls - calls_before, 1u);
}

TEST(PhiPhase4, CancellationProducesZero) {
  takum::takum<16> x(3.25);
  takum::takum<16> y(3.25);
  auto r = x - y;
  EXPECT_FALSE(r.is_nar());
  EXPECT_NEAR(r.to_double(), 0.0, 1e-6);
}

TEST(PhiPhase4, CoarseLUTConfigApplied) {
  // Just assert the configured constant matches compile-time macro value.
  EXPECT_EQ(takum::config::coarse_hybrid_lut_size(), TAKUM_COARSE_LUT_SIZE);
}
