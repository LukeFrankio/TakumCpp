/**
 * @file scientific_computing.cpp
 * @brief Demonstrates takum's advantages in scientific computing scenarios.
 *
 * This example showcases how takum's unique properties benefit scientific
 * computations, particularly those involving:
 * - Wide dynamic ranges
 * - Iterative algorithms
 * - Precision-critical calculations
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <chrono>
#include "takum/types.h"
#include "takum/arithmetic.h"

using namespace takum::types;

void wide_dynamic_range_demo() {
    std::cout << "\n=== Wide Dynamic Range Demonstration ===\n";
    
    // Test representation of values across many orders of magnitude
    std::vector<double> test_values = {
        1e-20, 1e-15, 1e-10, 1e-5, 1e-3, 1e-1,
        1.0, 1e1, 1e3, 1e5, 1e10, 1e15, 1e20
    };
    
    std::cout << "Testing representation across dynamic range:\n";
    std::cout << std::setprecision(6) << std::scientific;
    
    for (double val : test_values) {
        takum64 val_takum{val};
        double roundtrip = val_takum.to_double();
        double relative_error = std::abs(roundtrip - val) / val;
        
        std::cout << "Original: " << std::setw(12) << val 
                  << " → Takum64: " << std::setw(12) << roundtrip
                  << " (rel. error: " << relative_error << ")\n";
    }
}

void newton_raphson_sqrt() {
    std::cout << "\n=== Newton-Raphson Square Root Comparison ===\n";
    
    const double target = 2.0;
    const int max_iterations = 20;
    
    // Newton-Raphson: x_{n+1} = (x_n + a/x_n) / 2
    
    // Using double precision
    double x_double = 1.0;  // Initial guess
    std::cout << "Newton-Raphson for sqrt(2) using double:\n";
    for (int i = 0; i < max_iterations; ++i) {
        x_double = (x_double + target / x_double) / 2.0;
        if (i < 10 || i % 5 == 0) {
            std::cout << "Iteration " << std::setw(2) << i+1 << ": " 
                      << std::setprecision(15) << std::fixed << x_double << "\n";
        }
    }
    
    // Using takum64
    takum64 x_takum{1.0};  // Initial guess
    takum64 target_takum{target};
    takum64 two_takum{2.0};
    
    std::cout << "\nNewton-Raphson for sqrt(2) using takum64:\n";
    for (int i = 0; i < max_iterations; ++i) {
        x_takum = (x_takum + target_takum / x_takum) / two_takum;
        if (i < 10 || i % 5 == 0) {
            std::cout << "Iteration " << std::setw(2) << i+1 << ": " 
                      << std::setprecision(15) << std::fixed << x_takum.to_double() << "\n";
        }
    }
    
    double true_sqrt2 = std::sqrt(2.0);
    double error_double = std::abs(x_double - true_sqrt2);
    double error_takum = std::abs(x_takum.to_double() - true_sqrt2);
    
    std::cout << "\nFinal comparison:\n";
    std::cout << "True sqrt(2):    " << true_sqrt2 << "\n";
    std::cout << "Double result:   " << x_double << " (error: " << std::scientific << error_double << ")\n";
    std::cout << "Takum64 result:  " << std::fixed << x_takum.to_double() << " (error: " << std::scientific << error_takum << ")\n";
}

void monte_carlo_pi() {
    std::cout << "\n=== Monte Carlo Pi Estimation ===\n";
    
    const int num_samples = 1000000;
    std::cout << "Estimating π using " << num_samples << " random samples\n";
    
    // Simple Monte Carlo: count points inside unit circle
    int inside_circle_double = 0;
    int inside_circle_takum = 0;
    
    // Using a simple LCG for reproducible results
    uint32_t seed = 12345;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_samples; ++i) {
        // Simple LCG random number generation
        seed = seed * 1664525 + 1013904223;
        double x_d = (seed / double(UINT32_MAX)) * 2.0 - 1.0;
        
        seed = seed * 1664525 + 1013904223;
        double y_d = (seed / double(UINT32_MAX)) * 2.0 - 1.0;
        
        // Check if inside unit circle using double
        if (x_d * x_d + y_d * y_d <= 1.0) {
            inside_circle_double++;
        }
        
        // Same calculation using takum32
        takum32 x_t{x_d};
        takum32 y_t{y_d};
        takum32 one_t{1.0};
        
        if ((x_t * x_t + y_t * y_t).to_double() <= 1.0) {
            inside_circle_takum++;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    double pi_estimate_double = 4.0 * inside_circle_double / num_samples;
    double pi_estimate_takum = 4.0 * inside_circle_takum / num_samples;
    
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "π estimate (double): " << pi_estimate_double << "\n";
    std::cout << "π estimate (takum32): " << pi_estimate_takum << "\n";
    std::cout << "True π:              " << M_PI << "\n";
    
    double error_double = std::abs(pi_estimate_double - M_PI);
    double error_takum = std::abs(pi_estimate_takum - M_PI);
    
    std::cout << "Error (double):  " << std::scientific << error_double << "\n";
    std::cout << "Error (takum32): " << error_takum << "\n";
    std::cout << "Computation time: " << duration.count() << " ms\n";
}

void exponential_decay_simulation() {
    std::cout << "\n=== Exponential Decay Simulation ===\n";
    
    // Simulate radioactive decay: N(t) = N₀ * e^(-λt)
    // Using small time steps to test numerical stability
    
    const double initial_amount = 1000.0;
    const double decay_constant = 0.001;  // Small decay constant
    const double time_step = 0.1;
    const int num_steps = 1000;
    
    double amount_double = initial_amount;
    takum64 amount_takum{initial_amount};
    takum64 decay_takum{decay_constant};
    takum64 dt_takum{time_step};
    
    std::cout << "Simulating exponential decay over " << num_steps * time_step << " time units\n";
    std::cout << "Initial amount: " << initial_amount << "\n";
    std::cout << "Decay constant: " << decay_constant << "\n";
    std::cout << "Time step: " << time_step << "\n\n";
    
    // Numerical integration using Euler's method: dN/dt = -λN
    std::cout << "Time\tDouble\t\tTakum64\t\tAnalytical\n";
    std::cout << "----\t------\t\t-------\t\t----------\n";
    
    for (int i = 0; i <= num_steps; i += 100) {
        double t = i * time_step;
        double analytical = initial_amount * std::exp(-decay_constant * t);
        
        if (i % 100 == 0) {
            std::cout << std::fixed << std::setprecision(1) << t << "\t"
                      << std::setprecision(3) << amount_double << "\t\t"
                      << amount_takum.to_double() << "\t\t"
                      << analytical << "\n";
        }
        
        if (i < num_steps) {
            // Euler step: N(t+dt) = N(t) - λ*N(t)*dt
            amount_double -= decay_constant * amount_double * time_step;
            takum64 decay_term = decay_takum * amount_takum * dt_takum;
            amount_takum = amount_takum - decay_term;
        }
    }
    
    double final_time = num_steps * time_step;
    double analytical_final = initial_amount * std::exp(-decay_constant * final_time);
    
    double error_double = std::abs(amount_double - analytical_final);
    double error_takum = std::abs(amount_takum.to_double() - analytical_final);
    
    std::cout << "\nFinal numerical errors:\n";
    std::cout << "Double error:  " << std::scientific << error_double << "\n";
    std::cout << "Takum64 error: " << error_takum << "\n";
    
    if (error_takum < error_double) {
        std::cout << "→ Takum64 shows better numerical stability!\n";
    }
}

void harmonic_series_convergence() {
    std::cout << "\n=== Harmonic Series Convergence Test ===\n";
    
    // Test precision in summing the harmonic series: 1 + 1/2 + 1/3 + 1/4 + ...
    // This is a challenging test for floating-point precision
    
    const int max_terms = 100000;
    
    double sum_double = 0.0;
    takum64 sum_takum{0.0};
    
    std::cout << "Computing partial sums of harmonic series up to " << max_terms << " terms\n\n";
    std::cout << "Terms\t\tDouble\t\t\tTakum64\n";
    std::cout << "-----\t\t------\t\t\t-------\n";
    
    for (int n = 1; n <= max_terms; ++n) {
        double term = 1.0 / n;
        sum_double += term;
        
        takum64 n_takum{static_cast<double>(n)};
        takum64 term_takum = takum64{1.0} / n_takum;
        sum_takum = sum_takum + term_takum;
        
        if (n == 10 || n == 100 || n == 1000 || n == 10000 || n == max_terms) {
            std::cout << std::setw(5) << n << "\t\t"
                      << std::setprecision(8) << std::fixed << sum_double << "\t\t"
                      << sum_takum.to_double() << "\n";
        }
    }
    
    // The harmonic series diverges, but we can compare intermediate precision
    double difference = std::abs(sum_takum.to_double() - sum_double);
    std::cout << "\nFinal difference between double and takum64: " 
              << std::scientific << difference << "\n";
}

int main() {
    std::cout << "TakumCpp Scientific Computing Demonstration\n";
    std::cout << "===========================================\n";
    
    wide_dynamic_range_demo();
    newton_raphson_sqrt();
    monte_carlo_pi();
    exponential_decay_simulation();
    harmonic_series_convergence();
    
    std::cout << "\n=== Summary ===\n";
    std::cout << "This demonstration shows how takum numbers can provide\n";
    std::cout << "advantages in scientific computing scenarios requiring:\n";
    std::cout << "• Wide dynamic range representation\n";
    std::cout << "• Numerical stability in iterative algorithms\n";
    std::cout << "• Precision in accumulative calculations\n";
    
    return 0;
}