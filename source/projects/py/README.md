# py: Python3 Max/MSP External

A powerful Python integration for Max/MSP that embeds a Python3 interpreter directly into Max, enabling seamless two-way communication between Max and Python.

## Table of Contents

- [Quick Start](#quick-start)
- [What is py?](#what-is-py)
- [Key Features](#key-features)
- [Installation](#installation)
  - [Pre-built Externals](#pre-built-externals)
  - [Building from Source](#building-from-source)
- [Basic Usage](#basic-usage)
- [Function Caching and Memoization](#function-caching-and-memoization)
- [Architecture](#architecture)
- [Advanced Features](#advanced-features)
  - [API Module](#api-module)
  - [Scripting Max with Python](#scripting-max-with-python)
  - [Editor Integration](#editor-integration)
- [Build Variants](#build-variants)
- [Deployment](#deployment)
- [Packaging and Distribution](#packaging-and-distribution)
- [Troubleshooting](#troubleshooting)
- [Development](#development)

## Quick Start

### Using Pre-built Externals

1. Download the latest release from [releases](https://github.com/shakfu/py-js/releases)
2. Copy `py.mxo` to your Max externals folder
3. Create a new Max patch and add a `py` object
4. Try this simple example:

```text
[py]
|
[print]
```

Send these messages to the `py` object:

- `eval 2 + 2` → outputs `4`
- `exec x = [1, 2, 3, 4]` → creates a list variable
- `eval len(x)` → outputs `4`
- `call sum x` → outputs `10`

### Building from Source

```bash
git clone --recursive https://github.com/shakfu/py-js.git
cd py-js
make setup  # Initialize and create Max package symlinks
make        # Build py.mxo and pyjs.mxo linked to system Python
```

## What is py?

The `py` external embeds a Python3 interpreter into Max/MSP, providing:

- **Full Python Environment**: Access to Python's standard library and third-party packages
- **Bidirectional Communication**: Send data between Max and Python seamlessly
- **Max API Access**: Script Max objects directly from Python using the built-in `api` module
- **Multiple Deployment Options**: From development builds to standalone applications

## Key Features

- **Per-object Python namespaces** - Each `py` object has its own isolated Python environment
- **Standard Python operations** - `eval`, `exec`, `import` with full Python syntax support
- **Max-friendly syntax** - `call`, `pipe`, `fold` methods for easier Max integration
- **Function caching** - Compile and cache Python functions for improved performance
- **Automatic memoization** - Recursive functions automatically optimized with LRU caching
- **Built-in editor support** - Code editor, REPL, and external editor integration
- **Inter-object communication** - Send messages between Max objects from Python
- **Portable deployment** - Self-contained externals for packages and standalones

## Installation

### Pre-built Externals

Download codesigned and notarized externals from [releases](https://github.com/shakfu/py-js/releases). These work immediately without compilation.

### Building from Source

**Requirements:**

- macOS: Xcode, Python 3.8-3.13, Cython (`pip install cython`)
- Windows: Visual Studio, Python 3.8-3.13 from python.org

**Quick Build (linked to system Python):**

```bash
git clone --recursive https://github.com/shakfu/py-js.git
cd py-js
make setup  # Initialize submodules and create symlinks
make        # Build externals
```

## Basic Usage

### Core Methods

| Method | Description | Example |
|--------|-------------|---------|
| `eval <expr>` | Evaluate Python expression | `eval 2 + 2` |
| `exec <stmt>` | Execute Python statement | `exec x = [1,2,3]` |
| `execfile <path>` | Execute Python file | `execfile script.py` |
| `import <module>` | Import Python module | `import math` |
| `call <func> <args>` | Call Python function | `call len x` |
| `pipe <data> <funcs>` | Pipe data through functions | `pipe 5 abs str len` |
| `cache <code>` | Cache Python function | `cache def fib(n): ...` |
| `cachefile <path>` | Cache function from file | `cachefile fib.py` |
| `clear_cache` | Clear all cached functions | `clear_cache` |
| `set_function <name>` | Set active function for int/float | `set_function square` |
| `int <value>` | Call active cached function | `int 10` |

### Simple Examples

**Basic arithmetic:**

```text
[eval 10 * 3.14159( → [30.14159]
```

**Working with lists:**

```text
[exec numbers = list(range(10))(
[call sum numbers( → [45]
```

**Using Python libraries:**

```text
[import random(
[eval random.randint(1, 10)( → [7]
```

## Function Caching and Memoization

The `py` external includes a function caching system that compiles and stores Python functions for efficient repeated execution. Functions cached using the `cache` handler are automatically validated, compiled to bytecode, and optimized.

### Basic Caching

Cache a function for repeated calls:

```text
[cache def square(x): return x * x]
[int 5] → [25]
[int 10] → [100]
```

### Calling Cached Functions

Cached functions can be called using three methods:

**Via numeric handlers:**
```text
[int 42]        # Calls active cached function with integer argument
[float 3.14]    # Calls active cached function with float argument
```

The numeric handlers call the "active" function, which is:
- The most recently cached function (via `cache` or `cachefile`)
- Or a function explicitly set via `set_function`

**Via call handler:**
```text
[call square 7]         # Calls specific cached function by name
[call fib 20]           # Works with any cached function
```

**Multiple arguments:**
```text
[cache def add(a, b): return a + b]
[call add 10 20] → [30]
```

**Switching between cached functions:**
```text
[cachefile algorithms.py]    # Caches multiple functions
[set_function square]        # Set square as active function
[int 5] → [25]              # Calls square(5)
[set_function cube]          # Switch to cube
[int 5] → [125]             # Calls cube(5)
```

### Automatic Memoization

All cached functions are automatically optimized with LRU (Least Recently Used) caching. This prevents redundant computation, particularly beneficial for recursive algorithms:

```text
[cache def fib(n):
    if n <= 1:
        return n
    return fib(n-1) + fib(n-2)]

[int 10] → [55]      # First call computes recursively
[int 50] → [12586269025]  # Returns instantly via memoization
```

Without memoization, `fib(50)` would require millions of recursive calls. With memoization, it completes in under 1ms after the first computation.

### Memoization Details

- **Cache size**: 128 entries per function (LRU eviction policy)
- **Overhead**: Minimal one-time cost during compilation
- **Scope**: Applies to all cached functions automatically
- **Multi-parameter support**: Functions with multiple arguments are memoized correctly
- **Memory**: Approximately 10KB per cached function

### Nested Functions

The caching system supports functions with nested helper functions:

```text
[cache def fibonacci_optimized(n):
    from functools import lru_cache
    @lru_cache(maxsize=None)
    def helper(n):
        if n <= 1: return n
        return helper(n-1) + helper(n-2)
    return helper(n)]
```

Only top-level function definitions are counted for validation. Nested functions are permitted.

### Cache File Loading

Load and cache all functions from external Python files:

```text
[cachefile algorithms.py]
[call fibonacci 100]
[call factorial 20]
```

The file can contain multiple top-level function definitions. All functions will be cached and available for calling by name.

### Clearing the Cache

Remove all cached functions and reset statistics:

```text
[clear_cache]
```

This clears all cached functions while preserving configuration settings. The cache is automatically reinitialized after clearing. Useful for:
- Reloading modified function definitions
- Freeing memory from unused cached functions
- Resetting statistics during development

**Workflow example:**

```text
# Cache functions from file
[cachefile my_functions.py]
[call process_data 100] → [result]

# Edit my_functions.py externally...

# Reload the modified file
[clear_cache]
[cachefile my_functions.py]
[call process_data 100] → [new result]
```

### Limitations

- Extremely deep recursion may still hit Python's stack limit (typically around 1000 frames)
- Functions with side effects will be memoized based on arguments only
- Cache does not persist between Max sessions

## Architecture

The `py` external consists of three integrated components:

### 1. C External (`py.c`)

- Embeds Python3 interpreter using Python C-API
- Handles Max/MSP object lifecycle and message routing
- Provides security framework and memory management
- **Size**: ~1,600 lines of handwritten C code

### 2. Python Prelude (`py_prelude.py`)

- Pure Python utility functions available in every `py` instance
- Provides Max-friendly function calling syntax
- Includes functional programming utilities (`pipe`, `fold`, `compose`)
- **Auto-generated**: Compiled into `py_prelude.h` during build

### 3. API Module (`api.pyx`)

- Cython wrapper for Max C-API functionality
- Enables Python scripts to control Max objects directly
- Provides classes for `Buffer`, `Table`, `Patcher`, `Matrix`, etc.
- **Size**: ~3,700 lines of Cython code → ~148,000 lines of generated C

## Advanced Features

### API Module

The built-in `api` module provides Python access to Max C-API functionality:

```python
import api
import numpy as np
from scipy import signal

# Work with Max buffer~ objects
def process_buffer(name: str, sample_file: str) -> np.array:
    buf = api.create_buffer(name, sample_file)
    xs = np.array(buf.get_samples())

    # Apply signal processing
    t = np.linspace(0, 1, buf.n_samples, endpoint=False)
    processed = signal.sawtooth(2 * np.pi * 5 * t)
    buf.set_samples(processed)

    api.post(f"Processed {buf.n_samples} samples in buffer {name}")
    return processed
```

**Available API Classes:**

- `Buffer` - Max buffer~ objects
- `Table` - Max table objects
- `Matrix` - Jitter matrix objects with NumPy integration
- `Patcher` - Max patcher scripting
- `Dictionary` - Max dictionary objects
- `Database` - Max database functionality

### Scripting Max with Python

**Inter-object Communication:**

```python
# Scan patcher for named objects
api.send("scan")

# Send messages to Max objects
api.send("mydac~", "start")
api.send("myslider", "set", 127)
```

**Scheduler Integration:**

```text
[sched 1000 my_function arg1 arg2]  # Call function after 1 second
```

### Editor Integration

**Built-in Code Editor:**

- Double-click `py` object to open editor
- `read <file>` - Load file into editor
- `load <file>` - Load and execute file
- `run` - Execute current editor content

**REPL Options:**

- `py_repl.maxpat` - Basic single-line REPL
- `py_repl_plus.maxpat` - REPL with embedded `py` object
- `py_multiedit.maxpat` - Multi-line editor with REPL

**External Editor Support:**

- `py_extedit.maxpat` - File watcher for external editors
- Remote console via UDP (experimental)

## Build Variants

Choose the appropriate build variant based on your deployment needs:

### Development Builds (Linked to System Python)

**Advantages:** Access to all installed Python packages, smallest external size
**Disadvantages:** Not portable, requires Python installation on target machine

```bash
make              # Basic build linked to system Python
make projects     # Build all externals using cmake
```

### Self-contained Builds (Portable)

**For Packages and Standalones:**

| Build Command | Type | Size (MB) | Description |
|---------------|------|-----------|-------------|
| `make static-ext` | Static | 14.4 | Python statically linked, most portable |
| `make shared-ext` | Shared | 19.1 | Python as shared library, full features |
| `make framework-pkg` | Framework | 21.2 | Python framework in package support folder |

**Tiny Variants (Reduced Features):**

```bash
make static-tiny-ext    # 10.2 MB - Minimal Python, static linking
make shared-tiny-ext    # 10.6 MB - Minimal Python, shared library
```

### Platform-Specific Instructions

#### macOS

```bash
git clone --recursive https://github.com/shakfu/py-js.git
cd py-js
make setup                # Initialize submodules and symlinks
pip install cython       # Required for API module compilation
make static-ext           # Build portable external
```

#### Windows

```bash
git clone --recursive https://github.com/shakfu/py-js
cd py-js
# Build basic external
mkdir build && cd build
cmake .. -DBUILD_TARGETS=py
cmake --build . --config Release

# Build portable external
python source\scripts\buildpy.py -t windows-pkg
cmake .. -DBUILD_TARGETS=py -DBUILD_VARIANT=windows-pkg
cmake --build . --config Release
```

## Deployment

### Deployment Options

| Scenario | Python Location | Portability | Use Case |
|----------|----------------|-------------|----------|
| **System Linked** | System installation | No | Development, existing Python setup |
| **Package Embedded** | Max package `support/` folder | Yes | Distributable Max packages |
| **External Embedded** | Inside `.mxo` bundle | Yes | Standalone applications |

### Using in Max Standalones

1. **Build or download** self-contained externals
2. **Include externals** in your standalone build
3. **Test functionality** before distribution

**For `py.mxo`:**

- Max automatically includes the external during standalone build
- Test with included example patches: `py_test_standalone_info_py.maxpat`

**For `pyjs.mxo`:**

- May require manual copying to standalone bundle
- Use provided script: `source/projects/py/scripts/fix-pyjs-standalone.sh`

## Packaging and Distribution

### Code Signing and Notarization

For distribution on macOS, externals must be signed and notarized:

```bash
# Sign all externals
make sign

# Create distributable DMG
make dmg

# Complete release process (sign, notarize, package)
make release
```

**Requirements for Notarization:**

- Apple Developer Account ($100/year)
- App-specific password
- Developer ID certificate

**Setup credentials:**

```bash
xcrun notarytool store-credentials "keychain-profile" \
  --apple-id "your-apple-id" \
  --team-id "team-id" \
  --password "app-specific-password"

export DEV_ID="Your Name"
export KEYCHAIN_PROFILE="keychain-profile"
```

### Build Testing

Test all build variants on your system:

```bash
make test                 # Test all variants
make test-shared-pkg      # Test specific variant
```

## Troubleshooting

### Common Issues

**Python Module Reload Problems:**

- **Symptom:** `SystemError` when reimporting NumPy or other C extensions
- **Cause:** Known Python bug with C extension unloading
- **Solution:** Restart Max between patches that use the same C extensions

**Standalone Build Issues:**

- **Symptom:** `pyjs.mxo` missing from standalone bundle
- **Solution:** Use provided script `scripts/fix-pyjs-standalone.sh`

**iCloud Sync Problems:**

- **Symptom:** Codesigning failures during development
- **Cause:** iCloud creates hidden files that interfere with signing
- **Solution:** Move project outside iCloud-synced directories, run `xattr -cr .`

**Build Failures:**

- **Symptom:** CMake or compilation errors
- **Solution:** Ensure all dependencies installed, check Python version compatibility (3.8-3.13)

### Compatibility Notes

**Python Versions:** 3.8.20, 3.9.22, 3.10.17, 3.11.12, 3.12.10, 3.13.3 tested
**Max Versions:** Max 8 and Max 9 supported
**Platforms:** macOS (mature), Windows (experimental)

### Feature Stability

- **Core features** (`eval`, `exec`, `import`) - Stable
- **Extra features** (`call`, `pipe`, `fold`) - Mostly stable
- **API module** - Experimental, requires Max restart between patches

### Getting Help

- **Examples:** Check `examples/tests/` and `patchers/` directories
- **Issues:** Report bugs at [GitHub Issues](https://github.com/shakfu/py-js/issues)
- **Documentation:** Additional help files in `.maxhelp` patches

## Development

### Build System Architecture

The project uses a multi-layered build system:

1. **Makefile** - High-level interface for common tasks
2. **CMake** - Standard Max SDK build process for development
3. **Builder** - Custom Python build system for complex packaging

### Advanced Build Options

**Custom Python Builder:**

```bash
cd source/projects/py
python3 -m builder --help  # See all options
```

**Specify Python Version:**

```bash
make shared-ext PYTHON_VERSION=3.11.12
```

**Test All Build Variants:**

```bash
make test                    # Test all variants
make test-static-ext         # Test specific variant
```

### Development Workflow

1. **Setup development environment:**

   ```bash
   git clone --recursive https://github.com/shakfu/py-js.git
   cd py-js
   make setup
   pip install cython
   ```

2. **Iterative development:**

   ```bash
   make                     # Initial build
   # Make code changes
   cd build && cmake --build .  # Incremental build
   ```

3. **Test changes:**

   ```bash
   # Open Max patches in patchers/ directory
   # Test with example patches in examples/tests/
   ```

### Code Style

Automatic formatting with clang-format:

```bash
brew install clang-format
# Format applied automatically during build
```

### Project Structure

```text
py-js/
├── source/projects/py/       # Main external source
│   ├── py.c                  # Core C implementation
│   ├── py_prelude.py         # Python utilities
│   ├── api.pyx               # Cython Max API wrapper
│   └── builder/              # Custom build system
├── examples/                 # Example patches and scripts
├── patchers/                # Max patches and helpers
└── externals/               # Built external outputs
```

### Contributing

1. **Focus Areas:**
   - Windows platform support
   - API module stability improvements
   - Documentation and examples
   - Performance optimizations

2. **Testing:**
   - Test with multiple Python versions (3.8-3.13)
   - Verify both development and portable builds
   - Test standalone deployment scenarios

3. **Known Limitations:**
   - API module requires Max restart between sessions
   - NumPy reload issues in same Max session
   - Windows builds still experimental

### Architecture Details

**Message Flow:**

```text
Max Message → py.c → Python C-API → Python Code → Results → Max
```

**Python Integration:**

- Each `py` object has isolated namespace
- Shared global registry for inter-object communication
- Security framework prevents dangerous operations
- Memory management handles Python/Max boundary

**API Module Integration:**

- Cython wrapper exposes Max C-API to Python
- Buffer protocol enables NumPy integration
- Direct Max object manipulation from Python scripts
