#include <iostream>
#include <cmath>
#include "takum/core.h"
#include "takum/types.h"

int main() {
    for (uint64_t bits = 0; bits < (1ULL << 12); ++bits) {
        takum::takum<12> t;
        t.storage = static_cast<uint32_t>(bits);
        double val = t.to_double();
        if (std::isnan(val)) {
            std::cout << "NaN for bits " << bits << std::endl;
        }
    }
    return 0;
}