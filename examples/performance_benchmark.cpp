/**
 * @file performance_benchmark.cpp
 * @brief Performance comparison between takum and IEEE 754 arithmetic.
 *
 * This benchmark compares the performance characteristics of takum arithmetic
 * operations against standard IEEE 754 floating-point operations across
 * different scenarios and data sizes.
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <random>
#include <numeric>
#include <cmath>
#include "takum/types.h"
#include "takum/arithmetic.h"

using namespace takum::types;

class Timer {
private:
    std::chrono::high_resolution_clock::time_point start_time;
    
public:
    void start() {
        start_time = std::chrono::high_resolution_clock::now();
    }
    
    double elapsed_ms() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        return duration.count() / 1000.0;
    }
};

void benchmark_basic_operations() {
    std::cout << "\n=== Basic Arithmetic Operations Benchmark ===\n";
    
    const int num_ops = 1000000;
    std::mt19937 gen(42);  // Fixed seed for reproducibility
    std::uniform_real_distribution<double> dist(-1000.0, 1000.0);
    
    // Generate test data
    std::vector<double> a_data, b_data;
    a_data.reserve(num_ops);
    b_data.reserve(num_ops);
    
    for (int i = 0; i < num_ops; ++i) {
        a_data.push_back(dist(gen));
        b_data.push_back(dist(gen));
    }
    
    Timer timer;
    
    // Benchmark IEEE 754 double addition
    timer.start();
    double sum_double = 0.0;
    for (int i = 0; i < num_ops; ++i) {
        sum_double += a_data[i] + b_data[i];
    }
    double time_double_add = timer.elapsed_ms();
    
    // Benchmark takum32 addition
    timer.start();
    takum32 sum_takum32{0.0};
    for (int i = 0; i < num_ops; ++i) {
        takum32 a{a_data[i]};
        takum32 b{b_data[i]};
        sum_takum32 = sum_takum32 + (a + b);
    }
    double time_takum32_add = timer.elapsed_ms();
    
    // Benchmark IEEE 754 double multiplication  
    timer.start();
    double prod_double = 1.0;
    for (int i = 0; i < num_ops; ++i) {
        prod_double += a_data[i] * b_data[i];
    }
    double time_double_mul = timer.elapsed_ms();
    
    // Benchmark takum32 multiplication
    timer.start();
    takum32 prod_takum32{1.0};
    for (int i = 0; i < num_ops; ++i) {
        takum32 a{a_data[i]};
        takum32 b{b_data[i]};
        prod_takum32 = prod_takum32 + (a * b);
    }
    double time_takum32_mul = timer.elapsed_ms();
    
    std::cout << "Operations: " << num_ops << "\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\nAddition benchmarks:\n";
    std::cout << "Double:   " << time_double_add << " ms\n";
    std::cout << "Takum32:  " << time_takum32_add << " ms\n";
    std::cout << "Ratio:    " << time_takum32_add / time_double_add << "x\n";
    
    std::cout << "\nMultiplication benchmarks:\n";
    std::cout << "Double:   " << time_double_mul << " ms\n"; 
    std::cout << "Takum32:  " << time_takum32_mul << " ms\n";
    std::cout << "Ratio:    " << time_takum32_mul / time_double_mul << "x\n";
    
    // Prevent optimization
    volatile double result_d = sum_double + prod_double;
    volatile double result_t = sum_takum32.to_double() + prod_takum32.to_double();
    (void)result_d; (void)result_t;
}

void benchmark_vector_operations() {
    std::cout << "\n=== Vector Operations Benchmark ===\n";
    
    const int vector_size = 100000;
    std::mt19937 gen(42);
    std::uniform_real_distribution<double> dist(-10.0, 10.0);
    
    // Generate test vectors
    std::vector<double> vec_a_double, vec_b_double;
    std::vector<takum32> vec_a_takum, vec_b_takum;
    
    for (int i = 0; i < vector_size; ++i) {
        double a = dist(gen);
        double b = dist(gen);
        vec_a_double.push_back(a);
        vec_b_double.push_back(b);
        vec_a_takum.push_back(takum32{a});
        vec_b_takum.push_back(takum32{b});
    }
    
    Timer timer;
    
    // Dot product with doubles
    timer.start();
    double dot_double = 0.0;
    for (int i = 0; i < vector_size; ++i) {
        dot_double += vec_a_double[i] * vec_b_double[i];
    }
    double time_dot_double = timer.elapsed_ms();
    
    // Dot product with takum32
    timer.start();
    takum32 dot_takum{0.0};
    for (int i = 0; i < vector_size; ++i) {
        dot_takum = dot_takum + (vec_a_takum[i] * vec_b_takum[i]);
    }
    double time_dot_takum = timer.elapsed_ms();
    
    // Vector normalization with doubles
    timer.start();
    double norm_double = 0.0;
    for (int i = 0; i < vector_size; ++i) {
        norm_double += vec_a_double[i] * vec_a_double[i];
    }
    norm_double = std::sqrt(norm_double);
    std::vector<double> normalized_double;
    for (int i = 0; i < vector_size; ++i) {
        normalized_double.push_back(vec_a_double[i] / norm_double);
    }
    double time_norm_double = timer.elapsed_ms();
    
    // Vector normalization with takum32
    timer.start();
    takum32 norm_squared_takum{0.0};
    for (int i = 0; i < vector_size; ++i) {
        norm_squared_takum = norm_squared_takum + (vec_a_takum[i] * vec_a_takum[i]);
    }
    double norm_takum_d = std::sqrt(norm_squared_takum.to_double());
    takum32 norm_takum{norm_takum_d};
    std::vector<takum32> normalized_takum;
    for (int i = 0; i < vector_size; ++i) {
        normalized_takum.push_back(vec_a_takum[i] / norm_takum);
    }
    double time_norm_takum = timer.elapsed_ms();
    
    std::cout << "Vector size: " << vector_size << "\n";
    std::cout << std::fixed << std::setprecision(2);
    
    std::cout << "\nDot product:\n";
    std::cout << "Double:  " << time_dot_double << " ms (result: " << std::setprecision(6) << dot_double << ")\n";
    std::cout << "Takum32: " << std::setprecision(2) << time_dot_takum << " ms (result: " << std::setprecision(6) << dot_takum.to_double() << ")\n";
    std::cout << "Ratio:   " << std::setprecision(2) << time_dot_takum / time_dot_double << "x\n";
    
    std::cout << "\nVector normalization:\n";
    std::cout << "Double:  " << time_norm_double << " ms\n";
    std::cout << "Takum32: " << time_norm_takum << " ms\n";
    std::cout << "Ratio:   " << time_norm_takum / time_norm_double << "x\n";
    
    // Prevent optimization
    volatile double prevent_opt = dot_double + dot_takum.to_double() + normalized_double[0] + normalized_takum[0].to_double();
    (void)prevent_opt;
}

void benchmark_conversion_overhead() {
    std::cout << "\n=== Conversion Overhead Benchmark ===\n";
    
    const int num_conversions = 1000000;
    std::mt19937 gen(42);
    std::uniform_real_distribution<double> dist(-1e6, 1e6);
    
    std::vector<double> test_values;
    for (int i = 0; i < num_conversions; ++i) {
        test_values.push_back(dist(gen));
    }
    
    Timer timer;
    
    // Benchmark double to takum32 conversion
    timer.start();
    std::vector<takum32> converted_to_takum;
    converted_to_takum.reserve(num_conversions);
    for (double val : test_values) {
        converted_to_takum.emplace_back(val);
    }
    double time_to_takum = timer.elapsed_ms();
    
    // Benchmark takum32 to double conversion
    timer.start();
    std::vector<double> converted_to_double;
    converted_to_double.reserve(num_conversions);
    for (const auto& val : converted_to_takum) {
        converted_to_double.push_back(val.to_double());
    }
    double time_to_double = timer.elapsed_ms();
    
    // Benchmark round-trip conversion
    timer.start();
    double sum_roundtrip = 0.0;
    for (double val : test_values) {
        takum32 takum_val{val};
        sum_roundtrip += takum_val.to_double();
    }
    double time_roundtrip = timer.elapsed_ms();
    
    std::cout << "Conversions: " << num_conversions << "\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Double → Takum32: " << time_to_takum << " ms (" 
              << (time_to_takum * 1000 / num_conversions) << " μs per conversion)\n";
    std::cout << "Takum32 → Double: " << time_to_double << " ms (" 
              << (time_to_double * 1000 / num_conversions) << " μs per conversion)\n";
    std::cout << "Round-trip:       " << time_roundtrip << " ms (" 
              << (time_roundtrip * 1000 / num_conversions) << " μs per conversion)\n";
    
    // Prevent optimization
    volatile double prevent_opt = sum_roundtrip;
    (void)prevent_opt;
}

void benchmark_memory_usage() {
    std::cout << "\n=== Memory Usage Comparison ===\n";
    
    const int array_size = 1000000;
    
    // Create arrays of different types
    std::vector<float> float_array(array_size, 3.14f);
    std::vector<double> double_array(array_size, 3.14);
    std::vector<takum32> takum32_array(array_size, takum32{3.14});
    std::vector<takum64> takum64_array(array_size, takum64{3.14});
    
    std::cout << "Array size: " << array_size << " elements\n";
    std::cout << "Memory usage:\n";
    std::cout << "float:   " << (float_array.size() * sizeof(float)) / 1024 << " KB\n";
    std::cout << "double:  " << (double_array.size() * sizeof(double)) / 1024 << " KB\n";
    std::cout << "takum32: " << (takum32_array.size() * sizeof(takum32)) / 1024 << " KB\n";
    std::cout << "takum64: " << (takum64_array.size() * sizeof(takum64)) / 1024 << " KB\n";
    
    Timer timer;
    
    // Sequential access benchmark
    timer.start();
    double sum_double = 0.0;
    for (const auto& val : double_array) {
        sum_double += val;
    }
    double time_double_access = timer.elapsed_ms();
    
    timer.start();
    takum32 sum_takum32{0.0};
    for (const auto& val : takum32_array) {
        sum_takum32 = sum_takum32 + val;
    }
    double time_takum32_access = timer.elapsed_ms();
    
    std::cout << "\nSequential access performance:\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Double:  " << time_double_access << " ms\n";
    std::cout << "Takum32: " << time_takum32_access << " ms\n";
    std::cout << "Ratio:   " << time_takum32_access / time_double_access << "x\n";
    
    // Prevent optimization
    volatile double prevent_opt = sum_double + sum_takum32.to_double();
    (void)prevent_opt;
}

void benchmark_precision_vs_performance() {
    std::cout << "\n=== Precision vs Performance Trade-off ===\n";
    
    const int num_operations = 100000;
    std::mt19937 gen(42);
    std::uniform_real_distribution<double> dist(0.1, 10.0);
    
    std::vector<double> test_data;
    for (int i = 0; i < num_operations; ++i) {
        test_data.push_back(dist(gen));
    }
    
    Timer timer;
    
    // Iterative square root calculation (precision test)
    auto sqrt_iterations = [](double x, int iterations) -> double {
        double result = x / 2.0;
        for (int i = 0; i < iterations; ++i) {
            result = (result + x / result) / 2.0;
        }
        return result;
    };
    
    auto sqrt_iterations_takum = [](takum32 x, int iterations) -> takum32 {
        takum32 result = x / takum32{2.0};
        for (int i = 0; i < iterations; ++i) {
            result = (result + x / result) / takum32{2.0};
        }
        return result;
    };
    
    const int sqrt_iters = 10;
    
    // Double precision square root
    timer.start();
    std::vector<double> sqrt_results_double;
    for (double val : test_data) {
        sqrt_results_double.push_back(sqrt_iterations(val, sqrt_iters));
    }
    double time_sqrt_double = timer.elapsed_ms();
    
    // Takum32 square root
    timer.start();
    std::vector<takum32> sqrt_results_takum;
    for (double val : test_data) {
        sqrt_results_takum.push_back(sqrt_iterations_takum(takum32{val}, sqrt_iters));
    }
    double time_sqrt_takum = timer.elapsed_ms();
    
    // Calculate average precision error
    double avg_error_double = 0.0;
    double avg_error_takum = 0.0;
    
    for (size_t i = 0; i < test_data.size(); ++i) {
        double true_sqrt = std::sqrt(test_data[i]);
        avg_error_double += std::abs(sqrt_results_double[i] - true_sqrt);
        avg_error_takum += std::abs(sqrt_results_takum[i].to_double() - true_sqrt);
    }
    avg_error_double /= test_data.size();
    avg_error_takum /= test_data.size();
    
    std::cout << "Iterative square root (" << sqrt_iters << " iterations each):\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Double:  " << time_sqrt_double << " ms (avg error: " << std::scientific << avg_error_double << ")\n";
    std::cout << "Takum32: " << std::fixed << time_sqrt_takum << " ms (avg error: " << std::scientific << avg_error_takum << ")\n";
    std::cout << "Time ratio: " << std::fixed << std::setprecision(2) << time_sqrt_takum / time_sqrt_double << "x\n";
    std::cout << "Error ratio: " << avg_error_takum / avg_error_double << "x\n";
}

int main() {
    std::cout << "TakumCpp Performance Benchmark\n";
    std::cout << "==============================\n";
    std::cout << "Comparing takum arithmetic performance against IEEE 754\n";
    
    benchmark_basic_operations();
    benchmark_vector_operations();
    benchmark_conversion_overhead();
    benchmark_memory_usage();
    benchmark_precision_vs_performance();
    
    std::cout << "\n=== Summary ===\n";
    std::cout << "This benchmark provides insights into:\n";
    std::cout << "• Arithmetic operation performance\n";
    std::cout << "• Conversion overhead costs\n";
    std::cout << "• Memory usage characteristics\n";
    std::cout << "• Precision vs performance trade-offs\n";
    std::cout << "\nNote: Performance results depend on compiler optimizations,\n";
    std::cout << "hardware architecture, and implementation details.\n";
    
    return 0;
}