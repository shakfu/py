# Changelog

All notable changes to the py external will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Added `make static-ext`, `shared-ext`, `framework-ext` and `framework-pkg`, which build Python with `buildpy.py` and then a self-contained `py.mxo` against it, and `make local` for the local build. Each deletes the `.mxo` first, since another variant's Python would stay in the bundle, and reconfigures with `--fresh`, because the Python found earlier stays cached otherwise. See `README.md`.

- Added `make strip-support`, which moves the C headers out of `support/Python.framework` before a `framework-pkg` package is distributed (13.8 MB to 11 MB). `make framework-pkg` moves them back, because CMake reads them from there.

- Added `api.pyi`, a type stub for the built-in `api` module, so language servers can complete it; `api` is compiled into the external and has no source they can read. The build copies it to `py.mxo/Contents/Resources/stubs/`. `scripts/gen_api_stub.py` (`make stub`) generates it from `api.pyx` with Cython's parser, over a hand-written stub that would drift from 899 functions; the `py_api_stub` test fails when it is stale.

- Added `py_prelude_legacy.py`, which uses `eval` so inline expressions such as `pipe 10+1 f` work. It is now the default prelude. Building with `-DRESTRICTED_PYTHON=ON` selects `py_prelude.py`, which resolves names without `eval`. Both over a single prelude because expressions need `eval` and name lookup alone cannot provide them. `RESTRICTED_PYTHON` is not a security boundary: `eval` and `shell` stay callable.

- Added Python 3.15 to `buildpy.py`. On 3.15 the hand-compiled `pyexpat` drops `-DXML_DEV_URANDOM`, which fails on the new `random_dev_urandom.h` header. Static builds still compile expat into `libpython`, because the externals do not link `libexpat.a`. `_math_integer` is added so `math.integer` works.

### Changed

- Changed every `buildpy.py` `*_mid` configuration to one module set: `_ssl`, `_hashlib`, `_ctypes`, `_bz2`, `_lzma`, `_sqlite3`, `_scproxy`, `zlib` and `binascii` are built into Python, and `_decimal` is left out. Before, `shared-ext` and the framework variants had no `ssl` or `hashlib`, `static-ext` and `shared-ext` had no `ctypes`, and the framework variants loaded seven of these from `lib-dynload`. Built in, one OpenSSL copy serves both modules. `framework-ext` also leaves the C headers out of the bundle: 14.0 MB, down from 17.0 MB. Built-in `_ctypes` uses configure's flags for the SDK's libffi, written as `-DNAME`, because `makesetup` reads any `Setup` line containing `=` as a Make variable.

- Changed success bangs: each message now bangs once. Calls previously banged up to three times, and also banged success after failed output. `sched` also accepts a delay of 0.

- Changed `api` calls to act on the calling `py` object by its module name, over the newest instance. Code in a separately imported module still resolves to the newest instance.

- Changed `Matrix` buffer format to `B` for char and `i` for long: `l` is 8 bytes on 64-bit platforms and cells are 4. Multi-plane matrices now export planes as a final dimension.

- Raised the `eval` input limit from 1024 characters to `PY_MAX_CODE_LENGTH` (65536), the `exec` limit, so long expressions from `py_code` are accepted.

### Fixed

- Fixed `@name` being applied after the object was registered, its module created and its scripting name set, so `[py @name foo]` stayed registered as `__main__` or `u...` and could not be found as `foo`. `name` can no longer change after creation: a rename would have to move the module while code and `api` lookups still use the old name. A name taken by another object, or by a module such as `json`, is refused, because the object would otherwise run in that module's namespace. A freed `py` removes its module from `sys.modules`, so a recreated `[py @name foo]` can take the name again.

- Fixed `libpython3.13.dylib` and `Python.framework` lacking the libraries of their built-in modules: CPython links neither with them, so built-in `_ssl` failed with `symbol not found '_X509_OBJECT_free'`, and `_bz2`, `_lzma` and `_scproxy` of `shared-ext` resolved only if the host had loaded those libraries. `buildpy.py` now passes OpenSSL, bzip2 and xz in `LIBS` with `-hidden-l`, plus libffi and SystemConfiguration. Hidden, OpenSSL cannot clash with another copy in Max; `static-ext` links it the same way, so `py.mxo` exports 1937 symbols, down from 7503.

