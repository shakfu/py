# API Module Comprehensive Guide

The `api` module is a Cython-based wrapper that provides Python access to Max/MSP's C-API. It enables Python scripts to directly control Max objects, manipulate data structures, and interact with the Max environment.

## Table of Contents

- [Overview](#overview)
- [Module Architecture](#module-architecture)
  - [Core Cython Files](#core-cython-files)
  - [Namespace Organization](#namespace-organization)
- [Extension Classes](#extension-classes)
  - [Core Classes](#core-classes)
    - [MaxObject](#maxobject)
    - [Object](#object)
  - [Data Structure Classes](#data-structure-classes)
    - [Atom](#atom)
    - [AtomArray](#atomarray)
    - [Dictionary](#dictionary)
    - [Database](#database)
    - [Hashtab, List, Binbuf](#hashtab-list-binbuf)
  - [Audio/MSP Classes](#audiomsp-classes)
    - [Buffer](#buffer)
  - [Jitter/Matrix Classes](#jittermatrix-classes)
    - [Matrix](#matrix)
  - [Simple Wrapper Classes](#simple-wrapper-classes)
  - [External Interface Classes](#external-interface-classes)
    - [PyExternal](#pyexternal)
    - [Patcher](#patcher)
    - [Box](#box)
- [Utility Functions](#utility-functions)
  - [Console and Debugging](#console-and-debugging)
  - [Object Communication](#object-communication)
  - [Patcher Access](#patcher-access)
  - [Buffer/Audio Utilities](#bufferaudio-utilities)
  - [Path and File System](#path-and-file-system)
  - [Type Conversion](#type-conversion)
- [Usage Examples](#usage-examples)
  - [Basic API Import and Usage](#basic-api-import-and-usage)
  - [Working with Buffers](#working-with-buffers)
  - [Matrix/Jitter Integration](#matrixjitter-integration)
  - [Dictionary Operations](#dictionary-operations)
  - [Patcher Manipulation](#patcher-manipulation)
  - [Inter-object Communication](#inter-object-communication)
  - [Real-time Audio Processing](#real-time-audio-processing)
- [Buffer Protocol Integration](#buffer-protocol-integration)
  - [Matrix Buffer Protocol Features](#matrix-buffer-protocol-features)
  - [Supported Matrix Types](#supported-matrix-types)
  - [Buffer Protocol Implementation Details](#buffer-protocol-implementation-details)
- [Advanced Features](#advanced-features)
  - [Error Handling and Safety](#error-handling-and-safety)
  - [Memory Management](#memory-management)
  - [Performance Considerations](#performance-considerations)
  - [Integration with Max Scheduling](#integration-with-max-scheduling)
- [Development Notes](#development-notes)
  - [Import Methods](#import-methods)
  - [Header File Organization](#header-file-organization)
  - [Implementation Status](#implementation-status)
  - [Matrix Implementation Details](#matrix-implementation-details)
  - [Workarounds for Non-API Objects](#workarounds-for-non-api-objects)
  - [Performance Optimization](#performance-optimization)
  - [Error Handling Strategy](#error-handling-strategy)
  - [Future Development](#future-development)
  - [Integration Testing](#integration-testing)
  - [Documentation Resources](#documentation-resources)

## Overview

The `api` module bridges Python and Max's C-API through Cython, providing:

- **Direct Max object manipulation** from Python scripts
- **Type-safe wrappers** for Max data structures
- **Buffer protocol support** for efficient NumPy integration
- **Memory management** that prevents crashes
- **Comprehensive coverage** of Max, MSP, and Jitter APIs

**Key Benefits:**

- Write Max object controllers in Python
- Seamless NumPy integration with audio buffers and matrices
- Error handling that doesn't crash Max
- Access to the full Max API ecosystem

## Module Architecture

The `api` module consists of several components:

### Core Cython Files

- **`api.pyx`** - Main Cython implementation with extension classes and functions
- **`api_max.pxd`** - Max C-API header declarations
- **`api_msp.pxd`** - MSP (audio) C-API header declarations
- **`api_jit.pxd`** - Jitter (video/matrix) C-API header declarations
- **`api_py.pxd`** - py external C-API header declarations

### Namespace Organization

```python
cimport api_max as mx    # Max core functions
cimport api_msp as mp    # MSP audio functions
cimport api_jit as jt    # Jitter matrix functions
cimport api_py as px     # py external functions
```

## Extension Classes

### Core Classes

#### MaxObject

Base wrapper for Max `t_object*` pointers providing fundamental object operations.

```python
class MaxObject:
    """Base wrapper for Max t_object* objects."""

    # Core functionality inherited by all wrapper classes
    def get_name(self) -> str
    def send_message(self, message: str, *args)
    def get_attribute(self, attr_name: str)
    def set_attribute(self, attr_name: str, value)
```

#### Object

Generic object wrapper that can instantiate any Max object class.

```python
# Create any Max object
mycoll = api.Object("coll", "mycoll", embed=True)
mytable = api.Object("table", "mytable", 128)
mymetro = api.Object("metro", 1000)
```

### Data Structure Classes

#### Atom

Wrapper for Max's fundamental data type `t_atom`.

```python
class Atom:
    """Wrapper for Max t_atom* atoms/messages."""

    def __init__(self, value=None)
    def get_type(self) -> str          # Returns: 'long', 'float', 'sym'
    def get_long(self) -> int
    def get_float(self) -> float
    def get_symbol(self) -> str
    def set_long(self, value: int)
    def set_float(self, value: float)
    def set_symbol(self, value: str)
```

#### AtomArray

Wrapper for arrays of atoms; useful for message passing.

```python
class AtomArray:
    """Interface to Max atom arrays."""

    def __init__(self, size: int)
    def __len__(self) -> int
    def __getitem__(self, index: int) -> Atom
    def __setitem__(self, index: int, value)
    def to_list(self) -> list
    def from_list(self, data: list)
```

#### Dictionary

Interface to Max dictionary objects for key-value storage.

```python
class Dictionary:
    """Interface to Max dictionaries."""

    def __init__(self, name: str = None)
    def get(self, key: str)
    def set(self, key: str, value)
    def remove(self, key: str)
    def get_keys(self) -> list
    def get_size(self) -> int
    def clear()
    def post()                         # Print contents to Max window
    def register(self, name: str)      # Register in global namespace
    def to_dict(self) -> dict          # Convert to Python dict
    def from_dict(self, data: dict)    # Populate from Python dict
```

#### Database

Interface to Max database objects for structured data storage.

```python
class Database:
    """Interface to Max databases."""

    def __init__(self, name: str = None)
    def query(self, sql: str) -> DatabaseResult
    def execute(self, sql: str)
    def get_table_names(self) -> list
    def close()
```

#### Hashtab, List, Binbuf

Additional data structure wrappers for specialized Max objects.

```python
class Hashtab:         # Hash table implementation
class List:            # Linked list implementation
class Binbuf:          # Binary buffer for Max messages
class Atombuf:         # Specialized atom buffer
```

### Audio/MSP Classes

#### Buffer

Comprehensive interface to MSP buffer~ objects with NumPy integration.

```python
class Buffer:
    """Interface to MSP buffers with NumPy support."""

    def __init__(self, name: str)

    # Basic properties
    def get_name(self) -> str
    def get_length_ms(self) -> float
    def get_length_samples(self) -> int
    def get_channels(self) -> int
    def get_sample_rate(self) -> float

    # Data access
    def get_samples(self, channel: int = 0) -> list
    def set_samples(self, data, channel: int = 0)
    def get_sample(self, index: int, channel: int = 0) -> float
    def set_sample(self, index: int, value: float, channel: int = 0)

    # File operations
    def read_file(self, filepath: str)
    def write_file(self, filepath: str)

    # Utility methods
    def clear()
    def normalize()
    def reverse()
    def resize(self, length_ms: float)
```

### Jitter/Matrix Classes

#### Matrix

Advanced wrapper for Jitter matrices with full buffer protocol support.

```python
class Matrix:
    """Interface to Jitter matrices with NumPy integration."""

    def __init__(self, name: str)

    # Matrix properties
    def get_info(self) -> dict
    def get_dimensions(self) -> tuple
    def get_planecount(self) -> int
    def get_type(self) -> str           # 'char', 'long', 'float32', 'float64'

    # Data access via buffer protocol
    def __array__(self)                 # NumPy array conversion
    def as_float32_memoryview(self)     # Typed memoryview access
    def as_float64_memoryview(self)
    def as_char_memoryview(self)
    def as_long_memoryview(self)

    # Matrix operations
    def clear()
    def setall(self, value)
    def get_cell(self, *coords)
    def set_cell(self, *coords, value)
```

### Simple Wrapper Classes

High-level wrappers that inherit from `Object` for common Max objects.

```python
class Table(Object):
    """Wrapper for Max table objects."""
    def get_size(self) -> int
    def get_value(self, index: int) -> float
    def set_value(self, index: int, value: float)
    def get_values(self) -> list
    def set_values(self, data: list)

class Coll(Object):
    """Wrapper for Max coll objects."""
    def clear()
    def insert(self, index: int, *values)
    def delete(self, index: int)
    def get_size(self) -> int

class Array(Object):
    """Wrapper for Max array objects."""
    def get_size(self) -> int
    def get_data(self) -> list
    def set_data(self, data: list)

class Max(Object):
    """Wrapper for the Max application."""
    def get_version(self) -> str
    def get_maxpath(self) -> str
    def refresh_browser()
```

### External Interface Classes

#### PyExternal

Direct interface to the `py` external's internal state and methods.

```python
class PyExternal:
    """Main interface for the py external."""

    def get_name(self) -> str
    def get_outlet_count(self) -> int
    def get_inlet_count(self) -> int
    def scan_objects()                 # Scan patcher for named objects
    def send_message(self, obj_name: str, *args)
    def get_patcher() -> Patcher
```

#### Patcher

Interface to Max patcher objects for patch manipulation.

```python
class Patcher:
    """Interface to Max patchers."""

    def get_name(self) -> str
    def get_filepath(self) -> str
    def get_box_count(self) -> int
    def get_object_count(self) -> int
    def add_object(self, x: int, y: int, classname: str, *args) -> Box
    def remove_object(self, obj: Box)
    def get_objects(self) -> list[Box]
    def save()
    def close()
```

#### Box

Interface to Max box objects (visual representations of objects).

```python
class Box:
    """Interface to Max boxes/objects."""

    def get_object(self) -> MaxObject
    def get_position(self) -> tuple[int, int]
    def set_position(self, x: int, y: int)
    def get_size(self) -> tuple[int, int]
    def set_size(self, width: int, height: int)
    def get_text(self) -> str
    def set_text(self, text: str)
```

## Utility Functions

The `api` module provides numerous utility functions for common Max operations:

### Console and Debugging

```python
def post(message: str)               # Post message to Max window
def error(message: str)              # Post error message to Max window
def hello()                          # Test function - posts "hello world"
def get_globals() -> dict            # Get py external's global namespace
```

### Object Communication

```python
def send(name: str, *args)           # Send message to named object
def lookup(name: str) -> MaxObject   # Find object by name
def scan_objects() -> dict           # Scan patcher for all named objects
def bang()                           # Send bang from py external
def bang_success()                   # Send success bang
def bang_failure()                   # Send failure bang
def out(obj)                         # Output object from left outlet
def out2(obj)                        # Output object from middle outlet
```

### Patcher Access

```python
def get_patcher() -> Patcher         # Get current patcher
def get_max() -> Max                 # Get Max application instance
```

### Buffer/Audio Utilities

```python
def get_buffer(name: str) -> Buffer                    # Get existing buffer
def create_buffer(name: str, file: str) -> Buffer      # Create buffer from file
def create_empty_buffer(name: str, ms: int) -> Buffer  # Create empty buffer
```

### Path and File System

```python
def resources_dir() -> str           # Get resources directory path
def support_dir() -> str             # Get support directory path
```

### Type Conversion

```python
def fourchar_to_int(code: str) -> int    # Convert fourchar code to int
def int_to_fourchar(n: int) -> str       # Convert int to fourchar code
```

## Usage Examples

### Basic API Import and Usage

```python
# In a py external
import api

# Post to Max window
api.post("Hello from Python!")

# Get current patcher info
patcher = api.get_patcher()
api.post(f"Patcher: {patcher.get_name()}")

# Scan for objects in patcher
objects = api.scan_objects()
api.post(f"Found {len(objects)} named objects")
```

### Working with Buffers

```python
import api
import numpy as np
from scipy import signal

# Create or get a buffer
buf = api.create_empty_buffer("mybuffer", 1000)  # 1 second at default SR

# Generate a sine wave
sr = 44100
duration = 1.0
t = np.linspace(0, duration, int(sr * duration), False)
sine_wave = np.sin(2 * np.pi * 440 * t)  # 440 Hz

# Set buffer contents
buf.set_samples(sine_wave.tolist())

# Read back as NumPy array for processing
samples = np.array(buf.get_samples())

# Apply effects
filtered = signal.butter(4, 1000, 'low', fs=sr, output='sos')
processed = signal.sosfilt(filtered, samples)

# Write back to buffer
buf.set_samples(processed.tolist())
```

### Matrix/Jitter Integration

```python
import api
import numpy as np

# Get a named jitter matrix
matrix = api.Matrix("mymatrix")

# Convert to NumPy array using buffer protocol
np_array = np.array(matrix)

# Process with NumPy
processed = np.rot90(np_array)

# Create new matrix and set data
# Note: Direct assignment back to matrix is limited
# Usually done through jit.matrix object messages
```

### Dictionary Operations

```python
import api

# Create a dictionary
d = api.Dictionary("mydict")

# Set values
d.set("tempo", 120)
d.set("key", "C major")
d.set("notes", [60, 64, 67, 72])

# Get values
tempo = d.get("tempo")
notes = d.get("notes")

# Convert to Python dict for easy manipulation
py_dict = d.to_dict()
py_dict["modified"] = True

# Update dictionary from Python dict
d.from_dict(py_dict)

# Register in global namespace
d.register("globals")
```

### Patcher Manipulation

```python
import api

# Get current patcher
p = api.get_patcher()

# Add objects programmatically
metro = p.add_object(100, 100, "metro", 1000)
print_box = p.add_object(100, 150, "print", "output")

# Connect objects (requires additional connection API)
# This would typically be done through Max object messages

# Get all objects
all_objects = p.get_objects()
api.post(f"Patcher has {len(all_objects)} objects")
```

### Inter-object Communication

```python
import api

# Scan patcher for named objects
api.scan_objects()

# Send messages to named objects
api.send("mydac~", "start")           # Start audio
api.send("myslider", "set", 127)      # Set slider value
api.send("mytable", "clear")          # Clear table contents

# Advanced: send to multiple objects
for i in range(8):
    api.send(f"channel{i}", "mute", 1)
```

### Real-time Audio Processing

```python
import api
import numpy as np

def process_audio_realtime():
    """Example of real-time buffer processing."""

    # Get input and output buffers
    input_buf = api.get_buffer("input~")
    output_buf = api.get_buffer("output~")

    # Read current buffer contents
    samples = np.array(input_buf.get_samples())

    # Apply processing (example: simple gain)
    gain = 0.5
    processed = samples * gain

    # Clip to prevent overflow
    processed = np.clip(processed, -1.0, 1.0)

    # Write to output buffer
    output_buf.set_samples(processed.tolist())

    # Trigger output
    api.send("output~", "bang")

# This would typically be called from a Max scheduler or timer
```

## Buffer Protocol Integration

The `Matrix` class implements Python's buffer protocol, enabling efficient data exchange with NumPy and other libraries.

### Matrix Buffer Protocol Features

```python
import api
import numpy as np

# Get a matrix object
matrix = api.Matrix("mymatrix")

# Automatic NumPy conversion via buffer protocol
np_array = np.array(matrix)

# Typed memoryview access for different data types
if matrix.get_type() == "float32":
    mv = matrix.as_float32_memoryview()
elif matrix.get_type() == "char":
    mv = matrix.as_char_memoryview()

# Zero-copy access to matrix data
# Changes to memoryview directly affect matrix data
mv[0, 0] = 255  # Modify matrix directly
```

### Supported Matrix Types

- **char** — 8-bit unsigned integers (0-255)
- **long** — 32-bit signed integers
- **float32** — 32-bit floating point
- **float64** — 64-bit floating point

### Buffer Protocol Implementation Details

The Matrix class provides:

- **Shape information** — Proper n-dimensional array shape
- **Stride information** — Memory layout for efficient access
- **Type information** — NumPy-compatible format strings
- **Error handling** — Safe memory access with bounds checking

```python
# Example: Accessing buffer protocol information
matrix = api.Matrix("mymatrix")
info = matrix.get_info()

print(f"Dimensions: {matrix.get_dimensions()}")
print(f"Type: {matrix.get_type()}")
print(f"Planes: {matrix.get_planecount()}")

# Direct memoryview manipulation
mv = matrix.as_float32_memoryview()
print(f"Shape: {mv.shape}")
print(f"Strides: {mv.strides}")
```

## Advanced Features

### Error Handling and Safety

The API module includes comprehensive error handling:

```python
import api

try:
    # Attempt to access non-existent buffer
    buf = api.get_buffer("nonexistent")
except ValueError as e:
    api.error(f"Buffer error: {e}")

try:
    # Attempt to access invalid matrix
    matrix = api.Matrix("invalid")
except ValueError as e:
    api.error(f"Matrix error: {e}")
```

### Memory Management

The API automatically handles memory management for Max objects:

- **Automatic cleanup** — Objects are properly released when Python references are deleted
- **Safe pointer access** — Null pointer checks prevent crashes
- **Reference counting** — Proper handling of Max object lifetimes

### Performance Considerations

For optimal performance:

```python
# Good: Reuse matrix references
matrix = api.Matrix("mymatrix")
for i in range(1000):
    data = np.array(matrix)  # Buffer protocol is efficient
    # process data...

# Better: Use memoryviews for repeated access
matrix = api.Matrix("mymatrix")
mv = matrix.as_float32_memoryview()
for i in range(1000):
    # Direct memory access, no copying
    value = mv[i, j]
```

### Integration with Max Scheduling

```python
import api

def scheduled_function():
    """Function to be called by Max scheduler."""
    # Do processing
    buffer = api.get_buffer("processing~")
    samples = buffer.get_samples()

    # Process and update
    # ...

    # Schedule next call
    api.send("metro", "bang")

# From Max: [py sched 1000 scheduled_function]
```

## Development Notes

### Import Methods

There are several ways to access the API module in Python code:

1. **Direct import message**:

   ```text
   [py] ← [import api]
   ```

2. **Python import in loaded script**:

   ```python
   # In your Python file
   import api
   api.post("API loaded")
   ```

3. **Load file with API usage**:

   ```text
   [py] ← [load script.py]
   ```

4. **Use with message syntax**:

   ```text
   [py] ← [api.post hello world]
   ```

### Header File Organization

The API module's header mappings are organized in separate `.pxd` files:

#### api_max.pxd

Maps core Max C-API functions and structures:

```python
cimport api_max as mx

# Usage in Cython code:
mx.gensym()      # Max symbol creation
mx.post()        # Console output
mx.error()       # Error reporting
mx.object_new()  # Object creation
```

#### api_msp.pxd

Maps MSP (audio) C-API functions:

```python
cimport api_msp as mp

# Usage for audio operations:
mp.buffer_ref_new()     # Buffer reference
mp.buffer_getsamples()  # Sample access
```

#### api_jit.pxd

Maps Jitter (matrix/video) C-API functions:

```python
cimport api_jit as jt

# Usage for matrix operations:
jt.jit_object_method()     # Generic object method
jt.jit_matrix_getinfo()    # Matrix information
```

#### api_py.pxd

Maps the py external's internal API:

```python
cimport api_py as px

# Usage for py external integration:
px.py_scan()           # Scan patcher objects
px.py_send()           # Send to named objects
```

### Implementation Status

#### Completed Extension Classes

- **MaxObject** - Base object wrapper with generic functionality
- **Atom** - Type-safe atom manipulation
- **Buffer** - Full MSP buffer integration with NumPy support
- **Dictionary** - Complete dictionary API with Python dict conversion
- **Database/DatabaseResult/DatabaseView** - Database integration
- **List/Binbuf/Atombuf/Hashtab** - Data structure wrappers
- **AtomArray** - Array handling with Python list conversion
- **Patcher** - Patcher manipulation and object creation
- **Box** - Visual object representation and control
- **Matrix** - Advanced Jitter matrix with buffer protocol
- **PyExternal** - Direct py external interface
- **Path** - File system path handling

#### Simple Wrapper Classes

- **Object** - Generic Max object instantiation
- **Max** - Max application interface
- **Table** - Max table object wrapper
- **Coll** - Collection object wrapper
- **Array** - Array object wrapper

### Matrix Implementation Details

The Matrix class implements advanced buffer protocol support:

#### Buffer Protocol Methods

```python
def __getbuffer__(self, Py_buffer *buffer, int flags)
def __releasebuffer__(self, Py_buffer *buffer)
```

#### Typed Memoryview Methods

```python
def as_float32_memoryview(self)
def as_float64_memoryview(self)
def as_char_memoryview(self)
def as_long_memoryview(self)
def as_memoryview(self)  # Generic access
```

#### Matrix Method Coverage

The Matrix class provides access to Jitter matrix methods through `jit_object_method()` calls, since many matrix functions are not directly exported:

**Available Operations:**

- Matrix info retrieval (`getinfo`)
- Data access (`getdata`, `setdata`)
- Cell manipulation (`setcell`, `getcell`)
- Bulk operations (`setall`, `clear`)
- Plane operations (`setplane`, `fillplane`)

**Note:** Many jit_matrix functions require `jit_object_method()` calls rather than direct function access, as documented in the Max SDK.

### Workarounds for Non-API Objects

Some Max objects don't expose C-API access and require workarounds:

#### coll Object

```python
# Indirect access via dictionary messages
api.send("mycoll", "dump")              # Export to dict
api.send("mycoll", "read", "data.txt")  # Load from file
```

#### jit.cellblock

```python
# Link with coll for data transfer
api.send("mycoll", "refer", "mycellblock")
api.send("mycoll", "insert", 0, 1, 2, 3)  # Data appears in cellblock
```

#### jit.matrix Population

```python
# Use jit.fill from coll data
api.send("mycoll", "dump")
api.send("jit.fill", "fromcoll", "mycoll")
```

### Performance Optimization

#### Memory Management

- **Object lifecycle** — Automatic cleanup when Python references are released
- **Pointer safety** — Null checks prevent crashes
- **Reference counting** — Proper Max object lifetime management

#### Buffer Protocol Efficiency

- **Zero-copy access** — Direct memory access via memoryviews
- **Type safety** — Compile-time type checking for matrix data
- **NumPy integration** — Efficient array conversion without copying

#### Best Practices

```python
# Efficient: Reuse object references
buffer = api.get_buffer("mybuffer")
for i in range(1000):
    samples = buffer.get_samples()  # Cached reference

# More efficient: Use memoryviews for repeated access
matrix = api.Matrix("mymatrix")
mv = matrix.as_float32_memoryview()
for i in range(height):
    for j in range(width):
        value = mv[i, j]  # Direct memory access
```

### Error Handling Strategy

The API implements comprehensive error handling:

#### Safe Pointer Access

```python
if self.ptr is NULL:
    raise ValueError("Invalid object pointer")
```

#### Type Validation

```python
if not isinstance(value, (int, float)):
    raise TypeError("Expected numeric value")
```

#### Max Integration

```python
try:
    result = api.dangerous_operation()
except Exception as e:
    api.error(f"Operation failed: {e}")
    api.bang_failure()  # Signal failure to Max
```

### Future Development

#### Priority Improvements

- **Object method unification** — Refactor duplicated `object_method_typed` calls across classes
- **Connection API** — Enable programmatic patch cable creation
- **More wrapper classes** — Additional coverage of Max object types
- **Performance optimization** — Further buffer protocol enhancements

#### Extension Areas

- **OpenGL/GPU integration** — Direct GPU memory access for matrices
- **Real-time audio** — Lower-latency buffer access methods
- **MIDI integration** — Comprehensive MIDI object wrappers
- **Network objects** — UDP/TCP object integration
- **Database expansion** — More sophisticated SQL integration

### Integration Testing

The API module is tested through:

- **Max patches** — Located in `patchers/tests/test_api/`
- **Python examples** — Located in `examples/tests/`
- **Unit tests** — Cython-level validation
- **Integration tests** — Max/Python round-trip testing

### Documentation Resources

- **API reference** — Complete class and method documentation in docstrings
- **Example patches** — Practical usage demonstrations in Max
- **Python examples** — Real-world integration scenarios
- **Max SDK documentation** — Original C-API reference materials
