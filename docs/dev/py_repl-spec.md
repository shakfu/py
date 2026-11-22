# py_repl: implementation spec

Behavioural specification for `py_repl`, an interactive Python console box for
Max that runs code in a linked `py` object's namespace. Written so it can be
implemented from this document and `docs/dev/py_code-spec.md`. MIT-licensed.

Directory: `source/projects/py_repl/`, plus additions to `source/projects/py/`.
Language: C++17 for `py_repl` (ObjC++ for macOS helpers), C and Python for `py`.
Target: macOS 11+, arm64 and x86_64, Max 8/9 SDK.

Prerequisites, already in `py`:

- `@name` is fixed at creation and matches the registration, the module
  `__name__` and the scripting name.
- The main thread releases the GIL between messages; outlet calls release it.

Conventions: as `py_code-spec.md` (namespace `pyc`, UTF-8 byte offsets, cells
are code points, headless core under `make test`, ASCII, SPDX headers, warnings
clean).

---

## 0. Decisions

| decision | over | because |
|-|-|-|
| `py_repl` calls `py` through `A_CANT` methods | patch messages and a return cord | output stays typed (stdout, stderr, result, traceback); no symbol interning per chunk; completeness and completion are synchronous |
| REPL logic in Python (`py_repl_support.py`), embedded in `py` | C | `ast`, `codeop`, `traceback` and `rlcompleter` do the work; testable with pytest outside Max |
| code runs on the calling (main) thread | a worker thread | user code calls `api`, which must run on the main or scheduler thread |
| interrupt via `Py_AddPendingCall` from a key-polling thread | no interrupt, or a timeout | long loops must be stoppable; the UI cannot read keys while the main thread is busy |
| two `Editor`s (transcript, input) | one `Editor` with a read-only prefix | no change to `py_code`'s `Editor`; output never shifts input offsets |
| resolve the `py` object by name on every use | a stored, subscribed pointer | a lookup cannot dangle; no dependence on subscription notifications |
| no completeness fallback when unlinked | a tree-sitter heuristic | nothing can run unlinked, so Return only needs to show the error |
| runtime completion only (`rlcompleter`) | the LSP client | the REPL's names exist at runtime; pylsp sees only the input text |

---

## 1. `py` additions

### 1.1 `py_repl_abi.h`

New header `source/projects/py/py_repl_abi.h`, included by `py.c` and by
`py_repl` (include path `../py`). Plain C, no Max or Python types.

```c
#define PY_REPL_ABI 1

enum { PY_REPL_OUT, PY_REPL_ERR, PY_REPL_RESULT, PY_REPL_TRACEBACK, PY_REPL_NAME };
enum { PY_REPL_OK, PY_REPL_EXCEPTION, PY_REPL_INTERRUPTED, PY_REPL_BUSY, PY_REPL_REFUSED };
enum { PY_REPL_COMPLETE, PY_REPL_INCOMPLETE, PY_REPL_INVALID };

typedef struct py_repl_sink {
    long size;   /* sizeof(py_repl_sink): py rejects a smaller struct */
    void* ctx;
    void (*emit)(void* ctx, long kind, const char* text, long len);
} py_repl_sink;
```

`emit` receives UTF-8 that is valid only during the call. Kinds: `OUT` and
`ERR` are `sys.stdout`/`sys.stderr` writes, `RESULT` is the `repr` of a
displayed value with a trailing `\n`, `TRACEBACK` is formatted exception text,
`NAME` is one completion.

### 1.2 `A_CANT` methods on `py`

Registered with `class_addmethod(..., A_CANT, 0)`; called with
`object_method(py, gensym(name), ...)`. Integer results are returned as
`(void*)(intptr_t)`.

