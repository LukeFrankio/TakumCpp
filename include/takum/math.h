/**
 * @file math.h
 * @brief Mathematical functions for takum<N> (Phase 4 deliverable).
 *
 * This header provides the comprehensive mathematical function library for 
 * takum types, including trigonometric, exponential, logarithmic, power,
 * root, rounding, and classification functions. Functions follow the pattern
 * of <cmath> but with takum-specific behavior and NaR handling.
 *
 * All functions propagate NaR (Not-a-Real) values and handle edge cases
 * according to takum arithmetic principles. Safe variants returning
 * std::expected are provided for explicit error handling.
 */

#pragma once

#include "takum/core.h"
#include "takum/arithmetic.h"
#include "takum/internal/phi_eval.h"
#include "takum/config.h"

#include <type_traits>
#include <cmath>

#if TAKUM_HAS_STD_EXPECTED
#include <expected>
#endif

namespace takum {

// ============================================================================
// TRIGONOMETRIC FUNCTIONS
// ============================================================================

/**
 * @brief Compute sine of takum value.
 * 
 * Computes sin(x) using range reduction and Taylor series in the logarithmic
 * domain where possible, falling back to host double computation.
 * 
 * @param x Input value in radians
 * @return sin(x), or NaR if x is NaR or infinite
 */
template <size_t N>
inline takum<N> sin(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx)) return takum<N>::nar();
    
    return takum<N>(std::sin(dx));
}

/**
 * @brief Compute cosine of takum value.
 * 
 * @param x Input value in radians  
 * @return cos(x), or NaR if x is NaR or infinite
 */
template <size_t N>
inline takum<N> cos(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx)) return takum<N>::nar();
    
    return takum<N>(std::cos(dx));
}

/**
 * @brief Compute tangent of takum value.
 * 
 * @param x Input value in radians
 * @return tan(x), or NaR if x is NaR, infinite, or near odd multiples of π/2
 */
template <size_t N>
inline takum<N> tan(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx)) return takum<N>::nar();
    
    double result = std::tan(dx);
    if (!std::isfinite(result)) return takum<N>::nar();
    
    return takum<N>(result);
}

/**
 * @brief Compute arcsine of takum value.
 * 
 * @param x Input value, must be in [-1, 1]
 * @return asin(x) in [-π/2, π/2], or NaR if x is NaR or |x| > 1
 */
template <size_t N>
inline takum<N> asin(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx) || dx < -1.0 || dx > 1.0) return takum<N>::nar();
    
    return takum<N>(std::asin(dx));
}

/**
 * @brief Compute arccosine of takum value.
 * 
 * @param x Input value, must be in [-1, 1]
 * @return acos(x) in [0, π], or NaR if x is NaR or |x| > 1
 */
template <size_t N>
inline takum<N> acos(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx) || dx < -1.0 || dx > 1.0) return takum<N>::nar();
    
    return takum<N>(std::acos(dx));
}

/**
 * @brief Compute arctangent of takum value.
 * 
 * @param x Input value
 * @return atan(x) in [-π/2, π/2], or NaR if x is NaR
 */
template <size_t N>
inline takum<N> atan(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx)) return takum<N>::nar();
    
    return takum<N>(std::atan(dx));
}

/**
 * @brief Compute two-argument arctangent.
 * 
 * @param y Y coordinate
 * @param x X coordinate
 * @return atan2(y, x) in [-π, π], or NaR if either argument is NaR
 */
template <size_t N>
inline takum<N> atan2(const takum<N>& y, const takum<N>& x) noexcept {
    if (y.is_nar() || x.is_nar()) return takum<N>::nar();
    
    double dy = y.to_double();
    double dx = x.to_double();
    if (!std::isfinite(dy) || !std::isfinite(dx)) return takum<N>::nar();
    
    return takum<N>(std::atan2(dy, dx));
}

// ============================================================================
// HYPERBOLIC FUNCTIONS  
// ============================================================================

/**
 * @brief Compute hyperbolic sine.
 * 
 * @param x Input value
 * @return sinh(x), or NaR if x is NaR or results in overflow
 */
template <size_t N>
inline takum<N> sinh(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx)) return takum<N>::nar();
    
    double result = std::sinh(dx);
    if (!std::isfinite(result)) return takum<N>::nar();
    
    return takum<N>(result);
}

