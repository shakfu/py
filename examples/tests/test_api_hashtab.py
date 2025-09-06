import api


mem = {}


def test_hashtab_init():
    mem["d"] = api.Hashtab()
    return mem["d"]


def test_hashtab_size():
    d = mem["d"]
    return len(d)


def test_hashtab_setget_long():
    d = mem["d"]
    d["a1"] = 100
    return d["a1"]


def test_hashtab_setget_sym():
    d = mem["d"]
    d["a2"] = "hello word"
    return d["a2"]


def test_hashtab_setget_object():
    d = mem["d"]
    d["a3"] = api.MaxObject("buffer~", "buf", "jongly.aif")
    return d["a3"]


def test_hashtab_get_keys():
    d = mem["d"]
    return d.get_keys()


def test_hashtab_delete_long():
    d = mem["d"]
    del d["a1"]


def test_hashtab_delete_sym():
    d = mem["d"]
    del d["a2"]


def test_hashtab_delete_object():
    d = mem["d"]
    del d["a3"]


def test_hashtab_delete_all():
    d = mem["d"]
    for key in d:
        del d[key]


def test_hashtab_contains():
    d = mem["d"]
    return "a1" in d


def test_hashtab_clear():
    d = mem["d"]
    d.clear()
