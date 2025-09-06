import api


def test_patcher_init():
    p = api.get_patcher()
    assert p.is_patcher(), "object is not a patcher!"
    api.bang_success()


def test_patcher_registered_names():
    p = api.get_patcher()
    return p.registered_names()


def test_patcher_attrs():
    p = api.get_patcher()
    # dump patch attributes
    # symbol attrs
    api.post(f"patcher.name: {p.name}")
    api.post(f"patcher.filepath: {p.filepath}")
    # int attrs
    api.post(f"patcher.fileversion: {p.fileversion}")
    api.post(f"patcher.count: {p.count}")
    api.post(f"patcher.fgcount: {p.fgcount}")
    api.post(f"patcher.bgcount: {p.bgcount}")
    api.post(f"patcher.numviews: {p.numviews}")
    api.post(f"patcher.numwindowviews: {p.numwindowviews}")
    # chars/bool attrs
    api.post(f"patcher.locked: {p.locked}")
    api.post(f"patcher.bglocked: {p.bglocked}")
    api.post(f"patcher.presentation: {p.presentation}")
    api.post(f"patcher.openinpresentation: {p.openinpresentation}")
    api.post(f"patcher.cansave: {p.cansave}")
    api.post(f"patcher.dirty: {p.dirty}")
    api.post(f"patcher.toolbarvisible: {p.toolbarvisible}")
    # array attr
    api.post(f"patcher.rect: {p.rect}")
    api.bang_success()


def test_patcher_add_box():
    p = api.get_patcher()
    box = p.add_box("toggle", 240.0, 150.0)
    return str(box.rect)


def test_patcher_add_textbox():
    p = api.get_patcher()
    textbox = p.add_textbox("metro 400", 100.0, 150.0)
    return str(textbox.rect)


def test_patcher_add_tbox():
    p = api.get_patcher()
    tbox = p.add_tbox("buffer~ buf jongly.aif")
    return str(tbox.rect)


def test_patcher_add_box_from_dict():
    p = api.get_patcher()
    d = api.Dictionary(maxclass="toggle", patching_position=[10.0, 20.0])
    box = p.add_box_from_dict(d)
    return str(box.rect)


def test_patcher_script_newdefault():
    p = api.get_patcher()
    p.newdefault("var3", 200, 150, "number")
    api.bang_success()


def test_patcher_script_connect():
    p = api.get_patcher()
    p.connect("var1", 0, "var2", 0)
    api.bang_success()


def test_patcher_script_disconnect():
    p = api.get_patcher()
    p.disconnect("var1", 0, "var2", 0)
    api.bang_success()


def test_patcher_script_hide():
    p = api.get_patcher()
    p.hide("var1")
    api.bang_success()


def test_patcher_script_show():
    p = api.get_patcher()
    p.show("var1")
    api.bang_success()
