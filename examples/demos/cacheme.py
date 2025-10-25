# examples to be used by `cachefile`
# send a msg with `cachefile cacheme.py` to a `py` external instance
# to cache all the functions in this file

def square(x):
	return x*x

def process_signal(x):
    result = 0
    for i in range(10):
        result += x * i
    return result

def fib(n):
    if n <= 1:
        return n
    else:
        return fib(n-1) + fib(n-2)

def count_paths(m, n):
      if m == 1 or n == 1:
          return 1
      return count_paths(m-1, n) + count_paths(m, n-1)

def factorial(n):
    if n <= 1:
        return 1
    return n * factorial(n-1)

def ackermann(m, n):
    if m == 0:
        return n + 1
    elif n == 0:
        return ackermann(m - 1, 1)
    else:
        return ackermann(m - 1, ackermann(m, n - 1))
