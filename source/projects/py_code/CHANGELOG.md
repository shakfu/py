# Changelog

All notable changes to the py_code external are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.2.0] - Unreleased

### Added

- Added `core/scroll.{h,cpp}`: `Layout`'s scroll and scroll bar as `pyc::Scroll`, which `Layout` forwards to. `py_repl` scrolls two `Layout`s with one `Scroll`, so the scroll math exists once.

- Added a Language Server Protocol client. `@lsp` names the server command (default `auto`: the first of `pylsp`, `basedpyright-langserver`, `pyright-langserver`, `jedi-language-server`, `ty` found on the login-shell `PATH`); empty turns it off. Boxes with the same command and root share one server process. The server starts through the login shell because Max launched from the Dock has a minimal `PATH`.

- Added a completion popup: Tab after a name or `.`, or typing `.` while the server is ready, lists candidates from the server, else from the document's names and Python keywords and builtins. Up/Down select, Tab or Return accept, typing filters, Esc closes.

- The server analyzes against the `py` external's Python: `py_code` finds `py.mxo` (from the loaded `py` class, else beside `py_code.mxo`) and passes its `api` stub, plus the bundled interpreter (framework builds) or bundled `site-packages` (shared and static builds), as server settings. Without this, `import api` has no completions and bundled packages are invisible to the server. Read by pylsp, jedi-language-server, pyright and basedpyright; ty ignores them.

- Added signature help and diagnostics: `@statusbar` shows the signature of the call at the caret with the current parameter highlighted, else the diagnostic on the caret line. Diagnostics are underlined in `@errorcolor` or `@warningcolor`.

### Changed

- Rewrote the editor as MIT code from a behavioural spec (`docs/dev/py_code-spec.md`), replacing the 0.1.0 sources derived from `sc_code` (GPL-3.0-only). Undo is an edit log grouped by word; Tab and Shift+Tab indent or outdent every selected line; Up/Down move by visual row.

- `@completion` now enables the popup; the reserved bottom line is `@statusbar`.

## [0.1.0]

### Added

- Added `py_code`, an in-patch Python editor for `py`. Shift-Return evaluates the line, Cmd-Return the top-level statement under the caret, Cmd-Shift-Return the document; a selection always wins. A single expression is sent as `eval <code>` so `py` outputs its value; anything else as `exec <code>`. Indented code is dedented first.

- Added tree-sitter-python parsing (`py_syntax`) for highlighting, bracket matching, indentation and the evaluation span. Brackets inside strings and comments never match.
