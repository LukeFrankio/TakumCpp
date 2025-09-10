#include <iostream>
#include <iomanip>
#include "include/takum/types.h"
#include "include/takum/arithmetic.h"

using namespace takum::types;

int main() {
    takum128 a = takum128(3.0);
    takum128 b = takum128(2.0);
    
    std::cout << std::setprecision(17);
    std::cout << "a = " << a.to_double() << std::endl;
    std::cout << "b = " << b.to_double() << std::endl;
    std::cout << "(a + b) = " << (a + b).to_double() << std::endl;
    std::cout << "(a - b) = " << (a - b).to_double() << std::endl;
    
    return 0;
}
