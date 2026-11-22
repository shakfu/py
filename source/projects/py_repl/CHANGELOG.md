# Changelog

All notable changes to the py_repl external are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - Unreleased

### Added

- Added the `py_repl` box: a Python console that runs code in the `py` object named by `@py` (default `__main__`), through the `A_CANT` methods in `py_repl_abi.h`. Return submits complete input and adds a line otherwise; Up/Down browse history; Tab completes from the namespace at runtime; Ctrl+C stops a run. Code runs on the main thread, so `api` calls are safe, and the patch UI waits until a run ends while audio and Overdrive's scheduler keep running; a thread polls Ctrl+C with CoreGraphics and stops the run with `repl_interrupt`. History and the unsent input are saved with the patch, the transcript is not. `@promptcolor` sets the color of `>>>` and `...`; it follows the theme's comment color unless set. Help: `help/py_repl.maxhelp`; reference: `docs/py_repl.maxref.xml`. Test: `patchers/tests/test_py_repl/test_py_repl.maxpat`.

- Added the headless core: `Transcript` (input and output as kind spans, capped by whole lines), `History` (Up/Down browsing that keeps the draft), `return_action` (Return submits or adds a line) and `Stack` (one scroll over the transcript and input layouts). Tested by `py_repl_tests` under `make test`. Spec: `docs/dev/py_repl-spec.md`, section 2.
