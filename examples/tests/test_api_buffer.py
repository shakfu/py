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
    api.bang_success()


def test_view_buffer():
    # this step is necessary to get a reference to the `py` external instance
    ext = api.PyExternal()
    b = ext.get_buffer("drum")  # b is a buffer instance

    # test it
    api.post(f"framecount: {b.framecount})")
    api.send("drum_scope", "set", "drum")
    b.view()
    api.bang_success()


def test_create_buffer():
    name = "drum1"
    sample_file = "jongly.aif"
    buf = api.create_buffer(name, sample_file)
    api.post(f"created buffer name: '{name}' sample_file: '{sample_file}'")
    api.bang_success()


def test_get_buffer():
    buf = api.get_buffer("drum1")
    api.post(f"framecount: {buf.framecount}")
    api.send("drum_scope1", "set", "drum1")
    buf.view()
    api.bang_success()
    return buf.samplerate


def resize(buffer_name, frames):
    buf = api.get_buffer(buffer_name)
    api.post(f"framecount: {buf.framecount}")
    buf.set_framecount(frames)
    api.bang_success()
    return buf.framecount


def test_change_framecount():
    buf = api.get_buffer("drum")
    api.post(f"before: framecount: {buf.framecount}")
    buf.set_framecount(buf.framecount / 2)
    api.post(f"after: framecount: {buf.framecount}")
    api.bang_success()
    return buf.framecount


def test_change_framecount_prop():
    buf = api.get_buffer("drum")
    api.post(f"before: framecount: {buf.framecount}")
    # set as property
    buf.framecount = buf.framecount / 2
    api.post(f"after: framecount: {buf.framecount}")
    api.bang_success()
    return buf.framecount


def test_change_duration():
    buf = api.get_buffer("drum")
    api.post(f"before: duration (secs): {buf.duration}")
    buf.set_duration(buf.duration / 2)
    api.post(f"after: duration (secs): {buf.duration}")
    api.bang_success()
    return buf.duration


def test_change_duration_prop():
    buf = api.get_buffer("drum")
    api.post(f"before: duration (secs): {buf.duration}")
    buf.duration = buf.duration / 2
    api.post(f"after: duration (secs): {buf.duration}")
    api.bang_success()
    return buf.duration


def test_change_duration_ms():
    buf = api.get_buffer("drum")
    api.post(f"before duration (ms): {buf.duration_ms}")
    buf.set_duration_ms(buf.duration_ms / 2)
    api.post(f"after duration (ms): {buf.duration_ms}")
    api.bang_success()
    return buf.duration_ms


def test_change_duration_ms_prop():
    buf = api.get_buffer("drum")
    api.post(f"before duration (ms): {buf.duration_ms}")
    buf.duration_ms = buf.duration_ms / 2
    api.post(f"after duration (ms): {buf.duration_ms}")
    api.bang_success()
    return buf.duration_ms


def test_change_samplerate():
    buf = api.get_buffer("drum")
    buf.set_samplerate(22500)
    api.bang_success()
    return buf.samplerate


def test_change_samplerate_prop():
    buf = api.get_buffer("drum")
    buf.samplerate = 22500
    api.bang_success()
    return buf.samplerate


def test_change_filename_prop():
    buf = api.get_buffer("drum")
    buf.filename = "vibes-a1.aif"
    api.bang_success()
    return buf.filename


def test_change_reference():
    buf = api.get_buffer("drum")
    buf.change_reference("other")
    api.bang_success()
    return buf.filename


# ----------------------------------------------------------------------
# generic methods


def test_send():
    buf = api.get_buffer("drum")
    buf.send("fill", "sin", 24)
    api.bang_success()


def test_change():
    buf = api.get_buffer("drum")
    buf.change("sizeinsamps", 20000)
    api.bang_success()


# ----------------------------------------------------------------------
# message methods


# fixture
def buf(name="drum"):
    api.bang_success()
    return api.get_buffer(name)


def test_bang():
    buf().bang()
    api.bang_success()


def test_clear():
    buf().clear()
    api.bang_success()


def test_apply():
    buf().apply("gain", "0.3")
    api.bang_success()


def test_clearlow():
    buf().clearlow()
    api.bang_success()


def test_crop():
    buf().crop(100, 10_000)
    api.bang_success()


def test_duplicate():
    buf().duplicate("other")  # duplicate from <other-buf> which is stereo
    api.bang_success()


def test_enumerate():
    buf().enumerate()
    api.bang_success()


def test_fill():
    buf().fill("sin", 12)
    api.bang_success()

def test_import1():
    buf().import_("drumLoop.aif")
    api.bang_success()


def test_import2():
    buf().import_("drumLoop.aif", start=100)
    api.bang_success()


def test_import3():
    buf().import_("drumLoop.aif", duration=1000)
    api.bang_success()


def test_import4():
    buf().import_("drumLoop.aif", channels=1)
    api.bang_success()


def test_importreplace():
    buf().importreplace("drumLoop.aif")
    api.bang_success()


def test_rename():
    buf("drum").rename("drumx")
    buf("drumx").rename("drum")
    api.bang_success()


def test_normalize():
    buf().normalize(0.4)
    api.bang_success()


def test_open():
    buf().open()
    api.bang_success()


def test_close():
    buf().close()
    api.bang_success()


def test_printmodtime():
    buf().printmodtime()
    api.bang_success()


def test_write():
    endings = [".wav", ".aiff", ".raw", ".flac"]
    name = "drum"
    buf = api.get_buffer(name)

    tmp = Path("/tmp")

    for ending in endings:
        p = tmp / f"{name}{ending}"
        buf.write(str(p))
        assert p.exists()
    api.bang_success()