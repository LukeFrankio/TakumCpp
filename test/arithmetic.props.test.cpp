#include <gtest/gtest.h>
#include "takum/types.h"
#include "takum/arithmetic.h"

using namespace takum::types;

template <size_t N>
void test_add_commutativity() {
    takum::takum<N> a = takum::takum<N>(1.25);
    takum::takum<N> b = takum::takum<N>(2.5);
    auto r1 = a + b;
    auto r2 = b + a;
    EXPECT_EQ(r1, r2);
}

TEST(ArithmeticProps, CommutativityVariousWidths) {
    test_add_commutativity<32>();
    test_add_commutativity<64>();
    test_add_commutativity<128>();
}

TEST(ArithmeticProps, IdentityZero) {
    takum32 a = takum32(3.5);
    takum32 z = takum32(0.0);
    EXPECT_EQ(a + z, a);
}

TEST(ArithmeticProps, MultiplicativeIdentity) {
    takum64 a = takum64(4.0);
    takum64 one = takum64(1.0);
    EXPECT_EQ(a * one, a);
}
