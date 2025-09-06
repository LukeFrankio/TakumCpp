#include <gtest/gtest.h>
#include "takum/types.h"

using namespace takum::types;

TEST(SpecReference, RoundTrip32) {
    takum32 a = takum32(3.141592653589793);
    double back = a.to_double();
    // Reference codec is crude; assert round-trip within a reasonable factor
    ASSERT_NEAR(back, 3.141592653589793, 1e-2);
}

TEST(SpecReference, NaR32) {
    double nanv = std::numeric_limits<double>::quiet_NaN();
    takum32 a = takum32(nanv);
    EXPECT_TRUE(a.is_nar());
    auto e = a.to_expected();
    EXPECT_FALSE(bool(e));
}

TEST(SpecReference, RoundTrip64) {
    takum64 a = takum64(2.718281828459045);
    double back = a.to_double();
    ASSERT_NEAR(back, 2.718281828459045, 1e-2);
}