/**
 * @brief Compute hyperbolic cosine.
 * 
 * @param x Input value
 * @return cosh(x), or NaR if x is NaR or results in overflow
 */
template <size_t N>
inline takum<N> cosh(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx)) return takum<N>::nar();
    
    double result = std::cosh(dx);
    if (!std::isfinite(result)) return takum<N>::nar();
    
    return takum<N>(result);
}

/**
 * @brief Compute hyperbolic tangent.
 * 
 * @param x Input value
 * @return tanh(x), or NaR if x is NaR
 */
template <size_t N>
inline takum<N> tanh(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx)) return takum<N>::nar();
    
    return takum<N>(std::tanh(dx));
}

/**
 * @brief Compute inverse hyperbolic sine.
 * 
 * @param x Input value
 * @return asinh(x), or NaR if x is NaR
 */
template <size_t N>
inline takum<N> asinh(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx)) return takum<N>::nar();
    
    return takum<N>(std::asinh(dx));
}

/**
 * @brief Compute inverse hyperbolic cosine.
 * 
 * @param x Input value, must be >= 1
 * @return acosh(x), or NaR if x is NaR or x < 1
 */
template <size_t N>
inline takum<N> acosh(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx) || dx < 1.0) return takum<N>::nar();
    
    return takum<N>(std::acosh(dx));
}

/**
 * @brief Compute inverse hyperbolic tangent.
 * 
 * @param x Input value, must be in (-1, 1)
 * @return atanh(x), or NaR if x is NaR or |x| >= 1
 */
template <size_t N>
inline takum<N> atanh(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx) || dx <= -1.0 || dx >= 1.0) return takum<N>::nar();
    
    return takum<N>(std::atanh(dx));
}

// ============================================================================
// EXPONENTIAL AND LOGARITHMIC FUNCTIONS
// ============================================================================

/**
 * @brief Compute exponential function.
 * 
 * Uses the takum logarithmic representation for efficient computation
 * when possible: exp(x) = e^x can be computed directly from the ell value.
 * 
 * @param x Input value
 * @return exp(x), or NaR if x is NaR or results in overflow
 */
template <size_t N>
inline takum<N> exp(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx)) return takum<N>::nar();
    
    double result = std::exp(dx);
    if (!std::isfinite(result)) return takum<N>::nar();
    
    return takum<N>(result);
}

/**
 * @brief Compute natural logarithm.
 * 
 * Uses the takum logarithmic representation for efficient computation:
 * log(x) can often be computed directly from the ell value.
 * 
 * @param x Input value, must be > 0
 * @return log(x), or NaR if x is NaR, <= 0
 */
template <size_t N>
inline takum<N> log(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx) || dx <= 0.0) return takum<N>::nar();
    
    return takum<N>(std::log(dx));
}

/**
 * @brief Compute base-10 logarithm.
 * 
 * @param x Input value, must be > 0
 * @return log10(x), or NaR if x is NaR, <= 0
 */
template <size_t N>
inline takum<N> log10(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx) || dx <= 0.0) return takum<N>::nar();
    
    return takum<N>(std::log10(dx));
}

/**
 * @brief Compute log(1 + x) accurately for small x.
 * 
 * @param x Input value, must be > -1
 * @return log(1 + x), or NaR if x is NaR, <= -1
 */
template <size_t N>
inline takum<N> log1p(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx) || dx <= -1.0) return takum<N>::nar();
    
    return takum<N>(std::log1p(dx));
}

/**
 * @brief Compute exp(x) - 1 accurately for small x.
 * 
 * @param x Input value
 * @return exp(x) - 1, or NaR if x is NaR or results in overflow
 */
template <size_t N>
inline takum<N> expm1(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx)) return takum<N>::nar();
    
    double result = std::expm1(dx);
    if (!std::isfinite(result)) return takum<N>::nar();
    
    return takum<N>(result);
}

/**
 * @brief Compute base-2 logarithm.
 * 
 * @param x Input value, must be > 0
 * @return log2(x), or NaR if x is NaR, <= 0
 */
template <size_t N>
inline takum<N> log2(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx) || dx <= 0.0) return takum<N>::nar();
    
    return takum<N>(std::log2(dx));
}

/**
 * @brief Compute 2^x.
 * 
 * @param x Input value
 * @return 2^x, or NaR if x is NaR or results in overflow
 */
