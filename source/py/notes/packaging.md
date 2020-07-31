# Packaging notes

## Setup.local

with configure as follows:

```
MACOSX_DEPLOYMENT_TARGET=10.13 ./configure --prefix=`pwd`/../python-min --without-doc-strings --enable-ipv6 --without-ensurepip && make altinstall &> build.log
```


```
# -*- makefile -*-
# The file Setup is used by the makesetup script to construct the files
# Makefile and config.c, from Makefile.pre and config.c.in,
# respectively.  Note that Makefile.pre is created from Makefile.pre.in
# by the toplevel configure script.

# (VPATH notes: Setup and Makefile.pre are in the build directory, as
# are Makefile and config.c; the *.in files are in the source directory.)

# Each line in this file describes one or more optional modules.
# Modules configured here will not be compiled by the setup.py script,
# so the file can be used to override setup.py's behavior.
# Tag lines containing just the word "*static*", "*shared*" or "*disabled*"
# (without the quotes but with the stars) are used to tag the following module
# descriptions. Tag lines may alternate throughout this file.  Modules are
# built statically when they are preceded by a "*static*" tag line or when
# there is no tag line between the start of the file and the module
# description.  Modules are built as a shared library when they are preceded by
# a "*shared*" tag line.  Modules are not built at all, not by the Makefile,
# nor by the setup.py script, when they are preceded by a "*disabled*" tag
# line.

# Lines have the following structure:
#
# <module> ... [<sourcefile> ...] [<cpparg> ...] [<library> ...]
#
# <sourcefile> is anything ending in .c (.C, .cc, .c++ are C++ files)
# <cpparg> is anything starting with -I, -D, -U or -C
# <library> is anything ending in .a or beginning with -l or -L
# <module> is anything else but should be a valid Python
# identifier (letters, digits, underscores, beginning with non-digit)
#
# (As the makesetup script changes, it may recognize some other
# arguments as well, e.g. *.so and *.sl as libraries.  See the big
# case statement in the makesetup script.)
#
# Lines can also have the form
#
# <name> = <value>
#
# which defines a Make variable definition inserted into Makefile.in
#
# The build process works like this:
#
# 1. Build all modules that are declared as static in Modules/Setup,
#    combine them into libpythonxy.a, combine that into python.
# 2. Build all modules that are listed as shared in Modules/Setup.
# 3. Invoke setup.py. That builds all modules that
#    a) are not builtin, and
#    b) are not listed in Modules/Setup, and
#    c) can be build on the target
#
# Therefore, modules declared to be shared will not be
# included in the config.c file, nor in the list of objects to be
# added to the library archive, and their linker options won't be
# added to the linker options. Rules to create their .o files and
# their shared libraries will still be added to the Makefile, and
# their names will be collected in the Make variable SHAREDMODS.  This
# is used to build modules as shared libraries.  (They can be
# installed using "make sharedinstall", which is implied by the
# toplevel "make install" target.)  (For compatibility,
# *noconfig* has the same effect as *shared*.)
#
# NOTE: As a standard policy, as many modules as can be supported by a
# platform should be present.  The distribution comes with all modules
# enabled that are supported by most platforms and don't require you
# to ftp sources from elsewhere.


# Some special rules to define PYTHONPATH.
# Edit the definitions below to indicate which options you are using.
# Don't add any whitespace or comments!

# Directories where library files get installed.
# DESTLIB is for Python modules; MACHDESTLIB for shared libraries.
DESTLIB=$(LIBDEST)
MACHDESTLIB=$(BINLIBDEST)

# NOTE: all the paths are now relative to the prefix that is computed
# at run time!

# Standard path -- don't edit.
# No leading colon since this is the first entry.
# Empty since this is now just the runtime prefix.
DESTPATH=

# Site specific path components -- should begin with : if non-empty
SITEPATH=

# Standard path components for test modules
TESTPATH=

COREPYTHONPATH=$(DESTPATH)$(SITEPATH)$(TESTPATH)
PYTHONPATH=$(COREPYTHONPATH)


# The modules listed here can't be built as shared libraries for
# various reasons; therefore they are listed here instead of in the
# normal order.

# This only contains the minimal set of modules required to run the
# setup.py script in the root of the Python source tree.

posix -DPy_BUILD_CORE_BUILTIN -I$(srcdir)/Include/internal posixmodule.c # posix (UNIX) system calls
errno errnomodule.c         # posix (UNIX) errno values
pwd pwdmodule.c             # this is needed to find out the user's home dir
                    # if $HOME is not set
_sre _sre.c             # Fredrik Lundh's new regular expressions
_codecs _codecsmodule.c         # access to the builtin codecs and codec registry
_weakref _weakref.c         # weak references
_functools -DPy_BUILD_CORE_BUILTIN -I$(srcdir)/Include/internal _functoolsmodule.c   # Tools for working with functions and callable objects
_operator _operator.c               # operator.add() and similar goodies
_collections _collectionsmodule.c   # Container types
_abc _abc.c             # Abstract base classes
itertools itertoolsmodule.c     # Functions creating iterators for efficient looping
atexit atexitmodule.c           # Register functions to be run at interpreter-shutdown
_signal -DPy_BUILD_CORE_BUILTIN -I$(srcdir)/Include/internal signalmodule.c
_stat _stat.c               # stat.h interface
time -DPy_BUILD_CORE_BUILTIN -I$(srcdir)/Include/internal timemodule.c  # -lm # time operations and variables
_thread -DPy_BUILD_CORE_BUILTIN -I$(srcdir)/Include/internal _threadmodule.c    # low-level threading interface

# access to ISO C locale support
_locale -DPy_BUILD_CORE_BUILTIN _localemodule.c  # -lintl

# Standard I/O baseline
_io -DPy_BUILD_CORE_BUILTIN -I$(srcdir)/Include/internal -I$(srcdir)/Modules/_io _io/_iomodule.c _io/iobase.c _io/fileio.c _io/bytesio.c _io/bufferedio.c _io/textio.c _io/stringio.c

# faulthandler module
faulthandler faulthandler.c

# debug tool to trace memory blocks allocated by Python
#
# bpo-35053: The module must be builtin since _Py_NewReference()
# can call _PyTraceMalloc_NewReference().
_tracemalloc _tracemalloc.c hashtable.c

# The rest of the modules listed in this file are all commented out by
# default.  Usually they can be detected and built as dynamically
# loaded modules by the new setup.py script added in Python 2.1.  If
# you're on a platform that doesn't support dynamic loading, want to
# compile modules statically into the Python binary, or need to
# specify some odd set of compiler switches, you can uncomment the
# appropriate lines below.

# ======================================================================

# The Python symtable module depends on .h files that setup.py doesn't track
_symtable symtablemodule.c

# Uncommenting the following line tells makesetup that all following
# modules are to be built as shared libraries (see above for more
# detail; also note that *static* or *disabled* cancels this effect):

*static*

# GNU readline.  Unlike previous Python incarnations, GNU readline is
# now incorporated in an optional module, configured in the Setup file
# instead of by a configure script switch.  You may have to insert a
# -L option pointing to the directory where libreadline.* lives,
# and you may have to change -ltermcap to -ltermlib or perhaps remove
# it, depending on your system -- see the GNU readline instructions.
# It's okay for this to be a shared library, too.

readline readline.c -lreadline -ltermcap


# Modules that should always be present (non UNIX dependent):

array arraymodule.c    # array objects
cmath cmathmodule.c _math.c # -lm # complex math library functions
math mathmodule.c _math.c # -lm # math library functions, e.g. sin()
_contextvars _contextvarsmodule.c  # Context Variables
_struct _struct.c  # binary structure packing/unpacking
_weakref _weakref.c    # basic weak reference support
#_testcapi _testcapimodule.c    # Python C API test module
#_testinternalcapi _testinternalcapi.c -I$(srcdir)/Include/internal -DPy_BUILD_CORE_MODULE  # Python internal C API test module
_random _randommodule.c    # Random number generator
_elementtree -I$(srcdir)/Modules/expat -DHAVE_EXPAT_CONFIG_H -DUSE_PYEXPAT_CAPI _elementtree.c # elementtree accelerator
_pickle _pickle.c  # pickle accelerator
_datetime _datetimemodule.c    # datetime accelerator
_bisect _bisectmodule.c    # Bisection algorithms
_heapq _heapqmodule.c  # Heap queue algorithm
_asyncio _asynciomodule.c  # Fast asyncio Future
_json -I$(srcdir)/Include/internal -DPy_BUILD_CORE_BUILTIN _json.c # _json speedups
_statistics _statisticsmodule.c # statistics accelerator

unicodedata unicodedata.c    # static Unicode character database

_uuid _uuidmodule.c
_opcode _opcode.c
_multiprocessing _multiprocessing/multiprocessing.c _multiprocessing/semaphore.c

# Modules with some UNIX dependencies -- on by default:
# (If you have a really backward UNIX, select and socket may not be
# supported...)

fcntl fcntlmodule.c    # fcntl(2) and ioctl(2)
#spwd spwdmodule.c      # spwd(3)
grp grpmodule.c        # grp(3)
select selectmodule.c  # select(2); not on ancient System V

# Memory-mapped files (also works on Win32).
#mmap mmapmodule.c

# CSV file helper
_csv _csv.c

_posixsubprocess _posixsubprocess.c  # POSIX subprocess module helper
_blake2 _blake2/blake2module.c _blake2/blake2b_impl.c _blake2/blake2s_impl.c
binascii binascii.c
parser parsermodule.c

_queue _queuemodule.c
_md5 md5module.c
_sha1 sha1module.c
_sha256 sha256module.c
_sha512 sha512module.c
_sha3 _sha3/sha3module.c
_socket socketmodule.c

_lsprof _lsprof.o rotatingtree.c

pyexpat expat/xmlparse.c \
    expat/xmlrole.c \
    expat/xmltok.c \
    pyexpat.c \
    -I$(srcdir)/Modules/expat \
    -DHAVE_EXPAT_CONFIG_H -DUSE_PYEXPAT_CAPI -DXML_DEV_URANDOM

zlib zlibmodule.c -I$(prefix)/include -lz

_decimal _decimal/_decimal.c \
    _decimal/libmpdec/basearith.c \
    _decimal/libmpdec/constants.c \
    _decimal/libmpdec/context.c \
    _decimal/libmpdec/convolute.c \
    _decimal/libmpdec/crt.c \
    _decimal/libmpdec/difradix2.c \
    _decimal/libmpdec/fnt.c \
    _decimal/libmpdec/fourstep.c \
    _decimal/libmpdec/io.c \
    _decimal/libmpdec/memory.c \
    _decimal/libmpdec/mpdecimal.c \
    _decimal/libmpdec/numbertheory.c \
    _decimal/libmpdec/sixstep.c \
    _decimal/libmpdec/transpose.c \
    -I$(srcdir)/Modules/_decimal/libmpdec \
    -DCONFIG_64=1 -DANSI=1 -DHAVE_UINT128_T=1




*disabled*

# added

_crypt

_dbm
_gdbm
audioop
mmap
nis
resource
syslog
termios

_ctypes
_tkinter 
_curses
_curses_panel

xxlimited
xxsubtype
_xxsubinterpreters

_multibytecodec
_codecs_cn
_codecs_hk
_codecs_iso2022
_codecs_jp 
_codecs_kr 
_codecs_tw

_scproxy
#_posixshmem
_bz2
_lzma
_sqlite3


```

