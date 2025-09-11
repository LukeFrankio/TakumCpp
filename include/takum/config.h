/**
 * @file config.h
 * @brief Central configuration and feature toggles for TakumCpp Phase-4 implementation.
 *
 * This header provides compile-time configuration options that control various
 * performance and accuracy trade-offs in the takum arithmetic library. All
 * configuration options have sensible defaults and can be overridden by defining
 * macros before including any takum headers.
 *
 * @details
 * The configuration system allows fine-tuning of:
 * - Addition algorithm variants (fast heuristics vs. precision)
 * - Φ function interpolation methods (linear vs. cubic)
 * - Lookup table sizes for hybrid approximation
 * - Diagnostic and instrumentation features
 *
 * **Usage Example:**
 * ```cpp
 * #define TAKUM_ENABLE_FAST_ADD 1
 * #define TAKUM_COARSE_LUT_SIZE 512  
 * #include "takum/arithmetic.h"
 * ```
 *
 * @see takum::config namespace for runtime configuration queries
 */

#pragma once

#include "compiler_detection.h"

/**
 * @def TAKUM_ENABLE_FAST_ADD
 * @brief Enable fast heuristic addition pre-pass for improved performance.
 *
 * When enabled (non-zero), activates a fast heuristic path in addition operations
 * that may trade some accuracy for significant speed improvements in common cases.
 * The heuristic detects when operands have large magnitude differences and uses
 * simplified computation paths.
 *
 * @note Default: 0 (disabled) - prioritizes accuracy over speed
 * @note May introduce small additional rounding errors in edge cases
 */
#ifndef TAKUM_ENABLE_FAST_ADD
#define TAKUM_ENABLE_FAST_ADD 0
#endif

/**
 * @def TAKUM_ENABLE_CUBIC_PHI_LUT
 * @brief Enable cubic Catmull-Rom interpolation in Φ function lookup tables.
 *
 * When enabled (non-zero), uses cubic spline interpolation instead of linear
 * interpolation for small Φ lookup tables. This typically halves interpolation
 * error for the same table size but increases computational cost.
 *
 * @note Default: 0 (disabled) - uses linear interpolation for speed
 * @note Cubic interpolation provides smoother derivatives and better accuracy
 * @see takum::internal::phi::detail::phi_lut_cubic for implementation details
 */
#ifndef TAKUM_ENABLE_CUBIC_PHI_LUT
#define TAKUM_ENABLE_CUBIC_PHI_LUT 0
#endif

/**
 * @def TAKUM_COARSE_LUT_SIZE
 * @brief Coarse hybrid lookup table size for takum64+ Φ evaluation.
 *
 * Specifies the number of entries in the coarse lookup table used for initial
 * domain partitioning in high-precision Φ evaluation. Must be a power-of-two
 * multiple of 32 for optimal performance.
 *
 * @note Default: 256 entries - balances memory usage and accuracy
 * @note Larger values improve accuracy but increase memory footprint
 * @note Must be power-of-two multiple of 32 (32, 64, 128, 256, 512, ...)
 */
#ifndef TAKUM_COARSE_LUT_SIZE
#define TAKUM_COARSE_LUT_SIZE 256
#endif

/**
 * @def TAKUM_ENABLE_PHI_DIAGNOSTICS
 * @brief Enable lightweight performance counter instrumentation.
 *
 * When enabled (non-zero), collects non-atomic performance counters for
 * Φ function evaluation paths. Useful for profiling and optimization but
 * adds minimal runtime overhead.
 *
 * @note Default: 1 (enabled) - overhead is negligible
 * @note Counters are thread-local and non-atomic for minimal performance impact
 * @see takum::internal::phi::diagnostics for counter access
 */
#ifndef TAKUM_ENABLE_PHI_DIAGNOSTICS
#define TAKUM_ENABLE_PHI_DIAGNOSTICS 1
#endif

/**
 * @namespace takum::config
 * @brief Runtime configuration query interface for takum library settings.
 *
 * This namespace provides constexpr functions to query the current configuration
 * settings at runtime. All functions are compile-time constants and have zero
 * runtime overhead.
 */
namespace takum { namespace config {

/**
 * @brief Query whether fast addition heuristics are enabled.
 * @return true if TAKUM_ENABLE_FAST_ADD is non-zero, false otherwise
 */
constexpr bool fast_add() noexcept { return TAKUM_ENABLE_FAST_ADD != 0; }

/**
 * @brief Query whether cubic Φ LUT interpolation is enabled.
 * @return true if TAKUM_ENABLE_CUBIC_PHI_LUT is non-zero, false otherwise
 */
constexpr bool cubic_phi_lut() noexcept { return TAKUM_ENABLE_CUBIC_PHI_LUT != 0; }

/**
 * @brief Get the configured coarse hybrid LUT size.
 * @return Number of entries in coarse LUT (always power-of-two multiple of 32)
 */
constexpr int coarse_hybrid_lut_size() noexcept { return TAKUM_COARSE_LUT_SIZE; }

/**
 * @brief Query whether Φ diagnostics instrumentation is enabled.
 * @return true if TAKUM_ENABLE_PHI_DIAGNOSTICS is non-zero, false otherwise
 */
constexpr bool phi_diagnostics() noexcept { return TAKUM_ENABLE_PHI_DIAGNOSTICS != 0; }

} } // namespace takum::config
