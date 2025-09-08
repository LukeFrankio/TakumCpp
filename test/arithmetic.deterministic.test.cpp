#include <gtest/gtest.h>
#include "takum/types.h"
#include "takum/arithmetic.h"
#include "test_helpers.h"

using namespace takum::types;

TEST(ArithmeticDeterministic, QuantizedOperandReference) {
    // deterministic set of operand pairs
    double pairs[][2] = {{1.25, 2.5}, {-0.75, 0.125}, {3.0, -1.0}, {0.0, 5.0}};
    for (size_t i = 0; i < (sizeof(pairs)/sizeof(pairs[0])); ++i) {
        double a = pairs[i][0];
        double b = pairs[i][1];
        takum64 ta(a), tb(b);
        double qa = ta.to_double();
        double qb = tb.to_double();
        takum64 r_add = ta + tb;
        takum64 r_mul = ta * tb;
        takum64 ref_add(qa + qb);
        takum64 ref_mul(qa * qb);
        if (r_add != ref_add) {
            emit_failure_log("QuantizedAdd64", i, r_add.raw_bits());
        }
        if (r_mul != ref_mul) {
            emit_failure_log("QuantizedMul64", i, r_mul.raw_bits());
        }
        EXPECT_EQ(r_add, ref_add);
        EXPECT_EQ(r_mul, ref_mul);
    }
}
