
# ============================================================================
# usage_example.py - How to use the compiled extension
# ============================================================================

"""
Example usage of the math_extension module with function caching.

To run this example:
1. Save the files: py_function_cache.h, math_extension.c, setup.py
2. Build the extension: python setup.py build_ext --inplace
3. Run this script: python usage_example.py
"""

import math_extension
import time

def main():
    print("=== Math Extension with Function Cache Demo ===\n")
    
    # 1. Basic function addition and calling
    print("1. Adding and calling basic functions:")
    
    # Add some mathematical functions
    math_extension.add_function("""
def quadratic(a, b, c, x):
    \"\"\"Evaluate quadratic equation ax^2 + bx + c at point x\"\"\"
    return a * x * x + b * x + c
    """, "quadratic.py")
    
    math_extension.add_function("""
def compound_interest(principal, rate, time, frequency):
    \"\"\"Calculate compound interest: A = P(1 + r/n)^(nt)\"\"\"
    return principal * ((1 + rate/frequency) ** (frequency * time))
    """, "finance.py")
    
    # Call the functions with different parameters
    result1 = math_extension.call_function('quadratic', 1, 2, 1, 3)
    print(f"quadratic(1, 2, 1, 3) = {result1}")
    
    result2 = math_extension.call_function('compound_interest', 1000, 0.05, 5, 12)
    print(f"compound_interest(1000, 0.05, 5, 12) = ${result2:.2f}")
    
    # 2. Using built-in functions that were pre-loaded
    print("\n2. Using pre-loaded functions:")
    
    print(f"add(10, 20) = {math_extension.call_function('add', 10, 20)}")
    print(f"multiply(7, 8) = {math_extension.call_function('multiply', 7, 8)}")
    print(f"power(2, 10) = {math_extension.call_function('power', 2, 10)}")
    # print(f"factorial(5) = {math_extension.call_function('factorial', 5)}")
    
    # 3. Advanced function with list processing
    print("\n3. Advanced data processing function:")
    
    math_extension.add_function("""
def analyze_data(numbers, operation):
    \"\"\"Analyze a list of numbers with specified operation\"\"\"
    if operation == 'mean':
        return sum(numbers) / len(numbers)
    elif operation == 'variance':
        mean = sum(numbers) / len(numbers)
        return sum((x - mean) ** 2 for x in numbers) / len(numbers)
    elif operation == 'std_dev':
        mean = sum(numbers) / len(numbers)
        variance = sum((x - mean) ** 2 for x in numbers) / len(numbers)
        return variance ** 0.5
    elif operation == 'range':
        return max(numbers) - min(numbers)
    else:
        return None
    """, "statistics.py")
    
    data = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
    print(f"Data: {data}")
    print(f"Mean: {math_extension.call_function('analyze_data', data, 'mean'):.2f}")
    print(f"Std Dev: {math_extension.call_function('analyze_data', data, 'std_dev'):.2f}")
    print(f"Range: {math_extension.call_function('analyze_data', data, 'range')}")
    
    # 4. Performance demonstration
    print("\n4. Performance benchmarking:")
    
    # Benchmark the quadratic function
    benchmark_result = math_extension.benchmark('quadratic', (1, 2, 1, 3), 10000)
    print(f"Quadratic function benchmark (10,000 calls):")
    print(f"  Total time: {benchmark_result['total_time']:.4f} seconds")
    print(f"  Average time: {benchmark_result['average_time']:.8f} seconds")
    print(f"  Calls per second: {benchmark_result['calls_per_second']:.0f}")
    
    # Compare with direct Python calculation
    print(f"\nComparing with direct Python calculation:")
    
    def python_quadratic(a, b, c, x):
        return a * x * x + b * x + c
    
    start_time = time.time()
    for _ in range(10000):
        python_quadratic(1, 2, 1, 3)
    python_time = time.time() - start_time
    
    print(f"  Python function: {python_time:.4f} seconds")
    print(f"  Cached function: {benchmark_result['total_time']:.4f} seconds")
    print(f"  Speedup: {python_time / benchmark_result['total_time']:.2f}x")
    
    # 5. Cache statistics and management
    print("\n5. Cache statistics:")
    
    stats = math_extension.get_stats()
    print(f"  Total cached functions: {stats['total_entries']}")
    print(f"  Cache hits: {stats['cache_hits']}")
    print(f"  Cache misses: {stats['cache_misses']}")
    print(f"  Hit ratio: {stats['hit_ratio']:.2%}")
    
    # 6. Function existence checking
    print("\n6. Function management:")
    
    print(f"Has 'quadratic' function: {math_extension.has_function('quadratic')}")
    print(f"Has 'nonexistent' function: {math_extension.has_function('nonexistent')}")
    
    # 7. Error handling demonstration
    print("\n7. Error handling:")
    
    try:
        # Try to call non-existent function
        math_extension.call_function('nonexistent', 1, 2, 3)
    except KeyError as e:
        print(f"Expected error for nonexistent function: {e}")
    
    try:
        # Try to add invalid function
        math_extension.add_function("invalid python code", "error.py")
    except SyntaxError as e:
        print(f"Expected error for invalid syntax: {e}")
    
    # 8. Complex mathematical functions
    print("\n8. Complex mathematical operations:")
    
    math_extension.add_function("""
def matrix_multiply_2x2(a, b):
    \"\"\"Multiply two 2x2 matrices represented as lists of lists\"\"\"
    result = [[0, 0], [0, 0]]
    for i in range(2):
        for j in range(2):
            for k in range(2):
                result[i][j] += a[i][k] * b[k][j]
    return result
    """, "matrix.py")
    
    matrix_a = [[1, 2], [3, 4]]
    matrix_b = [[5, 6], [7, 8]]
    result_matrix = math_extension.call_function('matrix_multiply_2x2', matrix_a, matrix_b)
    print(f"Matrix A: {matrix_a}")
    print(f"Matrix B: {matrix_b}")
    print(f"A × B: {result_matrix}")
    
    print("\n=== Demo Complete ===")
    
    # Final stats
    final_stats = math_extension.get_stats()
    print(f"\nFinal cache statistics:")
    print(f"  Functions cached: {final_stats['total_entries']}")
    print(f"  Total function calls: {final_stats['cache_hits']}")
    print(f"  Cache efficiency: {final_stats['hit_ratio']:.2%}")

if __name__ == "__main__":
    main()
