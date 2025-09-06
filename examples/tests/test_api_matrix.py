import os

import api


mem = {}


# first init per the matrix type (in the respective tab)


def test_matrix_char_init():
    mem["m"] = api.Matrix("m_char")
    api.bang_success()


def test_matrix_long_init():
    mem["m"] = api.Matrix("m_long")
    api.bang_success()


def test_matrix_float_init():
    mem["m"] = api.Matrix("m_float")
    api.bang_success()


def test_matrix_double_init():
    mem["m"] = api.Matrix("m_double")
    api.bang_success()


# then run tests as usual (this eliminates type specific tests)


def test_matrix_info():
    m = mem["m"]
    api.post(f"size: {m.size}")
    api.post(f"type: {m.type}")
    api.post(f"flags: {m.flags}")
    api.post(f"dimcount: {m.dimcount}")
    api.post(f"dim: {m.dim}")
    api.post(f"dimstride: {m.dimstride}")
    api.post(f"planecount: {m.planecount}")
    # custom properties
    api.post(f"itemsize: {m.itemsize}")
    api.post(f"plane_len: {m.plane_len}")
    api.post(f"matrix_len: {m.matrix_len}")


def test_matrix_bang():
    m = mem["m"]
    m.bang()
    api.bang_success()


def test_matrix_set_int():
    m = mem["m"]
    m.set_int(3)
    api.bang_success()


def test_matrix_set_float():
    m = mem["m"]
    m.set_float(3.5, 4.2)
    api.bang_success()


def test_matrix_export_image():
    m = mem["m"]
    img = "/tmp/ok"
    m.export_image("/tmp/ok")  # defaults to png
    assert os.exists("/tmp/ok.png"), "could not find exported image"
    api.bang_success()


def test_matrix_export_movie():
    m = mem["m"]


def test_matrix_exprfill():
    m = mem["m"]
    m.exprfill("pow(jit.noise(), 4)")


def test_matrix_fill_plane():
    m = mem["m"]
    m.fill_plane(7, plane=1)


def test_matrix_get_cell():
    m = mem["m"]
    m.get_cell(0, 0)
    api.bang_success()


def test_matrix_import_movie():
    m = mem["m"]


def test_matrix_add_gl_texture():
    m = mem["m"]


def test_matrix_op():
    m = mem["m"]
    m.op("+", 2)
    api.bang_success()


def test_matrix_read():
    m = mem["m"]


def test_matrix_set_all():
    m = mem["m"]
    m.set_all(5)
    api.bang_success()


def test_matrix_set_val():
    m = mem["m"]
    m.set_val(4)
    api.bang_success()


def test_matrix_get_data():
    m = mem["m"]
    return m.get_data()


def test_matrix_set_cell2d():
    m = mem["m"]
    if m.type in ["char", "long"]:
        m.set_cell2d(8, x=3, y=2, plane=0)
        m.set_cell2d(9, x=3, y=2, plane=1)
    else:
        m.set_cell2d(2.5, x=3, y=2, plane=0)
        m.set_cell2d(3.5, x=2, y=3, plane=1)
    api.bang_success()


def test_matrix_set_cell2d():
    m = mem["m"]
    m.set_cell2d(5, x=3, y=2, plane=0)
    api.bang_success()


# def test_matrix_set_char_data():
#     m = mem["m"]
#     length = m.matrix_len
#     m.set_char_data(list(range(length)))

# def test_matrix_set_char_data():
#     m = mem["m"]
#     length = m.matrix_len
#     m.set_char_data(list(range(length)))


def test_matrix_set_data():
    m = mem["m"]
    length = m.matrix_len
    if m.type in ["char", "long"]:
        m.set_data(list(range(length)))
    elif m.type in ["float32", "float64"]:
        m.set_data(list(float(i) for i in range(length)))
    api.bang_success()


def test_matrix_get_data():
    m = mem["m"]
    return m.get_data()


def test_matrix_fill():
    m = mem["m"]
    a = api.Atom.from_seq([5] * m.plane_len)
    m.fill(a, plane=0)
    api.bang_success()

def test_matrix_fill_all():
    m = mem["m"]
    m.myfill2d()    
    # a = api.Atom.from_seq([5] * m.matrix_len)
    # m.fill_all(a)
    api.bang_success()


def test_matrix_clear():
    m = mem["m"]
    m.clear()
