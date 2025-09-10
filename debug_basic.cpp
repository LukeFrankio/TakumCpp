#include <iostream>
#include "include/takum/types.h"
#include "include/takum/arithmetic.h"

using namespace takum::types;

int main() {
    takum128 a(3.0);
    takum128 b(2.0);
    
    std::cout << "a = " << a.to_double() << std::endl;
    std::cout << "b = " << b.to_double() << std::endl;
    std::cout << "a.get_exact_ell() = " << a.get_exact_ell() << std::endl;
    std::cout << "b.get_exact_ell() = " << b.get_exact_ell() << std::endl;
    
    auto sum = a + b;
    auto diff = a - b;
    
    std::cout << "a + b = " << sum.to_double() << std::endl;
    std::cout << "a - b = " << diff.to_double() << std::endl;
    
    return 0;
}
