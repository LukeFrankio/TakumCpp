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

namespace takum::types {

using takum8  = ::takum::takum<8>;
using takum16 = ::takum::takum<16>;
using takum19 = ::takum::takum<19>;
using takum32 = ::takum::takum<32>;
using takum64 = ::takum::takum<64>;
using takum128 = ::takum::takum<128>;

} // namespace takum::types
