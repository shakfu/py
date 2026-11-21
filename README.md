# py: a python3 external for max/msp

This project includes only the `py` external which is developed in the [py-js](https://github.com/shakfu/py-js.git) project.

This is just a brief experiment to develop it individually, and does not contain anything except the minimal requirements to do. If you are interested in this external or the topic of python3 externals for Max/MSP in general, you should visit the [py-js](https://github.com/shakfu/py-js.git) project for pre-compiled binaries and extensive documentation and examples.

## Building

Requirements: macOS, Xcode command line tools, CMake 3.24 or later, and a Python 3 for the local build. Run `make setup` once to fetch the submodules and link the repo into `~/Documents/Max {8,9}/Packages`.

### Local build

```sh
make local
```

Builds `externals/py.mxo` against the Python CMake finds, for example Homebrew's. The external links that Python from its install location, so it only works on machines with the same Python installed.

### Relocatable builds

These targets build a self-contained `py.mxo` that carries its own Python:

| Target | Python | Where Python goes |
|-|-|-|
| `make static-ext` | `libpython` linked into the external | `py.mxo/Contents/Resources/lib` (standard library) |
| `make shared-ext` | `libpython` dylib | `py.mxo/Contents/Resources/lib` |
| `make framework-ext` | `Python.framework` | `py.mxo/Contents/Resources/Python.framework` |
| `make framework-pkg` | `Python.framework`, shared by the package | `support/Python.framework` |

Each target runs `source/scripts/buildpy.py -t <variant>`, which downloads and builds Python with OpenSSL, bzip2 and xz into `build/install`, then deletes and rebuilds `externals/py.mxo` against it. Python is built once; later runs reuse it. All four variants have been tested in Max 9 on macOS (Apple silicon) with `py.maxhelp`.

All variants use buildpy's `*_mid` configurations: `_decimal` is left out, and `_ssl`, `_hashlib`, `_bz2`, `_lzma`, `_sqlite3`, `_scproxy`, `zlib` and `binascii` are built into Python, with OpenSSL, bzip2 and xz linked statically and their symbols hidden. The framework variants also build in `_ctypes`; `static-ext` and `shared-ext` have no `ctypes`.

Options:

- `MACOSX_DEPLOYMENT_TARGET=12.6`: minimum macOS for the bundled Python and its dependencies, default 11.0. `py.mxo` always uses the value the bundled Python was built with. To change it after Python is built, delete `build/install` first, since the dependencies are built with it too.
- `TARGETS="py zedit"`: the CMake targets to build, default `py`.
- `PYTHON=python3.13`: the interpreter that runs `buildpy.py`.

`make build` deletes `build/`, including the Python under `build/install`; use the targets above instead.

### Other targets

- `make test`: builds and runs the tests (`py_code` and the `api.pyi` check).
- `make stub`: regenerates `source/projects/py/api.pyi` from `api.pyx`. Needs Cython.
- `make api`: regenerates `api.c` with Cython, then the stub.
