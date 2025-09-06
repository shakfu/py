from collections.abc import Iterable


# some helpers


def flatten(items):
    """Yield items from any nested iterable; see Reference.

    see: https://stackoverflow.com/questions/952914/how-do-i-make-a-flat-list-out-of-a-list-of-lists
    """
    for x in items:
        if isinstance(x, Iterable) and not isinstance(x, (str, bytes)):
            for sub_x in flatten(x):
                yield sub_x
        else:
            yield x


# some test functions

identity = lambda x: x

add = lambda x, y: x + y

product = lambda x, y: x * y

add100 = lambda x: x + 100

sub20 = lambda x: x - 20

div2 = lambda x: x / 2

mul2 = lambda x: x * 2

mul10 = lambda x: x * 10

mul5 = lambda x: x * 5

mul6 = lambda x: x * 7

sumargs = lambda *args, **kwargs: sum(args)

sumvals = lambda *args, **kwargs: sum(v for (k, v) in kwargs.items())


def func1(*args, **kwds):
    return "{}({} {})".format(func1.__name__, str(args), str(kwds))


def func2(*args, **kwds):
    return sum(args) + len(kwds.items())
