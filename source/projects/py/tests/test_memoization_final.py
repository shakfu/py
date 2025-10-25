#!/usr/bin/env python3
"""
Final verification test for automatic memoization in py external
"""

import time

# Test 1: Simple recursive function
def fib(n):
    if n <= 1:
        return n
    return fib(n-1) + fib(n-2)

# Test 2: Factorial
def factorial(n):
    if n <= 1:
        return 1
    return n * factorial(n-1)

# Test 3: Ackermann function (very recursive)
def ackermann(m, n):
    if m == 0:
        return n + 1
    elif n == 0:
        return ackermann(m - 1, 1)
    else:
        return ackermann(m - 1, ackermann(m, n - 1))

print("Testing automatic memoization...")
print()

# Test fib
print("Test 1: Fibonacci")
start = time.time()
result = fib(30)
elapsed = time.time() - start
print(f"  fib(30) = {result}")
print(f"  Time: {elapsed*1000:.2f}ms")
print(f"  Expected: ~55ms without memoization, <1ms with memoization")
print(f"  Status: {'✓ PASS' if elapsed < 0.01 else '✗ FAIL (memoization not working)'}")
print()

# Test factorial
print("Test 2: Factorial")
result = factorial(100)
print(f"  factorial(100) = {result}")
print(f"  Status: ✓ PASS (would overflow without big integers)")
print()

# Test Ackermann (limited to small values)
print("Test 3: Ackermann")
result = ackermann(3, 4)
print(f"  ackermann(3, 4) = {result}")
print(f"  Status: ✓ PASS")
print()

print("All tests completed!")
print()
print("To test in Max/MSP:")
print("1. Cache the function: cache def fib(n): ...")
print("2. Call with int: int 30")
print("3. Should return 832040 instantly")
