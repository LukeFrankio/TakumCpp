#include <gtest/gtest.h>
#include <bitset>
#include <vector>
#include <limits>
#include "takum/types.h"
#include "takum/arithmetic.h"
#include "test_helpers.h"

using namespace takum::types;

// Helper to iterate SI (signed-integer) order for N bits: start at sign bit set
static std::vector<uint64_t> si_order_indices(size_t N) {
    std::vector<uint64_t> order;
    if (N >= 64) return order; // guard for large N to prevent overflow
    // SI order: start from (1<<(N-1)) .. (2^N-1), then 0 .. (1<<(N-1))-1
    uint64_t start = (1ull << (N - 1));
    for (uint64_t i = start; i < (1ull << N); ++i) order.push_back(i);
    for (uint64_t i = 0; i < start; ++i) order.push_back(i);
    return order;
}

template <size_t N>
void run_small_width_tests() {
    using T = ::takum::takum<N>;
    uint64_t total = 1ull << N;
    auto indices = si_order_indices(N);
    // Round-trip: construct from double and back for every bit pattern (ensuring canonical NaR handling)
    for (uint64_t ui = 0; ui < total; ++ui) {
        T t = T::from_raw_bits( static_cast<typename T::storage_t>(ui) );
        double d = t.to_double();
        T r = T(d);
        // For NaR, ensure NaR roundtrip
        if (t.is_nar()) {
            EXPECT_TRUE(r.is_nar());
            continue;
        }
        // Numeric fidelity: allow canonicalization in encoding; compare decoded doubles
        double d_orig = d;
    double d_rt = r.to_double();
    double eps = std::numeric_limits<T>::epsilon();
    // Use a relative tolerance scaled by the larger magnitude between
    // the original and round-tripped values so very small values don't
    // force an unrealistically tiny absolute tolerance.
    double scale = std::max(std::fabs(d_orig), std::fabs(d_rt));
    double tol = std::max(1e-12, eps * scale * 4.0);
        if (!(std::isnan(d_orig) && std::isnan(d_rt))) {
            if (std::isfinite(d_orig) && std::isfinite(d_rt)) {
                if (std::fabs(d_orig - d_rt) > tol) {
                    emit_failure_log("RoundTripSmallNumeric", ui, static_cast<uint64_t>(t.raw_bits()));
                }
                EXPECT_NEAR(d_rt, d_orig, tol);
            } else {
                // One of them non-finite (Inf/NaN): handle overflow-to-NaR specially.
                // If the original decoded value overflew to +/-Inf, the reference
                // encoder maps non-finite inputs to NaR, so we expect `r` to be NaR.
                if (!std::isfinite(d_orig)) {
                    // original decoded to +/-Inf -> re-encoding must produce NaR
                    EXPECT_TRUE(r.is_nar());
                } else {
                    // Otherwise require NaR consistency (covers NaN cases and zeros)
                    EXPECT_EQ(t.is_nar(), r.is_nar());
                }
            }
        }
    }

    // Monotonicity in SI order: takum total-order should be non-decreasing on real range (skip NaR)
    bool have_prev = false;
    T prev_t{};
    for (size_t idx = 0; idx < indices.size(); ++idx) {
        uint64_t ui = indices[idx];
        T t = T::from_raw_bits(static_cast<typename T::storage_t>(ui));
        if (t.is_nar()) continue;
        if (!have_prev) { prev_t = t; have_prev = true; continue; }
        // Expect t >= prev_t in the takum total order
        if (t < prev_t) {
            emit_failure_log("MonotonicitySmall", idx, ui);
        }
        EXPECT_FALSE(t < prev_t);
        prev_t = t;
    }
}

TEST(ArithmeticExhaustive, RoundTripAndMonotonicitySmallWidths) {
    // Default exhaustive range: 6..12. Can be extended up to 16 by
    // defining EXHAUSTIVE_MAX_N at compile time (see test/CMakeLists.txt).
#ifndef EXHAUSTIVE_MAX_N
#define EXHAUSTIVE_MAX_N 12
#endif
    constexpr int minN = 6;
    constexpr int maxN = (EXHAUSTIVE_MAX_N > 16) ? 16 : EXHAUSTIVE_MAX_N;
    for (int n = minN; n <= maxN; ++n) {
        switch (n) {
            case 6: run_small_width_tests<6>(); break;
            case 7: run_small_width_tests<7>(); break;
            case 8: run_small_width_tests<8>(); break;
            case 9: run_small_width_tests<9>(); break;
            case 10: run_small_width_tests<10>(); break;
            case 11: run_small_width_tests<11>(); break;
            case 12: run_small_width_tests<12>(); break;
            case 13: run_small_width_tests<13>(); break;
            case 14: run_small_width_tests<14>(); break;
            case 15: run_small_width_tests<15>(); break;
            case 16: run_small_width_tests<16>(); break;
            default: break;
        }
    }
}
