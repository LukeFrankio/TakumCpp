#pragma once

/**
 * @file phi_spec.h
 * @brief Specification and constants for Gaussian-log (Φ) approximation in TakumCpp.
 * 
 * This header defines the parameters for the LUT + interpolation / hybrid polynomial
 * approximation of the Gaussian-log function Φ used for addition/subtraction in log domain.
 * 
 * Strategy:
 * - For takum16: LUT size 1024, linear interpolation (Q-format Q15.16).
 * - For takum32: LUT size 4096, linear interpolation (Q-format Q15.16).
 * - For takum64+: Coarse LUT size 256, minimax polynomials degree 5-7 per interval (Q-format Q16.16).
 * 
 * Q-format: Fixed-point representation for LUT entries and coefficients. Qm.n means m integer bits, n fractional bits.
 * Error budgets: Per-interval max/mean absolute errors computed offline and included in generated header.
 * 
 * Generated content: #include "generated/phi_coeffs.h" contains poly_coeffs array, max_errors array.
 * To regenerate: Run `python3 scripts/gen_poly_coeffs.py > include/takum/internal/generated/phi_coeffs.h`
 * 
 * Accuracy targets (Proposition 11): Worst-case error per operation within λ(p) ulp bound for the precision.
 * The offline script produces machine-checkable error reports; see gen_poly_coeffs.py for details.
 */

#include <cstdint>

namespace takum::internal::phi {

constexpr int LUT_SIZE_TAKUM16 = 1024;
constexpr int LUT_SIZE_TAKUM32 = 4096;
constexpr int LUT_SIZE_TAKUM64 = 256;  // Coarse LUT for hybrid

constexpr int POLY_DEGREE_MIN = 5;
constexpr int POLY_DEGREE_MAX = 7;
constexpr int DEFAULT_POLY_DEGREE = 5;  // Used for takum64+

// Q-format for LUT entries (fractional bits)
constexpr int LUT_Q_FRAC_BITS_TAKUM16 = 16;  // Q15.16
constexpr int LUT_Q_FRAC_BITS_TAKUM32 = 16;  // Q15.16
constexpr int LUT_Q_FRAC_BITS_TAKUM64 = 16;  // Q16.16 for polys

// Q-format for polynomial coefficients
constexpr int POLY_Q_FRAC_BITS = 16;  // Q16.16

// Include generated coefficients and error bounds
#include "generated/phi_coeffs.h"

// Interpolation formula: Linear fixed-point (default); cubic details in implementation.
// Worst-case error bounds: Use max_errors[interval] from generated header.
// For Prop 11 guarantee: Sum of Φ errors + rounding in encode/decode < total budget.

}  // namespace takum::internal::phi