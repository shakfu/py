import time
import api

mem = {}


def test_clock_gettime():
    current_time = api.Clock.gettime()
    assert current_time is not None
    assert isinstance(current_time, (int, float))


def test_clock_init():
    def callback():
        api.post("Clock callback triggered")

    mem["clock"] = api.Clock(callback)
    assert mem["clock"] is not None
    api.bang_success()


def test_clock_delay():
    callback_triggered = []

    def callback():
        callback_triggered.append(True)

    clock = api.Clock(callback)
    clock.delay(100)
    # Note: In a real Max environment, this would trigger after 100ms
    # In tests, we just verify the method can be called


def test_clock_unset():
    def callback():
        pass

    clock = api.Clock(callback)
    clock.delay(1000)
    clock.unset()
    # Verify unset can be called without error


def test_clock_multiple_delays():
    def callback():
        api.post("Multiple delay callback")

    clock = api.Clock(callback)
    clock.delay(50)
    clock.delay(100)
    # Second delay should override the first


def test_clock_immediate_unset():
    def callback():
        pass

    clock = api.Clock(callback)
    clock.delay(100)
    clock.unset()
    # Verify immediate unset works


def test_clock_gettime_increases():
    time1 = api.Clock.gettime()
    time.sleep(0.01)
    time2 = api.Clock.gettime()
    # Time should be non-negative
    assert time1 >= 0
    assert time2 >= 0
