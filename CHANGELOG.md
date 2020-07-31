
# CHANGELOG


## v0.1


### Features

#### Core

- [x] pyjs in-function calls to code did not work well as strings (conversion to array fixed it)

- [x] no-return ops in`pyjs` such as `exec` and `import` somehow make javascript assume an error has occured.


- [x] Convert `py` into a `jsextension` class

- [x] create new `py_anything` with heuristics to decide whether to delegate to `py_call` or `py_code`.

- [x] add `autoload` attribute to trigger autoload (`load` msg) of code editor code

- [x] for `pythonpath` add file location feature (try pkg/examples/scripts then absolute paths)

- [x] branch `embed-pkg`, embeds a local python install with a zipped stdlib in `support` already successfully tested embedding the python distro in the external itself.

- [x] made it possible to get the py object's name from any module in its namespace!

- [x] enhance `py_exec` method to create a single string from argv so it can import easily

- [x] enhance `py_anything` method to eval if identifier is not a callable yet exists in ns

- [x] Refactor 'py_eval' to make it more consistent with the others

- [x] Implementation of a few high level python api functions in max (eval, exec) to allow the evaluation of python code in a python `globals` namespace associated with the py object.

- [x] Each py object has its own python 'globals' namespace and responds to the following
      msgs
    - [x] `import <module>`: adds module to the namespace
    - [x] `eval <expression>`: evaluate expression within the context of the namespace (cannot modify ns)
    - [x] `exec <statement>`: executes statement into the namespace (can modify ns)
    - [x] `execfile <file.py>`: executes python file into the namespace (can modify ns)
    - [x] `run <file.py>`: executes python file into the namespace (can modify ns)

- [x] Add right inlet bang after eval op ends

- [x] add third (middle) outlet which bangs on an error

#### Extra


- [x] check whether setting a normal attr name, can also set scripting name

- [x] Implement 'send' msg, which sends typed messages to (script) named objects (see: https://cycling74.com/forums/error-handling-with-object_method_typed)

- [x] Add `call (anything)` method to call python callables in a namespace


#### Code Editor (Usability)

- [x] Edit default with text editor

- [x] Add text edit object
    - [x] enable code to be run from editor


#### Line REPL (Usability)

- [x] Add line repl
    - [x] Add up-arrow last line recall (great for 'random.random()')


#### Extensibility

- [x] Create type conversion method in `api.pyx` which could serve python code and also c-code calling python code

- [x] Implement section on two-way globals setting and reading (from python and c) in https://pythonextensionpatterns.readthedocs.io/en/latest/module_globals.html (deferred for now)

- [x] Add bpatcher line repl

- [x] add python scripts to 'examples/scripts'

- [x] Add cythonized access to max c-api..?

- [x] Extensible by embedded cython based python extensions which can call a library of wrapped max_api functions in python code. There is a proof of concept of the python code in the namsepace calling the max api `post` function successfully.

- [x] Exposing of good portion of the max api to cython scripting


#### Architectural

- [x] Global object/dict/ref mgmt (so two externals can exist without Py_Finalize() causing a crash


#### Documentation

- [x] Add .maxref.xml to docs


#### Code Quality

- [x] refactor error handling code (if possible)

- [x] Refactor eval code from py_eval into a function to allow for exec and execfile or PyRun_File scenarios

- [x] Refactor into functions


#### Testing

- [x] pytest testing harness

- [x] make test between test_translate and test_py2 which includes references to a the struct which is missing in the former


### Bug Fixes

- [x] fixed 'send' which did not have enough error checking and was crashing frequently

- [x] fixed STRANGE bug, single quotes in `py_log` cased a crash in `py_scan_callback`, it's based on post but post alone with the same does not cause a crash. Should simplify logging!

- [x] globex remains after all objects are freed. solution: `PyXDECREF x->p_globals` on `py_free`

- [x] space in `eval` without quotes will cause a crash!

- [x] space in path causes "sprintf" type debugging in execfile to crash max!

- [x] codesigning errors are due to Package being developed in Documents/... which causes issues. If it's a non icloud exposed folder it works ok.

- [x] make exec work! (needs globals in both slots: `PyRun_String(py_argv, Py_single_input, x->p_globals, x->p_globals)`

- [x] `import` statement in eval causes a segmentation fault. see: https://docs.python.org/3/c-api/intro.html exception handling example -> needed to changed Py_DECREF to Py_XDECREF in error handling code

- [x] do not give attr has same name as method (the import saga) as this will crash. fix by making them different.