| method | args | returns | thread |
|-|-|-|-|
| `repl_abi` | - | `PY_REPL_ABI` | any |
| `repl_run` | `const char* code, py_repl_sink* sink` | `PY_REPL_OK/EXCEPTION/INTERRUPTED/BUSY/REFUSED` | main |
| `repl_complete` | `const char* code` | `PY_REPL_COMPLETE/INCOMPLETE/INVALID` | main |
| `repl_names` | `const char* text, py_repl_sink* sink` | number of names emitted | main |
| `repl_interrupt` | - | 0 queued, -1 not running or queue full | any |

`repl_run`:

- `REFUSED` when `sink->size < sizeof(py_repl_sink)`, the code exceeds
  `PY_MAX_CODE_LENGTH`, or `py_validate_code` fails (the reason is emitted as
  `ERR`).
- `BUSY` when a `repl_run` is already active on this object (re-entry through
  an outlet). Nothing runs.
- Otherwise: take the GIL, set the object's `repl_running` flag, call
  `py_repl_support.run(code, globals, emit)`, clear the flag, release the GIL.
- Never calls `PyErr_Print`: on `SystemExit` it would exit Max.
- Saves and restores `sys.stdout`, `sys.stderr` and `sys.displayhook` itself:
  a second interrupt can land during `run`'s own cleanup.
- The emitter passed to `run` writes through a slot that is cleared when the
  call returns. User code that keeps `sys.stdout` then writes nowhere.

`repl_complete` and `repl_names` take the GIL and call the functions in 1.3.
They never run user code, except attribute access in `names` (1.3).

`repl_interrupt` queues `Py_AddPendingCall(py_repl_raise, x)` when
`repl_running` is set. `py_repl_raise` raises `KeyboardInterrupt` only if
`repl_running` is still set, so a press that lands after the run ends is
dropped. `repl_running` is atomic.

### 1.3 `py_repl_support.py`

New file in `source/projects/py/`, embedded as `py_repl_support.h` by
`scripts/py2c.py` (add it to `inputs`). `py` executes it once, on first use,
into a module `sys.modules["_py_repl_support"]`, so it never pollutes a `py`
namespace.

`run(code, ns, emit) -> status`:

1. Parse with `ast.parse(code, "<py_repl>", "exec")`. A `SyntaxError` emits
   `traceback.format_exception_only` as `TRACEBACK` and returns `EXCEPTION`.
2. Execute every statement except the last as one `exec` module. Execute the
   last as `"single"` (`ast.Interactive`), so an expression is displayed.
3. While running: `sys.stdout`/`sys.stderr` are writers that emit `OUT`/`ERR`
   on every `write`. `sys.displayhook` emits `repr(value) + "\n"` as `RESULT`
   and sets `builtins._`, skipping `None`. All three are restored in `finally`.
4. `SystemExit` emits `SystemExit: <code>\n` as `ERR` and returns
   `EXCEPTION`. Any other exception emits its traceback, without the frame of
   `run` itself, as `TRACEBACK`, and returns `INTERRUPTED` for
   `KeyboardInterrupt`, else `EXCEPTION`. The traceback shows where an
   interrupt stopped the code.

`is_complete(code) -> state`:

1. Empty or whitespace only: `COMPLETE`.
2. `codeop.compile_command(code, "<py_repl>", "exec")` returns `None`:
   `INCOMPLETE` (open bracket, trailing `:`, backslash, open triple quote).
   `SyntaxError`, `ValueError` or `OverflowError`: `INVALID`.
3. It compiles, the last non-blank line is indented, and the last line is not
   blank: `INCOMPLETE`. A blank line ends a block, as in the CPython REPL.
4. Else `COMPLETE`.

| input | result |
|-|-|
| `x = 1` | COMPLETE |
| `def f():` | INCOMPLETE |
| `def f():\n    return 1` | INCOMPLETE |
| `def f():\n    return 1\n    ` | COMPLETE |
| `a = 1\nb = 2` | COMPLETE |
| `for i in x: print(i)` | COMPLETE |
| `f(1,` | INCOMPLETE |
| `1 +* 2` | INVALID |

The table and the `run` steps were checked with a prototype on CPython 3.15.2.

