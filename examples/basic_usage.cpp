#include <iostream>
#include <iomanip>
#include "takum/types.h"
#include "takum/core.h"

int main() {
    using namespace takum::types;

    // Create takum instances from doubles
    takum32 a{3.14};
    takum32 b{-2.71};

    // Basic output via to_double
    std::cout << "a (from 3.14): " << std::fixed << std::setprecision(10) << a.to_double() << std::endl;
    std::cout << "b (from -2.71): " << std::fixed << std::setprecision(10) << b.to_double() << std::endl;

    // Negation
    takum32 neg_a = -a;
    std::cout << "-a: " << neg_a.to_double() << std::endl;

    // Comparison
    std::cout << "a < b: " << (a < b ? "true" : "false") << std::endl;
    std::cout << "a == a: " << (a == a ? "true" : "false") << std::endl;

    // NaR handling
    takum32 nar = takum32::nar();
    std::cout << "Is NaR: " << (nar.is_nar() ? "true" : "false") << std::endl;
    std::cout << "nar < a: " << (nar < a ? "true" : "false") << std::endl;

    // Bitwise inversion (for demonstration)
    takum32 inv_a = ~a;
    std::cout << "~a to_double: " << inv_a.to_double() << std::endl;

    // Debug view
    std::cout << "a debug bits: " << a.debug_view().to_string() << std::endl;

    return 0;
}