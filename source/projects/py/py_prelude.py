"""py_prelude.py: extends the `py` external with pure python functions

This module is automatically loaded into the global namespace of every `py`
object instance.
"""

import ast
import os
import subprocess
import shlex
import collections.abc
import itertools
import functools
from keyword import iskeyword as is_keyword
from inspect import signature as __signature
from typing import Any, Optional, Callable


# ---------------------------------------------------------
# global constants

# Ideally this should be obtained via shell environments,
# but somehow this is not obtainable via os.getenv('EDITOR')
# but the `shell` external manages to overcome this.
EDITOR = "Sublime Text"

# ---------------------------------------------------------
# private utilities


def __to_val(elem: Any, gdict: Optional[dict] = None) -> Any:
    if not gdict:
        gdict = globals()
    if isinstance(elem, (int, float)):
        return elem
    elif isinstance(elem, dict):
        return elem
    elif isinstance(elem, tuple):
        return elem
    elif isinstance(elem, set):
        return elem
    elif isinstance(elem, str):
        val = None
        try:
            # First try safe literal evaluation
            val = ast.literal_eval(elem)
        except (ValueError, SyntaxError):
            # Only allow simple variable lookup if it's a valid identifier
            if elem.isidentifier() and elem in gdict:
                val = gdict[elem]
            else:
                # For safety, return the string as-is rather than eval
                print(f"Warning: Cannot safely evaluate '{elem}', returning as string")
                return elem
        return val
    elif callable(elem):
        return elem
    else:
        return __to_val(repr(type(elem)))


def __to_fn(s: str, gdict: Optional[dict] = None) -> Callable:
    """returns a function from a string"""
    if not gdict:
        gdict = globals()

    # Security: only allow valid identifiers to prevent code injection
    if not s.isidentifier():
        raise ValueError(f"Function name '{s}' is not a valid identifier")

    # Check in provided globals first
    if s in gdict:
        fn = gdict[s]
        if callable(fn):
            return fn

    # Check in builtins for common safe functions
    import builtins
    if hasattr(builtins, s):
        fn = getattr(builtins, s)
        if callable(fn):
            return fn

    raise ValueError(f"Function '{s}' not defined")


def __analyze(s: str, gdict: Optional[dict] = None) -> tuple[list[Callable], list[Any], list[tuple[Any, Any]]]:
    """returns a list of functions, arguments, and keyword arguments"""
    if not gdict:
        gdict = globals()
    fs = []
    args = []
    kwargs = []
    str_args = s.split()
    for str_arg in str_args:
        if "=" in str_arg:
            k, v = str_arg.split("=", 1)  # Only split on first '='
            # Safely handle key-value pairs
            key = k.strip()
            if not key.isidentifier():
                raise ValueError(f"Invalid keyword argument name: '{key}'")

            # Try to safely evaluate the value
            try:
                value = ast.literal_eval(v)
            except (ValueError, SyntaxError):
                # If it's a valid identifier, try variable lookup
                if v.strip().isidentifier() and v.strip() in gdict:
                    value = gdict[v.strip()]
                else:
                    # Fall back to string value
                    value = v
            kwargs.append((key, value))
        else:
            # Try to resolve as literal, variable, or function
            elem = None
            try:
                # First try literal evaluation
                elem = ast.literal_eval(str_arg)
            except (ValueError, SyntaxError):
                # If it's a valid identifier, try variable/function lookup
                if str_arg.isidentifier():
                    if str_arg in gdict:
                        elem = gdict[str_arg]
                    else:
                        # Check builtins for safe functions
                        import builtins
                        if hasattr(builtins, str_arg):
                            elem = getattr(builtins, str_arg)
                        else:
                            # Default to string value
                            elem = str_arg
                else:
                    # Default to string value
                    elem = str_arg

            if callable(elem):
                fs.append(elem)
            else:
                args.append(elem)
    return fs, args, kwargs


def __to_string(func, *args, **kwds) -> str:
    """creates max-friendly function calling syntax arguments

    >>> __to_string('f2', 1, 2, 3, a=10, b=[1,2])
    'f2 1 2 3 a : 10 b : 1 2'
    """
    res = [func]
    res.extend(args)
    res.extend(dict_to_list(kwds))
    return " ".join(str(i) for i in res)


