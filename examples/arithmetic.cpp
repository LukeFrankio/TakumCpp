#include <iostream>
#include "takum/types.h"
#include "takum/arithmetic.h"

int main() {
    using namespace takum::types;
    takum32 x = takum32(1.5f);
    takum32 y = takum32(2.25f);
    takum32 z = x * y + takum32(0.5);
    std::cout << "x=" << x.to_double() << " y=" << y.to_double() << " z=" << z.to_double() << std::endl;
    return 0;
}
