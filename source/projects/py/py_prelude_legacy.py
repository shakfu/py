"""py_prelude.py: extends the `py` external with pure python functions

This module is automatically loaded into the global namespace of every `py`
object instance.
"""
import os
import subprocess
import shlex
import collections.abc
import functools
import itertools
from keyword import iskeyword as is_keyword
from inspect import signature as __signature
from typing import Any, Callable


# ---------------------------------------------------------
# global constants

# Ideally this should be obtained via shell environments,
# but somehow this is not obtainable via os.getenv('EDITOR')
# but the `shell` external manages to overcome this.
EDITOR = "Sublime Text"

# ---------------------------------------------------------
# private utilities

def is_sequence(obj):
    if isinstance(obj, str):
        return False
    return isinstance(obj, collections.abc.Sequence)


def __compose(f, g):
    """f . g"""
    return lambda x: f(g(x))


def __analyze(s: str):
    fs = []
    args = []
    kwargs = []
    str_args = s.split()
    for str_arg in str_args:
        if '=' in str_arg:
            k, v = str_arg.split('=')
            kwargs.append((
                eval(repr(k), locals(), globals()),
                eval(v, locals(), globals())
            ))
        else:
            try:
                elem = eval(str_arg, locals(), globals())
            except SyntaxError:
                elem = eval(repr(str_arg), locals(), globals())
            if callable(elem):
                fs.append(elem)
            else:
                args.append(elem)
    return fs, args, kwargs



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
            values = [__eval_arg(val) for val in values]

        # If single value, don't keep it as a list
        if len(values) == 1:
            values = values[0]

        result[key] = values
        prev_idx = end_idx

    return result


# ---------------------------------------------------------
# funcs are used by methods



def shell(cmd: str, err_func=None):
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


def dict_to_list(py_dict: dict):
    """Output python dict in Max dict-syntax
    
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


def pipe(s: str):
    """pipe variable(s) through a list of functions

    With a single variable a list of function:

    >>> pipe('10 math.sin math.cos')
    0.8556343548213665

    Acts like a chain of maps With many variables
    and many functions:

    >>> pipe('math.sin math.cos 10 20 30')
    [0.8556343548213665, 0.6114178044194122, 0.5503344099628433]

    Consistent with the pipe function can apply several
    variables to a single function like `sum` in this case:

    >>> pipe('math.sin math.cos sum 10 20 30')
    2.017386569203622

    :param      s:    code string
    :type       s:    str
    """
    fs, args, kwargs = __analyze(s)
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



def call(s: str):
    """
    Applies args to a function

    >>> call('sum 1 2 3')
    6

    >>> call("add 'abc' 'def'")
    abcdef
    
    >>> f = lambda *args, **kwargs: print(args, kwargs)
    >>> call('f 10 20 a=1')
    (10, 20) {'a': 1}
    
    :param      s:    code string
    :type       s:    str
    """
    fs, args, kwargs = __analyze(s)
    if len(fs) == 1:
        f = fs[0]
        try:
            return f(*args, **dict(kwargs))
        except TypeError:
            return f(args, **dict(kwargs))
        # except:
        return f(args[0])


def fold(s: str):
    """
    Uses functools.reduce internally, applies a left fold function of two 
    arguments cumulatively to the items of the iterable, from left to right, 
    so as to reduce the iterable to a single value.

    >>> fold('add 0 10 20 30 40')
    100

    >>> fold('product add 5 10 20 30 40')
    [1200000, 105]

    >>> txts = ['abc', 'def']
    >>> fold('add "" txts')
    abcdef
    
    :param      s:    code string
    :type       s:    str
    """
    fs, args, kwargs = __analyze(s)
    if len(fs) == 1:
        f = fs[0]
        accum, seq = args[0], args[1:]
        if len(seq) == 1:
            seq = seq[0]
        return functools.reduce(f, seq, accum)
    else:
        res = []
        for f in fs:
            accum, seq = args[0], args[1:]
            if len(seq) == 1:
                seq = seq[0]
            res.append(functools.reduce(f, seq, accum))
        return res


def to_string(func, *args, **kwds):
    """creates max-friendly function calling syntax arguments
    
    >>> to_string('f2', 1, 2, 3, a=10, b=[1,2])
    'f2 1 2 3 a : 10 b : 1 2'
    """
    res = [func]
    res.extend(args)
    res.extend(dict_to_list(kwds))
    return " ".join(str(i) for i in res)


def from_string(s: str):
    """converts a max-friendly function calling 
    syntax from a string to py objects

    >>> s = 'f 1 2 3 a : 5 6 b : 10'
    >>> from_string(s)
    f(1, 2, 3, a=[5, 6], b=10)
    """
    args = []
    kwds = []
    xs = s.split()
    f = xs[0]
    xs = xs[1:]
    if ':' in xs:
        z = xs.index(':')
        kwds = xs[z-1:]
        args = xs[:z-1]
    else:
        kwds = []
        args = xs
    return f, tuple(args), list_to_dict(kwds)


def __eval_arg(s: str):
    """evaluates an argument, keeping it as a string if it does not parse"""
    try:
        return eval(s, globals())
    except (SyntaxError, NameError):
        return s


def apply(s: str) -> Any:
    """calls a function given in max-friendly syntax

    >>> f = lambda *args, **kwds: (args, kwds)
    >>> apply('f 1 2 3 a : 5 6 b : 10')
    ((1, 2, 3), {'a': [5, 6], 'b': 10})
    """
    f, args, kwds = from_string(s)
    f = eval(f, globals())
    args = [__eval_arg(arg) for arg in args]
    kwds = {
        k: [__eval_arg(i) for i in v] if isinstance(v, list) else __eval_arg(v)
        for k, v in kwds.items()
    }
    return f(*args, **kwds)


# ---------------------------------------------------------
# misc funcs


def edit(path: str):
    """open the file in the editor"""
    editor = os.getenv("EDITOR", EDITOR)
    path = os.path.expanduser(path)
    shell(f"open -a '{editor}' '{path}'")


def product(*args):
    """return result of multiplying arguments with each other
    
    >>> product(1, 2, 4, 6, 20)
    960
    """
    result = 1
    for arg in args:
        result *= arg
    return result


def sig(func):
    """returns func name and signature

    >>> def f(x: int = 10) -> int: return x+1
    >>> sig(f)
    '<function f(x: int = 10) -> int>'
    """
    name = func.__qualname__
    signature = str(__signature(func))
    return f"<function {name}{signature}>"


if __name__ == '__main__':
    if 1:
        xs = ['a', ':', 5, 'a', 'b', ':', 10, 'abv', 'c', ':', 23]
        print(list_to_dict(xs))
    else:
        # SHOULD FAIL
        xs = [1, ':', 5, 'a', 'b', ':', 10, 'abv', 'c', ':', 23]
        print(list_to_dict(xs))

