#include <iostream>
#include "include/takum/types.h"
#include "include/takum/arithmetic.h"

using namespace takum::types;

int main() {
    takum32 a(3.0);
    takum32 b(2.0);
    
    std::cout << "a = " << a.to_double() << std::endl;
    std::cout << "b = " << b.to_double() << std::endl;
    
    long double eb = b.get_exact_ell();
    bool Sb = (eb < 0.0L);
    long double mb = fabsl(eb);
    std::cout << "eb = " << eb << ", Sb = " << Sb << ", mb = " << mb << std::endl;
    
    takum32 negb = takum32::from_ell(!Sb, mb);
    std::cout << "negb = from_ell(" << (!Sb) << ", " << mb << ") = " << negb.to_double() << std::endl;
    
    auto diff = a - b;
    std::cout << "a - b = " << diff.to_double() << std::endl;
    
    return 0;
}
