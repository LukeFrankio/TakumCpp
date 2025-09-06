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

TEST_F(CoreTest, UniquenessTakum12) {
    // Prop 3: injective τ, all bit patterns map to unique values (except NaR/0 specials)
    std::set<double> values;
    for (uint64_t bits = 0; bits < (1ULL << 12); ++bits) {
        ::takum::takum<12> t;
        t.storage = static_cast<uint32_t>(bits);
        double val = t.to_double();
        if (!values.insert(val).second) {
if (std::isnan(val)) {
    std::cout << "NaN for bits " << bits << std::endl;
}
            FAIL() << "Duplicate value " << val << " for bits " << bits;
        }
    }
    EXPECT_EQ(values.size(), 1ULL << 12);  // All unique
}

TEST_F(CoreTest, MonotonicityTakum12) {
    // Prop 4: bit increment -> value increases (except wrap to NaR)
    std::vector<double> vals;
    for (uint64_t bits = 0; bits < (1ULL << 12); ++bits) {
        ::takum::takum<12> t;
        t.storage = static_cast<uint32_t>(bits);
        vals.push_back(t.to_double());
    }
    for (size_t i = 0; i < vals.size() - 1; ++i) {
        if (std::isnan(vals[i]) || std::isnan(vals[i + 1])) continue;
        EXPECT_LE(vals[i], vals[i + 1]) << "At index " << i << ": " << vals[i] << " > " << vals[i + 1];
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