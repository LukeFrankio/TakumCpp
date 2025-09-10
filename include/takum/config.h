// takum/config.h
// Central configuration & feature toggles for TakumCpp Phase-4.
#pragma once

// Enable fast heuristic add pre-pass (may trade some accuracy for speed).
#ifndef TAKUM_ENABLE_FAST_ADD
#define TAKUM_ENABLE_FAST_ADD 0
#endif

// Enable cubic Catmull-Rom interpolation in small Φ LUTs.
#ifndef TAKUM_ENABLE_CUBIC_PHI_LUT
#define TAKUM_ENABLE_CUBIC_PHI_LUT 0
#endif

// Coarse hybrid LUT size for takum64+ Φ evaluation (must be power of two multiple of 32).
#ifndef TAKUM_COARSE_LUT_SIZE
#define TAKUM_COARSE_LUT_SIZE 256
#endif

// Instrumentation: collect lightweight (non-atomic) counters.
#ifndef TAKUM_ENABLE_PHI_DIAGNOSTICS
#define TAKUM_ENABLE_PHI_DIAGNOSTICS 1
#endif

namespace takum { namespace config {
constexpr bool fast_add() noexcept { return TAKUM_ENABLE_FAST_ADD != 0; }
constexpr bool cubic_phi_lut() noexcept { return TAKUM_ENABLE_CUBIC_PHI_LUT != 0; }
constexpr int coarse_hybrid_lut_size() noexcept { return TAKUM_COARSE_LUT_SIZE; }
constexpr bool phi_diagnostics() noexcept { return TAKUM_ENABLE_PHI_DIAGNOSTICS != 0; }
} } // namespace takum::config
