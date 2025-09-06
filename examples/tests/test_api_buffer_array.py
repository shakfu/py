"""
testing python array.arrays and memoryviews

The latter implement the buffer protocol

see:

memory views:
- https://docs.python.org/dev/library/stdtypes.html#memoryview
- https://docs.python.org/3/c-api/memoryview.html
- https://stackoverflow.com/questions/18655648/what-exactly-is-the-point-of-memoryview-in-python
- https://github.com/python/cpython/issues/101071
- https://github.com/python/cpython/issues/84620

"""

import math
from array import array

import api


def test_create_empty_buffer():
    name = "drum"
    duration_ms = 500
    # TODO: check if a buffer already exists
    buf = api.create_empty_buffer(name, duration_ms)
    api.post(f"created buffer name: '{name}' duration(ms): '{duration_ms}'")
    api.post(f"framecount: {buf.framecount}")


def test_buffer_get_samples():
    name = "drum1"
    sample_file = "jongly.aif"
    buf = api.create_buffer(name, sample_file)
    api.post(f"created buffer name: '{name}' sample_file: '{sample_file}'")
    n_samples = buf.n_samples
    xs = array("f", buf.get_samples())
    assert len(xs) == n_samples
    api.post(f"get {n_samples} samples from buffer {name}")


def linspace(start, stop, num=50, endpoint=True):
    """pure python version of np.linspace
    from: https://gist.github.com/pmav99/d124872c879f3e9fa51e
    """
    num = int(num)
    start = start * 1.0
    stop = stop * 1.0

    if num == 1:
        yield stop
        return
    if endpoint:
        step = (stop - start) / (num - 1)
    else:
        step = (stop - start) / num

    for i in range(num):
        yield start + step * i


def test_buffer_set_samples():
    name = "drum1"
    duration_ms = 500
    buf = api.create_empty_buffer(name, duration_ms)
    api.post("buffer.set_sample example with numpy and scipy.signal")
    api.post(f"created buffer name: '{name}' duration(ms): '{duration_ms}'")
    api.post(f"framecount: {buf.framecount}")
    n_samples = buf.n_samples
    t = linspace(0, 1, n_samples, endpoint=False)
    xs = array("f", [math.sin(i * 2 * math.pi * 5) for i in t])
    buf.set_samples(xs)
    api.post(f"set {n_samples} samples to buffer {name}")


def test_buffer_set_samples2():
    name = "drum1"
    duration_ms = 500
    buf = api.create_empty_buffer(name, duration_ms)
    api.post(f"testing auto-resizing of buffer size")
    api.post("buffer.set_sample example with numpy and scipy.signal")
    api.post(f"created buffer name: '{name}' duration(ms): '{duration_ms}'")
    api.post(f"framecount: {buf.framecount}")
    n_samples = 2 * buf.framecount
    t = linspace(0, 1, buf.framecount, endpoint=False)
    xs = array("f", [math.sin(i * 2 * math.pi * 5) for i in t])
    buf.set_samples(xs)
    api.post(f"set {buf.framecount} samples to buffer {name}")


def test_buffer_protocol_read():
    name = "drum1"
    sample_file = "jongly.aif"
    buf = api.create_buffer(name, sample_file)
    api.post(f"created buffer name: '{name}' sample_file: '{sample_file}'")
    with memoryview(buf) as mv_buf:
        xs = array("f", mv_buf)
        assert len(xs) == buf.n_samples
        api.post(f"len(xs): {len(xs)} == buf.n_samples: {buf.n_samples}")


def test_buffer_protocol_write():
    name = "drum1"
    duration_ms = 500
    buf = api.create_empty_buffer(name, duration_ms)
    api.post("buffer.set_sample example with numpy and scipy.signal")
    api.post(f"created buffer name: '{name}' duration(ms): '{duration_ms}'")
    api.post(f"framecount: {buf.framecount}")
    t = linspace(0, 1, buf.framecount, endpoint=False)
    ys = array("f", [math.sin(i * 2 * math.pi * 5) for i in t])
    with memoryview(buf) as xs:
        xs[:] = ys


def test_buffer_memoryview_read():
    name = "drum1"
    sample_file = "jongly.aif"
    mybuf = api.create_buffer(name, sample_file)
    n = mybuf.framecount
    with memoryview(mybuf) as buf:
        assert not buf.readonly
        assert buf.obj is mybuf
        api.post(f"first: buf[0] = {buf[0]}")
        api.post(f"last: buf[{n - 1}] = {buf[n - 1]}")


def test_buffer_memoryview_write():
    name = "drum1"
    sample_file = "jongly.aif"
    mybuf = api.create_buffer(name, sample_file)
    n = mybuf.framecount
    t = linspace(0, 1, n, endpoint=False)
    ys = array("f", [math.sin(i * 2 * math.pi * 5) for i in t])
    with memoryview(mybuf) as buf:
        buf[:] = ys
