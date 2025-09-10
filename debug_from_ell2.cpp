#include <iostream>
#include <cmath>
#include "include/takum/types.h"

using namespace takum::types;

int main() {
    auto result = takum32::from_ell(false, 1.6094);
    std::cout << "from_ell(false, 1.6094) = " << result.to_double() << std::endl;
    
    // What should ell=1.6094 represent?
    // ell = 2*ln(|x|), so |x| = exp(ell/2) = exp(1.6094/2) = exp(0.8047)
    double expected = std::exp(1.6094 / 2.0);
    std::cout << "Expected magnitude: exp(1.6094/2) = " << expected << std::endl;
    
    // Check: what's ell for x=1.0?
    takum32 one(1.0);
    std::cout << "takum32(1.0) has ell = " << one.get_exact_ell() << std::endl;
    
    return 0;
}
