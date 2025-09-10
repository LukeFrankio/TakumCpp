#include <iostream>
#include "include/takum/types.h"
#include "include/takum/arithmetic.h"

using namespace takum::types;

int main() {
    takum32 a(3.0);
    takum32 b(2.0);
    
    std::cout << "a = " << a.to_double() << std::endl;
    std::cout << "b = " << b.to_double() << std::endl;
    std::cout << "a.get_exact_ell() = " << a.get_exact_ell() << std::endl;
    std::cout << "b.get_exact_ell() = " << b.get_exact_ell() << std::endl;
    
    // Test manual negation
    long double eb = b.get_exact_ell();
    bool Sb = (eb < 0.0L);
    long double mb = fabsl(eb);
    std::cout << "eb = " << eb << ", Sb = " << Sb << ", mb = " << mb << std::endl;
    
    takum32 negb = takum32::from_ell(!Sb, (Sb ? mb : -mb));
    std::cout << "negb = " << negb.to_double() << std::endl;
    std::cout << "Expected: " << -b.to_double() << std::endl;
    
    auto sum = a + negb;
    std::cout << "a + negb = " << sum.to_double() << std::endl;
    
    return 0;
}
