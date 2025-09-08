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
    
    api.post("----- extended properties ------")

    api.post(f"ndim: {m.ndim}")
    api.post(f"ncols == width: {m.ncols} == {m.width}")
    api.post(f"nrows == height: {m.nrows} == {m.height}")
    
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
    m.exprfill("cell[0]==10")

def test_matrix_exprfill2():
    m = mem["m"]
    m.exprfill("cell[0]!=10")


def test_matrix_fill_plane():
    m = mem["m"]
    m.fill_plane(7, plane=1)


def test_matrix_set_cell():
    m = mem["m"]
    m.set_cell(positions=[2, 3], values=[50, 110])
    api.bang_success()


def test_matrix_get_cell():
    m = mem["m"]
    res = m.get_cell(1, 2)
    api.bang_success()
    return res
    

def test_matrix_set_cell1d():
    m = mem["m"]
    if m.type in ["char", "long"]:
        m.set_cell1d(0, values=[2, 3])
    else:
        m.set_cell1d(0, values=[2.5, 3.3])
    api.bang_success()


def test_matrix_set_cell2d():
    m = mem["m"]
    if m.type in ["char", "long"]:
        m.set_cell2d(0, 1, values=[2, 3])
    else:
        m.set_cell2d(0, 1, values=[2.5, 3.3])
    api.bang_success()


def test_matrix_set_cell3d():
    m = mem["m"]
    if m.type in ["char", "long"]:
        m.set_cell2d(0, 1, 2, values=[2, 3, 4])
    else:
        m.set_cell2d(0, 1, 2, values=[2.5, 3.3, 4.4])
    api.bang_success()


def test_matrix_memoryview():
    m = mem["m"]
    mv = m.as_memoryview()

    api.post(f"shape: {mv.shape}")
    api.post(f"strides: {mv.strides}")
    api.post(f"ndim: {mv.ndim}")
    api.post(f"itemsize: {mv.itemsize}")
    api.post(f"nbytes: {mv.nbytes}")
    api.post(f"size: {len(mv)}")
    api.post(f"format: {mv.format}")
    api.post(f"readonly: {mv.readonly}")

    if m.is_char_matrix():
        api.post(f"[1,2] = {mv[1,2]}")
        api.post(f"[2,1] = {mv[2,1]}")

    # api.post(str(dir(mv)))


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
    m.set_all(1, 2)
    api.bang_success()

def test_matrix_set_val():
    m = mem["m"]
    m.set_val(4)
    api.bang_success()


def test_matrix_get_data():
    m = mem["m"]
    return m.get_data()





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
