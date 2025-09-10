#include <iostream>
#include <cmath>
#include "include/takum/types.h"
#include "include/takum/arithmetic.h"

using namespace takum::types;

int main() {
    takum64 base(1.0);
    std::cout << "base(1.0) = " << base.to_double() << ", ell = " << base.get_exact_ell() << std::endl;
    
    for (int k = -12; k <= 12; ++k) {
        double scale = std::exp(k * 0.25);
        takum64 other(scale);
        std::cout << "k=" << k << ", scale=" << scale << ", other.ell=" << other.get_exact_ell() << std::endl;
        
        long double ell_a = base.get_exact_ell();
        long double ell_b = other.get_exact_ell();
        long double mag_a = fabsl(ell_a);
        long double mag_b = fabsl(ell_b);
        if (mag_b > mag_a) { std::swap(mag_a, mag_b); }
        long double ratio = (mag_a == 0.0L) ? 0.0L : (mag_b / mag_a);
        std::cout << "  mag_a=" << mag_a << ", mag_b=" << mag_b << ", ratio=" << ratio << std::endl;
        
        auto r = base + other;
        std::cout << "  result=" << r.to_double() << std::endl;
        if (k == 0) break; // Just test a few
    }
    
    return 0;
}