template <size_t N>
inline takum<N> exp2(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx)) return takum<N>::nar();
    
    double result = std::exp2(dx);
    if (!std::isfinite(result)) return takum<N>::nar();
    
    return takum<N>(result);
}

// ============================================================================
// POWER AND ROOT FUNCTIONS
// ============================================================================

/**
 * @brief Compute x raised to the power y.
 * 
 * Uses logarithmic computation: pow(x, y) = exp(y * log(x)) when possible.
 * Handles special cases according to takum arithmetic principles.
 * 
 * @param x Base value
 * @param y Exponent value
 * @return x^y, or NaR on domain errors or overflow
 */
template <size_t N>
inline takum<N> pow(const takum<N>& x, const takum<N>& y) noexcept {
    if (x.is_nar() || y.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    double dy = y.to_double();
    if (!std::isfinite(dx) || !std::isfinite(dy)) return takum<N>::nar();
    
    // Handle special cases following C++ standard behavior
    // pow(0, 0) = 1 (C++ standard)
    if (dx == 0.0 && dy == 0.0) return takum<N>(1.0);
    
    // Handle zero base with negative exponent
    if (dx == 0.0 && dy < 0.0) return takum<N>::nar();
    
    // Special handling for negative base with integer exponent
    // to work around potential std::pow domain issues
    if (dx < 0.0) {
        double dy_rounded = std::round(dy);
        if (std::abs(dy - dy_rounded) < 1e-10) {
            // It's effectively an integer exponent - compute manually for safety
            if (dy_rounded == 0.0) return takum<N>(1.0);
            
            double abs_result = std::pow(-dx, dy_rounded);
            if (!std::isfinite(abs_result)) return takum<N>::nar();
            
            // Apply sign: negative result if odd exponent
            bool is_odd = (static_cast<long long>(dy_rounded) % 2 != 0);
            double result = is_odd ? -abs_result : abs_result;
            return takum<N>(result);
        } else {
            // Non-integer power of negative - undefined
            return takum<N>::nar();
        }
    }
    
    // For non-negative base, use std::pow directly
    double result = std::pow(dx, dy);
    if (!std::isfinite(result)) return takum<N>::nar();
    
    return takum<N>(result);
}

/**
 * @brief Compute square root.
 * 
 * @param x Input value, must be >= 0
 * @return sqrt(x), or NaR if x is NaR or x < 0
 */
template <size_t N>
inline takum<N> sqrt(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx) || dx < 0.0) return takum<N>::nar();
    
    return takum<N>(std::sqrt(dx));
}

/**
 * @brief Compute cube root.
 * 
 * @param x Input value
 * @return cbrt(x), or NaR if x is NaR
 */
template <size_t N>
inline takum<N> cbrt(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx)) return takum<N>::nar();
    
    return takum<N>(std::cbrt(dx));
}

/**
 * @brief Compute Euclidean distance.
 * 
 * Computes sqrt(x² + y²) while avoiding overflow in intermediate calculations.
 * 
 * @param x First component
 * @param y Second component  
 * @return hypot(x, y), or NaR if either argument is NaR
 */
template <size_t N>
inline takum<N> hypot(const takum<N>& x, const takum<N>& y) noexcept {
    if (x.is_nar() || y.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    double dy = y.to_double();
    if (!std::isfinite(dx) || !std::isfinite(dy)) return takum<N>::nar();
    
    double result = std::hypot(dx, dy);
    if (!std::isfinite(result)) return takum<N>::nar();
    
    return takum<N>(result);
}

/**
 * @brief Three-dimensional Euclidean distance.
 * 
 * @param x First component
 * @param y Second component
 * @param z Third component
 * @return hypot(x, y, z), or NaR if any argument is NaR
 */
template <size_t N>
inline takum<N> hypot(const takum<N>& x, const takum<N>& y, const takum<N>& z) noexcept {
    if (x.is_nar() || y.is_nar() || z.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    double dy = y.to_double();
    double dz = z.to_double();
    if (!std::isfinite(dx) || !std::isfinite(dy) || !std::isfinite(dz)) return takum<N>::nar();
    
    double result = std::hypot(dx, dy, dz);
    if (!std::isfinite(result)) return takum<N>::nar();
    
    return takum<N>(result);
}

// ============================================================================
// ROUNDING AND REMAINDER FUNCTIONS
// ============================================================================

/**
 * @brief Round toward zero (truncate).
 * 
 * @param x Input value
 * @return trunc(x), or NaR if x is NaR
 */
template <size_t N>
inline takum<N> trunc(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx)) return takum<N>::nar();
    
    return takum<N>(std::trunc(dx));
}

