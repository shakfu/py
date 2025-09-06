"""
tests for api.Buffer wrapper in api.pyx

also see:

    - test_buffer_array.py for use of arrays with buffer

    - test_buffer_np.py for use of numpy/scipy with buffer

"""

import api

from pathlib import Path


def test_init_buffer():
    # creating a buffer the pythonic way!! (Experimental)
    b = api.Buffer("drum", "drumLoop.aif")
    # test it
    api.post(f"framecount: {b.framecount})")
    b.view()


def test_view_buffer():
    # this step is necessary to get a reference to the `py` external instance
    ext = api.PyExternal()
    b = ext.get_buffer("drum")  # b is a buffer instance

    # test it
    api.post(f"framecount: {b.framecount})")
    api.send("drum_scope", "set", "drum")
    b.view()


def test_create_buffer():
    name = "drum1"
    sample_file = "jongly.aif"
    buf = api.create_buffer(name, sample_file)
    api.post(f"created buffer name: '{name}' sample_file: '{sample_file}'")


def test_get_buffer():
    buf = api.get_buffer("drum1")
    api.post(f"framecount: {buf.framecount}")
    api.send("drum_scope1", "set", "drum1")
    buf.view()
    return buf.samplerate


def resize(buffer_name, frames):
    buf = api.get_buffer(buffer_name)
    api.post(f"framecount: {buf.framecount}")
    buf.set_framecount(frames)
    return buf.framecount


def test_change_framecount():
    buf = api.get_buffer("drum")
    api.post(f"before: framecount: {buf.framecount}")
    buf.set_framecount(buf.framecount / 2)
    api.post(f"after: framecount: {buf.framecount}")
    return buf.framecount


def test_change_framecount_prop():
    buf = api.get_buffer("drum")
    api.post(f"before: framecount: {buf.framecount}")
    # set as property
    buf.framecount = buf.framecount / 2
    api.post(f"after: framecount: {buf.framecount}")
    return buf.framecount


def test_change_duration():
    buf = api.get_buffer("drum")
    api.post(f"before: duration (secs): {buf.duration}")
    buf.set_duration(buf.duration / 2)
    api.post(f"after: duration (secs): {buf.duration}")
    return buf.duration


def test_change_duration_prop():
    buf = api.get_buffer("drum")
    api.post(f"before: duration (secs): {buf.duration}")
    buf.duration = buf.duration / 2
    api.post(f"after: duration (secs): {buf.duration}")
    return buf.duration


def test_change_duration_ms():
    buf = api.get_buffer("drum")
    api.post(f"before duration (ms): {buf.duration_ms}")
    buf.set_duration_ms(buf.duration_ms / 2)
    api.post(f"after duration (ms): {buf.duration_ms}")
    return buf.duration_ms


def test_change_duration_ms_prop():
    buf = api.get_buffer("drum")
    api.post(f"before duration (ms): {buf.duration_ms}")
    buf.duration_ms = buf.duration_ms / 2
    api.post(f"after duration (ms): {buf.duration_ms}")
    return buf.duration_ms


def test_change_samplerate():
    buf = api.get_buffer("drum")
    buf.set_samplerate(22500)
    return buf.samplerate


def test_change_samplerate_prop():
    buf = api.get_buffer("drum")
    buf.samplerate = 22500
    return buf.samplerate


def test_change_filename_prop():
    buf = api.get_buffer("drum")
    buf.filename = "vibes-a1.aif"
    return buf.filename


def test_change_reference():
    buf = api.get_buffer("drum")
    buf.change_reference("other")
    return buf.filename


# ----------------------------------------------------------------------
# generic methods


def test_send():
    buf = api.get_buffer("drum")
    buf.send("fill", "sin", 24)


def test_change():
    buf = api.get_buffer("drum")
    buf.change("sizeinsamps", 20000)


# ----------------------------------------------------------------------
# message methods


# fixture
def buf(name="drum"):
    return api.get_buffer(name)


def test_bang():
    buf().bang()


def test_clear():
    buf().clear()


def test_apply():
    buf().apply("gain", "0.3")


def test_clearlow():
    buf().clearlow()


def test_crop():
    buf().crop(100, 10_000)


def test_duplicate():
    buf().duplicate("other")  # duplicate from <other-buf>


def test_enumerate():
    buf().enumerate()


def test_fill():
    buf().fill("sin", 12)


def test_import1():
    buf().import_("drumLoop.aif")


def test_import2():
    buf().import_("drumLoop.aif", start=100)


def test_import3():
    buf().import_("drumLoop.aif", duration=1000)


def test_import4():
    buf().import_("drumLoop.aif", channels=1)


def test_importreplace():
    buf().importreplace("drumLoop.aif")


def test_rename():
    buf("drum").rename("drumx")
    buf("drumx").rename("drum")


def test_normalize():
    buf().normalize(0.4)


def test_open():
    buf().open()


def test_close():
    buf().close()


def test_printmodtime():
    buf().printmodtime()


def test_write():
    endings = [".wav", ".aiff", ".raw", ".flac"]
    name = "drum"
    buf = api.get_buffer(name)

    tmp = Path("/tmp")

    for ending in endings:
        p = tmp / f"{name}{ending}"
        buf.write(str(p))
        assert p.exists()
