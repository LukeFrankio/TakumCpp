/**
 * @file test_helpers.h
 * @brief Small utilities used across the unit tests (bit unpacking, dumping helpers).
 *
 * Contains readable decode helpers for test verification of exact tuples (S, c, r, m_int)
 * and a small dump function used during debugging of collisions.
 */

#pragma once

#include <tuple>
#include <cstdint>
#include <limits>
#include <iostream>
#include <sstream>
#include <iomanip>

// Robust decode_tuple: supports 6 <= N <= 64. Returns (S, c, r, m_int)
// Note: m_int packs the low p bits into a uint64_t. For p > 64 this function is not sufficient.
template <size_t N>
inline auto decode_tuple(uint64_t ui) -> std::tuple<int, int, int, uint64_t> {
    static_assert(N >= 6 && N <= 64, "decode_tuple<N> only supported for 6 <= N <= 64");

    // mask input to low N bits to avoid stray upper bits
    const uint64_t maskN = (N == 64) ? UINT64_MAX : ((1ull << N) - 1ull);
    ui &= maskN;

    // S and D (safe: N-1 and N-2 are < 64 because N <= 64)
    const int S = static_cast<int>((ui >> (N - 1)) & 1u);
    const int D = static_cast<int>((ui >> (N - 2)) & 1u);

    // R field: three bits starting at bit (N-5)
    const int R_val = static_cast<int>((ui >> (N - 5)) & 7u);
    const int r = (D == 1) ? R_val : (7 - R_val);

    // mantissa width
    int p = static_cast<int>(N) - 5 - r;
    if (p < 0) p = 0;

    // Extract C (r bits) if present: C occupies bits [N-6 .. N-6-(r-1)] -> LSB at (N-5-r)
    uint64_t C_val = 0;
    if (r > 0) {
        int c_pos = static_cast<int>(N) - 5 - r;
        if (c_pos >= 0) {
            // build mask safely (avoid 1<<64 UB)
            const uint64_t maskC = (r >= 64) ? UINT64_MAX : ((1ull << r) - 1ull);
            C_val = (ui >> c_pos) & maskC;
        } else {
            C_val = 0; // defensive: no room for C bits
        }
    }

    // compute c (use int64_t for intermediate). Guard against too-large r to avoid UB.
    int64_t c64 = 0;
    if (r < 62) {
        if (D == 1) {
            int64_t base = (r == 0) ? 0 : ((1ll << r) - 1ll);
            c64 = base + static_cast<int64_t>(C_val);
        } else {
            int64_t base = (1ll << (r + 1)) - 1ll; // safe because r+1 < 63 here
            c64 = - (base - static_cast<int64_t>(C_val));
        }
    } else {
        // defensive fallback for extremely large r: avoid UB but also warn (shouldn&#x27;t occur for normal takum sizes)
        // build base using a safe loop, but this will saturate for practical int width
        int64_t base = 0;
        for (int i = 0; i < r; ++i) {
            // break if base would overflow; saturate to large value
            if (base > (INT64_MAX >> 1)) { base = INT64_MAX; break; }
            base = (base << 1) | 1;
        }
        if (D == 1) c64 = base + static_cast<int64_t>(C_val);
        else {
            int64_t base2 = base;
            // extra one bit for r+1 ones
            if (base2 <= (INT64_MAX >> 1)) base2 = (base2 << 1) | 1;
            else base2 = INT64_MAX;
            c64 = - (base2 - static_cast<int64_t>(C_val));
        }
    }

    // final cast to int (caller should ensure c fits in &#x27;int&#x27;)
    const int c = static_cast<int>(c64);

    // lowest p bits are mantissa
    uint64_t m_int = 0;
    if (p > 0) {
        const uint64_t maskM = (p >= 64) ? UINT64_MAX : ((1ull << p) - 1ull);
        m_int = ui & maskM;
    }

    return {S, c, r, m_int};
}

// Small deterministic failure logger used by tests to emit reproducible
// failure artifacts to stderr for CI capture. Writes a short JSON-like line.
inline void emit_failure_log(const char* test_name, size_t idx, uint64_t bits) {
    std::ostringstream os;
    os << "{\"test\":\"" << test_name << "\",\"idx\":" << idx << ",\"bits\":\"0x" << std::hex << bits << "\"}";
    std::cerr << os.str() << std::endl;
}


template <size_t N>
void dump_ui(uint64_t ui) {
    std::ostringstream os;
    os << std::hex << "0x" << ui;
    auto tpl = decode_tuple<N>(ui);
    int S = std::get<0>(tpl);
    int c = std::get<1>(tpl);
    int r = std::get<2>(tpl);
    uint64_t m = std::get<3>(tpl);
    os << " S=" << std::dec << S << " c=" << c << " r=" << r << " m=" << m;
    std::cerr << os.str() << std::endl;
}