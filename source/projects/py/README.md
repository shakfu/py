# py: a general purpose python3 max external

## Overview

This overview will cover the following the `py` external implementation:

The `py` external provides a more featureful two-way interface between max and python in a way that feels natural to both languages.

The external has access to builtin python modules and the whole universe of 3rd party modules, and further has the option of importing a builtin `api` module which uses [cython](https://cython.org) to wrap selective portions of the max c-api. This allows regular python code to directly access the max-c-api and script Max objects. In addition, a pure python `py_prelude.py` module is pre-loaded in every `py` instance as an extended set of builtins.

So in summary, `py` is a general purpose Max external that embeds a python3 interpreter and is made up of three integrated parts which make it quite straightforward to extend:

1. The `py` Max external which is written in c using both the `Max c-api` and the `Python3 c-api`.

2. A pure python module, `py_prelude.py` which is converted to `py_prelude.h` and compiled with `py` and then pre-loaded into the `globals()` namespace of every `py` instance.

3. A powerful builtin `api` module which is derived from a cython-based wrapper of a subset of the `Max c-api`.

As of 24, March 2025, the relative size of these modules (in number of tokens) is:

id  | name             | language   | comment       | code          | note
:-- | :--------------- | :--------- | :------------ | :------------ | :-------------
1   | `py.c`           | c          | 729           | 1,598         | handwritten
2   | `py_prelude.py`  | python     | 95            | 181           | handwritten
3   | `api.pyx`        | cython     | 2,014         | 3,714         | handwritten
4   | `api.c`         | c          | 3,185         | 147,855       | generated from (3)

The following cheatsheat provides a brief view of key attributes and methods of the `py` external:

```text
globals
    obj_count                    : number of active py objects
    registry                     : global registry to lookup object names

patchers
    subpatchers
        py_repl                  : a basic single line repl for py
        py_repl_plus             : embeds a py object in a py_repl

py max external
    attributes
        name                     : unique object name
        file                     : file to load into editor
        autoload                 : load file at start
        pythonpath               : add path to python sys.path
        debug                    : switch debug logging on/off

    methods (messages) 
        core
            import <module>      : python import to object namespace
            eval <expression>    : python 'eval' semantics
            exec <statement>     : python 'exec' semantics
            execfile <path>      : python 'execfile' semantics
        
        extra
            assign <var> [arg]   : max-friendly msg assignments to py object namespace
            call <pyfunc> [arg]  : max-friendly python function calling
            pipe <arg> [pyfunc]  : process py/max value(s) via a pipe of py funcs
            fold <f> <n> [arg]   : applies a two-arg function cumulatively to a sequence
            code <expr|stmt>     : alternative way to eval or exec py code
            anything <expr|stmt> : anything version of the code method 

        time-based
            sched <t> <fn> [arg] : defer a python function call by t millisecs

        code editor
            read <path>          : read text file into editor
            load <path>          : combo of read <path> -> execfile <path>
            run                  : run the current code in the editor
     
        interobject
            scan                 : scan patcher and store names of child objects
            send <msg>           : send an arbitrary message to a named object

        meta
            count                : give a int count of current live py objects

    inlets
        single inlet             : primary input (anything)

    outlets
        left outlet              : primary output (anything)
        middle outlet            : bang on failure
        right outlet             : bang on success 
```

### Key Features

The `py` external has the following c-level methods:

category | method   | param(s)      | in/out | can change ns
:------- | :--------| :------------ | :----: | :------------:
core     | import   | module        | in     | yes
core     | eval     | expression    | out    | no
core     | exec     | statement     | in     | yes
core     | execfile | file          | in     | yes
extra    | assign   | var, data     | in     | yes
extra    | call     | var(s), data  | out    | no
extra    | code     | expr or stmt  | out?   | yes
extra    | anything | expr or stmt  | out?   | yes
extra    | pipe     | var, funcs    | out    | no
extra    | fold     | f, n, args    | out    | no
time     | sched    | ms, fun, args | out    | no
editor   | read     | file          | n/a    | no
editor   | load     | file          | n/a    | no
interobj | scan     |               | n/a    | no
interobj | send     | name, msg, .. | n/a    | no
meta     | count    |               | n/a    | no

Note that he `code` method allows for import/exec/eval of python code, which can be said to make those 'fit-for-purpose' methods redundant. However, it has been retained because it provides additional strictness and provides a helpful prefix in messages which indicates message intent.

#### Core

py/js's *core* features have a one-to-one correspondance to python's [very high layer](https://docs.python.org/3/c-api/veryhigh.html). In the following, when we refer to an *object*, we refer to instances of the `py` external.

- **Per-object namespaces**. Each object has a unique name (which is provided automatically or can be set by the user), and responds to an `import <module>` message which loads the specified python module in its namespace (essentially a `globals` dictionary). Notably, namespaces can be different for each instance.

- **Eval Messages**. Responds to an `eval <expression>` message in the left inlet which is evaluated in the context of the namespace. `py` objects output results to the left outlet, send a bang from the right outlet upon success or a bang from the middle outlet upon failure.

- **Exec Messages**. Responds to an `exec <statement>` message and an `execfile <filepath>` message which executes the statement or the file's code in the object's namespace. For `py` objects, this produces no output from the left outlet, sends a bang from the right outlet upon success or a bang from the middle outlet upon failure.

#### Extra

The *extra* category of methods  makes the `py` object play nice with the max/msp ecosystem:

- **Assign Messages**. Responds to an `assign <varname> [x1, x2, ..., xN]` which is equivalent to `<varname> = [x1, x2, ..., xN]` in the python namespace. This is a way of creating variables in the object's python namespace using max message syntax. This produces no output from the left outlet, a bang from the right outlet upon success, or a bang from the middle outlet upon failure.

- **Call Messages**. Responds to a `call <func> arg1 arg2 ... argN` kind of message where `func` is a python callable in the py object's namespace. This corresponds to the python `callable(*args)` syntax. This makes it easier to call python functions in a max-friendly way. If the callable does not have variable arguments, it will alternatively try to apply the arguments as a list i.e. `call func(args)`. Future work will try make `call` correspond to a python generic function call: `<callable> [arg1 arg2 ... arg_n] [key1=val1 key2=val2 ... keyN=valN]`. This outputs results to the left outlet, a bang from the right outlet upon success, or a bang from the middle outlet upon failure.

- **Pipe message**. Like a `call` in reverse, responds to a `pipe <arg> <f1> <f2> ... <fN>` message. In this sense, a value is *piped* through a chain of python functions in the objects namespace and returns the output to the left outlet, a bang from the right outlet upon success, or a bang from the middle outlet upon failure.

- **Code or Anything Messages**. Responds to a `code <expression || statement>` or (anything) `<expression || statement>` message. Arbitrary python code (expression or statement) can be used here, because the whole message body is converted to a string, the complexity of the code is only limited by Max's parsing and excaping rules. (This is classified as EXPERIMENTAL and evolving).

#### Interobject Communication

- **Scan Message**. Responds to a `scan` message with arguments. This scans the parent patcher of the object and stores scripting names in the global registry.

- **Send Message**. Responds to a `send <object-name> <msg> <msg-body>` message. Used to send *typed* messages to any named object. Evokes a `scan` for the patcher's objects if a `registry` of names is empty.

#### Editing Support

- **Line REPL**. The `py` object has two bpatcher line `repls`: one, `py_repl_plux.maxpat` which embeds a `py` object and another, `py_repl.maxpat` which has an outlet to connect to one. The repls include a convenient menu with all of the `py` object's methods and also feature `coll`-based history via arrow-up/arrow-down recall of entries in a session. A `coll` can made to save all commands if required.

- **Multiedit REPL**. Another bpatcher, `py_multiedit.maxpat`, combines a `textedit` object for writing multiliine python code to be executed in the respective `py` external's namespace, and a simple line repl strictly for evaluating objects in the namespace.

- **External Editor Filewatcher**. `py_extedit.maxpat` is a bpatcher which wraps the `filewatcher` object and opens a *watched* file in an external editor. If the file is saved by the editor, it will be sent out as text via the outlet and can be received, for example, by the `py` object's inlet, to enable a kind of load-on-save workflow.

- **Code Editor**. Double-clicking on the `py` object opens a code-editor. This is populated by a `read` message which reads a file into the editor and saves the filepath to the external's attribute. A `load` message also `reads` the file followed by `execfile`. Saving the text in the editor uses the attribute filepath and execs the saved code to the object's namespace.

- **Experimental Remote Console**. A method (due to [Iain Duncan](https://github.com/iainctduncan)) of sending code to the `py` node via `udp` has been implemented and allows for send-from-editor and send-from-interactive-console capabilities. The clients are still in their infancy, but this method looks promising since you get syntax highlighting, syntax checking, and other features. It assumes you want to treat your `py` nodes as remotely accessible `server/interpreters-in-max`.

```text
zedit: [python interpreter / web server] <-> [web-editor / web-console]
```

- **`zedit`: a python3 external with an embedded web server**. `zedit` is a python3-enabled external by virtue of using the `mamba` single-header library and also embeds the [mongoose embedded webserver](https://mongoose.ws). On the frontend, it uses modern javascript, [jquery-terminal](https://terminal.jcubic.pl) and the widely-used [code-mirror](https://codemirror.net) web text editor widget to create a web-editor / web-console which can be accessed from a browser and which communicates via the mongoose webserver with the underlying python interpreter.

#### Scripting Max with Python via the builtin `api` module

- **Exposing Max API to Python** A portion of the Max api in `c74support/max-includes` has been converted to a cython `.pxd` file called `api_max.pxd`. This makes it available for a cython implementation file, `api.pyx` which is converted to c-code during builds and embedded in the external. This code enables a custom python builtin module called `api` which can be imported by python scripts in `py` objects or via `import` messages to the object. This allows the subset of the Max-api which has been wrapped in cython code to be called directly by python scripts or via messages in a patcher.

The `api` module provides a number of functions and cython extension classes which make it relatively easy to call `Max c-api` methods from python. This is without doubt the most powerful feature of the `py` external.

As of this writing the following extension classes which wrap their corresponding Max objects are included in the `api` module: `Atom`, `AtomArray`, `Table`, `Buffer`, `Dictionary`, `Database`, `DatabaseView`, `DatabaseResult`, `Linklist`, `Binbuf`, `Hashtab`, `Patcher`, `MaxObject` and `MaxApp`.

In addition, a cython extension class, `PyExternal`, gives python code access to the c-based `py` external's data and methods.

To give a sense of the level of integration which is possible as a result of this module, the following example demonstrates how `numpy` and `scipy.signal` can be used to read and write to and from a live Max `buffer~` object using the `api` module's `Buffer` extension class:

```python
import api

import numpy as np
from scipy import signal

def get_buffer_samples(name: str, sample_file: str) -> np.array:
    buf = api.create_buffer(name, sample_file)
    xs = np.array(buf.get_samples())
    assert len(xs) == buf.n_samples
    api.post(f"get {n_samples} samples from buffer {name}")
    return xs


def set_buffer_samples(name: str, duration_ms: int):
    buf = api.create_empty_buffer(name, duration_ms)
    t = np.linspace(0, 1, buf.n_samples, endpoint=False, dtype=np.float64)
    xs = signal.sawtooth(2 * np.pi * 5 * t)
    buf.set_samples(xs)
    api.post(f"set {buf.n_samples} samples to buffer {name}")
```

See the `examples/tests` folder and the `patchers/tests`  folder for more examples.

### Deployment Scenarios

There are 3 general deployment variations:

1. **Linked to system python**. Linking the externals to your system python (homebrew, built from source, etc.) This has the benefit of re-using your existing python modules and is the default option.

2. **Embedded in package**. Embedding the python interpreter in a Max package: in this variation, a custom python distribution (zipped or otherwise) is placed in the `support` folder of the `py/js` package (or any other package) and is linked to the `py` external. This makes it size efficient and usable in standalones.

3. **Embedded in external**. The external itself as a container for the python interpreter: a custom python distribution (zipped or otherwise) is stored inside the external bundle itself, which can make it portable and usable in standalones.

As of this writing all three deployment scenarios are availabe, however it is worth looking more closely into the tradeoffs in each case, and the [related build variations which exist](#building-self-contained-python3-externals-for-packages-and-standalones).

Deployment Scenario    | Available | Python Versions
:--------------------- | :-------: | :--------------------
Linked to sys python   | yes       | version of sys python
Embeddded in package   | yes       | 3.8 to 3.13 inclusive
Embeddded in external  | yes       | 3.8 to 3.13 inclusive

## Quickstart

This repo has a git submodule dependency with [max-sdk-base](https://github.com/cycling74/max-sdk-base). This is quite typical for Max externals.

This means you should `git clone` as follows:

```sh
git clone https://github.com/shakfu/py-js.git
git submodule init
git submodule update
```

or more concisely:

```sh
git clone --recursive https://github.com/shakfu/py-js.git
```

### Windows

Since Windows support for relocatable builds still is relatively new, releases will be made pending further testing.

Currently, the externals which are enabled by default in this project can be built with only a few requirements:

1. Install [Visual Studio Community Edition](https://visualstudio.microsoft.com/vs/community/) or use the commercial versions as you like.

2. Install [Python3 for Windows](https://www.python.org/downloads/windows) from python.org

3. (Optional) since Visual Studio has its captive cmake, [you can use that](https://stackoverflow.com/questions/70178963/where-is-cmake-located-when-downloaded-from-visual-studio-2022), but it is preferable to [install cmake](https://cmake.org/download/#latest) independently.

After installation of the above you can build the `py` external which links to local system python by doing the following inside your `Documents/Max [8 or 9]/Packages` folder:

```sh
git clone --recursive https://github.com/shakfu/py-js
cd py-js
mkdir build
cd build
cmake .. -DBUILD_TARGETS=py
cmake --build . --config Release
```

If you want to build a relocatable python3 external for Windows which can be used in Max packages and standalones then type the following in the previously cloned `py-js` directory:

```sh
python source\\scripts\\buildpy.py -t windows-pkg
mkdir build
cd build
cmake .. -DBUILD_TARGETS=py -DBUILD_VARIANT=windows-pkg
cmake --build . --config Release
```

Open one of the `.maxhelp` files or any of the files in the `patchers` folders to see how things work.

### macOS

As mentioned earlier, the `py` and `pyjs` objects are the most mature and best documented of the collection. Happily, there is also no need to compile them as they are available for download, fully codesigned and notarized, from the [releases](https://github.com/shakfu/py-js/releases) section.

If you'd rather build them or any of the other externals yourself then the process is straightforward:

1. You should have a modern `python3` cpython implementation installed on your Mac: preferably either from [python.org](https://www.python.org) or from [Homebrew](https://brew.sh). Note that even system python3 provided by Apple will work in a number of cases. Python versions from 3.8 to 3.13 are tested and known to work.

2. Make sure you also have [Xcode](https://xcodereleases.com/) installed.

3. Git clone the `py-js` [repo](https://github.com/shakfu/py-js) as per the above method to a path without a space and without possible icloud syncing (i.e don't clone to `$HOME/Documents/Max 8/Packages`) [?] and run the following in the cloned repo:

    ```bash
    make setup
    ```

    The above will initialize and update the required git submodules and symlink the repo to `$HOME/Documents/Max 8/Packages/py-js` to install it as a Max Package and enable you to test the externals and run the patches.

    [?] It is possible to install `py-js` directly into `$HOME/Documents/Max 8/Packages`, but it requires moving the place of compilation to a location in your filesystem that is not exposed to errors due to icloud syncing or spaces in the path. This split is possible, but it is not recommended for the purposes of this quickstart.

4. Install [cython](https://cython.org) via `pip3 install cython`, required for translating the cython-based `api.pyx`, which wraps the the Max c-api, to c.

5. To build only the `py` and `pyjs` externals, type the following in the root directory of the `py-js` project (other installation options are detailed below):

    ```sh
    make
    ```

Note that typing `make` here is the same as typing `make default` or `make all`. This will create two externals `py.mxo` and `pyjs.mxo` in your `externals` folder. These are relatively small in size and are linked to your system python3 installation. This has the immediate benefit that you have access to your curated collection of existing python packages. The tradeoff is that these externals are dynamically linked with local dependencies and therefore not usable in standalones and relocatable Max packages.

No worries, if you need portable relocatable python3 externals for your package or standalone then make sure to read the [Building self-contained Python3 Externals for Packages and Standalones](https://github.com/shakfu/py-js#building-self-contained-python3-externals-for-packages-and-standalones) section

Open up any of the patch files in the `patchers` directory of the repo or the generated Max package, and also look at the `.maxhelp` patchers to understand how the `py` and the `pyjs` objects work.

### Building Experimental Externals using Cmake

You can also use `cmake` to build **all** externals using similar methods to the `max-sdk`.

First make sure you have completed the [Quickstart](#quickstart) section above. Next you will install `cmake` if necessary and a couple of additional dependencies for some of the subprojects. Of course, skip what is already installed:

```sh
brew install cmake zmq czmq
```

Now you can build all externals (including `py` and `pyjs`) in one shot using cmake:

```sh
make projects
```

After doing the above, the recommended iterative development workflow is to make changes to the source code in the respective project and then `cd py-js/build` and `cmake --build .`. This will cause cmake to only build modified projects efficiently.

Note that for some of the less developed externals and more experimental features please don't be surprised if Max seg-faults (especially if you start experimenting with the cython wrapped `api` module which operates on the c-level of the Max SDK).

Also note that for `py` and `pyjs` externals the `cmake` build method described does not yet create self-contained python externals which can be used in Max Packages and Standalones.

The following section addresses this requirement.

### Building self-contained Python3 Externals for Packages and Standalones

The `py` and `pyjs` externals have a custom python [build manager](https://github.com/shakfu/py-js/tree/main/source/projects/py/builder) which provides the flexibility to create a number of `build variants` which can vary in size and features, or be selected depending on whether the external is to be packaged in standalones or Max packages.

The `Makefile` in the project root provides a simplified interface to this builder. See the [Current Status of Builders](https://github.com/shakfu/py-js#current-status-of-builders) section for further information.

idx  | command                | type       | format     | py size |  pyjs size
:--: | :--------------------- | :--------- | :--------- | :------ | :----------
1    | `make static-ext`      | static     | external   | 9.0     | 8.8
2    | `make static-tiny-ext` | static     | external   | 6.7     | 6.2
3    | `make shared-ext`      | shared     | external   | 16.4    | 15.8
4    | `make shared-tiny-ext` | shared     | external   | 6.7     | 6.2
5    | `make framework-pkg`   | framework  | package    | 22.8    | 22.8

In this table, size figures are for python 3.10.x but for python 3.11.4 they increase to 8.5 MB and 8.1 respectively. Generally, external size increases with each new python version as features are added, but this is also somewhat mitigated by the removal of deprecated builtin packages and extensions. If you want to achieve the theoretical minimal size for the `py` and `pyjs` externals, use python 3.8.x and/or a tiny variant (with a more recent version). Another option, if you need circa 1 MB size for a self-contained external, look at the `pktpy` subproject in this repo. Note the size of externals in Python 3.12.4 (although some of extra size is attributed improved ssl integration):

Also note that size, in this case, is not the individual external but the uncompressed size of the package which includes patches, help files and **both** externals. This can also vary by python version used to compile the external.

idx  | command                | type       | format     | py size |  pyjs size
:--: | :--------------------- | :--------- | :--------- | :------ | :----------
1    | `make static-ext`      | static     | external   | 15.0    | 13.3
2    | `make static-tiny-ext` | static     | external   | 11.4    | 9.8
3    | `make shared-ext`      | shared     | external   | 20.4    | 18.7
4    | `make shared-tiny-ext` | shared     | external   | 11.4    | 9.6
5    | `make framework-ext`   | shared     | external   | 22.5    | 20.8

for Python 3.13.0, which implemented a number of deprecations, external sizes have come down a little:

idx  | command                | type       | format     | py size |  pyjs size
:--: | :--------------------- | :--------- | :--------- | :------ | :----------
1    | `make static-ext`      | static     | external   | 14.4    | 12.6
2    | `make static-tiny-ext` | static     | external   | 10.2    | 8.5
3    | `make shared-ext`      | shared     | external   | 19.1    | 17.3
4    | `make shared-tiny-ext` | shared     | external   | 10.6    | 8.8
5    | `make framework-ext`   | shared     | external   | 21.2    | 20.2

This section assumes that you have completed the [Quickstart](#quickstart) above and have a recent python3 installation (python.org, homebrew or otherwise).

Again, if you'd rather not compile anything there are self-contained python3 externals which can be included in standalones in the [releases](https://github.com/shakfu/py-js/releases) section.

If you don't mind compiling (and have Xcode installed) then pick one of the following options:

1. To build statically-compiled self-contained python3 externals:

    ```sh
    make static-ext
    ```

    You may also prefer the tiny variant:

    ```sh
    make static-tiny-ext
    ```

2. To build self-contained python3 exernals which include a dynamically linked libpythonX.Y.dylib:

    ```sh
    make shared-ext
    ```

    or for the corresponding tiny variant:

    ```sh
    make shared-tiny-ext
    ```

3. To build python3 externals in a package, linked to a python installation in its `support` folder

    ```sh
    make framework-pkg
    ```

With all of the above options, a python3 source distribution (matching your own python3 version) is automatically downloaded from [python.org](https://www.python.org) with dependencies, and then compiled into a static or shared version of python3 which is then used to compile the externals.

If you would rather specify the python version, then you can specify the `PYTHON_VERSION` environment variable, as in:

```sh
make shared-ext PYTHON_VERSION=3.10.17
```

Note that as of this writing, the `py` external and the `pyjs` externals are tested successfully to build and run with Python versions `3.8.20`, `3.9.22`, `3.10.17`, `3.11.12`, `3.12.10` and `3.13.3`. Feel free to use the version specification method above with any build variant.

At the end of this process you should find two externals in the `py-js/externals` folder: `py.mxo` and `pyjs.mxo`.

Although the above options deliver somewhat different products (see below for details), with options (1) and (2) the external 'bundle' contains an embedded python3 interpreter with a zipped standard library in the `Resources` folder and also has a `site-packages` directory for your own code; with option (3), the externals are linked to, and have been compiled against, a relocatable python3 installation in the `support` folder.

Depending on your choice above, the python interpreter in each external is either statically compiled or dynamically linked, and in all three cases we have a self-contained and relocatable structure (external or package) without any non-system dependencies. This makes it appropriate for use in Max Packages and Standalones.

There are other [build variations](#build-variations) which are discussed in more detail below. You can always see which ones are available via typing `make help` in the `py-js` project folder:

```sh
$ make help

>>> general
make projects             : build all subprojects using standard cmake process

>>> pyjs targets
make                      : non-portable pyjs externals linked to your system
make homebrew-pkg         : portable package w/ pyjs (requires homebrew python)
make homebrew-ext         : portable pyjs externals (requires homebrew python)
make shared-pkg           : portable package with pyjs externals (shared)
make shared-ext           : portable pyjs externals (shared)
make shared-tiny-ext      : tiny portable pyjs externals (shared)
make static-ext           : portable pyjs externals (static)
make static-tiny-ext      : tiny portable pyjs externals (static)
make framework-pkg        : portable package with pyjs externals (framework)
make framework-ext        : portable pyjs externals (framework)
make relocatable-pkg      : portable package w/ more custom options (framework)

>>> python targets
make python-shared        : minimal shared python build
make python-shared-ext    : minimal shared python build for externals
make python-shared-pkg    : minimal shared python build for packages
make python-static        : minimal statically-linked python build
make python-framework     : minimal framework python build
make python-framework-ext : minimal framework python build for externals
make python-framework-pkg : minimal framework python build for packages
make python-relocatable   : custom relocatable python framework build
```

### Automated Test of Build Variations

If you would like to see which build variations are compatible with your current setup, there's an automated test which attempts to compile all build variations in sequence and will log all results to a `logs` directory:

```sh
make test
````

This can take a long time, but it is worth doing to understand which variants work on your particular setup.

If you want to test or retest one individual variant, just prefix `test-` to the name of variant as follows:

```sh
make test-shared-pkg
```

### Using Self-contained Python Externals in a macOS Standalone

If you have downloaded any pre-built externals from [releases](https://github.com/shakfu/py-js/releases) or if you have built self-contained python externals as per the methods above, then you should be ready to use these in a standalone.

To release externals in a standalone they must be codesigned and notarized. To this end, there are scripts in `py-js/source/projects/py/scripts` to make this a little easier.

#### py external

If you included `py.mxo` as an external in your standalone, then you should have no issue as Max will install it automatically during its build-as-standalone process.

You can test if it works without issues by building either of these two example patcher documents, included in `py-js/patchers`, as a max standalone:

1. `py_test_standalone_info_py.maxpat`

2. `py_test_standalone_only_py.maxpat`

Open the resulting standalone and test that the `py` object works as expected.

To demonstrate the above, a pre-built standalone that was built using exactly the same steps as above is in the releases section: `py_test_standalone_demo.zip`.

#### pyjs external

If you opted to include `pyjs.mxo` as an external in your standalone, then it may be a litte more involved:

You can first test if it works without issues by building 'a max standalone' from the `test_standalone_pyjs.maxpat` patcher which is included in `py-js/patchers/tests/test_standalone`.

Open the resulting standalone and test that the `pyjs` object works as expected. If it doesn't then try the following workaround:

To fix a sometimes recurrent issue where the standalone build algorithm doesn't pick up `pyjs.mxo`: if you look inside the built standalone bundle, `py_test_standalone_only_pyjs.app/Contents/Resources/C74/externals` you may not find `pyjs.mxo`. This is likely a bug in Max 8 but easily resolved. Fix it by manually copying the `pyjs.mxo` external into this folder and then copy the `javascript` and `jsextensions` folders from the root of the `py-js` project and place them into the `pyjs_test_standalone.app/Contents/Resources/C74` folder. Now re-run the standalone app again and now the `pyjs` external should work. A script is provided in `py-js/source/projects/py/scripts/fix-pyjs-standalone.sh` to do the above in an automated way.

Please read on for further details about what the py-js externals can do.

Have fun!

## Packaging

As mentioned previously, the py-js `builder` subproject can be used to build fit-for-purpose python variants for python3 externals. In addition, it can also package, sign, notarize and deploy the same externals for distribution.

These features are implemented in `py-js/source/project/py/builder/packaging.py` and are exposed via two interfaces:

### The argparse-based interface of builder

```sh
$ python3 -m builder package --help
usage: builder package [-h] [-v VARIANT] [-d] [-k KEYCHAIN_PROFILE]
                           [-i DEV_ID]
                           ...

options:
  -h, --help            show this help message and exit
  -v VARIANT, --variant VARIANT
                        build variant name
  -d, --dry-run         run without actual changes.
  -k KEYCHAIN_PROFILE, --keychain-profile KEYCHAIN_PROFILE
                        Keychain Profile
  -i DEV_ID, --dev-id DEV_ID
                        Developer ID

package subcommands:
  package, sign and release external

                        additional help
    collect_dmg         collect dmg
    dist                create project distribution folder
    dmg                 package distribution folder as .dmg
    notarize_dmg        notarize dmg
    sign                sign all required folders recursively
    sign_dmg            sign dmg
    staple_dmg          staple dmg
```

### The Project's Makefile frontend

Since the `Makefile` frontend basically just calls the `builder` interface in a simplified way, we will use it to explain the basic sequential packaging steps.

1. Recursively sign all externals in the `external folder` and/or binaries in the `support` folder

    ```sh
    make sign
    ```

2. Gather all project resources into a distribution folder and then convert it into a `.dmg`

    ```sh
    make dmg
    ```

3. Sign the DMG

    ```sh
    make sign-dmg
    ```

4. Notarize the DMG (send it to Apple for validation and notarization)

    ```sh
    make notarize-dmg
    ```

5. Staple a valid notarization ticket to the DMG

    ```sh
    make staple-dmg
    ```

6. Zip the DMG and collect into in the `$HOME/Downloads/PY-JS` folder

    ```sh
    make collect-dmg
    ```

To do all of the above in one step:

```sh
make release
```

Note that it is important to sign externals (this is done by Xcode automatically) if you want to to distribute to others (or in the case of Apple Silicon, even use yourself). If the externals are signed, then you can proceed to the notarization step if you have an Apple Developer License (100 USD/year) or, alternatively, you can ask users to remove the product's quarantine state or let Max do this automatically on opening the external.

### Notarization Requirements

To complete the notarization process, an Apple Developer Account and an [app-specific password](https://support.apple.com/en-sa/102654) are required.

1. Create local credentials based on your apple developer id and app-specific password

    ```sh
    xcrun notarytool store-credentials "<keychain-profile-name>" --apple-id "<apple-id>" --team-id <developer-team-id> --password "<app-specific-password>"
    ```

2. Export `DEV_ID` and `KEYCHAIN_PROFILE` environment variables:

    ```sh
    export DEV_ID="<first> <lastname>"
    export KEYCHAIN_PROFILE="<name-of-credentials>"
    ```

3. Run the whole process (i.e. steps 1-6) with one command:

    ```sh
    make release
    ```

### Github Actions

There are a number of Github actions in the project which basically automate the testing, packaging, and possibly the notariztion steps described above.

### Caveats

- The externals in this project have been mostly developed on MacOS and have not yet been extensively tested on Windows.

- Despite their relative maturity, the `py` and `pyjs` objects are still only v0.2.x and still need further unit/functional/integration/field testing!

- As of this writing, the `api` module, does not (like apparently all 3rd party python c-extensions) unload properly between patches and requires a restart of Max to work after you close the first patch which uses it. Unfortunately, this is a known [bug](https://bugs.python.org/issue34309) in python which is being worked on and may be [fixed](https://groups.google.com/forum/?utm_medium=email&utm_source=footer#!msg/cython-users/SnVpCE7Sq8M/hdT8S2iFBgAJ) in future versions (python 3.13 perhaps?).

- `Numpy`, the popular python numerical analysis package, falls in the above category. As of python 3.9.x, it thankfully doesn't crash but gives the following error:

```sh
[py __main__] import numpy: SystemError('Objects/structseq.c:401: bad argument to internal function')
```

This just means that the user opened a patch with a `py-js` external that imports `numpy`, then closed the patch and (in the same Max session) re-opened it, or created a new patch importing `numpy` again.

To fix it, just restart Max and use it normally in your patch. Treat each patch as a session and restart Max after each session. It's a pain, but unfortunately a limitation of current python c-extensions.

- `core` features relying on pure python code are supposed to be the most stable, and *should* not crash under most circumstances, `extra` features are less stable since they are more experimental, etc..

- The `api` module is the most experimental and evolving part of this project, and is completely optional. If you don't want to use it, don't import it or don't use an external which provides it.

### Current Status of Builders

As mentioned earlier, as of this writing this project uses a combination of a `Makefile` in the project root, a basic `cmake` build option and a custom python build system, `builder`, which resides in the `py-js/source/py/builder` package. The `Makefile` is a kind of 'frontend' to the more complex python build system. The latter can be used directly of course. A view into its many options can be obtained by typing the following:

```sh
cd py-js/source/py
python3 -m builder --help
```

`builder` was developed to handle the more complex case of downloading the source code of python (from python.org) and also its dependencies from their respective sites and then building custom python binaries with which to reliably compile python3 externals which are portable, relocatable, self-contained, small-in-size, and usable in Max Packages and Standalones.

### Build Variations

One of the objectives of this project is to cater to a number of build variations. As of this writing, the following table gives an overview of the different builds and their differences:

There is generally tradeoff of size vs. portability:

build command       | format       | size_mb  | deploy_as | pip      | portable | numpy
:-------------------| :----------- | :------: | :-------: | :-------:| :-------:| :-------:
make                | framework    | 0.3      | external  | yes [^1] | no       | yes
make homebrew-ext   | hybrid  [^3] | 13.6     | external  | no       | yes      | yes
make homebrew-pkg   | hybrid       | 13.9     | package   | yes      | yes      | yes
make static-ext     | static       | 9.0      | external  | no       | yes      | no [^2]
make shared-ext     | shared       | 15.7     | external  | no       | yes      | yes
make shared-pkg     | shared       | 18.7     | package   | yes      | no [^4]  | yes
make framework-ext  | framework    | 16.8     | external  | no       | yes      | yes
make framework-pkg  | framework    | 16.8     | package   | yes      | yes      | yes

[^1]: In this case, `pip` has automatic access to your system python's site-packages

[^2]: The static external implementation does not work well with embedding `numpy` due to symbol access issues, but is still usable via the buffer protocol and memoryview interfaces.

[^3]: *hybrid* means that the source system was a `framework` and the destination system is `shared`.

[^4]: the `shared-pkg` variant does not build a compliant `.framework` bundle and hence cannot be notarized.

- *pip*: the build allows or provides for pip installation

- *portable*: the externals can be deployed as portable packages or standalones

- *numpy*: numpy compatibility

#### Packages vs Self-contained Externals

The Max package format is a great way to move a bunch of related patches and externals around. This format also makes a lot of sense for `py-js`, giving a number of advantages over other alternatives:

1. Portable: Relocatable, you can move it around and it still works.

2. Extendable: Can include a full fit-for-purpse python3 installation in the `support` directory with its own site-packages. Packages can be `pip` installed and all of the `site-packages` is automatically made available to the thin 'client' python3 externals in the package's `externals` folder.

3. Size-efficient, since you don't need to duplicate functionality in each external

4. Standalone installable: Recent changes in Max have allowed for this to work in standalones. Just create your standalone application from a patcher which which includes the `py` and `pyjs` objects. Once it is built into a `<STANDALONE>` then copy the whole aforementioned `py` package to `<STANDALONE>/Contents/Resources/C74/packages` and delete the redundant `py.mxo` in `<STANDALONE>/Contents/Resources/C74/externals` since it already exists in the just-copied package.

5. Better for codesigning / notarizing scenarios since Packages are not sealed bundles like externals.

On the other hand, sometimes you just want an external which embeds a python distribution and custom extensions and code:

1. Portable: Relocatable, you can move it around and it still works.

2. Extendable: Can include new pure python code and be provided with new additionas to `sys.path`

3. Size-efficient and fit-for-purpose

4. Standalone installable. Easiest to install in standalones

5. Can be codesigned and notarized relatively easily. [1]

[1] If you want to codesign and notarize it for use in your standalone or package, the [codesigning / notarization script](source/py/scripts/notarize.sh) and related [entitlements file](source/py/scripts/entitlements.plist) can be found in the [source/py/scripts](source/py/scripts) folder.

### The relocatable-python variation

[relocatable-python](https://github.com/gregneagle/relocatable-python) is Greg Neagle's excellent tool for building standalone relocatable Python.framework bundles.

It works so well, that its been included in the `builder` application as an external (embedded dependency).

It can be seen in the `relocatable-pkg` make option which will download a nice default `Python.framework` to the `support` directory used for compiled both `py` and `pyjs` externals:

```sh
make relocatable-pkg
```

More options are available if you use the `builder` package directly:

```sh
$ python3 -m builder pyjs relocatable_pkg --help
usage: __main__.py pyjs relocatable_pkg [-h] [--destination DESTINATION]
                                        [--baseurl BASEURL]
                                        [--os-version OS_VERSION]
                                        [--python-version PYTHON_VERSION]
                                        [--pip-requirements PIP_REQUIREMENTS]
                                        [--pip-modules PIP_MODULES]
                                        [--no-unsign] [--upgrade-pip]
                                        [--without-pip] [--release] [-b] [-i]
                                        [--dump]

optional arguments:
  -h, --help            show this help message and exit
  --destination DESTINATION
                        Directory destination for the Python.framework
  --baseurl BASEURL     Override the base URL used to download the framework.
  --os-version OS_VERSION
                        Override the macOS version of the downloaded pkg.
                        Current supported versions are "10.6", "10.9", and
                        "11". Not all Python version and macOS version
                        combinations are valid.
  --python-version PYTHON_VERSION
                        Override the version of the Python framework to be
                        downloaded. See available versions at
                        https://www.python.org/downloads/mac-osx/
  --pip-requirements PIP_REQUIREMENTS
                        Path to a pip freeze requirements.txt file that
                        describes extra Python modules to be installed. If not
                        provided, no modules will be installed.
  --pip-modules PIP_MODULES
                        list of extra Python modules to be installed.
  --no-unsign           Do not unsign binaries and libraries after they are
                        relocatablized.
  --upgrade-pip         Upgrade pip prior to installing extra python modules.
  --without-pip         Do not install pip.
  --release             set configuration to release
  -b, --build           build python
  -i, --install         install python to build/lib
  --dump                dump project and product vars
```

### Sidenote about building on a Mac

If you are developing the package in `$HOME/Documents/Max 8/Packages/py` and you have your iCloud drive on for Documents, you will find that `make` or `xcodebuild` will reliably fail with 1 error during development, a codesigning error that is due to icloud sync creating detritus in the dev folder. This can be mostly ignored (unless your only focus is codesigning the external).

The solution is to move the external project folder to folder that's not synced-with-icloud  (such as `$HOME/Downloads` for example) and then run `xattr -cr .` in the project directory to remove the detritus (which ironically Apple's system is itself creating) and then it should succeed (provided you have your `Info.plist` and `bundle id` correctly specified). Then just symlink the folder to `$HOME/Documents/Max 8/Packages/` to prevent this from recurring.

I've tried this several times and and it works (for "sign to run locally" case and for the "Development" case).

### Code Style

The coding style for this project can be applied automatically during the build process with `clang-format`. On OS X, you can easily install this using brew:

```sh
brew install clang-format
```

The style used in this project is specified in the `.clang-format` file.

## Caveats

- Packaging and deployment of python3 externals has improved considerably but is still a work-in-progress: basically needing further documentation, consolidation and cleanup. For example, there are currently two build systems which overlap: a python3 based build system to handle complex packaging cases and cmake for handling the quick and efficient development builds and general cases.

- As of this writing, the `api` module, does not (like apparently all 3rd party python c-extensions) unload properly between patches and requires a restart of Max to work after you close the first patch which uses it. Unfortunately, this is a known [bug](https://bugs.python.org/issue34309) in python which is being worked on and may be [fixed](https://groups.google.com/forum/?utm_medium=email&utm_source=footer#!msg/cython-users/SnVpCE7Sq8M/hdT8S2iFBgAJ) in future versions (python 3.12 perhaps?).

- `Numpy`, the popular python numerical analysis package, falls in the above category. In newer versions of Python the situation is improving as above, but in python 3.9.x, it thankfully doesn't crash but gives the following error:

```bash
[py __main__] import numpy: SystemError('Objects/structseq.c:401: bad argument to internal function')
```

This just means that the user opened a patch with a `py-js` external that imports `numpy`, then closed the patch and (in the same Max session) re-opened it, or created a new patch importing `numpy` again.

To fix it, just restart Max and use it normally in your patch. Treat each patch as a session and restart Max after each session.

- `core` features relying on pure python code are supposed to be the most stable, and *should* not crash under most circumstances, `extra` features are less stable since they are more experimental, etc..

- The `api` module is the most experimental, powerful and evolving part of this project, and is completely optional. If you don't want to use it, don't import it.
