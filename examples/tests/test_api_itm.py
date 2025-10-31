import api

mem = {}


def test_itm_get_global():
    itm = api.ITM.get_global()
    assert itm is not None
    mem["itm"] = itm
    api.bang_success()


def test_itm_get_resolution():
    resolution = api.ITM.get_resolution()
    assert resolution is not None
    assert isinstance(resolution, (int, float))
    assert resolution > 0


def test_itm_set_resolution():
    original_res = api.ITM.get_resolution()
    api.ITM.set_resolution(480)
    new_res = api.ITM.get_resolution()
    assert new_res == 480
    # Restore original
    api.ITM.set_resolution(original_res)


def test_itm_name():
    itm = mem["itm"]
    name = itm.name
    assert name is not None


def test_itm_time():
    itm = mem["itm"]
    time = itm.time
    assert time is not None
    assert isinstance(time, (int, float))


def test_itm_ticks():
    itm = mem["itm"]
    ticks = itm.ticks
    assert ticks is not None
    assert isinstance(ticks, (int, float))


def test_itm_tempo():
    itm = mem["itm"]
    tempo = itm.tempo
    assert tempo is not None
    assert isinstance(tempo, (int, float))
    assert tempo > 0


def test_itm_state():
    itm = mem["itm"]
    state = itm.state
    assert state is not None


def test_itm_is_running():
    itm = mem["itm"]
    running = itm.is_running
    assert isinstance(running, bool)


def test_itm_pause():
    itm = mem["itm"]
    itm.pause()
    # Verify pause can be called


def test_itm_resume():
    itm = mem["itm"]
    itm.resume()
    # Verify resume can be called


def test_itm_ticks_to_ms():
    itm = mem["itm"]
    ticks = 480
    ms = itm.ticks_to_ms(ticks)
    assert ms is not None
    assert isinstance(ms, (int, float))
    assert ms >= 0


def test_itm_ms_to_ticks():
    itm = mem["itm"]
    ms = 1000
    ticks = itm.ms_to_ticks(ms)
    assert ticks is not None
    assert isinstance(ticks, (int, float))
    assert ticks >= 0


def test_itm_ms_to_samples():
    itm = mem["itm"]
    ms = 1000
    samples = itm.ms_to_samples(ms)
    assert samples is not None
    assert isinstance(samples, (int, float))
    assert samples >= 0


def test_itm_samples_to_ms():
    itm = mem["itm"]
    samples = 44100
    ms = itm.samples_to_ms(samples)
    assert ms is not None
    assert isinstance(ms, (int, float))
    assert ms >= 0


def test_itm_bbu_to_ticks():
    itm = mem["itm"]
    bars = 1
    beats = 1
    units = 0
    ticks = itm.bbu_to_ticks(bars, beats, units)
    assert ticks is not None
    assert isinstance(ticks, (int, float))


def test_itm_ticks_to_bbu():
    itm = mem["itm"]
    ticks = 480
    bbu = itm.ticks_to_bbu(ticks)
    assert bbu is not None
    assert isinstance(bbu, tuple)
    assert len(bbu) == 3


def test_itm_timesignature_get():
    itm = mem["itm"]
    ts = itm.timesignature
    assert ts is not None
    assert isinstance(ts, tuple)
    assert len(ts) == 2


def test_itm_timesignature_set():
    itm = mem["itm"]
    original_ts = itm.timesignature
    itm.timesignature = (3, 4)
    new_ts = itm.timesignature
    assert new_ts[0] == 3
    assert new_ts[1] == 4
    # Restore original
    itm.timesignature = original_ts


def test_itm_conversions_roundtrip():
    itm = mem["itm"]
    original_ms = 1000
    ticks = itm.ms_to_ticks(original_ms)
    ms = itm.ticks_to_ms(ticks)
    # Should be approximately equal (allowing for rounding)
    assert abs(ms - original_ms) < 10
