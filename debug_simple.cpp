#include <iostream>
#include "include/takum/types.h"

using namespace takum::types;

int main() {
    takum32 a(3.0);
    takum32 b(-2.0);
    
    // Force the double-based path by creating values with non-finite ell
    double da = a.to_double();
    double db = b.to_double();
    std::cout << "da = " << da << ", db = " << db << std::endl;
    
    takum32 simple_result(da + db);
    std::cout << "takum32(da + db) = " << simple_result.to_double() << std::endl;
    
    return 0;
}
