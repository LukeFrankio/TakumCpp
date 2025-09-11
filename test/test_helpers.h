/**
 * @file test_helpers.h
 * @brief Comprehensive test utilities and diagnostic functions for TakumCpp unit tests.
 *
 * This header provides essential testing utilities for validating takum<N> implementations
 * across different bit widths and formats. The utilities focus on bit-level analysis,
 * field extraction, and readable diagnostic output for test debugging and verification.
 *
 * @details
 * **Core Testing Utilities:**
 * - Field decoding: Extract S, D, R, C, M fields from packed bit patterns
 * - Tuple validation: Verify encoding/decoding round-trip correctness  
 * - Diagnostic logging: Structured failure reporting for CI integration
 * - Bit manipulation: Safe extraction with overflow protection
 *
 * **Supported Formats:**
 * - Single-word formats: 6 ≤ N ≤ 64 bits (full field extraction)
 * - Multi-word formats: N > 64 bits (limited support, requires extension)
 * - Edge case handling: Zero patterns, NaR detection, boundary conditions
 *
 * **Usage Patterns:**
 * ```cpp
 * auto [S, c, r, m_int] = decode_tuple<32>(bit_pattern);
 * EXPECT_EQ(S, expected_sign);
 * 
 * if (test_failed) {
 *     emit_failure_log("TestName", index, failing_bits);
 *     dump_ui<32>(failing_bits);  // Human-readable field dump
 * }
 * ```
 *
 * @note These utilities are designed for testing only and should not be used in production
 * @note Field extraction follows the takum specification bit layout exactly
 * @see ci_failure_capture.h for CI integration utilities
 */

#pragma once

#include <tuple>
#include <cstdint>
#include <limits>
#include <iostream>
#include <sstream>
#include <iomanip>

/**
 * @brief Extract takum<N> field components from packed bit pattern.
 *
 * Decodes a takum<N> bit pattern into its constituent fields (S, c, r, m_int)
 * for test validation and debugging. This function provides safe field extraction
 * with overflow protection and boundary checking suitable for unit tests.
 *
 * @tparam N Bit width of the takum format (6 ≤ N ≤ 64)
 * @param ui Packed bit pattern containing the takum representation
 * @return std::tuple<int, int, int, uint64_t> Fields (S, c, r, m_int)
 *
 * @details
 * **Return Tuple Components:**
 * - S (int): Sign bit (0 = positive, 1 = negative) 
 * - c (int): Decoded characteristic value (signed integer)
 * - r (int): Regime width (0 ≤ r ≤ 7)
 * - m_int (uint64_t): Raw mantissa bits as integer (low p bits)
 *
 * **Field Extraction Process:**
 * 1. Mask input to exactly N bits (safety against stray high bits)
 * 2. Extract S, D bits from positions [N-1, N-2]
 * 3. Extract R field (3 bits) and compute regime width r
 * 4. Extract C field (r bits) and decode characteristic c
 * 5. Extract M field (p = N-5-r bits) as raw integer
 *
 * **Safety Features:**
 * - Guards against shift overflow for large r values (r ≥ 62)
 * - Saturates large bases to INT64_MAX to prevent undefined behavior
 * - Masks all extractions to prevent reading beyond field boundaries
 * - Handles edge cases like r=0 (no characteristic bits)
 *
 * **Limitations:**
 * - Single-word extraction only: N ≤ 64 bits
 * - Mantissa limited to 64 bits: p ≤ 64
 * - For multi-word formats (N > 64), this function cannot extract full mantissa
 *
 * @note This is a test utility only; production code should use takum<N> methods
 * @note The extracted fields exactly match the takum specification bit layout
 * @note For debugging, combine with dump_ui<N>() for human-readable output
 */
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

/**
 * @brief Emit structured failure log entry for CI/CD analysis.
 *
 * Generates a standardized JSON-like log entry for test failures that can be
 * automatically parsed by CI/CD systems. The output is written to stderr
 * for immediate visibility and potential capture by testing frameworks.
 *
 * @param test_name Descriptive name of the failing test (should be unique)
 * @param idx Index or iteration number where the failure occurred
 * @param bits The failing bit pattern as a hexadecimal value
 *
 * @details
 * **Output Format:**
 * ```
 * {"test":"TestName","idx":42,"bits":"0x1a2b3c4d"}
 * ```
 *
 * **Integration with CI:**
 * - Single-line JSON format for easy parsing by log analysis tools
 * - Hexadecimal bit representation for bit-level debugging
 * - Deterministic output for reproducible failure analysis
 * - Written to stderr to distinguish from normal test output
 *
 * **Usage in Tests:**
 * - Call when a specific bit pattern causes test failure
 * - Include sufficient context in test_name for debugging
 * - Use idx to identify the specific iteration in parameterized tests
 *
 * @note This function always writes output; use with CI capture utilities for conditional logging
 * @note The JSON format is intentionally simple to avoid parsing dependencies
 * @see ci_capture_failure_line() for environment-controlled logging
 */
// Small deterministic failure logger used by tests to emit reproducible
// failure artifacts to stderr for CI capture. Writes a short JSON-like line.
inline void emit_failure_log(const char* test_name, size_t idx, uint64_t bits) {
    std::ostringstream os;
    os << "{\"test\":\"" << test_name << "\",\"idx\":" << idx << ",\"bits\":\"0x" << std::hex << bits << "\"}";
    std::cerr << os.str() << std::endl;
}


/**
 * @brief Generate human-readable field dump for debugging takum bit patterns.
 *
 * Produces a detailed, human-readable breakdown of a takum<N> bit pattern
 * showing both the raw hexadecimal representation and the decoded field
 * values. This function is primarily used for interactive debugging and
 * test failure investigation.
 *
 * @tparam N Bit width of the takum format (6 ≤ N ≤ 64)
 * @param ui Packed bit pattern to analyze and display
 *
 * @details
 * **Output Format:**
 * ```
 * 0x1a2b3c4d S=1 c=-42 r=3 m=12345
 * ```
 *
 * **Field Descriptions:**
 * - Hex representation: Raw bit pattern in hexadecimal
 * - S: Sign bit (0=positive, 1=negative)
 * - c: Decoded characteristic value (signed)
 * - r: Regime width (affects precision)
 * - m: Raw mantissa bits as decimal integer
 *
 * **Usage Scenarios:**
 * - Interactive debugging of encoding/decoding issues
 * - Test failure investigation with bit-level detail
 * - Manual verification of specific bit patterns
 * - Educational exploration of the takum format
 *
 * **Output Destination:**
 * - Written to std::cerr for immediate visibility
 * - Appears in test output even with redirected stdout
 * - Single line format for easy log parsing
 *
 * @note This function always produces output; use judiciously in automated tests
 * @note Combines with decode_tuple<N>() to provide both fields and formatting
 * @see decode_tuple<N>() for programmatic field access
 */

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