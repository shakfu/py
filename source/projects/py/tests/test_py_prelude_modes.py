"""Tests for dotted names in py_prelude.py, py_prelude_legacy.py and py_prelude.h."""

import math
import operator
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

import py_prelude
import py_prelude_legacy

ROOT = Path(__file__).resolve().parent.parent
# called by name from py.c
C_REQUIRED = ["apply", "call", "dict_to_list", "fold", "pipe", "product", "shell", "sig"]


@pytest.fixture(params=[py_prelude, py_prelude_legacy], ids=["restricted", "legacy"])
def prelude(request, monkeypatch):
    module = request.param
    monkeypatch.setattr(module, "math", math, raising=False)
    monkeypatch.setattr(module, "operator", operator, raising=False)
    monkeypatch.setattr(module, "add", operator.add, raising=False)
    monkeypatch.setattr(module, "f", lambda *a, **k: (a, k), raising=False)
    return module


class TestBothPreludes:
    def test_dotted_call(self, prelude):
        assert prelude.call("math.sqrt 16") == 4.0

    def test_dotted_pipe(self, prelude):
        assert prelude.pipe("16 math.sqrt math.sqrt") == 2.0

    def test_dotted_fold(self, prelude):
        assert prelude.fold("operator.add 1 2 3") == 6

    def test_dotted_constant_is_an_argument(self, prelude):
        assert prelude.call("math.floor math.pi") == 3

    def test_apply_dotted(self, prelude):
        assert prelude.apply("math.sqrt 16") == 4.0

    def test_apply_converts_arguments(self, prelude):
        assert prelude.apply("f 1 2 a : 5 6 b : 10") == ((1, 2), {"a": [5, 6], "b": 10})

    def test_flatten(self, prelude):
        assert prelude.flatten([[1, 2], [3], []]) == [1, 2, 3]

    def test_compose(self, prelude):
        inc, double = (lambda x: x + 1), (lambda x: x * 2)
        assert prelude.compose(inc, double)(10) == 21
        assert prelude.compose(inc, double, reverse=False)(10) == 22

    def test_is_iterable(self, prelude):
        assert prelude.is_iterable("abc")
        assert not prelude.is_iterable(42)

    def test_list_to_dict(self, prelude):
        xs = ["a", ":", "1", "b", ":", "2", "3"]
        assert prelude.list_to_dict(xs) == {"a": "1", "b": ["2", "3"]}
        assert prelude.list_to_dict(xs, eval_values=True) == {"a": 1, "b": [2, 3]}
        with pytest.raises(ValueError):
            prelude.list_to_dict(["bad-key", ":", "1"])

    def test_dict_to_list(self, prelude):
        assert prelude.dict_to_list({"a": 1, "b": [1, 2]}) == ["a", ":", 1, "b", ":", 1, 2]


class TestRestricted:
    @pytest.mark.parametrize("name", ["math.nope", "nope.sqrt", "math.", "math.__loader__"])
    def test_unresolved_dotted_name_finds_no_function(self, monkeypatch, name):
        monkeypatch.setattr(py_prelude, "math", math, raising=False)
        assert py_prelude.call(f"{name} 16") is None

    def test_apply_rejects_private_attribute(self, monkeypatch):
        monkeypatch.setattr(py_prelude, "math", math, raising=False)
        with pytest.raises(ValueError):
            py_prelude.apply("math.__loader__ 1")

    def test_apply_rejects_non_callable(self, monkeypatch):
        monkeypatch.setattr(py_prelude, "math", math, raising=False)
        with pytest.raises(ValueError):
            py_prelude.apply("math.pi 1")

    def test_expressions_are_not_evaluated(self, monkeypatch):
        monkeypatch.setattr(py_prelude, "inc", lambda x: x + 1, raising=False)
        with pytest.raises(TypeError):
            py_prelude.pipe("10+1 inc")

    def test_dotted_text_stays_a_string(self):
        assert py_prelude.call("str file.txt") == "file.txt"

    def test_plain_string_value_is_not_a_builtin(self):
        assert py_prelude.list_to_dict(["a", ":", "max"], eval_values=True) == {"a": "max"}


class TestLegacy:
    def test_expressions_are_evaluated(self, monkeypatch):
        monkeypatch.setattr(py_prelude_legacy, "inc", lambda x: x + 1, raising=False)
        assert py_prelude_legacy.pipe("10+1 inc") == 12

    def test_apply_keeps_unparsed_argument_as_string(self):
        assert py_prelude_legacy.apply("str word") == "word"

    def test_from_string(self):
        assert py_prelude_legacy.from_string("f 1 a : 2") == ("f", ("1",), {"a": "2"})


class TestHeader:
    def test_header_matches_sources(self, tmp_path):
        for name in ("py_prelude.py", "py_prelude_legacy.py"):
            shutil.copy(ROOT / name, tmp_path)
        subprocess.run(
            [sys.executable, str(ROOT / "scripts" / "py2c.py")], cwd=tmp_path, check=True
        )
        assert (tmp_path / "py_prelude.h").read_text() == (ROOT / "py_prelude.h").read_text()

    @pytest.mark.skipif(shutil.which("cc") is None, reason="needs a C compiler")
    @pytest.mark.parametrize(
        "flags,evaluates", [([], True), (["-DRESTRICTED_PYTHON"], False)], ids=["default", "restricted"]
    )
    def test_compile_flag_selects_prelude(self, tmp_path, flags, evaluates):
        src = tmp_path / "main.c"
        src.write_text(
            f'#include <stdio.h>\n#include "{ROOT / "py_prelude.h"}"\n'
            'int main(void) { fputs(PY_PRELUDE_MODULE, stdout); return 0; }\n'
        )
        exe = tmp_path / "main"
        subprocess.run(["cc", *flags, str(src), "-o", str(exe)], check=True)
        code = subprocess.run([str(exe)], check=True, capture_output=True, text=True).stdout

        namespace = {"__name__": "prelude"}
        exec(code, namespace)  # py.c runs the module the same way
        for name in C_REQUIRED:
            assert callable(namespace[name]), name
        namespace["inc"] = lambda x: x + 1
        if evaluates:
            assert namespace["pipe"]("10+1 inc") == 12
        else:
            with pytest.raises(TypeError):
                namespace["pipe"]("10+1 inc")