`names(text, ns) -> list[str]`: the matches of
`rlcompleter.Completer(ns).complete(text, i)` for `i = 0, 1, ...` until `None`,
in order, without duplicates, at most 200. One rlcompleter suffix is removed:
`()` or `(` (callables), `:` or a space (keywords). `rlcompleter` evaluates dotted names, so a property
getter can run.

### 1.4 Tests

- `source/projects/py/tests/test_py_repl_support.py`: every rule in 1.3, the
  `is_complete` table, stream restoration, and that `py_repl_support.h` is
  current. Pytest-style, and runnable without pytest; ctest `py_repl_support`.
- The `A_CANT` methods cannot be called from a patch. Two tests call them:
  - `tests/repl_harness/` (ctest `py_repl_harness`): `py.c`'s repl section,
    between its markers, against embedded CPython with Max functions stubbed.
  - `patchers/tests/test_py_repl/test_repl_abi.maxpat` with
    `examples/tests/test_repl_abi.py`: the same checks in Max, through
    `ctypes` and `object_getmethod`. It also covers the bundled stdlib and the
    interrupt reaching Max's main thread (open question 3).
  Both check `BUSY`, `REFUSED`, an interrupt from another thread, a late
  interrupt dropped, a kept `sys.stdout` writing nowhere, and restored streams.

---

## 2. `py_repl` core (`core/`, headless)

Links `py_code_core` (`Editor`, `Layout`, `Syntax`, `palette`,
`CompletionSession`, `Words`). New modules:

### 2.1 `core/transcript.{h,cpp}`: `pyc::Transcript`

The console's past input and output.

- Kinds: `Input`, `Out`, `Err`, `Result`, `Traceback`, `Info`.
- `append_input(code)`: starts on a new line (adds `\n` if the text does not
  end with one), appends `code` and a `\n`, as one `Input` span.
- `append(kind, text)`: appends verbatim. Joins the previous span when it has
  the same kind and is not `Input`.
- `end_run()`: adds `\n` when the text does not end with one.
- `info(text)`: `end_run()`, then `text` + `\n` as `Info`.
- `text()`, `spans() -> {start, end, kind}`, `revision()`, `clear()`.
- Cap `max_lines`: when the line count exceeds it by 25 %, drop whole lines
  from the front until it is within the cap, trimming the first span.

### 2.2 `core/history.{h,cpp}`: `pyc::History`

- `add(code)`: skips empty input and input equal to the newest entry. Cap
  1000, dropping the oldest.
- Browsing: `prev(draft)` returns the previous entry; the first call stores
  `draft`. `next()` returns the next entry, then the stored draft, then stays.
  `add`, `reset()` or any edit end browsing.
- `entries()` (oldest first) and `load(entries)` for persistence.

### 2.3 `core/submit.{h,cpp}`

`return_action(const Editor& input, int completeness, bool force) -> Submit | Newline`:

- `force` (Cmd+Return): `Submit`.
- The caret is before the last non-whitespace character of the input:
  `Newline`.
- `INCOMPLETE`: `Newline`.
- Else (`COMPLETE`, `INVALID`, or unlinked): `Submit`.

### 2.4 `core/stack.{h,cpp}`: `pyc::Stack`

One vertical scroll over two `Layout`s: the transcript above the input.

- `content_rows(editor, layout)`: rows to draw; none for empty text, and not
  the empty row after a final newline (the transcript always ends with one).
- `update(top_rows, bottom_rows)`; `rows()` is their sum.
- `place(row) -> {part, row}` (clamped), `row_of(part, row)`.
- `scroll()`: a `pyc::Scroll` over `rows()`. `Scroll` is `Layout`'s scroll and
  scroll bar (1.2 of `py_code-spec.md`), moved into `py_code_core` so both use
  one implementation.
- `at_bottom()`: the view shows the last row. When it did before `update`, it
  still does after, so appended output stays in view.

### 2.5 Tests

One `*_test.cpp` per module in `py_repl_tests`, with the `py_code` test
harness. Cover every rule above with tables.

