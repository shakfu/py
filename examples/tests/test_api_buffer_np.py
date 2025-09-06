import math

import numpy as np

import api


def test_create_empty_buffer():
    name = "drum"
    duration_ms = 500
    buf = api.create_empty_buffer(name, duration_ms)
    api.post(f"created buffer name: '{name}' duration(ms): '{duration_ms}'")
    api.post(f"framecount: {buf.framecount}")


def test_buffer_get_samples():
    name = "drum1"
    sample_file = "jongly.aif"
    buf = api.create_buffer(name, sample_file)
    api.post(f"created buffer name: '{name}' sample_file: '{sample_file}'")
    n_samples = buf.n_samples
    xs = np.array(buf.get_samples())
    assert len(xs) == n_samples
    api.post(f"get {n_samples} samples from buffer {name}")


def test_buffer_set_samples():
    name = "drum1"
    duration_ms = 500
    buf = api.create_empty_buffer(name, duration_ms)
    api.post("buffer.set_samples example with numpy")
    api.post(f"created buffer name: '{name}' duration(ms): '{duration_ms}'")
    api.post(f"framecount: {buf.framecount}")
    n_samples = buf.n_samples
    t = np.linspace(0, 1, n_samples, endpoint=False, dtype=np.float32)
    xs = np.sin(t * 2 * math.pi * 5)
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
    n_samples = buf.n_samples * 2
    t = np.linspace(0, 1, n_samples, endpoint=False, dtype=np.float32)
    xs = np.sin(t * 2 * math.pi * 5)
    buf.set_samples(xs)
    api.post(f"set {buf.n_samples} samples to buffer {name}")


def test_buffer_set_samples3():
    name = "drum1"
    duration_ms = 500
    buf = api.create_empty_buffer(name, duration_ms)
    t = np.linspace(0, 1, buf.n_samples, endpoint=False, dtype=np.float32)
    buf.set_samples(t)


def test_buffer_protocol_read():
    name = "drum1"
    sample_file = "jongly.aif"
    buf = api.create_buffer(name, sample_file)
    api.post(f"created buffer name: '{name}' sample_file: '{sample_file}'")
    xs = np.asarray(buf)
    assert len(xs) == buf.n_samples
    api.post(f"len(x): {len(xs)} == buf.n_samples: {buf.n_samples}")


def test_buffer_protocol_write():
    name = "drum1"
    duration_ms = 500
    buf = api.create_empty_buffer(name, duration_ms)
    api.post("buffer.set_sample example with numpy and scipy.signal")
    api.post(f"created buffer name: '{name}' duration(ms): '{duration_ms}'")
    api.post(f"framecount: {buf.framecount}")
    xs = np.asarray(buf, dtype=np.float32)
    t = np.linspace(0, 1, buf.framecount, endpoint=False, dtype=np.float32)
    xs[:] = np.sin(t * 2 * math.pi * 5)
