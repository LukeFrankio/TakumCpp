#pragma once
#include <cstdint>

namespace takum::internal::phi {

// Result bundle for internal Φ evaluations.
struct PhiEvalResult {
    long double value;      // approximated Φ(t)
    long double abs_error;  // conservative bound for |true - value|
    int interval;           // interval / cell index (semantics depend on strategy)
};

} // namespace takum::internal::phi