---

## 3. The Max object (`py_repl*.cpp`)

### 3.1 Box

As `py_code` 3.2: `CLASS_BOX`, `jpatcher_syms_init`, same box flags. Default
rect 420 x 220. One inlet, no outlets. The `py_code` facts about keys, mouse,
popup menus and attribute order apply unchanged.

### 3.2 Link

- `@py` (symbol, default `__main__`) names the target.
- `target()`: `object_findregistered(CLASS_BOX, @py)`, accepted only when
  `object_classname` is `py`. Called on every use, never stored.
- On first use after `@py` changes: check `repl_abi == PY_REPL_ABI`, else
  treat as unlinked and post `py '<name>': repl abi <n>, expected <m>` once.
- Unlinked submit: `info("py '<name>' not found")`; the input is kept.

### 3.3 Layout and painting

From top to bottom: title strip, transcript rows, input rows.

- Title strip (as `py_code`): left `py_repl`, plus `history n/m` while
  browsing; right `py: <name>`, plus ` | not found` or ` | abi mismatch` from
  `target()` at paint time.
- Every row has a 4-cell prompt gutter. The first row of each input line shows
  `>>> ` for the first line and `... ` after it, in `@promptcolor`. Prompts are
  drawn, not stored, so copied text has none.
- Transcript colors: `Input` spans by `Syntax` tokens; `Out` and `Result` in
  `text`; `Err` and `Traceback` in `error`; `Info` in `comment`. Colors follow
  the spans within a row, since one line can hold two kinds. Each `Input` span
  is tokenized once by a scratch `Syntax`, cached by its text, so trimming
  (which shifts offsets) keeps the cache.
- Input: `Syntax` tokens, caret, selection, bracket match and completion popup
  as `py_code`.
- One scroll for both parts (`Stack`).

### 3.4 Keys (focused box)

Input editing keys are as `py_code` 3.2, applied to the input `Editor`, except:

| key | action |
|-|-|
| Return | popup open: accept. Else `return_action` with `repl_complete`: submit or newline with `indent_at` |
| Shift+Return | newline |
| Cmd+Return | submit |
| Up | popup open: move. Caret on the first input row: `History::prev`. Else motion |
| Down | popup open: move. Caret on the last input row: `History::next`. Else motion |
| Tab | completion from `repl_names` (word or dotted name before the caret), merged after with `Words` over the transcript and input; else indent. A dotted name shows its last part (`floor`) and replaces the whole name (`math.floor`) |
| Ctrl+C | idle: echo the input, `info("KeyboardInterrupt")`, clear the input, end browsing. Dropped within 300 ms of a run's end: Max delivers the press that stopped the run after it returns |
| PageUp, PageDown | scroll both parts by a page |
| Cmd+K, Ctrl+L | clear the transcript |
| Shift+Tab, Esc, motion, Cmd+A/C/X/V/Z | as `py_code`, within the input |

Cmd+C copies the transcript selection when the transcript has focus. A
printable key while the transcript has focus moves focus to the input end and
types there.

### 3.5 Submit

1. `code` = input text with trailing whitespace removed. Empty: do nothing.
2. `Transcript::append_input(code)`, `History::add(code)`, clear the input.
3. Start the interrupt monitor (3.6).
4. `repl_run(code, sink)`. `sink.emit` appends to the `Transcript`; nothing is
   repainted until the call returns, because the main thread is busy.
5. Stop the monitor. `BUSY`: `info("busy: a run is already active")`.
6. `end_run()`, rebuild the transcript `Editor` from `Transcript::text()`
   (`reset`), keep the bottom visible, repaint.

### 3.6 Interrupt monitor (`platform/keys.{h,cpp}`)

- A thread started before `repl_run` and joined after it (stop flag).
  CoreGraphics only: AppKit state is stale while the main thread is busy.
- Every 20 ms: Ctrl+C down, via `CGEventSourceKeyState` and
  `CGEventSourceFlagsState` with `kCGEventSourceStateCombinedSessionState`.
  Edge-triggered: one `repl_interrupt` per press. A key held at start is not
  a press.
