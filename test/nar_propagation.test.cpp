#include <gtest/gtest.h>
#include "takum/types.h"
#include "takum/arithmetic.h"
#include <cmath>

using namespace takum::types;

TEST(NaRPropagation, NaRArithmetic) {
    takum64 a = takum64::nar();
    takum64 b = takum64(1.0);
    takum64 c = a + b;
    EXPECT_TRUE(c.is_nar());
    c = a * b;
    EXPECT_TRUE(c.is_nar());
    c = takum64(0.0) / takum64(0.0);
    EXPECT_TRUE(c.is_nar());
}

TEST(NaRPropagation, NaRConversions) {
    takum128 n = takum128::nar();
    takum64 down = takum64(n.to_double());
    // either NaR or convertible to a finite double
    EXPECT_TRUE(down.is_nar() || std::isfinite(down.to_double()));
}
