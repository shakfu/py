"""
This file has an illustrative selection of tests drawn from various api.* unit_tests

"""

import api


def test_api_atom():
    ext = api.PyExternal()
    a1 = api.Atom([1, 2.5, b"hello", "world"])
    ext.out(a1.to_list())


def test_api_patcher():
    ext = api.PyExternal()
    p = ext.get_patcher()
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


def test_api_create():
    ext = api.PyExternal()
    p = ext.get_patcher()
    p.add_textbox("metro 400", 100.0, 150.0)
    p.add_box("toggle", 240.0, 150.0)


def test_api_dict():
    d = api.Dictionary()
    d["myfloat"] = 10.1
    d["myint"] = 3
    d["hello"] = "world"
    return d.getentrycount()


def test_api_send():
    api.send("intobj", 100)


def test_replace_buffer():
    b = api.get_buffer("drum")
    b.replace("vibes-a1.aif")


def test_create_buffer():
    name = "drum1"
    sample_file = "jongly.aif"
    buf = api.create_buffer(name, sample_file)
    api.post(f"created buffer name: '{name}' sample_file: '{sample_file}'")


def test_crop_buffer():
    buf = api.get_buffer("drum")
    buf.crop(100, 1_000)
    api.post(f"cropped 'drum' buffer")


def test_fill_buffer():
    buf = api.get_buffer("drum1")
    buf.fill("sin", 24)
