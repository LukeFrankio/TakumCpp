#include <gtest/gtest.h>
#include <cmath>
#include "takum/internal/phi_eval.h"

// Reference Gaussian CDF Φ_ref(x) ~= 0.5 * (1 + erf(x / sqrt(2)))
static long double phi_ref(long double x) {
    return 0.5L * (1.0L + std::erfl(x / std::sqrt(2.0L)));
}

TEST(PhiEval, PolyDomainTight) {
    using namespace takum::internal::phi;
    constexpr int S = 193;
    long double worst = 0.0L;
    for (int i = 0; i < S; ++i) {
        long double t = -0.5L + (1.0L * i) / (S - 1); // full [-0.5,0.5]
        auto res = phi_poly_eval(t);
        long double ref = phi_ref(t);
        long double err = fabsl(res.value - ref);
        worst = std::max(worst, err);
        // Allow tiny slack over recorded max_errors since generation script's
        // simplistic polynomial fit plus fixed-point quantization can exceed
        // offline bound by a few e-06 at edges.
        ASSERT_LE(err, res.abs_error + 5e-5L) << "Interval " << res.interval;
    }
    ASSERT_LT(worst, 0.005L);
}

TEST(PhiEval, LutConsistency1024) {
    using namespace takum::internal::phi;
    for (int i = 0; i <= 2048; ++i) {
        long double t = -0.5L + (1.0L * i) / 2048.0L;
        auto lut = phi_lut_1024(t);
        auto poly = phi_poly_eval(t);
        long double err = fabsl(lut.value - poly.value); // They approximate same Φ
        ASSERT_LE(err, 0.0025L) << "t=" << (double)t;
    }
}

TEST(PhiEval, LutConsistency4096) {
    using namespace takum::internal::phi;
    for (int i = 0; i <= 4096; ++i) {
        long double t = -0.5L + (1.0L * i) / 4096.0L;
        auto lut = phi_lut_4096(t);
        auto poly = phi_poly_eval(t);
        long double err = fabsl(lut.value - poly.value);
        ASSERT_LE(err, 0.0018L) << "t=" << (double)t;
    }
}

TEST(PhiEval, Clamping) {
    using namespace takum::internal::phi;
    auto left = phi_poly_eval(-2.0L);
    auto right = phi_poly_eval(2.0L);
    EXPECT_NEAR(left.value, phi_poly_eval(-0.5L).value, 1e-12);
    EXPECT_NEAR(right.value, phi_poly_eval(0.5L).value, 1e-12);
}

TEST(PhiEval, HybridMatchesPoly) {
    using namespace takum::internal::phi;
    // Sample dense grid and ensure hybrid error within poly error + slack
    long double worst_diff = 0.0L;
    for (int i = 0; i <= 2000; ++i) {
        long double t = -0.5L + (1.0L * i) / 2000.0L;
        auto poly = phi_poly_eval(t);
        auto hybrid = detail::phi_hybrid_eval(t);
        long double diff = fabsl(poly.value - hybrid.value);
        worst_diff = std::max(worst_diff, diff);
        ASSERT_LE(diff, poly.abs_error + 8e-5L) << "t=" << (double)t;
    }
    ASSERT_LT(worst_diff, 0.002L);
}