- Fixed `py.mxo` built against a `buildpy.py` Python targeting macOS 11.0 while `libpython` targeted 12.6, so it needed 12.6 regardless. The deployment target is now read from the bundled Python's `sysconfig`, over a second hard-coded value that can drift, and the `buildpy.py` default is 11.0.

- Fixed `buildpy.py` never rebuilding a missing OpenSSL, bzip2 or xz: the check tested that each library list was non-empty, not that the files exist. The externals now link those libraries by path or with `LINK_DEPENDS`, so CMake relinks when they are rebuilt.

- Fixed reconfiguring a `shared-ext`, `static-ext` or `framework-ext` build invalidating the signature of `py.mxo`. The bundled `lib/` or `Python.framework` was copied at configure time, but trimming it and code signing only run on a relink, so for example `libpython3.13.a` stayed in the signed bundle. Both are now copied after the build, the framework with `rsync -a`, which keeps its symlinks. `framework-pkg` now also signs `support/Python.framework`, which lies outside the bundle the SDK signs.

- Fixed `buildpy.py` building Python against host libraries found by `pkg-config`, such as Homebrew's. A `static-ext` build compiled `_blake2` against Homebrew `libb2`, so linking `py` failed with undefined `blake2b_*` symbols; `xz` and OpenSSL were also checked against Homebrew headers, not the bundled versions. `configure` now sees only the `pkg-config` files of the dependencies `buildpy.py` builds.

- Fixed `shared-ext` and `static-ext` builds not setting the Python home to `py.mxo/Contents/Resources`. `py.c` tests `PY_STATIC_EXT`, which the py-js Xcode targets defined but the CMake build never did. Removed the `PY_SHARED_PKG` home path, whose py-js `shared-pkg` variant has no CMake equivalent.

- Fixed `eval` rejecting valid expressions that contain `=`, `import `, `from `, `def ` or `class ` as text, such as `f(a=1)`, `x == 1` or `'from here'`. The check matched substrings regardless of `security_mode`. It was removed because `eval` already compiles with `Py_eval_input`, which rejects every statement with a `SyntaxError`.

- Fixed dotted names in `py_prelude.py`: `call math.sqrt 16` returned `None` and `fold operator.add 1 2 3` returned `[]`, because the name stayed a string and no function was found. Private attributes are rejected.

- Fixed memory-safety defects in `py.c`: use-after-free when logging every Python error, stack overflows from messages of 1024+ atoms and from sequences whose `__len__` understates `__iter__`, a double decref when `product` fails, and unterminated editor text from `read`.

- Fixed memory-safety defects in `api.pyx`: format-string use of `post`, `error` and the logging methods, `strcpy` overflow on non-ASCII text, unchecked `Atom` indexes, heap corruption in `Path.sysfile_write`, `Matrix.set_long_data` overrun, and double frees of objects stored in `Dictionary`, `Hashtab` and `List`. A failed `Buffer` lock now raises and is no longer unlocked.