- A press counts only when Max owns the frontmost on-screen window at layer 0,
  from `CGWindowListCopyWindowInfo` (the window server answers while Max's main
  thread is blocked). When that lookup fails, the press counts.
- It calls `repl_interrupt` through the method pointer looked up on the main
  thread before the run.
- `repl_interrupt` is the only call it makes into `py`. The `py` object cannot
  be freed during the run, because freeing happens on the busy main thread.
- Ctrl+C over Cmd+.: Max takes Cmd+., and Ctrl+C is the Python REPL key.

Limits, documented in the help file:

- Interruption happens between bytecodes. `time.sleep` and long C calls (numpy,
  I/O) run to completion first.
- The exception is raised in whatever Python code the main thread is running.
  If user code has triggered another `py` object through an outlet, that
  object's call fails and the REPL code continues.

### 3.7 Messages and attributes

Messages: `run <args>` (submit: one symbol verbatim, else atoms joined with
spaces, as `py`'s `eval`), `clear` (transcript), `history clear`,
`theme <name>`, `themes`.

| name | type | default | notes |
|-|-|-|-|
| py | sym | `__main__` | target `py` name |
| theme | sym | onedark | as `py_code` |
| fontsize | float | 12 | clamped 6..48 |
| tabwidth | long | 4 | clamped 1..16 |
| autoclose, completion, wrap, titlebar | long | 1 | |
| maxlines | long | 10000 | transcript cap, clamped 100..1000000 |
| savehistory | long | 1 | save the last 200 entries with the patch |
| scrollspeed | float | 3 | wheel rows per notch, clamped 0.05..50 |
| 23 colors | float[4] | theme | as `py_code` |
| promptcolor | float[4] | theme's `comment` | `>>>` and `...`; not a palette role, so `py_code` is unchanged. A theme change sets it unless the box dictionary has it |

### 3.8 Persistence and lifetime

- `jsave`: `py_repl_history` (when `@savehistory`) and `py_repl_input` (the
  draft). The transcript is not saved. `jload` at the end of `new`.
- `free`: stop clocks; no monitor can be running (it only lives inside a
  submit).

### 3.9 Reused files

- `py_code_core` (linked).
- `platform/clipboard.{h,mm}` (compiled into `py_repl`).
- `py_code`'s paint and input code is not reused: it is written against
  `t_py_code`. `py_repl` mirrors its structure (`py_repl.cpp`,
  `py_repl_paint.cpp`, `py_repl_input.cpp`).

---

## 4. Build

`source/projects/py_repl/CMakeLists.txt`: a static `py_repl_core` library
(`core/*.cpp`, linking `py_code_core`), the MODULE linking AppKit and
ApplicationServices and exporting only `_ext_main`, and `py_repl_tests`
registered with ctest. `py` gains `py_repl_abi.h`, `py_repl_support.py`,
`py_repl_support.h` (generated) and the ctest `py_repl_support`.

---

## 5. Open questions (verify in Max before or while building)

1. Answered (`probe_keys.maxpat`): `CGEventSourceKeyState` sees Ctrl+C
   pressed in Max without Input Monitoring permission, while Max's main thread
   is busy.
2. Answered: a press in another app is not seen at all, so it cannot
   interrupt a run. The frontmost-window check (3.6) stays as a backstop for a
   Max that has Input Monitoring permission, where other apps' keys may be
   visible.
3. Answered: `test_repl_abi.maxpat` interrupts a run on Max's main thread.
4. Answered: a message box splits `run` arguments at commas, as for `eval`.
   `help/py_repl.maxhelp` shows the quoted form, `run "max(1, 2)"`.

## 6. Out of scope

- A worker thread, live output during a run, and an execution timeout.
- LSP features (diagnostics, signature help) in the input.
- Saving the transcript; rich output (images, tables).
- Releasing the GIL around other `api` calls into Max objects.
