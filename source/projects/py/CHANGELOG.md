# Changelog

All notable changes to the py external will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Fixed

#### Critical Bugs
- **Atomic operations bug in object lifecycle**: Fixed inverted atomic increment/decrement operations in `py_init()` and `py_free()`. The counter was using `atomic_fetch_sub` when it should increment and `atomic_fetch_add` when it should decrement, causing the Python interpreter initialization and finalization logic to be completely broken. ([py.c:466-470, 521-525])

- **Memory leak in pythonpath management**: Fixed memory leak in `py_pythonpath_add()` where `PyUnicode_FromString()` created a new reference that was never decremented after `PyList_Append()`, causing one leaked Python string object per call. Added `Py_DECREF(py_path)` to properly release the reference. ([py.c:645])

#### Thread Safety
- **Unsafe mutex fallback paths**: Fixed race conditions in `py_get_global_registry()` and `py_get_object_ref()` where NULL mutex checks would fall back to unsynchronized access to global state. Now returns NULL/0 with clear error messages instead of potentially unsafe access during initialization/shutdown. ([py.c:841-854, 864-878])

### Added

#### Configuration
- **Security configuration presets**: Added three security presets (STRICT, BALANCED, PERMISSIVE) to simplify configuration complexity. Users can now choose a single preset instead of managing 11+ individual configuration options. ([py_config.h:23-40, 391-462])
  - **STRICT**: Maximum security for untrusted code (restricted imports, no file access, 3s timeout)
  - **BALANCED**: Moderate security for normal use (DEFAULT) (validation enabled, 5s timeout)
  - **PERMISSIVE**: Minimal restrictions for trusted environments (no validation, no timeout)
  - **CUSTOM**: Advanced users can define their own settings

- **Compile-time preset validation**: Added validation to ensure exactly one security preset is selected at compile time, preventing configuration errors. ([py_config.h:458-462])

- **Build-time preset reporting**: Added `#pragma message` directives to display which security preset is active during compilation for clarity. ([py_config.h:405, 420, 435, 439])

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
