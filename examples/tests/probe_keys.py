"""Probe for py_repl's interrupt key (docs/dev/py_repl-spec.md, open questions 1-2).

Driven by patchers/tests/test_py_repl/probe_keys.maxpat. `probe(5)` keeps Max's
main thread busy for 5 s, as a long REPL run does, and polls Ctrl+C with
CGEventSourceKeyState, as py_repl's monitor thread will. Press Ctrl+C while it
runs; switch to another app and press it again.
"""

import ctypes as C
import os
import time

_cg = C.CDLL("/System/Library/Frameworks/CoreGraphics.framework/CoreGraphics")
_cf = C.CDLL("/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation")

COMBINED = 0          # kCGEventSourceStateCombinedSessionState
KEY_C = 8             # kVK_ANSI_C
MASK_CONTROL = 0x40000  # kCGEventFlagMaskControl

_cg.CGEventSourceKeyState.restype = C.c_bool
_cg.CGEventSourceKeyState.argtypes = [C.c_int32, C.c_uint16]
_cg.CGEventSourceFlagsState.restype = C.c_uint64
_cg.CGEventSourceFlagsState.argtypes = [C.c_int32]
_cg.CGWindowListCopyWindowInfo.restype = C.c_void_p
_cg.CGWindowListCopyWindowInfo.argtypes = [C.c_uint32, C.c_uint32]
_cf.CFArrayGetCount.restype = C.c_long
_cf.CFArrayGetCount.argtypes = [C.c_void_p]
_cf.CFArrayGetValueAtIndex.restype = C.c_void_p
_cf.CFArrayGetValueAtIndex.argtypes = [C.c_void_p, C.c_long]
_cf.CFDictionaryGetValue.restype = C.c_void_p
_cf.CFDictionaryGetValue.argtypes = [C.c_void_p, C.c_void_p]
_cf.CFNumberGetValue.restype = C.c_bool
_cf.CFNumberGetValue.argtypes = [C.c_void_p, C.c_int, C.c_void_p]
_cf.CFRelease.argtypes = [C.c_void_p]

_LAYER = C.c_void_p.in_dll(_cg, "kCGWindowLayer")
_OWNER = C.c_void_p.in_dll(_cg, "kCGWindowOwnerPID")


def _int(d, key):
    v = C.c_int()
    n = _cf.CFDictionaryGetValue(d, key)
    return v.value if n and _cf.CFNumberGetValue(n, 9, C.byref(v)) else None  # kCFNumberIntType


def front_pid():
    """Owner of the frontmost layer-0 window, as py_repl's monitor checks it."""
    wins = _cg.CGWindowListCopyWindowInfo(1 | 16, 0)  # on screen only, no desktop
    if not wins:
        return None
    try:
        for i in range(_cf.CFArrayGetCount(wins)):
            w = _cf.CFArrayGetValueAtIndex(wins, i)
            if _int(w, _LAYER) == 0:
                return _int(w, _OWNER)
    finally:
        _cf.CFRelease(wins)
    return None


def ctrl_c():
    down = _cg.CGEventSourceKeyState(COMBINED, KEY_C)
    return down and bool(_cg.CGEventSourceFlagsState(COMBINED) & MASK_CONTROL)


def probe(seconds=5):
    """Busy the main thread for `seconds`, polling Ctrl+C; return what was seen."""
    me = os.getpid()
    in_max = elsewhere = 0
    was = False
    end = time.time() + seconds
    while time.time() < end:
        now = ctrl_c()
        if now and not was:
            if front_pid() == me:
                in_max += 1
            else:
                elsewhere += 1
        was = now
        time.sleep(0.02)
    return f"ctrl-c presses: {in_max} with Max in front, {elsewhere} with another app in front"
