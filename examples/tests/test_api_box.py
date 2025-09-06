import api


def get_named_box(name):
    ext = api.PyExternal()
    p = ext.get_patcher()
    return p.get_named_box(name)


def get_object_in_named_box(name):
    ext = api.PyExternal()
    p = ext.get_patcher()
    return p.get_object_in_named_box(name)


# ----------------------------------------------------------------------------


def test_pyexternal_box():
    ext = api.PyExternal()
    box = ext.get_box()
    ext.log_info(f"rec: {box.get_patching_rect()}")


def test_named_box():
    box = get_named_box("myfloat")
    ext.log_info(f"rec: {box.get_patching_rect()}")


def test_box_object_setvalueof():
    obj = get_object_in_named_box("myfloat")
    obj.set_value(10.5)


def test_box_object_getvalueof():
    obj = get_object_in_named_box("myfloat")
    return obj.get_value()


def test_next_box():
    box = get_named_box("myfloat")
    box2 = box.get_nextobject()
    return str(box2.get_patching_rect())


def test_prior_box():
    box = get_named_box("myfloat")
    box2 = box.get_prevobject()
    return str(box2.get_patching_rect())
