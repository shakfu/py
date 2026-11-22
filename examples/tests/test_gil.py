"""GIL hand-off test, driven by patchers/tests/test_gil/test_gil.maxpat.

A background thread ticks every 10 ms. It only runs when the main thread
releases the GIL, so `rate()` near 100 means py releases it while idle,
and near 0 means the main thread holds it between messages.
"""

import threading
import time

ticks = 0
started = 0.0
_stop = threading.Event()


def _tick():
    global ticks
    while not _stop.is_set():
        ticks += 1
        time.sleep(0.01)


def start():
    """Start the ticking thread; returns 1."""
    global ticks, started
    _stop.clear()
    ticks = 0
    started = time.time()
    threading.Thread(target=_tick, daemon=True).start()
    return 1


def rate():
    """Return ticks per second since start()."""
    return ticks / max(time.time() - started, 1e-9)


def stop():
    """Ask the thread to exit; returns 1."""
    _stop.set()
    return 1
