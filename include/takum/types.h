#pragma once

/**
 * @file types.h
 * @brief Convenience typedefs for common takum instantiations.
 *
 * This header provides short aliases like `takum32` and `takum64` that map to
 * `takum::takum<N>` for common bit widths used in examples and tests. These
 * aliases exist purely for convenience and do not change any semantics.
 */

#include "core.h"

/**
 * @namespace takum::types  
 * @brief Convenience namespace containing common takum type aliases.
 *
 * This namespace provides short, descriptive names for commonly used takum
 * instantiations. These aliases are purely for convenience and don't change
 * any semantics of the underlying takum<N> types.
 */
namespace takum::types {

/// @brief 8-bit takum type alias for ultra-compact representations
using takum8  = ::takum::takum<8>;
/// @brief 16-bit takum type alias, similar to half-precision float
using takum16 = ::takum::takum<16>;
/// @brief 19-bit takum type alias, similar to TensorFloat-32
using takum19 = ::takum::takum<19>;
/// @brief 32-bit takum type alias, similar to single-precision float  
using takum32 = ::takum::takum<32>;
/// @brief 64-bit takum type alias, similar to double-precision float
using takum64 = ::takum::takum<64>;
/// @brief 128-bit takum type alias for extended precision applications
using takum128 = ::takum::takum<128>;

} // namespace takum::types