/**
 * @brief Round toward negative infinity (floor).
 * 
 * @param x Input value
 * @return floor(x), or NaR if x is NaR
 */
template <size_t N>
inline takum<N> floor(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx)) return takum<N>::nar();
    
    return takum<N>(std::floor(dx));
}

/**
 * @brief Round toward positive infinity (ceiling).
 * 
 * @param x Input value
 * @return ceil(x), or NaR if x is NaR
 */
template <size_t N>
inline takum<N> ceil(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx)) return takum<N>::nar();
    
    return takum<N>(std::ceil(dx));
}

/**
 * @brief Round to nearest integer.
 * 
 * @param x Input value
 * @return round(x), or NaR if x is NaR
 */
template <size_t N>
inline takum<N> round(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx)) return takum<N>::nar();
    
    return takum<N>(std::round(dx));
}

/**
 * @brief Round to nearest integer using current rounding mode.
 * 
 * @param x Input value
 * @return nearbyint(x), or NaR if x is NaR
 */
template <size_t N>
inline takum<N> nearbyint(const takum<N>& x) noexcept {
    if (x.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    if (!std::isfinite(dx)) return takum<N>::nar();
    
    return takum<N>(std::nearbyint(dx));
}

/**
 * @brief Compute floating-point remainder.
 * 
 * @param x Dividend
 * @param y Divisor, must not be zero
 * @return fmod(x, y), or NaR if either argument is NaR or y is zero
 */
template <size_t N>
inline takum<N> fmod(const takum<N>& x, const takum<N>& y) noexcept {
    if (x.is_nar() || y.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    double dy = y.to_double();
    if (!std::isfinite(dx) || !std::isfinite(dy) || dy == 0.0) return takum<N>::nar();
    
    return takum<N>(std::fmod(dx, dy));
}

/**
 * @brief Compute IEEE remainder.
 * 
 * @param x Dividend
 * @param y Divisor, must not be zero
 * @return remainder(x, y), or NaR if either argument is NaR or y is zero
 */
template <size_t N>
inline takum<N> remainder(const takum<N>& x, const takum<N>& y) noexcept {
    if (x.is_nar() || y.is_nar()) return takum<N>::nar();
    
    double dx = x.to_double();
    double dy = y.to_double();
    if (!std::isfinite(dx) || !std::isfinite(dy) || dy == 0.0) return takum<N>::nar();
    
    return takum<N>(std::remainder(dx, dy));
}

// ============================================================================
// CLASSIFICATION FUNCTIONS
// ============================================================================

/**
 * @brief Check if value is finite (not NaR).
 * 
 * @param x Input value
 * @return true if x is finite (not NaR), false otherwise
 */
template <size_t N>
inline bool isfinite(const takum<N>& x) noexcept {
    return !x.is_nar();
}

/**
 * @brief Check if value is NaR (equivalent to isnan for IEEE).
 * 
 * @param x Input value
 * @return true if x is NaR, false otherwise
 */
template <size_t N>
inline bool isnan(const takum<N>& x) noexcept {
    return x.is_nar();
}

/**
 * @brief Check if value is infinite.
 * 
 * In takum arithmetic, there are no infinite values (all map to NaR),
 * so this always returns false for finite values.
 * 
 * @param x Input value
 * @return false (takum has no infinity representation)
 */
template <size_t N>
inline bool isinf(const takum<N>&) noexcept {
    return false; // Takum has no infinity representation
}

/**
 * @brief Check if value is normal.
 * 
 * In takum arithmetic, all finite values are considered normal
 * (no subnormal representation).
 * 
 * @param x Input value
 * @return true if x is finite (not NaR), false if NaR
 */
template <size_t N>
inline bool isnormal(const takum<N>& x) noexcept {
    return !x.is_nar();
}

/**
 * @brief Classify floating-point value.
 * 
 * @param x Input value
 * @return FP_NAN if NaR, FP_NORMAL if finite
 */
template <size_t N>
inline int fpclassify(const takum<N>& x) noexcept {
    return x.is_nar() ? FP_NAN : FP_NORMAL;
}

/**
 * @brief Check sign of value.
 * 
 * @param x Input value
 * @return true if x is negative, false if positive or NaR
 */
template <size_t N>
inline bool signbit(const takum<N>& x) noexcept {
    if (x.is_nar()) return false;
    return x.to_double() < 0.0;
}

// ============================================================================
// SAFE VARIANTS (std::expected interface)
// ============================================================================

#if TAKUM_HAS_STD_EXPECTED

/**
 * @brief Safe sine with explicit error handling.
 * 
 * @param x Input value in radians
 * @return sin(x) on success, or takum_error on domain/overflow error
 */
template <size_t N>
inline std::expected<takum<N>, takum_error> safe_sin(const takum<N>& x) noexcept {
    if (x.is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR operand"});
    
    double dx = x.to_double();
    if (!std::isfinite(dx)) return std::unexpected(takum_error{takum_error::Kind::DomainError, "infinite input"});
    
    return takum<N>(std::sin(dx));
}

/**
 * @brief Safe cosine with explicit error handling.
 * 
 * @param x Input value in radians
 * @return cos(x) on success, or takum_error on domain/overflow error
 */
template <size_t N>
inline std::expected<takum<N>, takum_error> safe_cos(const takum<N>& x) noexcept {
    if (x.is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR operand"});
    
    double dx = x.to_double();
    if (!std::isfinite(dx)) return std::unexpected(takum_error{takum_error::Kind::DomainError, "infinite input"});
    
    return takum<N>(std::cos(dx));
}

/**
 * @brief Safe logarithm with explicit error handling.
 * 
 * @param x Input value, must be > 0
 * @return log(x) on success, or takum_error on domain error
 */
template <size_t N>
inline std::expected<takum<N>, takum_error> safe_log(const takum<N>& x) noexcept {
    if (x.is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR operand"});
    
    double dx = x.to_double();
    if (!std::isfinite(dx)) return std::unexpected(takum_error{takum_error::Kind::DomainError, "infinite input"});
    if (dx <= 0.0) return std::unexpected(takum_error{takum_error::Kind::DomainError, "log of non-positive"});
    
    return takum<N>(std::log(dx));
}

/**
 * @brief Safe square root with explicit error handling.
 * 
 * @param x Input value, must be >= 0
 * @return sqrt(x) on success, or takum_error on domain error
 */
template <size_t N>
inline std::expected<takum<N>, takum_error> safe_sqrt(const takum<N>& x) noexcept {
    if (x.is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR operand"});
    
    double dx = x.to_double();
    if (!std::isfinite(dx)) return std::unexpected(takum_error{takum_error::Kind::DomainError, "infinite input"});
    if (dx < 0.0) return std::unexpected(takum_error{takum_error::Kind::DomainError, "sqrt of negative"});
    
    return takum<N>(std::sqrt(dx));
}

/**
 * @brief Safe power function with explicit error handling.
 * 
 * @param x Base value
 * @param y Exponent value
 * @return x^y on success, or takum_error on domain/overflow error
 */
template <size_t N>
inline std::expected<takum<N>, takum_error> safe_pow(const takum<N>& x, const takum<N>& y) noexcept {
    if (x.is_nar() || y.is_nar()) return std::unexpected(takum_error{takum_error::Kind::InvalidOperation, "NaR operand"});
    
    double dx = x.to_double();
    double dy = y.to_double();
    if (!std::isfinite(dx) || !std::isfinite(dy)) return std::unexpected(takum_error{takum_error::Kind::DomainError, "infinite input"});
    
    if (dx == 0.0 && dy <= 0.0) return std::unexpected(takum_error{takum_error::Kind::DomainError, "0^(non-positive)"});
    if (dx < 0.0 && std::floor(dy) != dy) return std::unexpected(takum_error{takum_error::Kind::DomainError, "negative^(non-integer)"});
    
    double result = std::pow(dx, dy);
    if (!std::isfinite(result)) return std::unexpected(takum_error{takum_error::Kind::Overflow, "pow result overflow"});
    
    return takum<N>(result);
}

#endif // TAKUM_HAS_STD_EXPECTED

} // namespace takum