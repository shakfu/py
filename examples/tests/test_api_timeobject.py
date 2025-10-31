import api

mem = {}


def test_timeobject_init():
    def callback():
        api.post("TimeObject callback triggered")

    mem["to"] = api.TimeObject(callback)
    assert mem["to"] is not None
    api.bang_success()


def test_timeobject_ms_property():
    def callback():
        pass

    to = api.TimeObject(callback)
    to.ms = 100
    ms = to.ms
    assert ms is not None
    assert isinstance(ms, (int, float))


def test_timeobject_ticks_property():
    def callback():
        pass

    to = api.TimeObject(callback)
    to.ticks = 480
    ticks = to.ticks
    assert ticks is not None
    assert isinstance(ticks, (int, float))


def test_timeobject_is_fixed():
    def callback():
        pass

    to = api.TimeObject(callback)
    to.ms = 100
    is_fixed = to.is_fixed
    assert isinstance(is_fixed, bool)


def test_timeobject_phase():
    def callback():
        pass

    to = api.TimeObject(callback)
    phase = to.phase
    assert phase is not None


def test_timeobject_itm():
    def callback():
        pass

    to = api.TimeObject(callback)
    itm = to.itm
    assert itm is not None


def test_timeobject_stop():
    def callback():
        api.post("Should not be called")

    to = api.TimeObject(callback)
    to.ms = 100
    to.tick()
    to.stop()
    # Verify stop can be called


def test_timeobject_tick():
    callback_count = []

    def callback():
        callback_count.append(1)

    to = api.TimeObject(callback)
    to.ms = 10
    to.tick()
    # Verify tick can be called


def test_timeobject_ms_vs_ticks():
    def callback():
        pass

    to = api.TimeObject(callback)

    # Set in milliseconds
    to.ms = 1000
    ms_value = to.ms

    # Set in ticks
    to.ticks = 480
    ticks_value = to.ticks

    assert ms_value >= 0
    assert ticks_value >= 0


def test_timeobject_stop_without_tick():
    def callback():
        pass

    to = api.TimeObject(callback)
    to.stop()
    # Should not error even if not ticking


def test_timeobject_multiple_ticks():
    def callback():
        api.post("Tick callback")

    to = api.TimeObject(callback)
    to.ms = 50
    to.tick()
    to.stop()
    to.tick()
    to.stop()
    # Multiple tick/stop cycles should work
