#include <iostream>
#include <cmath>
#include "include/takum/types.h"
#include "include/takum/arithmetic.h"

using namespace takum::types;

int main() {
    takum64 base(1.0);
    
    auto& before = takum::internal::phi::phi_diag<64>();
    unsigned long start_calls = before.eval_calls;
    std::cout << "Before: eval_calls=" << start_calls << std::endl;
    
    for (int k = 1; k <= 3; ++k) {
        double scale = std::exp(k * 0.25);
        takum64 other(scale);
        std::cout << "k=" << k << ", scale=" << scale << ", other.ell=" << other.get_exact_ell() << std::endl;
        
        auto r = base + other;
        std::cout << "  result=" << r.to_double() << std::endl;
    }
    
    auto& after = takum::internal::phi::phi_diag<64>();
    std::cout << "After: eval_calls=" << after.eval_calls << std::endl;
    
    return 0;
}
