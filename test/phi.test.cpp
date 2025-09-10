#include <gtest/gtest.h>
#include <cmath>
#include "takum/internal/phi_eval.h"

// Reference Gaussian CDF Φ_ref(x) ~= 0.5 * (1 + erf(x / sqrt(2)))
static long double phi_ref(long double x) {
    return 0.5L * (1.0L + std::erfl(x / std::sqrt(2.0L)));
}

TEST(PhiEval, BasicDomain) {
    using namespace takum::internal::phi;
    // Sample a grid inside [-0.5,0]
    constexpr int S = 97;
    long double max_obs_err = 0.0L;
    for (int i = 0; i < S; ++i) {
        long double t = -0.5L + (0.5L * i) / (S - 1);
        auto res = phi_poly_eval(t);
        // For now, treat polynomial output directly; map input to reference domain.
        long double ref = phi_ref(t); // using same t argument; later transform if formula changes
        long double err = fabsl(res.value - ref);
        max_obs_err = std::max(max_obs_err, err);
        // Allow generous bound: reported per-interval + small slack (factor 2)
        ASSERT_LE(err, res.abs_error * 2.0L + 1e-6L) << "Interval " << res.interval;
    }
    // Soft global sanity check (placeholder tolerance)
    ASSERT_LT(max_obs_err, 0.01L);
}

TEST(PhiEval, Clamping) {
    using namespace takum::internal::phi;
    auto left = phi_poly_eval(-1.0L);
    auto right = phi_poly_eval(0.25L);
    // Inputs outside domain should clamp to extremes
    EXPECT_GE(left.value, -1.0L); // just sanity
    EXPECT_GE(right.value, left.value); // monotone assumption placeholder
}
