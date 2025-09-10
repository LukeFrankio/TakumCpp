#include <iostream>
#include "include/takum/types.h"

using namespace takum::types;

int main() {
    takum32 a(2.0);
    takum32 b(-2.0);
    
    std::cout << "a(2.0) = " << a.to_double() << ", ell = " << a.get_exact_ell() << std::endl;
    std::cout << "b(-2.0) = " << b.to_double() << ", ell = " << b.get_exact_ell() << std::endl;
    
    // Test from_ell with different combinations
    auto test1 = takum32::from_ell(false, 1.38629);  // Should be +2.0
    auto test2 = takum32::from_ell(true, 1.38629);   // Should be -2.0
    auto test3 = takum32::from_ell(false, -1.38629); // ?
    auto test4 = takum32::from_ell(true, -1.38629);  // ?
    
    std::cout << "from_ell(false, 1.38629) = " << test1.to_double() << std::endl;
    std::cout << "from_ell(true, 1.38629) = " << test2.to_double() << std::endl;
    std::cout << "from_ell(false, -1.38629) = " << test3.to_double() << std::endl;
    std::cout << "from_ell(true, -1.38629) = " << test4.to_double() << std::endl;
    
    return 0;
}
