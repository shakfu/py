import codecaching

# Different expressions
print(codecaching.run_cached("x + y", {"x": 2, "y": 3}))   # compiles and runs
print(codecaching.run_cached("x + y", {"x": 2, "y": 3}))   # memoized result

print(codecaching.run_cached("x * y", {"x": 4, "y": 5}))   # compiles new expr
print(codecaching.run_cached("x * y", {"x": 4, "y": 5}))   # memoized result

# Works with functions too
def mul(a, b): return a * b
print(codecaching.run_cached("f(x, y)", {"x": 6, "y": 7, "f": mul}))
print(codecaching.run_cached("f(x, y)", {"x": 6, "y": 7, "f": mul}))  # memoized


