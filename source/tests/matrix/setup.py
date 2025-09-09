import os
import platform
from pathlib import Path
from setuptools import setup, Extension
from Cython.Build import cythonize

def getenv(variable, default=True):
    return bool(int((os.getenv(variable, default))))

# ----------------------------------------------------------------------------
# Constants

DEBUG = getenv("DEBUG", default=False)
PLATFORM = platform.system()

# ----------------------------------------------------------------------------
# Common

ROOT = Path(__file__).parent

LIBRARIES = []
INCLUDE_DIRS = []
LIBRARY_DIRS = []
EXTRA_OBJECTS = []
EXTRA_LINK_ARGS = []
EXTRA_COMPILE_ARGS = []
DEFINE_MACROS = []


# ----------------------------------------------------------------------------
# Extension Configuration

extensions = [
    Extension("matrix", 
        sources = ["matrix.pyx"],
        libraries=LIBRARIES,
        include_dirs=INCLUDE_DIRS,
        library_dirs=LIBRARY_DIRS,
        extra_objects=EXTRA_OBJECTS,
        extra_link_args=EXTRA_LINK_ARGS,
        extra_compile_args=EXTRA_COMPILE_ARGS,
        define_macros=DEFINE_MACROS,
    )
]


setup(
    name="matrix",
    ext_modules=cythonize(extensions,
        compiler_directives={
            'language_level' : '3',
            'binding': True,             # default: True
            'boundscheck': True,         # default: True
            'wraparound': True,          # default: True
            'initializedcheck': True,    # default: True
            'nonecheck': False,          # default: False
            'overflowcheck': False,      # default: False
            'overflowcheck.fold': True,  # default: True
            'embedsignature': False,     # default: False
            'cdivision': False,          # default: False
            'emit_code_comments': False, # default: True
            'annotation_typing': True,   # default: True
            'warn.undeclared': False,    # default: False
            'warn.unreachable': True,    # default: True
            'warn.unused': False,        # default: False
            'warn.unused_arg': False,    # default: False
            'warn.unused_result': False, # default: False
        },
    ),
)
