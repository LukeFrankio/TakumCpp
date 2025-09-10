#include <iostream>
#include <cmath>
#include "include/takum/types.h"
#include "include/takum/arithmetic.h"

using namespace takum::types;

int main() {
    takum64 a(2.0);  // Non-zero ell
    takum64 b(3.0);  // Non-zero ell  
    
    std::cout << "a(2.0) = " << a.to_double() << ", ell = " << a.get_exact_ell() << std::endl;
    std::cout << "b(3.0) = " << b.to_double() << ", ell = " << b.get_exact_ell() << std::endl;
    
    long double ell_a = a.get_exact_ell();
    long double ell_b = b.get_exact_ell();
    long double mag_a = fabsl(ell_a);
    long double mag_b = fabsl(ell_b);
    if (mag_b > mag_a) { std::swap(mag_a, mag_b); }
    long double ratio = mag_b / mag_a;
    std::cout << "mag_a=" << mag_a << ", mag_b=" << mag_b << ", ratio=" << ratio << std::endl;
    
    auto& before = takum::internal::phi::phi_diag<64>();
    unsigned long start_calls = before.eval_calls;
    std::cout << "Before: eval_calls=" << start_calls << std::endl;
    
    auto r = a + b;
    std::cout << "result=" << r.to_double() << std::endl;
    
    auto& after = takum::internal::phi::phi_diag<64>();
    std::cout << "After: eval_calls=" << after.eval_calls << std::endl;
    
    return 0;
}
