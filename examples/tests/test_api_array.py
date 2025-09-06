import api

mem = {}


def test_array_init():
    mem["a"] = api.Array("arr1")
    api.bang_success()


def test_array_bang():
    """Trigger output"""
    mem["a"].bang()


def test_array_int():
    """Convert an integer to an array"""
    mem["a"].int(10)


def test_array_float():
    """Convert a floating-point number to an array"""
    mem["a"].float(10.3)


def test_array_list():
    """Convert a list to an array"""
    mem["a"].list(10, 2, 3, 4, 1, 0)


def test_array_anything():
    """Convert a list to an array"""
    mem["a"].anything(10, 4, 2, 5, "sam", 0.5)


def test_array_append():
    """Append a value to the end of the current array"""
    mem["a"].append(102)


def test_array_array():
    """Make a copy of an array"""


def test_array_atoms():
    """Output the current array as a list"""
    mem["a"].atoms()


def test_array_clear():
    """Clear the current array"""
    mem["a"].clear()


def test_array_delete():
    """Delete an entry in the array"""
    mem["a"].delete(2)


def test_array_dictionary():
    """Wrap a dictionary in an array"""
    d = api.Dictionary(a=100)
    mem["a"].dictionary(d)


def test_array_get():
    """Get an array element"""
    mem["a"].get(0)


def test_array_insert():
    """Insert a value into the current array"""
    mem["a"].insert(0, "nice one")


def test_array_prepend():
    """Place a new entry at the start of the current array"""
    mem["a"].prepend(3)


def test_array_replace():
    """Replace a value in the current array"""
    mem["a"].replace(0, 1000)


def test_array_reserve():
    """Reserve memory for a provided number of entries (doesn't resize array)"""
    mem["a"].clear(20)


def test_array_shrink():
    """Reduce memory usage to the current array object length"""
    mem["a"].shrink()


def test_array_string():
    """Wrap a string in an array"""
    mem["a"].string("hello world")
