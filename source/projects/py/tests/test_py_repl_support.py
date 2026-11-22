"""Tests for py_repl_support.py (docs/dev/py_repl-spec.md, 1.3).

Run with pytest, or directly: `python3 tests/test_py_repl_support.py`.
"""

import os
import sys
import traceback

HERE = os.path.dirname(os.path.abspath(__file__))
PYDIR = os.path.dirname(HERE)
sys.path.insert(0, PYDIR)
sys.path.insert(0, os.path.join(PYDIR, "scripts"))

import py_repl_support as rs  # noqa: E402


def run(code, ns=None):
    """Run `code`; return (status, [(kind, text)]) with adjacent chunks of a kind joined."""
    out = []

    def emit(kind, data):
        text = data.decode("utf-8")
        if out and out[-1][0] == kind:
            out[-1] = (kind, out[-1][1] + text)
        else:
            out.append((kind, text))

    status = rs.run(code, {} if ns is None else ns, emit)
    return status, out


def test_expression_is_displayed():
    assert run("1 + 2") == (rs.OK, [(rs.RESULT, "3\n")])


def test_only_last_statement_is_displayed():
    assert run("1\n2") == (rs.OK, [(rs.RESULT, "2\n")])


def test_none_is_not_displayed():
    assert run("None") == (rs.OK, [])


def test_assignment_is_not_displayed():
    assert run("x = 1") == (rs.OK, [])


def test_compound_last_statement_displays_each_value():
    assert run("for i in range(2): i") == (rs.OK, [(rs.RESULT, "0\n1\n")])


def test_print_goes_to_out_and_stderr_to_err():
    status, out = run("import sys\nprint('a')\nprint('b', file=sys.stderr)")
    assert status == rs.OK
    assert out == [(rs.OUT, "a\n"), (rs.ERR, "b\n")]


def test_namespace_persists_and_name_is_kept():
    ns = {"__name__": "foo"}
    run("x = 41", ns)
    assert run("x + 1", ns) == (rs.OK, [(rs.RESULT, "42\n")])
    assert run("__name__", ns) == (rs.OK, [(rs.RESULT, "'foo'\n")])


def test_underscore_holds_last_result():
    ns = {}
    run("6 * 7", ns)
    assert run("_", ns) == (rs.OK, [(rs.RESULT, "42\n")])


def test_exception_traceback_starts_in_user_code():
    status, out = run("1 / 0")
    assert status == rs.EXCEPTION
    kind, text = out[-1]
    assert kind == rs.TRACEBACK
    assert text.splitlines()[1].strip().startswith('File "<py_repl>"')
    assert text.rstrip().endswith("ZeroDivisionError: division by zero")
    assert "py_repl_support" not in text


def test_parse_error_is_reported():
    status, out = run("def f(:")
    assert status == rs.EXCEPTION
    assert out[-1][0] == rs.TRACEBACK
    assert "SyntaxError" in out[-1][1]


def test_compile_error_is_reported():
    status, out = run("return 1")
    assert status == rs.EXCEPTION
    assert "SyntaxError" in out[-1][1]


def test_system_exit_is_reported_not_raised():
    assert run("raise SystemExit(3)") == (rs.EXCEPTION, [(rs.ERR, "SystemExit: 3\n")])


def test_keyboard_interrupt_returns_interrupted():
    status, out = run("raise KeyboardInterrupt")
    assert status == rs.INTERRUPTED
    assert out[-1][1].rstrip().endswith("KeyboardInterrupt")


def test_output_before_an_exception_is_kept():
    status, out = run("print('before')\n1 / 0")
    assert status == rs.EXCEPTION
    assert out[0] == (rs.OUT, "before\n")


def test_streams_are_restored():
    saved = sys.stdout, sys.stderr, sys.displayhook
    for code in ("1", "1 / 0", "raise SystemExit", "raise KeyboardInterrupt", "def f(:"):
        run(code)
        assert (sys.stdout, sys.stderr, sys.displayhook) == saved, code


def test_non_utf8_text_is_replaced():
    status, out = run("print('\\udc80')")
    assert status == rs.OK
    assert out == [(rs.OUT, "?\n")]


COMPLETENESS = [
    ("", rs.COMPLETE),
    ("   ", rs.COMPLETE),
    ("x = 1", rs.COMPLETE),
    ("def f():", rs.INCOMPLETE),
    ("def f():\n    return 1", rs.INCOMPLETE),
    ("def f():\n    return 1\n    ", rs.COMPLETE),
    ("def f():\n    return 1\n", rs.COMPLETE),
    ("a = 1\nb = 2", rs.COMPLETE),
    ("for i in x: print(i)", rs.COMPLETE),
    ("if x:\n    a\nelse:", rs.INCOMPLETE),
    ("f(1,", rs.INCOMPLETE),
    ("@d", rs.INCOMPLETE),
    ('s = """abc', rs.INCOMPLETE),
    ("x = 1 \\", rs.INCOMPLETE),
    ("1 +* 2", rs.INVALID),
    ("'\\N{nope}'", rs.INVALID),
]


def test_is_complete():
    for code, expected in COMPLETENESS:
        assert rs.is_complete(code) == expected, code


def test_names_strip_rlcompleter_suffixes():
    def one(x):
        pass

    def none():
        pass

    ns = {"fone": one, "fnone": none, "fval": 3}
    found = rs.names("f", ns)
    for name in ("fone", "fnone", "fval", "for", "from", "finally"):
        assert name in found, name
    assert all(not n.endswith(("(", ")", ":", " ")) for n in found)


def test_names_complete_attributes():
    import math
    assert rs.names("math.flo", {"math": math}) == ["math.floor"]


def test_names_of_blank_text_are_empty():
    assert rs.names("", {}) == []
    assert rs.names("  ", {}) == []


def test_names_are_unique_and_capped():
    ns = {f"v{i}": i for i in range(rs.MAX_NAMES + 50)}
    found = rs.names("v", ns)
    assert len(found) == rs.MAX_NAMES
    assert len(set(found)) == len(found)


def test_names_survive_a_failing_attribute():
    class Bad:
        def __getattr__(self, name):
            raise RuntimeError("no")

        def __dir__(self):
            raise RuntimeError("no")

    assert rs.names("b.x", {"b": Bad()}) == []


def test_header_is_current():
    import py2c
    with open(os.path.join(PYDIR, "py_repl_support.py")) as f:
        expected = py2c.repl_header(f.read())
    with open(os.path.join(PYDIR, "py_repl_support.h")) as f:
        assert f.read() == expected, "run scripts/py2c.py in source/projects/py"


if __name__ == "__main__":
    tests = [(name, fn) for name, fn in sorted(globals().items()) if name.startswith("test_")]
    failed = 0
    for name, fn in tests:
        try:
            fn()
        except Exception:
            failed += 1
            print(f"FAIL {name}")
            traceback.print_exc()
    print(f"{len(tests) - failed}/{len(tests)} passed")
    sys.exit(1 if failed else 0)
