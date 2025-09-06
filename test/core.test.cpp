#include <gtest/gtest.h>
#include <limits>
#include <vector>
#include <cmath>
#include <set>

#include "takum/core.h"
#include "takum/types.h"

using namespace ::testing;

class CoreTest : public ::testing::Test {
protected:
    static constexpr double EPS = 1e-6;  // Loose tolerance for ref approx, tighten later
};

TEST_F(CoreTest, RoundTripTakum32) {
    // Test round-trip for positive values
    double inputs[] = {0.0, 1.0, 3.14159, 1e10, 1e-10, std::exp(1.0)};
    for (double inp : inputs) {
        takum::takum<32> t(inp);
        double back = t.to_double();
        if (std::isnan(inp)) {
            EXPECT_TRUE(t.is_nar());
#if __cplusplus >= 202302L
            EXPECT_FALSE(t.to_expected().has_value());
#else
            EXPECT_FALSE(t.to_expected().has_value());
#endif
        } else {
            EXPECT_FALSE(t.is_nar());
            EXPECT_NEAR(back, inp, EPS * std::fabs(inp));
        }
    }
}

TEST_F(CoreTest, RoundTripTakum64) {
    double inputs[] = {0.0, 1.0, 3.141592653589793, 1e50, 1e-50};
    for (double inp : inputs) {
        takum::takum<64> t(inp);
        double back = t.to_double();
        EXPECT_NEAR(back, inp, EPS * std::fabs(inp));
    }
}

TEST_F(CoreTest, MonotonicityAndUniquenessTakum12_Corrected) {
    constexpr unsigned n = 12;
    const uint32_t num_patterns = 1u << n;
    const uint32_t nar_index = 1u << (n - 1); // 2048 for n=12

    // 1) Quick NaR check
    {
        takum::takum<n> t;
        t.storage = nar_index;
        EXPECT_TRUE(std::isnan(t.to_double())) << "NaR bit pattern should produce NaN.";
    }

    // 2) Iterate in two's-complement (SI) ascending order:
    //    unsigned order: nar_index, nar_index+1, ..., num_patterns-1, 0, 1, ..., nar_index-1
    bool have_prev = false;
    double prev_v = 0.0;

    for (uint32_t i = 0; i < num_patterns; ++i) {
        uint32_t ui = (nar_index + i) & (num_patterns - 1); // rotate start
        takum::takum<n> t;
        t.storage = ui;
        double v = t.to_double();

        // skip comparisons involving NaR (NaN)
        if (std::isnan(v)) {
            // ensure it's the designated NaR bit pattern (defensive)
            EXPECT_EQ(ui, nar_index) << "Unexpected NaR location.";
            have_prev = false; // reset previous so we don't compare across NaR
            continue;
        }

        if (have_prev) {
            // Proposition 4 / proof shows strict increase for consecutive non-NaR bitstrings:
            EXPECT_LT(prev_v, v) << "Monotonicity failed between UI " << std::hex << (nar_index + i - 1) << " and " << ui;
        }
        prev_v = v;
        have_prev = true;
    }

    // 3) Check largest-negative is < 0 (Proposition 3)
    {
        takum::takum<n> t_maxpos; t_maxpos.storage = num_patterns - 1; // unsigned 4095 -> SI -1
        double v_last_neg = t_maxpos.to_double();
        EXPECT_LT(v_last_neg, 0.0) << "Largest-negative (SI=-1) must be < 0.";
    }
}


TEST_F(CoreTest, SpecialCases) {
    // 0
    ::takum::takum<32> zero(0.0);
    EXPECT_DOUBLE_EQ(zero.to_double(), 0.0);
    EXPECT_FALSE(zero.is_nar());

    // NaR
    ::takum::takum<32> nar(std::numeric_limits<double>::quiet_NaN());
    EXPECT_TRUE(nar.is_nar());
#if __cplusplus >= 202302L
    auto exp_nar = nar.to_expected();
    EXPECT_FALSE(exp_nar.has_value());
#else
    auto opt_nar = nar.to_expected();
    EXPECT_FALSE(opt_nar.has_value());
#endif

    // Inf -> NaR
    ::takum::takum<32> inf(std::numeric_limits<double>::infinity());
    EXPECT_TRUE(inf.is_nar());

    // Saturation: large value clamped
    double large = std::exp(127.0);  // Within range for base sqrt(e)
    ::takum::takum<32> t_large(large);
    EXPECT_NEAR(t_large.to_double(), large, EPS * large);
    double too_large = std::exp(150.0);
    ::takum::takum<32> t_too_large(too_large);
    double max_takum = std::exp(127.4995);  // Approx max ell/2
    EXPECT_NEAR(t_too_large.to_double(), max_takum, EPS * max_takum);
}

TEST_F(CoreTest, NaRPropagationBasic) {
    ::takum::takum<32> nar = ::takum::takum<32>::nar();
    ::takum::takum<32> one(1.0);
    // Basic ops not implemented yet, but to_expected should fail
#if __cplusplus >= 202302L
    EXPECT_FALSE(nar.to_expected().has_value());
#else
    EXPECT_FALSE(nar.to_expected().has_value());
#endif
    // Placeholder for future arithmetic tests
}