# setup.py - Build configuration for the Python extension
from setuptools import setup, Extension
import sys

# Define the extension module
math_extension = Extension(
    'math_extension',
    sources=['math_extension.c'],
    include_dirs=['.'],  # Include current directory for py_function_cache.h
    extra_compile_args=[
        '-std=c99',
        '-O2',
        '-Wall',
        '-Wextra'
    ] + (['-pthread'] if sys.platform != 'win32' else []),
    extra_link_args=['-pthread'] if sys.platform != 'win32' else [],
)

setup(
    name='math_extension',
    version='1.0.0',
    description='Example Python C extension with function caching',
    author='Function Cache System',
    ext_modules=[math_extension],
    python_requires='>=3.6',
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
    ],
)