## packaging results (3.8.4)

method            | size  | works  | codesign  | standalone 
:---------------- | :---- | :----: | :-------: | :--------: 
bin-framework-ext |       |  0c    |           |      
bin-framework-pkg | 12+   |  1c    |           |   
src-framework-ext |       |  0c    |           |      
src-framework-pkg | 7.5   |  1c    | 1         |  0 
bin-homebrew-ext  | 8.5   |  1     | 0c        |  1 (py)
bin-homebrew-pkg  | 9.0   |  1     | 1c        |  0
bin-homebrew-sys  | 0.0   |  1     | 1c        |  1 (py)
src-static-ext    | 7.4   |  1     | 1         |  1 (py)
src-static-pkg    |       |        |           | 
src-shared-ext    |       |        |           | 
src-shared-pkg    |       |        |           | 


framework-ext has a build error (fix then retry) static-ext 



## design

The Homebrew copy of gettext leaks into the macOS build
if it is present. Uninstall gettext to make sure that doesn't happen.
```
sudo brew uninstall --ignore-dependencies gettext
```

simplified as much as possible

```
$tree -L 6 -n py.mxo
py.mxo
└── Contents
    ├── Frameworks
    │   ├── Python.framework
    │   │   ├── Python -> Versions/Current/Python
    │   │   ├── Resources -> Versions/Current/Resources
    │   │   └── Versions
    │   │       ├── 3.8
    │   │       │   ├── Python
    │   │       │   ├── Resources
    │   │       │   ├── include
    │   │       │   └── lib
    │   │       └── Current -> 3.8
    │   └── <non-python dylibs>
    ├── Info.plist
    ├── MacOS
    │   ├── <external>
    │   └── python
    ├── PkgInfo
    └── Resources
        └── lib
            ├── python3.8
            │   ├── lib-dynload
            │   │   └── <python .so extensions>
            │   └── site.py
            └── python38.zip

```



