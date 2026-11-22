"""REPL support for py_repl, run inside the py external.

py embeds this file as py_repl_support.h (scripts/py2c.py) and executes it once
into the module `_py_repl_support`. Spec: docs/dev/py_repl-spec.md, 1.3.
Constants match py_repl_abi.h.
"""

import ast
import builtins
import codeop
import rlcompleter
import sys
import traceback
import warnings

OUT, ERR, RESULT, TRACEBACK, NAME = range(5)
OK, EXCEPTION, INTERRUPTED = range(3)
COMPLETE, INCOMPLETE, INVALID = range(3)

FILENAME = "<py_repl>"
MAX_NAMES = 200


class _Writer:
    """A text stream that emits every write as UTF-8 bytes."""

    encoding = "utf-8"
    errors = "replace"

    def __init__(self, emit, kind):
        self._emit = emit
        self._kind = kind

    def write(self, text):
        if text:
            self._emit(self._kind, text.encode("utf-8", "replace"))
        return len(text)

    def flush(self):
        pass

    def writable(self):
        return True

    def isatty(self):
        return False


def _emit_text(emit, kind, text):
    emit(kind, text.encode("utf-8", "replace"))


def run(code, ns, emit):
    """Run `code` in `ns`; display the last expression. Return OK, EXCEPTION or INTERRUPTED.

    `emit(kind, data)` receives each chunk of output as bytes.
    """
    def displayhook(value):
        if value is None:
            return
        builtins._ = value
        _emit_text(emit, RESULT, repr(value) + "\n")

    saved = sys.stdout, sys.stderr, sys.displayhook
    sys.stdout, sys.stderr = _Writer(emit, OUT), _Writer(emit, ERR)
    sys.displayhook = displayhook
    try:
        tree = ast.parse(code, FILENAME, "exec")
        body, last = tree.body[:-1], tree.body[-1:]
        if body:
            exec(compile(ast.Module(body, []), FILENAME, "exec"), ns)
        if last:
            exec(compile(ast.Interactive(last), FILENAME, "single"), ns)
        return OK
    except SystemExit as e:
        _emit_text(emit, ERR, f"SystemExit: {e.code}\n")
        return EXCEPTION
    except BaseException as e:
        # drop this function's frame: the traceback starts in the user's code
        tb = e.__traceback__.tb_next if e.__traceback__ else None
        _emit_text(emit, TRACEBACK, "".join(traceback.format_exception(type(e), e, tb)))
        return INTERRUPTED if isinstance(e, KeyboardInterrupt) else EXCEPTION
    finally:
        sys.stdout, sys.stderr, sys.displayhook = saved


def is_complete(code):
    """Return COMPLETE, INCOMPLETE (Return adds a line) or INVALID (submit to show the error)."""
    if not code.strip():
        return COMPLETE
    try:
        with warnings.catch_warnings():
            warnings.simplefilter("ignore")
            compiled = codeop.compile_command(code, FILENAME, "exec")
    except (SyntaxError, ValueError, OverflowError):
        return INVALID
    if compiled is None:
        return INCOMPLETE
    lines = code.split("\n")
    last_code = next(line for line in reversed(lines) if line.strip())
    # an indented block continues until a blank line, as in the CPython REPL
    if last_code[:1] in (" ", "\t") and lines[-1].strip():
        return INCOMPLETE
    return COMPLETE


def names(text, ns):
    """Return up to MAX_NAMES completions of `text` in `ns`, without rlcompleter's suffixes."""
    if not text.strip():
        return []
    completer = rlcompleter.Completer(ns)
    found = []
    state = 0
    while len(found) < MAX_NAMES:
        try:
            match = completer.complete(text, state)
        except Exception:
            break
        if match is None:
            break
        state += 1
        for suffix in ("()", "(", ":", " "):  # callables, keywords
            if match.endswith(suffix):
                match = match[: -len(suffix)]
                break
        if match not in found:
            found.append(match)
    return found
