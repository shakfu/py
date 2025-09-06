import api


def test_dict_init():
    d = api.Dictionary(a=1, b=2)
    assert d.getentrycount() == 2
    return d.to_atoms()


def test_dict_contains():
    d = api.Dictionary(c=1, f=2)
    assert "c" in d
    return "c" in d


def test_dict_from_name():
    d = api.Dictionary("xdict")  # xdict is name of an exiting dict object
    return d.to_atoms()


def test_dict_registered():
    d = api.Dictionary("mydict")  # mydict is registered name of new dict
    d["myfloat"] = 10.1
    d["myint"] = 3
    d["hello"] = "world"
    assert d.getentrycount() == 3
    return d.to_atoms()


def test_dict():
    d = api.Dictionary()
    d["myfloat"] = 10.1
    d["myint"] = 3
    d["mylist"] = ["a", "b", "c"]
    assert d.getentrycount() == 3
    return d.to_atoms()


def test_dict_symbol():
    d = api.Dictionary()
    key, value = "hello", "world"
    d[key] = value
    assert d.getentrycount() == 1
    assert d[key] == value, f"{d[key]} != {value}"


def test_dict_float():
    d = api.Dictionary()
    key, value = "myfloat", 10.1
    d[key] = value
    assert d.getentrycount() == 1
    assert round(d[key], 2) == round(value, 2), f"{d[key]} != {value}"


def test_dict_int():
    d = api.Dictionary()
    key, value = "myint", 3
    d[key] = value
    assert d.getentrycount() == 1
    assert d[key] == value, f"{d[key]} != {value}"


def test_dict_atoms():
    d = api.Dictionary()
    key, value = "mylist", [1, "asym", 12.1]
    d[key] = value
    assert d.getentrycount() == 1
    _value = d[key]
    assert len(_value) == len(value)
    api.post(f"key '{key}' returned {_value}")


def test_dict_bytes():
    d = api.Dictionary()
    key, value = "mybytes", b"some-binary-value-here"
    d[key] = value
    assert d.getentrycount() == 1
    assert d[key] == value, f"{d[key]} != {value}"


def test_dict_entrycount():
    pydict = {"sam": 100, "abc": 121.10, "def": [1, b"pod", "sa", 4.1]}
    d = api.Dictionary()
    for key, value in pydict.items():
        d[key] = value
    return d.getentrycount()  # should be 3


def test_dict_getkeys():
    pydict = {"sam": 100, "abc": 121.10, "def": [1, b"pod", "sa", 4.1]}
    d = api.Dictionary()
    for key, value in pydict.items():
        d[key] = value
    return d.getkeys()


def test_dict_getstring():
    d = api.Dictionary()
    d["abc"] = "def"
    return d.get_string("abc")


def test_dict_delete_entry():
    d = api.Dictionary()
    d["abc"] = "def"
    d.delete_entry("abc")
    return d.getentrycount()


def test_dict_chuck_entry():
    d = api.Dictionary()
    d["abc"] = "def"
    d.chuck_entry("abc")
    return d.getentrycount()


def test_dict_clear():
    d = api.Dictionary()
    d["abc"] = "def"
    d.clear()
    return d.getentrycount()


def test_dict_clone():
    d = api.Dictionary()
    d["abc"] = "def"
    d2 = d.clone()
    return d2.get_string("abc")


def test_dict_clone_to_existing():
    d = api.Dictionary()
    d["abc"] = "def"
    d2 = api.Dictionary()
    d.clone_to_existing(d2)
    return d2.get_string("abc")


def test_dict_clone_to_self():
    d = api.Dictionary()
    d["abc"] = "def"
    d2 = api.Dictionary()
    d2.clone_to_self(d)
    return d2.get_string("abc")


def test_dict_merge_to_existing():
    d = api.Dictionary()
    d["abc"] = "def"
    d2 = api.Dictionary()
    d.merge_to_existing(d2)
    return d2.get_string("abc")


def test_dict_merge_to_self():
    d = api.Dictionary()
    d["abc"] = "def"
    d2 = api.Dictionary()
    d2.merge_to_self(d)
    return d2.get_string("abc")


def test_dict_copy_entries():
    d = api.Dictionary()
    d["abc"] = "def"
    d2 = api.Dictionary()
    d.copy_entries(d2, ["abc"])
    return d2.get_string("abc")


def test_dict_copy_unique():
    d = api.Dictionary()
    d["abc"] = "def"
    d2 = api.Dictionary()
    d2.copy_unique(d)
    return d2.get_string("abc")


def test_get_default_long():
    d = api.Dictionary()
    d["abc"] = 200
    return d.get_default_long("abc", 100)


def test_get_default_float():
    d = api.Dictionary()
    d["abc"] = 20.53
    return d.get_default_float("abc", 100.0)


def test_get_default_sym():
    d = api.Dictionary()
    d["abc"] = "def"
    return d.get_default_sym("abc", "boo")


def test_get_default_atom():
    d = api.Dictionary()
    d["abc"] = "def"
    return d.get_default_atom("abc", api.Atom(100))


def test_get_default_string():
    d = api.Dictionary()
    d["abc"] = "def"
    return d.get_default_string("abc", "def")


# def test_get_default_atom():
#     d = api.Dictionary()
#     d['abc'] = api.Atom("def")
#     boo = api.Atom("boo")
#     return d.get_default_atom('abc', boo)


def test_dict_dump():
    d = api.Dictionary()
    d["abc"] = "def"
    d.dump()


def test_dict_transaction_lock_unlock():
    d = api.Dictionary()
    d.transaction_lock()
    d["abc"] = "def"
    d.transaction_unlock()
    return d.get_string("abc")


def test_dict_findregistered_clone():
    d = api.Dictionary()
    d2 = d.findregistered_clone("xdict")
    return d2.get_long("x")


def test_dict_from_dict():
    d = api.Dictionary.from_dict(dict(a="abc", z=[1, 2, 3]))
    return d.get_list("z")


def test_dict_from_kwargs():
    d = api.Dictionary.from_kwargs(a="abc", z=[1, 2, 3])
    return d.get_string("a")


def test_dict_from_string():
    d = api.Dictionary.from_string("a : b c : 1 2 3 4")
    return d.get_string("a")


def test_dict_from_atoms():
    atoms = api.Atom(["abc", ":", "hello", "def", ":", 10.1])
    d = api.Dictionary.from_atoms(atoms)
    return d.get_string("abc")


def test_dict_from_atoms_extended():
    atoms = api.Atom(["abc", ":", "hello", "def", ":", 10.1])
    d = api.Dictionary.from_atoms_extended(atoms)
    return d.get_string("abc")


def test_dict_to_atoms():
    d = api.Dictionary()
    d["abc"] = "def"
    atoms = d.to_atoms()
    assert isinstance(atoms, api.Atom)
    return atoms.to_list()
