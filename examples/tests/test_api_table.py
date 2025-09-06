import api

mem = {}


def test_table_init():
    t = mem["t"] = api.Table("mytable")
    assert t.size == 128
    api.bang_success()


# api.print_peers()


def test_table_populate():
    t = mem["t"]
    t.populate([i * 2 for i in range(t.size)])
    api.bang_success()


def test_table_to_list():
    t = mem["t"]
    xs = t.to_list()
    assert len(xs) == t.size
    return xs
    api.bang_success()


def test_table_embed():
    t = mem["t"]
    t.embed(False)


def test_table_bang():
    t = mem["t"]
    t.bang()


def test_table_get_int():
    t = mem["t"]
    t.get_int(0)


def test_table_set_int():
    t = mem["t"]
    t.set_int(0, 10)


def test_table_cancel():
    t = mem["t"]
    t.cancel()


def test_table_clear():
    t = mem["t"]
    t.clear()


def test_table_const():
    t = mem["t"]
    t.const(2)


def test_table_dump():
    t = mem["t"]
    t.dump()


def test_table_fquantile():
    t = mem["t"]
    t.fquantile(0.75)


def test_table_goto():
    t = mem["t"]
    t.call("goto", 100)


def test_table_in1():
    t = mem["t"]
    t.call("in1", 200)


def test_table_inv():
    t = mem["t"]
    t.call("inv", 100)


def test_table_length():
    t = mem["t"]
    t.call("length")


def test_table_load():
    t = mem["t"]
    t.call("load")


def test_table_max():
    t = mem["t"]
    t.max()


def test_table_min():
    t = mem["t"]
    t.min()


def test_table_next():
    t = mem["t"]
    t.next()


def test_table_normal():
    t = mem["t"]
    t.normal()


def test_table_open():
    t = mem["t"]
    t.open()


def test_table_prev():
    t = mem["t"]
    t.prev()


def test_table_quantile():
    t = mem["t"]
    t.quantile(25)


def test_table_read():
    t = mem["t"]
    t.call("read", "mytable.txt")


def test_table_refer():
    t = mem["t"]
    t.call("refer", "ref")


def test_table_send():
    t = mem["t"]
    t.call("send", "bob", 1)


def test_table_set():
    t = mem["t"]
    t.call("set", 0, list(range(128)))


# def test_table_set():
#     t = mem['t']
#     args = [0] + list(range(128))
#     t.call("set", args)


def test_table_sum():
    t = mem["t"]
    t.sum()


def test_table_wclose():
    t = mem["t"]
    t.wclose()


def test_table_write():
    t = mem["t"]
    t.write("/tmp/mytable")
