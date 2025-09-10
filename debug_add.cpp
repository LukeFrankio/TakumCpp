#include <iostream>
#include "include/takum/types.h"
#include "include/takum/arithmetic.h"

using namespace takum::types;

int main() {
    takum32 a(3.0);
    takum32 neg_b(-2.0);
    
    std::cout << "a = " << a.to_double() << ", ell = " << a.get_exact_ell() << std::endl;
    std::cout << "neg_b = " << neg_b.to_double() << ", ell = " << neg_b.get_exact_ell() << std::endl;
    
    auto sum = a + neg_b;
    std::cout << "a + neg_b = " << sum.to_double() << std::endl;
    
    // Expected: 3 + (-2) = 1
    std::cout << "Expected: " << 1.0 << std::endl;
    
    return 0;
}
