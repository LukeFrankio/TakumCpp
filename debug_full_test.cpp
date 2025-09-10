#include <iostream>
#include <cmath>
#include "include/takum/types.h"
#include "include/takum/arithmetic.h"

using namespace takum::types;

static void exercise_add_range() {
  takum64 base(1.0); // ell ~ 0
  for (int k = -12; k <= 12; ++k) {
    double scale = std::exp(k * 0.25); // moderate dynamic range
    takum64 other(scale);
    auto r = base + other;
    (void)r;
  }
}

int main() {
    auto& before = takum::internal::phi::phi_diag<64>();
    unsigned long start_calls = before.eval_calls;
    std::cout << "Before: eval_calls=" << start_calls << std::endl;
    
    exercise_add_range();
    
    auto& after = takum::internal::phi::phi_diag<64>();
    std::cout << "After: eval_calls=" << after.eval_calls << std::endl;
    std::cout << "Delta: " << (after.eval_calls - start_calls) << std::endl;
    
    return 0;
}
