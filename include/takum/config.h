/**
 * @file config.h
 * @brief Central configuration and feature toggles for TakumCpp Phase-4.
 *
 * This header provides compile-time configuration macros and runtime accessors
 * for controlling advanced features and performance optimizations in the 
 * TakumCpp library. These settings allow fine-tuning of the Φ (Gaussian-log)
 * evaluation strategy, LUT sizes, and diagnostic instrumentation.
 *
 * Configuration options:
 * - TAKUM_ENABLE_FAST_ADD: Enable heuristic addition optimization
 * - TAKUM_ENABLE_CUBIC_PHI_LUT: Use cubic interpolation in small LUTs
 * - TAKUM_COARSE_LUT_SIZE: Size of coarse hybrid LUT for larger takum types
 * - TAKUM_ENABLE_PHI_DIAGNOSTICS: Enable lightweight performance counters
 */
#pragma once

#include "compiler_detection.h"

/**
 * @brief Enable fast heuristic addition pre-pass optimization.
 * 
 * When enabled, addition operations may use faster heuristics that trade
 * some accuracy for improved performance. This is primarily useful for
 * applications where speed is more critical than maximum precision.
 */
#ifndef TAKUM_ENABLE_FAST_ADD
#define TAKUM_ENABLE_FAST_ADD 0
#endif

/**
 * @brief Enable cubic Catmull-Rom interpolation in small Φ lookup tables.
 * 
 * When enabled, small lookup tables used for Φ (Gaussian-log) evaluation
 * will use cubic interpolation instead of linear interpolation, providing
 * better accuracy at the cost of increased computational complexity.
 */
#ifndef TAKUM_ENABLE_CUBIC_PHI_LUT
#define TAKUM_ENABLE_CUBIC_PHI_LUT 0
#endif

/**
 * @brief Coarse hybrid LUT size for takum64+ Φ evaluation.
 * 
 * Specifies the size of the coarse lookup table used in hybrid Φ evaluation
 * for larger takum types (64+ bits). Must be a power of two multiple of 32.
 * Larger values provide better accuracy but require more memory.
 */
#ifndef TAKUM_COARSE_LUT_SIZE
#define TAKUM_COARSE_LUT_SIZE 256
#endif

/**
 * @brief Enable lightweight diagnostic instrumentation.
 * 
 * When enabled, the library collects non-atomic performance counters that
 * can be used for profiling and optimization. These counters provide insight
 * into Φ evaluation performance and LUT usage patterns.
 */
#ifndef TAKUM_ENABLE_PHI_DIAGNOSTICS
#define TAKUM_ENABLE_PHI_DIAGNOSTICS 1
#endif

/**
 * @namespace takum::config
 * @brief Runtime configuration accessor functions for TakumCpp features.
 * 
 * This namespace provides constexpr functions that return the current
 * configuration values set by the compile-time macros. These functions
 * allow runtime code to query the active configuration without directly
 * using preprocessor macros.
 */
namespace takum { namespace config {

/// @brief Returns true if fast addition optimization is enabled
constexpr bool fast_add() noexcept { return TAKUM_ENABLE_FAST_ADD != 0; }

/// @brief Returns true if cubic Φ LUT interpolation is enabled  
constexpr bool cubic_phi_lut() noexcept { return TAKUM_ENABLE_CUBIC_PHI_LUT != 0; }

/// @brief Returns the configured coarse hybrid LUT size
constexpr int coarse_hybrid_lut_size() noexcept { return TAKUM_COARSE_LUT_SIZE; }
constexpr bool phi_diagnostics() noexcept { return TAKUM_ENABLE_PHI_DIAGNOSTICS != 0; }
} } // namespace takum::config