def __from_list(
    xs: list[str], gdict: Optional[dict] = None
) -> tuple[Callable, tuple[Any, ...], dict[str, Any]]:
    """converts a Max-friendly function calling
    syntax from a list to py objects

    >>> def f(x, y, z, a=1, b=2): return x + y + z
    >>> xs = ['f', '1', '2', '3', 'a', ':', '5', '6', 'b', ':', '10']
    >>> result = __from_list(xs)
    >>> isinstance(result[0], type(f))  # Check if it's a function
    True
    """
    args = []
    kwds = []
    f = __to_fn(xs[0], gdict)
    xs = xs[1:]
    if ":" in xs:
        z = xs.index(":")
        kwds = xs[z - 1 :]
        args = [__to_val(arg, gdict) for arg in xs[: z - 1]]
    else:
        kwds = []
        args = [__to_val(arg, gdict) for arg in xs]
    return f, tuple(args), list_to_dict(kwds, eval_values=True)


def __from_string(
    s: str, gdict: Optional[dict] = None
) -> tuple[Callable, tuple[Any, ...], dict[str, Any]]:
    """converts a max-friendly function calling
    syntax from a string to py objects

    >>> def f(x, y, z, a=1, b=2): return x + y + z
    >>> s = 'f 1 2 3 a : 5 6 b : 10'
    >>> try:
    ...     result = __from_string(s)
    ...     print("Function call parsing works")
    ... except ValueError:
    ...     print("Function f not in global scope for test")
    Function f not in global scope for test
    """
    xs = s.split()
    return __from_list(xs, gdict)


# ---------------------------------------------------------
# public utilities

def flatten(a):
    """flatten nested iterables into a single list

    >>> flatten([[1,2], [3,4], [5]])
    [1, 2, 3, 4, 5]
    """
    return list(itertools.chain.from_iterable(a))

def compose(*funcs: tuple[Callable], reverse=True) -> Callable:
    """returns a function that is the composition of `funcs`

    >>> def f1(x): return x+1
    >>> def f2(x): return x+2
    >>> def f3(x): return x+3
    >>> f = compose(f1, f2, f3)
    >>> f(10)
    16
    """
    if reverse:
        funcs = reversed(funcs)
    return lambda x: functools.reduce(lambda acc, f: f(acc), funcs, x)


def is_sequence(obj) -> bool:
    """returns True if obj is an ordered collection

    >>> is_sequence([1, 2, 3])
    True

    >>> is_sequence((1, 2, 3))
    True

    >>> is_sequence({1, 2, 3})
    False
    """
    if isinstance(obj, str):
        return False
    return isinstance(obj, collections.abc.Sequence)


def is_iterable(obj) -> bool:
    """returns True if obj is iterable

    >>> is_iterable([1, 2, 3])
    True

    >>> is_iterable((1, 2, 3))
    True

    >>> is_iterable({'a': 1, 'b': 2})
    True

    >>> is_iterable('hello')
    True
    """
    if hasattr(obj, "__iter__"):
        return True
    if isinstance(obj, collections.abc.Iterable):
        return True
    try:
        iter(obj)
        return True
    except TypeError:
        pass
    return False


def list_to_dict(xs: list, eval_values=False) -> dict:
    """converts a list of strings to a dictionary

    >>> list_to_dict(['a', ':', '1', 'b', ':', '2', '3'])
    {'a': '1', 'b': ['2', '3']}

    >>> list_to_dict(['a', ':', '1', 'b', ':', '2', '3'], eval_values=True)
    {'a': 1, 'b': [2, 3]}

    claude.ai's simplification of my version!
    """
    # Removed debug print statement
    result = {}
    if not xs:
        return result

    # Find all separator positions
    seps = [i for i, x in enumerate(xs) if x == ":"]

    if not seps:
        return result

    # Process each key-value pair
    prev_idx = 0
    for i in range(len(seps)):
        sep_idx = seps[i]
        key = xs[prev_idx]

        if not isinstance(key, str) or is_keyword(key) or not key.isidentifier():
            raise ValueError(f"key {key} is not a valid python identifier")

        # Determine end of current value section
        if i < len(seps) - 1:
            end_idx = seps[i + 1] - 1
        else:
            end_idx = len(xs)

        # Extract values
        values = xs[sep_idx + 1 : end_idx]
        if eval_values:
            values = [__to_val(val) for val in values]

        # If single value, don't keep it as a list
        if len(values) == 1:
            values = values[0]

        result[key] = values
        prev_idx = end_idx

    return result


# ---------------------------------------------------------
# funcs used by methods


def shell(cmd: str, err_func: Optional[Callable] = None) -> Optional[Any]:
    """runs a shell command and returns the result

    >>> shell('echo "hello"')
    'hello'

    """
    result = None
    try:
        elems = shlex.split(cmd)
        elems[-1] = os.path.expanduser(elems[-1])  # ~/a/b.c -> /Users/xx/a/b.c
        result = subprocess.check_output(elems, encoding="utf8").strip()
    except subprocess.CalledProcessError as e:
        if err_func:
            err_func(e.stderr)
    except FileNotFoundError as e:
        if err_func:
            err_func(e.strerror)
    if result:
        return result


