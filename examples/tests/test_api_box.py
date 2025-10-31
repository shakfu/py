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
    assert box is not None
    rect = box.get_patching_rect()
    assert rect is not None
    ext.log_info(f"rec: {rect}")


def test_named_box():
    box = get_named_box("myfloat")
    assert box is not None
    rect = box.get_patching_rect()
    assert rect is not None


def test_box_object_setvalueof():
    obj = get_object_in_named_box("myfloat")
    assert obj is not None
    obj.set_value(10.5)


def test_box_object_getvalueof():
    obj = get_object_in_named_box("myfloat")
    assert obj is not None
    value = obj.get_value()
    return value


def test_next_box():
    box = get_named_box("myfloat")
    assert box is not None
    box2 = box.get_nextobject()
    if box2:
        rect = box2.get_patching_rect()
        assert rect is not None
        return str(rect)


def test_prior_box():
    box = get_named_box("myfloat")
    assert box is not None
    box2 = box.get_prevobject()
    if box2:
        rect = box2.get_patching_rect()
        assert rect is not None
        return str(rect)


def test_box_properties():
    ext = api.PyExternal()
    box = ext.get_box()
    assert box is not None

    # Test various box properties
    classname = box.classname
    assert classname is not None

    rect = box.rect
    assert rect is not None

    # Test rect operations
    patching_rect = box.get_patching_rect()
    assert patching_rect is not None


def test_box_set_rect():
    ext = api.PyExternal()
    box = ext.get_box()
    original_rect = box.get_patching_rect()
    assert original_rect is not None

    # Set a new rect
    new_rect = [100, 100, 200, 150]
    box.set_patching_rect(new_rect)

    # Restore original
    box.set_patching_rect(original_rect)
