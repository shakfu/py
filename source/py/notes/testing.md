# Testing


## testing robustness to python version change

### bin-homebrew-sys

- test_external_lib

/usr/local/opt/python@3.8/Frameworks/Python.framework/Versions/3.8/lib/python3.8/os.py

- targets/bin-homebrew-sys

PREFIX = /usr/local/opt/python3/Frameworks/Python.framework/Versions/$(VERSION)
PY_HEADERS = $(PREFIX)/include/python$(VERSION)
PY_LIBS = $(PREFIX)/lib
PY_LDFLAGS = -lpython$(VERSION) -ldl


Note: tried to do it as a pure Framework, but had to change Header refs from "Python.h" to "Python/Python.h", too complex to experiment with this model (try it simpler)


### bin-homebrew-pkg

- test_external_lib

"/Users/sa/Documents/Max 8/Packages/py/externals/py.mxo/Contents/MacOS/../../../../support/python3.8/lib/python38.zip/os.py"

- targets/bin-homebrew-pkg

PREFIX = $(SUPPORT)/python$(VERSION)
PY_HEADERS = $(PREFIX)/include/python$(VERSION)
PY_LIBS = $(PREFIX)
PY_LDFLAGS = -lpython$(VERSION) -ldl


### bin-homebrew-ext

- test_external_lib

"/Users/sa/Documents/Max 8/Packages/py/externals/py.mxo/Contents/MacOS/../Frameworks/python3.8/lib/python38.zip/os.py"

works for py.mxo BUT

ERRORS
- Fails codesign step
- pyjs not working

possible solutions:

- cp -a Framework
- use Resource as bin-beeware-ext
- use copy_dylibs.py for dependencies


### src-framework-pkg


## Test c code

- As much as possible, code unrelated to max should be separated into individual general functions and placed in a library


## Max Tests

- Integrations tests using the `.maxtest.maxpat` format
- each method a separate test file

