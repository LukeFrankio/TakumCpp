/**
 * @file phi_bench.h
 * @brief Minimal inline benchmarking utilities for Φ (Gaussian-log) evaluation.
 *
 * This header provides lightweight performance measurement tools specifically
 * designed for profiling and optimizing Φ function evaluation methods. The
 * utilities are header-only and compile only when explicitly included by
 * benchmark translation units.
 *
 * @details
 * **Design Philosophy:**
 * - Minimal overhead: uses only standard library timing facilities
 * - Optional inclusion: not compiled unless explicitly used
 * - Zero-cost abstraction: all functions are inline templates
 * - Platform agnostic: works on any system with std::chrono
 *
 * **Usage Pattern:**
 * ```cpp
 * #include "takum/internal/phi_bench.h"
 * 
 * // Time a specific operation
 * auto ns = takum::internal::phi::bench::time_ns([]() {
 *     auto result = phi_eval<32>(0.25);
 * }, 10000);
 * 
 * // Sweep performance test
 * auto sum = takum::internal::phi::bench::sweep_sum<64>(1024);
 * ```
 *
 * @note This header deliberately avoids <chrono> inclusion in core library
 *       headers to minimize compilation dependencies and build times.
 *
 * @see takum::internal::phi::phi_eval for the functions being benchmarked
 */

#pragma once

#include <cstdint>
#include <chrono>
#include "takum/internal/phi_eval.h"

/**
 * @namespace takum::internal::phi::bench
 * @brief Benchmarking utilities for Φ function evaluation performance analysis.
 *
 * This namespace contains lightweight profiling tools designed specifically
 * for measuring and optimizing Φ function evaluation performance across
 * different takum precisions and evaluation strategies.
 */
namespace takum::internal::phi::bench {

/**
 * @brief High-resolution timing utility for performance measurement.
 *
 * Measures the total execution time of a callable object over multiple
 * iterations using high-resolution system timing. Designed for benchmarking
 * small, fast operations like Φ function evaluations.
 *
 * @tparam F Callable type (function, lambda, functor)
 * @param f Callable object to time
 * @param iters Number of iterations to perform (default: 1000)
 * @return Total execution time in nanoseconds
 *
 * @details
 * **Measurement Strategy:**
 * - Uses std::chrono::high_resolution_clock for maximum precision
 * - Measures total time over multiple iterations to reduce noise
 * - Returns raw nanosecond count for flexible post-processing
 * - Assumes callable has negligible setup/teardown overhead
 *
 * **Usage Guidelines:**
 * - Use enough iterations (≥1000) for stable measurements
 * - Ensure callable is deterministic and side-effect free
 * - Consider compiler optimizations when interpreting results
 * - Warm up CPU/cache before critical measurements
 *
 * @note Total time is measured; divide by iters for per-operation timing
 */
template <typename F>
inline uint64_t time_ns(F&& f, size_t iters = 1000) {
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iters; ++i) {
        f();
    }
    auto end = std::chrono::high_resolution_clock::now();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
}

/**
 * @brief Sweep-based performance test with numerical stability check.
 *
 * Evaluates Φ function across a uniform sampling of the domain [-0.5, 0.5]
 * and accumulates results to prevent compiler optimization while providing
 * a realistic workload pattern.
 *
 * @tparam N Takum bit width for the Φ evaluation
 * @param samples Number of evaluation points across the domain (default: 1024)
 * @return Sum of all Φ evaluations (for optimization prevention)
 *
 * @details
 * **Sweep Pattern:**
 * - Uniform sampling over domain [-0.5, 0.5]
 * - Processes samples * sizeof(long double) bytes of computation
 * - Accumulates results to prevent dead code elimination
 * - Exercises typical usage patterns for Φ evaluation
 *
 * **Performance Analysis:**
 * - Total operations: samples Φ evaluations + samples accumulations
 * - Memory access: depends on LUT sizes and polynomial coefficients
 * - Cache behavior: exhibits locality patterns of real usage
 * - Branch prediction: exercises conditional paths in evaluation
 *
 * **Return Value Usage:**
 * The returned sum serves dual purposes:
 * 1. Prevents compiler from optimizing away the computation
 * 2. Provides a checksum for correctness validation across runs
 *
 * @note Combine with time_ns() for complete performance profiling
 */
template <size_t N>
inline long double sweep_sum(size_t samples = 1024) {
    long double acc = 0.0L;
    for (size_t i = 0; i < samples; ++i) {
        long double t = -0.5L + (static_cast<long double>(i) / static_cast<long double>(samples));
        acc += phi_eval<N>(t).value;
    }
    return acc;
}
}

} // namespace takum::internal::phi::bench