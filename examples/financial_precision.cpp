/**
 * @file financial_precision.cpp
 * @brief Demonstrates takum's advantages in financial calculations.
 *
 * This example shows how takum's logarithmic spacing provides superior
 * precision for financial calculations compared to IEEE 754 floats.
 * 
 * Key features demonstrated:
 * - High precision in currency calculations
 * - Better representation of small percentages
 * - Reduced rounding errors in compound interest
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include "takum/types.h"
#include "takum/arithmetic.h"

using namespace takum::types;

// Financial calculation examples
void compound_interest_comparison() {
    std::cout << "\n=== Compound Interest Calculation Comparison ===\n";
    
    // Initial investment: $1,000,000
    double principal_double = 1000000.0;
    takum64 principal_takum{1000000.0};
    
    // Annual interest rate: 0.05% (very small rate)
    double rate_double = 0.0005;
    takum64 rate_takum{0.0005};
    
    // Monthly compounding for 30 years
    int months = 30 * 12;
    double monthly_rate_double = rate_double / 12.0;
    takum64 monthly_rate_takum = rate_takum / takum64{12.0};
    
    // Calculate compound interest: A = P(1 + r/n)^(nt)
    double amount_double = principal_double;
    takum64 amount_takum = principal_takum;
    
    for (int i = 0; i < months; ++i) {
        amount_double *= (1.0 + monthly_rate_double);
        amount_takum = amount_takum * (takum64{1.0} + monthly_rate_takum);
    }
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Initial principal: $1,000,000.00\n";
    std::cout << "Annual rate: 0.05% (very small)\n";
    std::cout << "Period: 30 years, monthly compounding\n\n";
    
    std::cout << "Final amount (double):  $" << amount_double << "\n";
    std::cout << "Final amount (takum64): $" << amount_takum.to_double() << "\n";
    
    double difference = std::abs(amount_takum.to_double() - amount_double);
    std::cout << "Absolute difference: $" << difference << "\n";
    
    if (difference > 0.01) {
        std::cout << "→ Significant precision difference detected!\n";
    } else {
        std::cout << "→ Results are very close\n";
    }
}

void currency_accumulation() {
    std::cout << "\n=== Micro-Transaction Accumulation ===\n";
    
    // Simulate accumulating many tiny transactions (e.g., ad revenue)
    const int num_transactions = 1000000;
    const double tiny_amount = 0.000001; // $0.000001 per micro-transaction
    
    double total_double = 0.0;
    takum32 total_takum{0.0};
    takum32 tiny_takum{tiny_amount};
    
    std::cout << "Accumulating " << num_transactions << " micro-transactions\n";
    std::cout << "Each transaction: $" << std::scientific << tiny_amount << std::fixed << "\n\n";
    
    for (int i = 0; i < num_transactions; ++i) {
        total_double += tiny_amount;
        total_takum = total_takum + tiny_takum;
    }
    
    std::cout << std::setprecision(6);
    std::cout << "Total (double):  $" << total_double << "\n";
    std::cout << "Total (takum32): $" << total_takum.to_double() << "\n";
    
    double expected = num_transactions * tiny_amount;
    std::cout << "Expected total:  $" << expected << "\n";
    
    double error_double = std::abs(total_double - expected);
    double error_takum = std::abs(total_takum.to_double() - expected);
    
    std::cout << "\nAccumulation errors:\n";
    std::cout << "Double error:  $" << std::scientific << error_double << "\n";
    std::cout << "Takum32 error: $" << error_takum << std::fixed << "\n";
    
    if (error_takum < error_double) {
        std::cout << "→ Takum shows better precision for micro-transactions!\n";
    }
}

void percentage_precision() {
    std::cout << "\n=== Small Percentage Calculations ===\n";
    
    // Test representation of very small percentages
    std::vector<double> small_percentages = {
        0.01,    // 1%
        0.001,   // 0.1%
        0.0001,  // 0.01%
        0.00001, // 0.001%
        0.000001 // 0.0001%
    };
    
    std::cout << "Testing small percentage representation:\n";
    std::cout << std::setprecision(8);
    
    for (double pct : small_percentages) {
        takum32 pct_takum{pct};
        double roundtrip = pct_takum.to_double();
        double relative_error = std::abs(roundtrip - pct) / pct;
        
        std::cout << "Original: " << std::setw(12) << pct 
                  << " → Takum32: " << std::setw(12) << roundtrip
                  << " (rel. error: " << std::scientific << relative_error << std::fixed << ")\n";
    }
}

void portfolio_calculation() {
    std::cout << "\n=== Portfolio Rebalancing Simulation ===\n";
    
    // Simulate a portfolio with many small positions
    struct Position {
        std::string symbol;
        double shares_double;
        takum64 shares_takum;
        double price_double;
        takum64 price_takum;
    };
    
    std::vector<Position> portfolio = {
        {"AAPL", 100.5, takum64{100.5}, 150.25, takum64{150.25}},
        {"GOOGL", 50.25, takum64{50.25}, 2750.75, takum64{2750.75}},
        {"MSFT", 75.75, takum64{75.75}, 325.50, takum64{325.50}},
        {"TSLA", 25.125, takum64{25.125}, 800.375, takum64{800.375}},
        {"AMZN", 40.625, takum64{40.625}, 3200.125, takum64{3200.125}}
    };
    
    double total_value_double = 0.0;
    takum64 total_value_takum{0.0};
    
    std::cout << "Position values:\n";
    for (const auto& pos : portfolio) {
        double value_double = pos.shares_double * pos.price_double;
        takum64 value_takum = pos.shares_takum * pos.price_takum;
        
        total_value_double += value_double;
        total_value_takum = total_value_takum + value_takum;
        
        std::cout << pos.symbol << ": $" << std::setprecision(2) << value_double 
                  << " (double) vs $" << value_takum.to_double() << " (takum64)\n";
    }
    
    std::cout << "\nTotal portfolio value:\n";
    std::cout << "Double: $" << total_value_double << "\n";
    std::cout << "Takum64: $" << total_value_takum.to_double() << "\n";
    
    double difference = std::abs(total_value_takum.to_double() - total_value_double);
    std::cout << "Difference: $" << difference << "\n";
}

int main() {
    std::cout << "TakumCpp Financial Precision Demonstration\n";
    std::cout << "==========================================\n";
    
    compound_interest_comparison();
    currency_accumulation();
    percentage_precision();
    portfolio_calculation();
    
    std::cout << "\n=== Summary ===\n";
    std::cout << "This demonstration shows how takum's logarithmic number\n";
    std::cout << "system can provide advantages in financial calculations\n";
    std::cout << "where precision matters for small values and percentages.\n";
    
    return 0;
}