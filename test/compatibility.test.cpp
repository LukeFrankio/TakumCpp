#include <gtest/gtest.h>
#include <cmath>
#include <limits>

#include "takum/compiler_detection.h"
#include "takum/compatibility.h"
#include "takum/core.h"

using namespace ::testing;

class CompatibilityTest : public ::testing::Test {
protected:
    static constexpr long double EPS = 1e-6L;
};

TEST_F(CompatibilityTest, Float8ShimRoundTrip) {
    using F8 = float8_t;
    using T8 = takum::takum<8>;

    // Test that float8_t is alias for takum<8>
    static_assert(std::is_same_v<F8, T8>, "float8_t must be alias for takum<8>");

    // Test round-trip for simple values, with ghost bits zero-padded
    double inputs[] = {0.0, 1.0, -1.0, 3.14159};
    for (double inp : inputs) {
        F8 f8(inp);
        double back = f8.to_double();
        if (inp == 0.0) {
            EXPECT_DOUBLE_EQ(back, 0.0);
        } else {
            // For small N=8, use larger tolerance based on min p ~3 bits, allowed_rel ~0.25
            double allowed_rel = 0.25; // Conservative for N=8 low precision
            double tol = std::fabs(inp) * (allowed_rel + 1e-12);
            EXPECT_NEAR(back, inp, tol);
        }

        // Check ghost bits are zero (upper bits beyond 8)
        uint64_t bits = static_cast<uint64_t>(f8.storage);
        EXPECT_EQ(bits >> 8, 0ULL) << "Ghost bits must be zero for N=8";
    }

    // Test NaR
    F8 nar = F8::nar();
    EXPECT_TRUE(nar.is_nar());
    EXPECT_TRUE(std::isnan(nar.to_double()));
}

#if !TAKUM_HAS_STD_EXPECTED
TEST_F(CompatibilityTest, ExpectedShim) {
    using E = takum::expected_shim<takum::takum<32>, takum::takum_error>;

    // Test valid expected
    takum::takum<32> valid_val(1.0);
    E valid_exp;
    valid_exp.value = valid_val;
    EXPECT_TRUE(valid_exp.has_value());
    EXPECT_EQ(*valid_exp, valid_val);
    EXPECT_EQ(valid_exp.value_or(takum::takum<32>(2.0)), valid_val);

    // Test unexpected
    E invalid_exp;
    invalid_exp.value = std::nullopt;
    invalid_exp.error = {takum::takum_error::Kind::InvalidOperation, "Test error"};
    EXPECT_FALSE(invalid_exp.has_value());
    EXPECT_EQ(invalid_exp.value_or(takum::takum<32>(2.0)), takum::takum<32>(2.0));
}
#endif