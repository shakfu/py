import api


def test_atom_init():
    ext = api.PyExternal()
    a1 = api.Atom([1, 2.5, b"hello", "world"])
    ext.out(a1.to_list())


def test_atom_float():
    a1 = api.Atom(1.2)
    return float(a1)


def test_atom_long():
    a1 = api.Atom(1021)
    return int(a1)


def test_atom_str():
    a1 = api.Atom("hello world!")
    return str(a1)


def test_atom_from_seq():
    # ext = api.PyExternal()
    a1 = api.Atom.from_seq([10, 20121.234, "world", b"hello"])
    # ext.out(a1.to_list())
    return a1.to_list()


def test_atom_from_str():
    # ext = api.PyExternal()
    a1 = api.Atom.from_str("foo bar 1 2 3.0")
    assert a1.size == 5
    # ext.out(a1.to_list())
    return a1.to_list()


def test_atom_to_str():
    parsestr = "foo bar 1 2 3.0"
    # ext = api.PyExternal()
    a1 = api.Atom.from_str(parsestr)
    assert a1.size == 5
    # api.post(a1.to_string())
    return a1.to_string()


def test_atom_setlong_array():
    # ext = api.PyExternal()
    a1 = api.Atom.from_str("foo bar 1 2 3.0")
    a1.setlong_array([10, 20, 30, 40, 50])
    # ext.out(a1.to_list())
    return a1.to_list()


def test_atom_getlong_array():
    # ext = api.PyExternal()
    a1 = api.Atom.from_str("foo bar 1 2 3.0")
    a1.setlong_array([10, 20, 30, 40, 50])
    return a1.getlong_array(5)


def test_atom_setfloat_array():
    # ext = api.PyExternal()
    a1 = api.Atom.from_str("foo bar 1 2 3.0")
    a1.setfloat_array([12.0, 24.0, 36.0, 48.0, 52.0])
    # ext.out(a1.to_list())
    return a1.to_list()


def test_atom_getfloat_array():
    # ext = api.PyExternal()
    a1 = api.Atom.from_str("foo bar 1 2 3.0")
    a1.setfloat_array([13.0, 25.0, 37.0, 49.0, 53.0])
    return a1.getfloat_array(5)


def test_atom_setdouble_array():
    # ext = api.PyExternal()
    a1 = api.Atom.from_str("foo bar 1 2 3.0")
    a1.setdouble_array([17.0, 27.0, 37.0, 47.0, 57.0])
    # ext.out(a1.to_list())
    return a1.to_list()


def test_atom_getdouble_array():
    ext = api.PyExternal()
    a1 = api.Atom.from_str("foo bar 1 2 3.0")
    a1.setdouble_array([11.0, 22.2, 34.4, 45.8, 59.8])
    return a1.getdouble_array(5)
