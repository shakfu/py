# Changelog

All notable changes to the py_code external are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - Unreleased

### Added

- Added `py_code`, an in-patch Python editor for `py`, derived from `sc_code` in gus74vv/sc3-tilde (GPL-3.0-only, see `LICENSE.md`). Shift-Return evaluates the line, Cmd-Return the top-level statement under the caret, Cmd-Shift-Return the document; a selection always wins. A single expression is sent as `eval <code>` so `py` outputs its value; anything else as `exec <code>`. Indented code is dedented first.

- Added tree-sitter-python parsing (`py_syntax`) for highlighting, bracket matching, indentation and the evaluation span. Brackets inside strings and comments never match. Each edit is recovered from the common prefix and suffix of the old and new text, so the buffer keeps no edit log.

- Added Python auto-close rules: a quote after a word does not pair (`don't`) unless the word is a string prefix (`f"`, `rb'`), and a third quote closes as a triple. These replace the auto-close functions `sc_code` had ported from the SuperCollider IDE.

- Dropped from `sc_code`: the call-signature bar and class index, which need the running interpreter in Python, and the panic key.
