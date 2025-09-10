#include <gtest/gtest.h>
#include "takum/arithmetic.h"

// Avoid 'using namespace takum;' because the class template takum<N> shares
// the same name as the namespace, creating ambiguity on GCC/Clang.

TEST(SafeVariantsExtra, SafeAbsBasic) {
  takum::takum<32> x(3.5);
#if __cplusplus >= 202302L
  auto r = takum::safe_abs<32>(x);
  EXPECT_TRUE(r.has_value());
  EXPECT_NEAR(r->to_double(), 3.5, 1e-6);
#else
  auto r = takum::safe_abs<32>(x);
  ASSERT_TRUE(r.has_value());
  EXPECT_NEAR(r->to_double(), 3.5, 1e-6);
#endif
}

TEST(SafeVariantsExtra, SafeReciprocalZero) {
  takum::takum<16> z(0.0);
#if __cplusplus >= 202302L
  auto r = takum::safe_recip<16>(z);
  EXPECT_FALSE(r.has_value());
#else
  auto r = takum::safe_recip<16>(z);
  EXPECT_FALSE(r.has_value());
#endif
}

TEST(SafeVariantsExtra, SafeReciprocalRoundTrip) {
  takum::takum<32> v(2.0);
#if __cplusplus >= 202302L
  auto r = takum::safe_recip<32>(v);
  ASSERT_TRUE(r.has_value());
  double dv = r->to_double();
  EXPECT_NEAR(dv * 2.0, 1.0, 1e-5);
#else
  auto r = takum::safe_recip<32>(v);
  ASSERT_TRUE(r.has_value());
  EXPECT_NEAR(r->to_double() * 2.0, 1.0, 1e-5);
#endif
}