based on py2app

```
$tree -L 6 -n py.mxo
py.mxo
└── Contents
    ├── Frameworks
    │   ├── Python.framework
    │   │   ├── Python -> Versions/Current/Python
    │   │   ├── Resources -> Versions/Current/Resources
    │   │   └── Versions
    │   │       ├── 3.8
    │   │       │   ├── Python
    │   │       │   ├── Resources
    │   │       │   ├── include
    │   │       │   └── lib
    │   │       └── Current -> 3.8
    │   └── <non-python dylibs>
    ├── Info.plist
    ├── MacOS
    │   ├── <external>
    │   └── python
    ├── PkgInfo
    └── Resources
        ├── include
        │   └── python3.8
        │       └── <includes>
        ├── lib
        │   ├── python3.8
        │   │   ├── config-3.8-darwin
        │   │   ├── lib-dynload
        │   │   │   └── <python .so extensions>
        │   │   └── site.pyc -> ../../site.pyc
        │   └── python38.zip
        ├── site.pyc
        └── zlib.cpython-38-darwin.so
```


## py2app

I have a use-case for an open-source project (https://github.com/shakfu/py-js) for a plugin which provides for arbitrary python code in [max](https://cycling74.com/products/max) and requires that I embed the python interpreter (via the c-api and cython) into a c-based plugin or 'external' in max/msp parlance.

The operational code works well and is tested but I am continuously struggling to properly structure a Python.Framework-based build (against which the external is linked via @loader_path) which is embedded in the external in a manner which will pass the opaque code-signing and notarization steps.

To give a concrete examples, the following 'bundle' actually works in the application and is based on compiling python.org's 3.7.7 as a framework:

```
$ tree -L 7 -n py.mxo
py.mxo
└── Contents
    ├── Frameworks
    │   └── Python.framework
    │       ├── Python -> Versions/Current/Python
    │       ├── Resources -> Versions/Current/Resources
    │       └── Versions
    │           ├── 3.7
    │           │   ├── Python
    │           │   ├── Resources
    │           │   │   ├── English.lproj
    │           │   │   ├── Info.plist
    │           │   │   └── Python.app
    │           │   ├── bin
    │           │   │   ├── get_pip.sh
    │           │   │   ├── python3.7
    │           │   │   └── python3.7m-config
    │           │   ├── include
    │           │   │   └── python3.7m
    │           │   └── lib
    │           │       ├── libpython3.7.dylib -> ../Python
    │           │       ├── libpython3.7m.dylib -> ../Python
    │           │       ├── python3.7
    │           │       └── python37.zip
    │           └── Current -> 3.7
    ├── Info.plist
    ├── MacOS
    │   └── py
    ├── PkgInfo
    └── _CodeSignature
        └── CodeResources
```
Of course, this fails the code signing step for reasons that will be well known to you.

In any case, I stumbled upon py2app and saw for the first time you had managed to place 'lib' in Resources which is a condition of a well-formed bundle I believe.

So my question is: would it be possible to use py2app and/or macholib to address the embedding scenario above where I would need a full python library (or a specified subset thereof) but would not need an initial 'script' per se or to turn it into a .app as per the typical py2app use-case. In this sense, py2app would be more of a 'py2bundle' (-:




```
$tree -L 6 -n hello.app
hello.app
└── Contents
    ├── Frameworks
    │   ├── Python.framework
    │   │   ├── Python -> Versions/Current/Python
    │   │   ├── Resources -> Versions/Current/Resources
    │   │   └── Versions
    │   │       ├── 3.8
    │   │       │   ├── Python
    │   │       │   ├── Resources
    │   │       │   ├── include
    │   │       │   └── lib
    │   │       └── Current -> 3.8
    │   ├── libcrypto.1.1.dylib
    │   ├── libgdbm.6.dylib
    │   ├── liblzma.5.dylib
    │   └── libssl.1.1.dylib
    ├── Info.plist
    ├── MacOS
    │   ├── hello
    │   └── python
    ├── PkgInfo
    └── Resources
        ├── PythonApplet.icns
        ├── __boot__.py
        ├── __error__.sh
        ├── __pycache__
        │   └── site.cpython-38.pyc
        ├── hello.py
        ├── include
        │   └── python3.8
        │       └── pyconfig.h
        ├── lib
        │   ├── python3.8
        │   │   ├── config-3.8-darwin
        │   │   ├── lib-dynload
        │   │   │   ├── _asyncio.so
        │   │   │   ├── _bisect.so
        │   │   │   ├── _blake2.so
        │   │   │   ├── _bz2.so
        │   │   │   ├── _codecs_cn.so
        │   │   │   ├── _codecs_hk.so
        │   │   │   ├── _codecs_iso2022.so
        │   │   │   ├── _codecs_jp.so
        │   │   │   ├── _codecs_kr.so
        │   │   │   ├── _codecs_tw.so
        │   │   │   ├── _contextvars.so
        │   │   │   ├── _csv.so
        │   │   │   ├── _ctypes.so
        │   │   │   ├── _datetime.so
        │   │   │   ├── _dbm.so
        │   │   │   ├── _decimal.so
        │   │   │   ├── _elementtree.so
        │   │   │   ├── _gdbm.so
        │   │   │   ├── _hashlib.so
        │   │   │   ├── _heapq.so
        │   │   │   ├── _lzma.so
        │   │   │   ├── _md5.so
        │   │   │   ├── _multibytecodec.so
        │   │   │   ├── _multiprocessing.so
        │   │   │   ├── _opcode.so
        │   │   │   ├── _pickle.so
        │   │   │   ├── _posixshmem.so
        │   │   │   ├── _posixsubprocess.so
        │   │   │   ├── _queue.so
        │   │   │   ├── _random.so
        │   │   │   ├── _scproxy.so
        │   │   │   ├── _sha1.so
        │   │   │   ├── _sha256.so
        │   │   │   ├── _sha3.so
        │   │   │   ├── _sha512.so
        │   │   │   ├── _socket.so
        │   │   │   ├── _ssl.so
        │   │   │   ├── _struct.so
        │   │   │   ├── _testcapi.so
        │   │   │   ├── _tkinter.so
        │   │   │   ├── _uuid.so
        │   │   │   ├── array.so
        │   │   │   ├── binascii.so
        │   │   │   ├── fcntl.so
        │   │   │   ├── grp.so
        │   │   │   ├── math.so
        │   │   │   ├── mmap.so
        │   │   │   ├── pyexpat.so
        │   │   │   ├── resource.so
        │   │   │   ├── select.so
        │   │   │   ├── termios.so
        │   │   │   ├── unicodedata.so
        │   │   │   └── zlib.so
        │   │   └── site.pyc -> ../../site.pyc
        │   └── python38.zip
        ├── site.pyc
        └── zlib.cpython-38-darwin.so
```

## codesigning and notarization


see:

- http://www.zarkonnen.com/signing_notarizing_catalina
- https://stackoverflow.com/questions/56890749/macos-notarize-in-script
- https://github.com/rednoah/notarize-app/blob/master/notarize-app
- https://glyphsapp.com/tutorials/how-to-notarize-your-plug-ins
- https://developer.apple.com/forums/thread/129228
- https://developer.apple.com/forums/thread/128166
- https://developer.apple.com/forums/thread/129546
- https://developer.apple.com/forums/thread/129045
- https://developer.apple.com/documentation/macos-release-notes/macos-catalina-10_15-release-notes?preferredLanguage=occ
- https://github.com/pyinstaller/pyinstaller/wiki/Recipe-OSX-Code-Signing-Qt

## packaging results (3.7.7)

method            | size  | works  | codesign  | standalone 
:---------------- | :---- | :----: | :-------: | :--------: 
bin-framework-ext |       |  0c    |           |      
bin-framework-pkg | 12+   |  1c    |           |   
src-framework-ext |       |  0c    |           |      
src-framework-pkg | 7.5   |  1c    | 1         |  0 
bin-homebrew-ext  | 8.5   |  1     | 0c        |  1 (py)
bin-homebrew-pkg  | 8.5   |  1     | 1c        |  0
bin-homebrew-sys  | 0.0   |  1     | 1c        |  1 (py)
src-static-ext    | 7.4   |  1     | 1         |  1 (py)
src-static-pkg    |       |        |           | 
src-shared-ext    |       |        |           | 
src-shared-pkg    |       |        |           | 


framework-ext has a build error (fix then retry) static-ext 


## relocatable python

see: 

- https://github.com/gregneagle/relocatable-python

- https://github.com/beeware/Python-Apple-support


## max best practices

see: https://cycling74.com/forums/frameworks-in-external-vs-framework-loading-in-standalone-app




## dylib search and linking

see:

- https://stackoverflow.com/questions/24598047/why-does-ld-need-rpath-link-when-linking-an-executable-against-a-so-that-needs

- https://stackoverflow.com/questions/9798623/how-to-properly-set-run-paths-search-paths-and-install-names

- https://matthew-brett.github.io/docosx/mac_runtime_link.html

- https://github.com/trojanfoe/xcodedevtools

- http://lessons.livecode.com/m/4071/l/15029-linking-an-osx-external-bundle-with-a-dylib-library

- https://wincent.com/wiki/@executable_path,_@load_path_and_@rpath

- https://medium.com/@donblas/fun-with-rpath-otool-and-install-name-tool-e3e41ae86172

- https://www.mulle-kybernetik.com/weblog/2015/how_to_embed_a_framework_in_a.html

- https://stackoverflow.com/questions/12521802/print-rpath-of-an-executable-on-macos

- https://stackoverflow.com/questions/10021428/macos-how-to-link-a-dynamic-library-with-a-relative-path-using-gcc-ld


- https://stackoverflow.com/questions/9798623/how-to-properly-set-run-paths-search-paths-and-install-names


- https://developer.apple.com/library/archive/documentation/DeveloperTools/Conceptual/DynamicLibraries/000-Introduction/Introduction.html

## xcconfig -- Targets

variables:

BUNDLE_CONTENTS_FOLDER_PATH
BUNDLE_EXECUTABLE_FOLDER_PATH
BUNDLE_FORMAT
BUNDLE_FRAMEWORKS_FOLDER_PATH
BUNDLE_LOADER
BUNDLE_PLUGINS_FOLDER_PATH
BUNDLE_PRIVATE_HEADERS_FOLDER_PATH
BUNDLE_PUBLIC_HEADERS_FOLDER_PATH

COLOR_DIAGNOSTICS

CONFIGURATION

CURRENT_PROJECT_VERSION

CURRENT_VERSION

DEAD_CODE_STRIPPING

DEBUGGING_SYMBOLS

DEPLOYMENT_LOCATION
DEPLOYMENT_POSTPROCESSING

DEVELOPMENT_ASSET_PATHS

FRAMEWORK_SEARCH_PATHS
FRAMEWORK_VERSION

HEADER_SEARCH_PATHS

INFOPLIST_FILE

INSTALL_DIR also INSTALL_PATH

LD_DYLIB_INSTALL_NAME
DYLIB_INSTALL_NAME_BASE
LD_RUNPATH_SEARCH_PATHS

LIBRARY_SEARCH_PATHS

LINK_WITH_STANDARD_LIBRARIES

MAC_OS_X_VERSION_MIN_REQUIRED

OTHER_CFLAGS
OTHER_LDFLAGS
PRODUCT_NAME
PROJECT_NAME
PROJECT_DIR = $(SRCROOT)
SOURCE_ROOT = $(SRCROOT)

STRIP_INSTALLED_PRODUCT

CONTENTS_FOLDER_PATH

see:

- https://xcodebuildsettings.com

- https://nshipster.com/xcconfig/

- https://developer.apple.com/library/archive/technotes/tn2339/_index.html


see: https://pewpewthespells.com/blog/xcconfig_guide.html

1 xcconfig per target


## Packaging Options Test

### homebrew-pkg

```
make clean-pkg
make homebrew-pkg
```
Issues:

- copies `pyjs.mxo` that is part of another build (system)

Should be:


```
make clean clean-pkg homebrew-pkg
```




## Preparing a standalone for Apple App Store

https://cycling74.com/forums/max-standalone-in-the-mac-app-store-2018


## Python Distribution

for python 3.8

```
NAME=mxpython
PWD=$(pwd)
PREFIX=$PWD/$NAME

./configure MACOSX_DEPLOYMENT_TARGET=10.13 \
  --prefix=$PREFIX \
  --enable-shared \
  --with-universal-archs=64-bit \
  --with-lto \
  --enable-optimizations


```


## `sys.prefix`

A string giving the site-specific directory prefix where the platform independent Python files are installed; by default, this is the string '/usr/local'. This can be set at build time with the --prefix argument to the configure script. The main collection of Python library modules is installed in the directory prefix/lib/pythonX.Y while the platform independent header files (all except pyconfig.h) are stored in prefix/include/pythonX.Y, where X.Y is the version number of Python, for example 3.2.

Note If a virtual environment is in effect, this value will be changed in site.py to point to the virtual environment. The value for the Python installation will still be available, via base_prefix.


## `sys.exec_prefix`

A string giving the site-specific directory prefix where the platform-dependent Python files are installed; by default, this is also '/usr/local'. This can be set at build time with the --exec-prefix argument to the configure script. Specifically, all configuration files (e.g. the pyconfig.h header file) are installed in the directory exec_prefix/lib/pythonX.Y/config, and shared library modules are installed in exec_prefix/lib/pythonX.Y/lib-dynload, where X.Y is the version number of Python, for example 3.2.

Note If a virtual environment is in effect, this value will be changed in site.py to point to the virtual environment. The value for the Python installation will still be available, via base_exec_prefix.




## Max Package Elements
see: https://docs.cycling74.com/max7/vignettes/packages

You may create your own packages, either for your own use or for distribution to others. The folders (ending with a slash) and files comprising a package may optionally include the following (items in folders marked with a star will automatically be included in the searchpath):

```
* clippings/            Patchers to list in the "Paste From..." contextual
                        menu when patching

* code/                 Gen patchers

  collections/          Collections to list in the File Browser that are 
                        associated with the package

  default-definitions/  Definition info for Object Defaults support in UI externals

  default-settings/     Saved color schemes for Object Defaults

* docs/                 Reference pages and Vignettes to be accessible from
                        the Documentation Window

* examples/             Example patchers and supporting material

* extensions/           Special external objects loaded on Max launch

* externals/            External objects

* extras/               Patchers to be listed in the "Extras" menu

* help/                 Help patchers and supporting material

  icon.png              A PNG graphic file (500x500px) for display in 
                        the Package Manager

  init/                 Text files interpreted by Max at launch

  interfaces/           Supporting files for objects to display in the top
                        patcher toolbar and other Max integration.

* java-classes/         Compiled Java classes for use in mxj/mxj~. Place .jar 
                        folders in a 'lib' subfolder.

  java-doc/             Documentation for Java classes

* javascript/           Javascript files to be used by js

* jsextensions/         Extensions to JS implemented as special externals or js files

* jsui/                 Javascript files to be used by jsui, and listed in 
                        the contextual menu for jsui

  license.txt|md        Terms of use / redistribution of your package
                        (plain text or Markdown permitted)

* media/                Media files to be included in the searchpath

* misc/                 Anything

* patchers/             Patchers or abstractions to be included in the searchpath

* object-icons/         An SVG-format object icon for a particular Max object
                        (named <objectname>.svg), used in the Object Browser

  object-prototypes/    Object Prototypes will be listed in the context menu for 
                        a selected UI object

  readme.txt|md         Information about your package (text or Markdown permitted)

  snippets/             Snippets associated with this package

  source/               Source code for external objects, ignored by Max

  support/              Special location for DLL or dylib dependencies of 
                        external objects. Added to the DLL search path on Windows.

  templates/            Patchers to be listed in the "File > New From Template" menu

```

## Directory Structure

- py
    - clippings
    - docs
        - refpages (.maxref.xml)
    - externals (.mxo)
    - extras
    - fonts (.otf)
    - help (.maxhelp)
    - interfaces (svg)
    - jsextensions (.mxo / .js)
    - jsui (.js)
    - media (.png/...)
    - object-icons (.svg)
    - object-prototypes (.maxproto)
    - patchers
    - snippets (.maxsnip)
    - styles (.maxstyle)
    - templates

