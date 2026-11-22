"""In-Max test of py's A_CANT repl methods (py_repl_abi.h), through ctypes.

Driven by patchers/tests/test_py_repl/test_repl_abi.maxpat. A patch cannot call
A_CANT methods, so ctypes finds object_getmethod in the Max process and calls
them on [py @name repl_target]. `run_all()` returns a summary; failures are
posted to the Max console.
"""

import builtins
import ctypes as C
import sys
import threading
import time
import traceback

import api

OUT, ERR, RESULT, TRACEBACK, NAME = range(5)
OK, EXCEPTION, INTERRUPTED, BUSY, REFUSED = range(5)
COMPLETE, INCOMPLETE, INVALID = range(3)
ABI = 1
TARGET = b"repl_target"

EMIT = C.CFUNCTYPE(None, C.c_void_p, C.c_long, C.POINTER(C.c_char), C.c_long)


class Sink(C.Structure):
    _fields_ = [("size", C.c_long), ("ctx", C.c_void_p), ("emit", EMIT)]


_max = C.CDLL(None)
for _name, _args in [("gensym", [C.c_char_p]),
                     ("object_findregistered", [C.c_void_p, C.c_void_p]),
                     ("object_getmethod", [C.c_void_p, C.c_void_p])]:
    getattr(_max, _name).restype = C.c_void_p
    getattr(_max, _name).argtypes = _args


class Target:
    """The A_CANT methods of a py object, called as C functions."""

    def __init__(self, name=TARGET):
        self.obj = _max.object_findregistered(_max.gensym(b"box"), _max.gensym(name))
        if not self.obj:
            raise RuntimeError(f"no py object named {name.decode()}")
        # void* results are read as intptr_t, so -1 stays -1
        self._abi = self._method("repl_abi")
        self._run = self._method("repl_run", C.c_char_p, C.POINTER(Sink))
        self._complete = self._method("repl_complete", C.c_char_p)
        self._names = self._method("repl_names", C.c_char_p, C.POINTER(Sink))
        self._interrupt = self._method("repl_interrupt")

    def _method(self, name, *argtypes):
        addr = _max.object_getmethod(self.obj, _max.gensym(name.encode()))
        if not addr:
            raise RuntimeError(f"py has no method {name}")
        return C.CFUNCTYPE(C.c_ssize_t, C.c_void_p, *argtypes)(addr)

    def _sink(self, out, size=None):
        def emit(ctx, kind, text, n):
            out.append((kind, C.string_at(text, n).decode("utf-8")))
        fn = EMIT(emit)
        return Sink(C.sizeof(Sink) if size is None else size, None, fn), fn

    def abi(self):
        return self._abi(self.obj)

    def run(self, code, size=None):
        """Return (status, [(kind, text)]), joining adjacent chunks of a kind."""
        chunks = []
        sink, keep = self._sink(chunks, size)
        status = self._run(self.obj, code.encode(), C.byref(sink))
        joined = []
        for kind, text in chunks:
            if joined and joined[-1][0] == kind:
                joined[-1] = (kind, joined[-1][1] + text)
            else:
                joined.append((kind, text))
        return status, joined

    def complete(self, code):
        return self._complete(self.obj, code.encode())

    def names(self, text):
        found = []
        sink, keep = self._sink(found)
        self._names(self.obj, text.encode(), C.byref(sink))
        return [t for _, t in found]

    def interrupt(self):
        return self._interrupt(self.obj)


def check(cond, msg):
    if not cond:
        raise AssertionError(msg)


def test_abi(t):
    check(t.abi() == ABI, f"abi {t.abi()}")


def test_expression(t):
    r = t.run("1 + 2")
    check(r == (OK, [(RESULT, "3\n")]), r)


def test_namespace_and_name(t):
    t.run("x = 41")
    r = t.run("x + 1")
    check(r == (OK, [(RESULT, "42\n")]), r)
    r = t.run("__name__")
    check(r == (OK, [(RESULT, "'repl_target'\n")]), r)


def test_print(t):
    r = t.run("print('a')")
    check(r == (OK, [(OUT, "a\n")]), r)


def test_traceback(t):
    status, out = t.run("1 / 0")
    check(status == EXCEPTION and out[-1][0] == TRACEBACK, (status, out))
    check("ZeroDivisionError" in out[-1][1], out)


def test_system_exit(t):
    r = t.run("raise SystemExit(3)")
    check(r == (EXCEPTION, [(ERR, "SystemExit: 3\n")]), r)


def test_kept_stdout_writes_nowhere(t):
    t.run("import sys\nkeep = sys.stdout")
    r = t.run("n = keep.write('stale')")
    check(r == (OK, []), r)


def test_stdout_restored(t):
    saved = sys.stdout, sys.stderr, sys.displayhook
    t.run("print('x')")
    t.run("1 / 0")
    check((sys.stdout, sys.stderr, sys.displayhook) == saved, "streams changed")


def test_complete(t):
    check(t.complete("def f():") == INCOMPLETE, "def f():")
    check(t.complete("x = 1") == COMPLETE, "x = 1")
    check(t.complete("1 +* 2") == INVALID, "1 +* 2")


def test_names(t):
    found = t.names("pri")
    check("print" in found, found)


def test_small_sink_refused(t):
    r = t.run("1", size=4)
    check(r[0] == REFUSED, r)


def test_reentry_is_busy(t):
    builtins._repl_reenter = lambda: t.run("1")[0]
    try:
        r = t.run("_repl_reenter()")
    finally:
        del builtins._repl_reenter
    check(r == (OK, [(RESULT, f"{BUSY}\n")]), r)


def test_interrupt(t):
    # bounded, so a failed interrupt reports instead of hanging Max
    def press():
        time.sleep(0.3)
        t.interrupt()
    threading.Thread(target=press, daemon=True).start()
    start = time.time()
    status, out = t.run("import time\nt0 = time.time()\nwhile time.time() - t0 < 10: pass")
    elapsed = time.time() - start
    check(status == INTERRUPTED, (status, out))
    check(elapsed < 5, f"took {elapsed:.1f} s")
    check(out and out[-1][1].rstrip().endswith("KeyboardInterrupt"), out)


def test_late_interrupt_refused(t):
    check(t.interrupt() == -1, "interrupt with no run")
    r = t.run("1")
    check(r == (OK, [(RESULT, "1\n")]), r)


TESTS = [v for k, v in sorted(globals().items()) if k.startswith("test_")]


def run_all():
    """Run every test on [py @name repl_target]; return 'n/m ok'."""
    try:
        t = Target()
    except Exception as e:
        api.error(f"test_repl_abi: {e}")
        return "setup failed"
    passed = 0
    for test in TESTS:
        try:
            test(t)
            passed += 1
        except Exception:
            api.error(f"FAIL {test.__name__}: {traceback.format_exc().strip().splitlines()[-1]}")
    return f"{passed}/{len(TESTS)} ok"