#### Critical Bugs
- **Multichannel buffer support (GitHub issue #19)**: Fixed `np.asarray(buf)` returning incorrect number of samples for multichannel buffers. The Buffer class now properly handles n-channel buffers:
  - `__getbuffer__`: Returns 2D shape `(frames, channels)` for multichannel, 1D `(frames,)` for mono
  - `n_samples` property: Now returns `framecount * channelcount` (total samples)
  - `n_frames` property: Added as alias for `framecount`
  - `set_samples()`: Now correctly divides sample count by channel count; validates divisibility
  - `get_samples()`: Now returns all samples across all channels
  ([api.pyx:1167-1240, 1411-1418, 1490-1528])

- **Atomic operations bug in object lifecycle**: Fixed inverted atomic increment/decrement operations in `py_init()` and `py_free()`. The counter was using `atomic_fetch_sub` when it should increment and `atomic_fetch_add` when it should decrement, causing the Python interpreter initialization and finalization logic to be completely broken. ([py.c:466-470, 521-525])

- **Memory leak in pythonpath management**: Fixed memory leak in `py_pythonpath_add()` where `PyUnicode_FromString()` created a new reference that was never decremented after `PyList_Append()`, causing one leaked Python string object per call. Added `Py_DECREF(py_path)` to properly release the reference. ([py.c:645])

#### Thread Safety
- **Unsafe mutex fallback paths**: Fixed race conditions in `py_get_global_registry()` and `py_get_object_ref()` where NULL mutex checks would fall back to unsynchronized access to global state. Now returns NULL/0 with clear error messages instead of potentially unsafe access during initialization/shutdown. ([py.c:841-854, 864-878])

#### Test Fixes
- **test_api_buffer_scipy.py**: Fixed assertion `len(xs) == buf.n_samples` to use `xs.size` for compatibility with multichannel buffers
- **test_api_buffer_array.py**: Added documentation for mono buffer assumptions in memoryview tests
- **test_api_matrix.py, test_api_matrix_np.py**: Fixed `os.exists()` to `os.path.exists()`
- **test_api_matrix_np.py**: Renamed duplicate function `test_matrix_set_cell2d` to `test_matrix_set_cell2d_simple`
- **test_api_array.py**: Added `pass` to empty test body; fixed `clear(20)` to `reserve(20)`
- **test_api_table.py**: Fixed unreachable code after `return` statement

### Added

#### Tests
- **test_api_buffer_multichannel.py**: Comprehensive test suite for multichannel buffer support (20 tests) covering:
  - `n_samples`/`n_frames` properties for mono and multichannel (1-8 channels)
  - `np.asarray()` shape verification for 1D mono and 2D multichannel
  - `get_samples()` and `set_samples()` with interleaved data
  - `memoryview` buffer protocol tests
  - Round-trip data integrity verification
  - GitHub issue #19 scenario reproduction

#### Configuration
- **Security configuration presets**: Added three security presets (STRICT, BALANCED, PERMISSIVE) to simplify configuration complexity. Users can now choose a single preset instead of managing 11+ individual configuration options. ([py_config.h:23-40, 391-462])
  - **STRICT**: Maximum security for untrusted code (restricted imports, no file access, 3s timeout)
  - **BALANCED**: Moderate security for normal use (DEFAULT) (validation enabled, 5s timeout)
  - **PERMISSIVE**: Minimal restrictions for trusted environments (no validation, no timeout)
  - **CUSTOM**: Advanced users can define their own settings

- **Compile-time preset validation**: Added validation to ensure exactly one security preset is selected at compile time, preventing configuration errors. ([py_config.h:458-462])

- **Build-time preset reporting**: Added `#pragma message` directives to display which security preset is active during compilation for clarity. ([py_config.h:405, 420, 435, 439])

#### Build System
- **Python 3.14 support**: Added `PythonConfig314` class to `buildpy.py` for building with Python 3.14. Changes from 3.13: ([buildpy.py:715-763, 1455])
  - Removed `_contextvars` from static modules (now a builtin)
  - Simplified `_blake2` extension (single `blake2module.c`)
  - Moved `_blake2` from static to shared modules
  - Added new `_hmac` module to shared
  - Disabled `_zstd` and `_remote_debugging` modules
  - Inherits `_interpchannels`, `_interpqueues`, `_interpreters` in static from 3.13

### Documentation
- **CODE_REVIEW.md**: Comprehensive code review document identifying critical issues, high-priority concerns, and areas for improvement across the entire codebase (~15K LOC).

- **FIXES_APPLIED.md**: Detailed documentation of all applied fixes with before/after code examples, impact analysis, and testing recommendations.

## Impact Summary

These fixes address:
1. **Correctness**: Python interpreter lifecycle now functions properly
2. **Memory safety**: Eliminated memory leak in common operation
3. **Thread safety**: Removed race conditions in global state access
4. **Usability**: Simplified security configuration with clear presets

All changes are minimal, surgical fixes that address real bugs without introducing new risks or contradicting the external's design.

## Testing Recommendations

- Test Python interpreter initialization/finalization with multiple object creation/deletion cycles
- Run memory leak detection tools (Valgrind/AddressSanitizer) on pythonpath operations
- Test thread safety during shutdown scenarios
- Verify each security preset builds correctly and behaves as expected