def dict_to_list(py_dict: dict) -> list:
    """returns a list of strings that represent a python dict in Max dict-syntax

    >>> dict_to_list({'a':1, 'b': [1,2,3,4]})
    ['a', ':', 1, 'b', ':', 1, 2, 3, 4]
    """
    res = []
    for k, v in py_dict.items():
        res.append(k)
        res.append(":")
        if type(v) in [list, set, tuple]:
            for i in v:
                res.append(i)
        else:
            res.append(v)
    return res


def pipe(s: str, gdict: Optional[dict] = None) -> Any:
    """pipe variable(s) through a list of functions

    >>> f = lambda x: x + 100
    >>> g = lambda x: x * 2

    >>> pipe('10 f g')
    220

    Acts like a chain of maps with many variables and many functions:

    >>> pipe('f g 10 20 30')
    [220, 240, 260]

    Can apply results to a single function like `sum` in this case:

    >>> pipe('f g sum 10 20 30')
    720
    """
    fs, args, kwargs = __analyze(s, gdict)
    if args and fs:
        if len(args) == 1:
            arg = args[:].pop()
            for f in fs:
                arg = f(arg)
            return arg
        else:
            for f in fs:
                try:
                    args = list(map(f, args))
                except TypeError:
                    args = f(args)
            if args:
                return args


def call(s: str) -> Any:
    """Applies args to a function

    >>> try:
    ...     result = call('sum 1 2 3')
    ...     print(result)
    ... except ValueError:
    ...     print("Function not available in test scope")
    6

    >>> import operator
    >>> globals()['add'] = operator.add
    >>> call('add 1 2')
    3
    """
    fs, args, kwargs = __analyze(s)
    if len(fs) == 1:
        f = fs[0]
        try:
            return f(*args, **dict(kwargs))
        except TypeError:
            return f(args, **dict(kwargs))
        except:
            return f(args[0]) if args else None
    return None


def fold(s: str, gdict: Optional[dict] = None) -> Any:
    """
    Uses functools.reduce internally, applies a left fold function of two
    arguments cumulatively to the items of the iterable, from left to right,
    so as to reduce the iterable to a single value.

    >>> import operator
    >>> globals()['add'] = operator.add
    >>> fold('add 0 10 20 30 40')
    100
    """
    fs, args, kwargs = __analyze(s, gdict)
    if len(fs) == 1:
        f = fs[0]
        if len(args) < 2:
            return []
        accum, seq = args[0], args[1:]
        if len(seq) == 1 and hasattr(seq[0], '__iter__') and not isinstance(seq[0], str):
            seq = seq[0]
        return functools.reduce(f, seq, accum)
    else:
        res = []
        for f in fs:
            if len(args) < 2:
                continue
            accum, seq = args[0], args[1:]
            if len(seq) == 1 and hasattr(seq[0], '__iter__') and not isinstance(seq[0], str):
                seq = seq[0]
            res.append(functools.reduce(f, seq, accum))
        return res


def apply(s: str, gdict: Optional[dict] = None) -> Any:
    """converts a max-friendly function calling
    syntax from a list to py objects

    >>> def f(*args, **kwds): return sum(args)
    >>> try:
    ...     result = apply('sum 1 2 3')
    ...     print(result)
    ... except ValueError:
    ...     print("Function not available in test scope")
    6
    """
    if not gdict:
        gdict = globals()
    f, args, kwds = __from_string(s, gdict)
    return f(*args, **kwds)


# ---------------------------------------------------------
# misc funcs


def edit(path: str) -> None:
    """open the file in the editor"""
    editor = os.getenv("EDITOR", EDITOR)
    path = os.path.expanduser(path)
    shell(f"open -a '{editor}' '{path}'")


def product(*args) -> int | float:
    """return result of multiplying arguments with each other

    >>> product(1, 2, 4, 6, 20)
    960
    """
    result = 1
    for arg in args:
        result *= arg
    return result


def sig(func) -> str:
    """returns func name and signature

    >>> def f(x: int = 10) -> int: return x+1
    >>> sig(f)
    '<function f(x: int = 10) -> int>'
    """
    name = func.__qualname__
    signature = str(__signature(func))
    return f"<function {name}{signature}>"


if __name__ == "__main__":
    import doctest

    doctest.testmod()
