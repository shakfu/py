"""api.pyx

This is a 'builtin' module, written in cython, which wraps and exposes parts of
the Max/MSP c-api python code while using `py` external.

Extension Classes:
- MaxObject: generic wrapper for Max t_object* objects
- Atom: wrapper for Max mx.t_atom* atoms/messages
- Buffer: wrapper for MSP buffers
- Dictionary: wrapper for Max dictionaries
- DatabaseView: wrapper for Max database views
- DatabaseResult: wrapper for Max database results
- Database: wrapper for Max databases
- List: wrapper for Max linked lists
- Binbuf: wrapper for Max binbufs
- Atombuf: wrapper for Max atom buffers
- Hashtab: wrapper for Max hash tables
- AtomArray: wrapper for Max atom arrays
- Patcher: wrapper for Max patchers
- Box: wrapper for Max boxes/objects
- Path: wrapper for Max path handling
- PyExternal: wrapper for the `py` external
- PyMxObject: Alternative `py` external extension type (obj pointer retrieved via uintptr_t)

WIP Extension Classes:
- Matrix: wrapper for Max jit matrices -- needs work to convert from and to via buffer protocol


Simple Wrappers:
    These are semi-generated wrappers (using the scripts/maxref.py -c <name>) which inherit
    from the `Object` extension class, which itself inherits from `MaxObject`.
- Object(MaxObject): the superclass for simple max objects
- Max(Object): wrapper for the Max application
- Table(Object): wrapper for Max tables
- Coll(Object): wrapper for max coll objects
- Array(Object): wrapper around `array` object

Helper Functions:
- Global utility functions for common Max operations
- Type conversion between Python and Max data types
- Memory management utilities
- Message passing and scripting helpers

The module provides both low-level Cython access to the C API as well as higher-level
Python wrappers for use in python scripts run by the `py` external.

see: `py-js/source/projects/py/api.md` for further details
     `py-js/examples/tests` and `py-js/patchers/tests/test_api` for examples of using
      the `api` module in python code and Max patchers respectively.
"""

# ----------------------------------------------------------------------------
# imports

import pathlib
from math import prod as product
from collections import namedtuple
from typing import Optional

from cython.view cimport array as cvarray
from cpython.ref cimport PyObject
from cpython cimport Py_buffer
from libc.string cimport strcpy, strlen

cimport api_max as mx  # api is a cython keyword!
cimport api_msp as mp
cimport api_jit as jt
cimport api_py as px

# ----------------------------------------------------------------------------
# compile-time conditional imports

cdef extern from *:
    """
    #define INCLUDE_NUMPY 0
    """
    bint INCLUDE_NUMPY

if INCLUDE_NUMPY:
    import numpy as np
    cimport numpy as np
    np.import_array()

# ----------------------------------------------------------------------------
# compile-time constants

cpdef enum:
    MAX_FILENAME_CHARS = 512
    MAX_PATH_CHARS = 2048
    # MAX_CHARS = 32767
    # PY_MAX_ATOMS = 1024

# ----------------------------------------------------------------------------
# run-time constants

DEBUG = 0

# ----------------------------------------------------------------------------
# python c-api imports

# TODO: can't this be imported from cimport!
cdef extern from "Python.h":
    const char* PyUnicode_AsUTF8(object unicode)
    unicode PyUnicode_FromString(const char *u)

# ----------------------------------------------------------------------------
# helper cdef functions

cdef mx.t_symbol* gensym(str name):
    """converts python string to a t_symbol*"""
    return mx.gensym(name.encode())


cdef mx.t_symbol* str_to_sym(str string):
    """converts a python string to a t_symbol*

    gensym(str s) -> t_symbol*
    """
    return mx.gensym(string.encode())


cdef str sym_to_str(mx.t_symbol* symbol):
    """converts a max symbol to a python string"""
    return symbol.s_name.decode()


cdef mx.t_symbol* bytes_to_sym(bytes string):
    """converts a python string to a t_symbol*"""
    return mx.gensym(string)


cdef bytes sym_to_bytes(mx.t_symbol* symbol):
    """converts a max symbol to a python string"""
    return <bytes>symbol.s_name


cdef long clamp(long x, long minimum, long maximum):
    """Limit a value to a range between a minimum and a maximum value.

    x = clamp(x, low, hi)
    """
    if x < minimum:
        return minimum
    if x > maximum:
        return maximum
    return x

cdef mx.t_ptr_uint string_to_date(str string):
    """Convert a string to a date."""
    cdef mx.t_ptr_uint date = 0
    mx.db_util_stringtodate(string, &date)
    return date

cdef str date_to_string(mx.t_ptr_uint date):
    """Convert a date to a string."""
    cdef char date_cstr[MAX_PATH_CHARS]
    mx.db_util_datetostring(date, date_cstr)
    return date_cstr.decode()

# ----------------------------------------------------------------------------
# helper python functions

def fourchar_to_int(code: str) -> int:
    """Convert fourcc chars to an int

    >>> fourchar_to_int('TEXT')
    1413830740
    """
    assert len(code) == 4, "should be four characters only"
    return ((ord(code[0]) << 24) | (ord(code[1]) << 16) |
            (ord(code[2]) << 8)  | ord(code[3]))

def int_to_fourchar(n: int) -> str:
    """convert int to fourcc 4 chars

    >>> int_to_fourchar(1413830740)
    'TEXT'
    """
    return (
          chr((n >> 24) & 255)
        + chr((n >> 16) & 255)
        + chr((n >> 8) & 255)
        + chr((n & 255))
    )

# ============================================================================
# Named Tuples

Rect = namedtuple('Rect', ['x', 'y', 'width', 'height'])
Rgb = namedtuple('Rgb', ['red', 'green', 'blue'])
Rgba = namedtuple('Rgba', ['red', 'green', 'blue', 'alpha'])

# ============================================================================
# EXTENSION TYPES


# ----------------------------------------------------------------------------
# api.MaxObject

cdef class MaxObject:
    """An generic wrapper for a Max t_object
    """

    cdef mx.t_object* ptr
    cdef bint ptr_owner
    cdef public str name             # registered name
    cdef str _classname              # object classname
    cdef str _namespace              # 'box' or 'nobox'
    cdef readonly dict type_map      # used by attributes if any

    def __cinit__(self):
        self.ptr = NULL
        self.ptr_owner = False
        self.name = ""
        self._classname = ""
        self._namespace = 'box'
        self.type_map = {}

    def __dealloc__(self):
        # De-allocate if not null and flag is set
        if self.ptr is not NULL and self.ptr_owner is True:
            mx.object_free(self.ptr)
            self.ptr = NULL

    def __init__(self, str classname, *args, **kwds):
        self._classname = classname
        self.name = kwds.get('name', '')
        self._namespace = kwds.get('namespace', 'box')

        # first try to get existing registered object
        if self.name: # name is required
            self.ptr = self._object_ptr_from_existing(self._classname, self.name)
            self.ptr_owner = False
            if self.ptr is not NULL:
                return
            # else fallthrough and try to create new object

        self.ptr = self._object_ptr_from_new(
            self._classname, self.name, self._namespace, args)
        self.ptr_owner = True
        if self.ptr is NULL:
            raise ValueError(
                f"could not create {self._classname} {self.name} object")

    cdef mx.t_object* _object_ptr_from_existing(self, str classname, str name):
        cdef mx.t_object *patcher = NULL
        cdef mx.t_object *box = NULL
        cdef mx.t_object *obj = NULL
        cdef px.t_py *x = <px.t_py*>px.py_get_object_ref()

        mx.object_obex_lookup(x, mx.gensym("#P"), &patcher)
        box = mx.jpatcher_get_firstobject(patcher)
        while box is not NULL:
            obj = mx.jbox_get_object(box)
            if obj:
                if mx.object_classname(obj) == str_to_sym(classname):
                    if mx.object_attr_getsym(obj, mx.gensym("name")) == str_to_sym(name):
                        post(f"found {classname} object named {name}")
                        return obj
            box = mx.jbox_get_nextobject(box)
        return NULL

    cdef mx.t_object* _object_ptr_from_new(self, str classname, str name, str namespace, object args):
        cdef Atom atom = Atom(*args)
        cdef mx.t_object* newobj = <mx.t_object*>mx.object_new_typed(
            str_to_sym(namespace), str_to_sym(classname), atom.size, atom.ptr)
        if newobj is NULL:
            raise TypeError(f"could not create a class {classname} named {name} with args {args}")
        if name:
            registered = <mx.t_object*>mx.object_register(
                str_to_sym(namespace), str_to_sym(name), <mx.t_object*>newobj)
            if registered is NULL:
                raise ValueError(f"could not register {classname} {name}")
            return registered
        else:
            return newobj

    def __repr__(self) -> str:
        return f"<MaxObject {self._classname} '{self.name}'>"

    @staticmethod
    cdef MaxObject from_ptr(mx.t_object *ptr, bint owner=False):
        """Create a MaxObject from an existing pointer."""
        # Call to __new__ bypasses __init__ constructor
        cdef MaxObject obj = MaxObject.__new__(MaxObject)
        obj.ptr = ptr
        obj.ptr_owner = owner
        return obj

    @staticmethod
    def from_name(name: str, namespace: str = "box") -> MaxObject:
        """Retrieves a registered object given its varname and namespace."""
        cdef mx.t_object* obj_ptr = <mx.t_object*>mx.object_findregistered(
            str_to_sym(namespace), str_to_sym(name))
        if obj_ptr is NULL:
            raise ValueError(f"could not find a registered {namespace} object with name {name}.")
        return MaxObject.from_ptr(obj_ptr)

    @staticmethod
    def from_str(classname: str, parsestr: str, namespace: str = "box") -> MaxObject:
        """Create a new object with one or more atoms parsed from a C-string.

        The object's new method must have an A_GIMME signature.
        """
        cdef mx.t_object* obj_ptr = <mx.t_object*>mx.object_new_parse(
            str_to_sym(namespace), str_to_sym(classname), parsestr.encode())
        return MaxObject.from_ptr(obj_ptr, True)

    @property
    def classname(self) -> str:
        """Get object's clsasname"""
        cdef mx.t_symbol* _classname = mx.object_classname(<mx.t_object*>self.ptr)
        return sym_to_str(_classname)

    @property
    def patcher(self) -> Patcher:
        """Get parent patcher of max object"""
        cdef Patcher p = Patcher.from_object(self.ptr)
        return p

    @property
    def box(self) -> Box:
        """Get object's box if any"""
        cdef Box b = Box.from_object_ptr(self.ptr)
        return b

    @property
    def namespace(self) -> str:
        """Get the object's namespace"""
        cdef mx.t_symbol* ns = mx.object_namespace(self.ptr)
        return sym_to_str(ns)

    def is_instance(self, str classname) -> bool:
        """Checks if object is an instance of a classname"""
        cdef long res = mx.object_classname_compare(<mx.t_object*>self.ptr, str_to_sym(classname))
        return <bint>res

    def set_value(self, *args):
        """Set value of object"""
        cdef Atom atom = Atom(*args)
        cdef mx.t_max_err err = mx.object_setvalueof(<mx.t_object*>self.ptr, atom.size, atom.ptr)
        if err:
            raise ValueError(f"could not set object value with {args}")

    def get_value(self) -> object:
        """Get value of object"""
        cdef mx.t_atom * argv = NULL
        cdef long argc = 0
        cdef mx.t_max_err err = mx.object_getvalueof(<mx.t_object*>self.ptr, &argc, &argv)
        cdef Atom atom = Atom.from_ptr(argv, argc)
        if err:
            raise ValueError("could not get object value")
        return atom.value

    # attribute-related funcs

    def add_attribute(self, str type, str name):
        """Add an attribute with default flags and get/set methods to the object

        supported types:
            char, long, float32, float64, symbol, pointer, object, atom
        """
        cdef mx.t_max_err err = mx.MAX_ERR_NONE
        assert type in [
            "char", "long", "float32", "float64", "symbol", "pointer", "object", "atom"
        ], "type {type} not supported"
        cdef mx.t_object* attr = mx.attribute_new(name.encode(), str_to_sym(type),
            mx.ATTR_FLAGS_NONE, NULL, NULL)
        if attr is NULL:
            raise ValueError(f"could not create an attribute: {type} {name}")

        err = mx.object_addattr(<mx.t_object*>self.ptr, attr)
        if err:
            raise AttributeError(f"could not add attribute {type} {name}")
        self.type_map[name] = type

    def remove_attribute(self, str name):
        """Delete attribute by name"""
        cdef mx.t_max_err err = mx.object_deleteattr(<mx.t_object*>self.ptr, str_to_sym(name))
        if err:
            raise AttributeError(f"could not delete attr {name}")
        del self.type_map[name]

    def set_attr_value(self, str name, object value):
        """Set value of named object attribute"""
        if name not in self.type_map:
            raise KeyError(f"attribute '{name}' is not found")
        attr_type = self.type_map[name]
        if attr_type == "symbol":
            mx.object_attr_setsym(<mx.t_object*>self.ptr, str_to_sym(name), str_to_sym(value))
        elif attr_type == "long":
            mx.object_attr_setlong(<mx.t_object*>self.ptr, str_to_sym(name), <long>value)
        elif attr_type == "float32":
            mx.object_attr_setfloat(<mx.t_object*>self.ptr, str_to_sym(name), <float>value)
        elif attr_type == "float64":
            mx.object_attr_setfloat(<mx.t_object*>self.ptr, str_to_sym(name), <double>value)
        elif attr_type == "object":
            assert isinstance(value, MaxObject)
            mx.object_attr_setobj(<mx.t_object*>self.ptr, str_to_sym(name), <mx.t_object*>value.ptr)
        else:
            raise NotImplementedError(f"attr {attr_type} setting not currently implemented")

    def get_attr_value(self, str name) -> object:
        """Get value from object's attribute"""
        cdef mx.t_object* obj = NULL
        if name not in self.type_map:
            raise KeyError(f"attribute '{name}' is not found")
        attr_type = self.type_map[name]
        if attr_type == "symbol":
            return sym_to_str(mx.object_attr_getsym(<mx.t_object*>self.ptr, str_to_sym(name)))
        elif attr_type == "long":
            return <long>mx.object_attr_getlong(<mx.t_object*>self.ptr, str_to_sym(name))
        elif attr_type == "float32":
            return <float>mx.object_attr_getfloat(<mx.t_object*>self.ptr, str_to_sym(name))
        elif attr_type == "float64":
            return <double>mx.object_attr_getfloat(<mx.t_object*>self.ptr, str_to_sym(name))
        elif attr_type == "object":
            obj = mx.object_attr_getobj(<mx.t_object*>self.ptr, str_to_sym(name))
            return MaxObject.from_ptr(obj)
        raise NotImplementedError(f"attr {attr_type} retrieval not currently implemented")

    # registration funcs

    def register(self, str name, str namespace = "box") -> MaxObject:
        """Registers an object in a namespace."""
        cdef mx.t_object* registered_obj =  <mx.t_object*>mx.object_register(
            str_to_sym(namespace), str_to_sym(name), self.ptr)
        if registered_obj is NULL:
            raise ValueError(f"could not register object {namespace} {name}")
        return MaxObject.from_ptr(registered_obj)

    def unregister(self):
        """Removes a registered object from a namespace."""
        cdef mx.t_max_err err = mx.object_unregister(self.ptr)
        if err:
            raise ValueError("could not unregister object")

    # def get_namespace_and_name(self) -> list[str]:
    def get_namespace_and_name(self) -> tuple[str, str]:
        """Determines the namespace and/or name of a registered object, given the object's pointer."""
        cdef mx.t_symbol* namespace = mx.gensym("")
        cdef mx.t_symbol* name = mx.gensym("")
        cdef mx.t_max_err err = mx.object_findregisteredbyptr(&namespace, &name, self.ptr)
        if err:
            raise ValueError("could not get object's namespace and name from ptr")
        # return [sym_to_str(namespace), sym_to_str(name)]
        return (sym_to_str(namespace), sym_to_str(name))

    # method-related funcs

    def method_exists(self, str name) -> bool:
        """checks if named method exists"""
        if mx.getfn(<mx.t_object *>self.ptr, str_to_sym(name)):
            return True
        return False

    def _method_noargs(self, str name):
        """object method call with no arguments"""
        cdef mx.t_max_err err = mx.object_method_typed(
            <mx.t_object *>self.ptr, str_to_sym(name), 0, NULL, NULL)
        if err:
            raise ValueError(f"no arg method '{name}' call failed")

    def _method_args(self, str name, *args):
        """strongly typed object method call with arguments"""
        cdef Atom atom = Atom(*args)
        cdef mx.t_max_err err = mx.object_method_typed(
            <mx.t_object *>self.ptr, str_to_sym(name), atom.size, atom.ptr, NULL)
        if err:
            raise ValueError(f"args method '{name}' call failed")

    def _method_parsestr(self, str name, str parsestr):
        """combines object_method_typed() + atom_setparse() to define method arguments."""
        cdef mx.t_max_err err = mx.object_method_parse(
            <mx.t_object *>self.ptr, str_to_sym(name), parsestr.encode(), NULL)
        if err:
            raise ValueError(f"parsestr method '{name}' call failed")

    def _method_float(self, str name, float number):
        """wrapper for object_method_typed() that passes a single float as an argument"""
        cdef mx.t_max_err err = mx.object_method_float(
            <mx.t_object *>self.ptr, str_to_sym(name), number, NULL)
        if err:
            raise ValueError(f"float method '{name}' call failed")

    def _method_double(self, str name, double number):
        """wrapper for object_method_typed() that passes a single double as an argument"""
        cdef mx.t_max_err err = mx.object_method_double(
            <mx.t_object *>self.ptr, str_to_sym(name), number, NULL)
        if err:
            raise ValueError(f"double method '{name}' call failed")

    def _method_long(self, str name, long number):
        """wrapper for object_method_typed() that passes a single long as an argument"""
        cdef mx.t_max_err err = mx.object_method_long(
            <mx.t_object *>self.ptr, str_to_sym(name), number, NULL)
        if err:
            raise ValueError(f"long method '{name}' call failed")

    def _method_sym(self, str name, str symbol):
        """wrapper for object_method_typed() that passes a single t_symbol as an argument"""
        cdef mx.t_max_err err = mx.object_method_sym(
            <mx.t_object *>self.ptr, str_to_sym(name), str_to_sym(symbol), NULL)
        if err:
            raise ValueError(f"sym method '{name}' call failed")

    def _method_obj(self, str method_name, MaxObject obj):
        """wrapper for object_method_typed() that passes a single #t_object* as an argument."""
        cdef mx.t_max_err err = mx.object_method_obj(
            <mx.t_object *>self.ptr, str_to_sym(method_name), obj.ptr, NULL)
        if err:
            raise ValueError(f"method '{method_name}' call failed")

    cdef mx.t_max_err object_method_char_array(self, str method_name, long ac, unsigned char *av, mx.t_atom *rv):
        """Convenience wrapper for object_method_typed() that passes an array of char values as an argument."""
        cdef mx.t_max_err err = mx.object_method_char_array(<mx.t_object *>self.ptr, str_to_sym(method_name), ac, av, rv)
        if err:
            return error(f"method '{method_name}' call failed")
        return mx.MAX_ERR_NONE

    cdef mx.t_max_err object_method_long_array(self, str method_name, long ac, mx.t_atom_long *av, mx.t_atom *rv):
        """Convenience wrapper for object_method_typed() that passes an array of long integers values as an argument."""
        cdef mx.t_max_err err = mx.object_method_long_array(<mx.t_object *>self.ptr, str_to_sym(method_name), ac, av, rv)
        if err:
            return error(f"method '{method_name}' call failed")
        return mx.MAX_ERR_NONE

    cdef mx.t_max_err object_method_float_array(self, str method_name, long ac, float *av, mx.t_atom *rv):
        """Convenience wrapper for object_method_typed() that passes an array of 32bit floats values as an argument."""
        cdef mx.t_max_err err = mx.object_method_float_array(<mx.t_object *>self.ptr, str_to_sym(method_name), ac, av, rv)
        if err:
            return error(f"method '{method_name}' call failed")
        return mx.MAX_ERR_NONE

    cdef mx.t_max_err object_method_double_array(self, str method_name, long ac, double *av, mx.t_atom *rv):
        """Convenience wrapper for object_method_typed() that passes an array of 64bit float values as an argument."""
        cdef mx.t_max_err err = mx.object_method_double_array(<mx.t_object *>self.ptr, str_to_sym(method_name), ac, av, rv)
        if err:
            return error(f"method '{method_name}' call failed")
        return mx.MAX_ERR_NONE

    cdef mx.t_max_err object_method_sym_array(self, str method_name, long ac, mx.t_symbol **av, mx.t_atom *rv):
        """Convenience wrapper for object_method_typed() that passes an array of symbol values as an argument."""
        cdef mx.t_max_err err = mx.object_method_sym_array(<mx.t_object *>self.ptr, str_to_sym(method_name), ac, av, rv)
        if err:
            return error(f"method '{method_name}' call failed")
        return mx.MAX_ERR_NONE

    cdef mx.t_max_err object_method_obj_array(self, str method_name, long ac, mx.t_object **av, mx.t_atom *rv):
        """Convenience wrapper for object_method_typed() that passes an array of #t_object* values as an argument."""
        cdef mx.t_max_err err = mx.object_method_obj_array(<mx.t_object *>self.ptr, str_to_sym(method_name), ac, av, rv)
        if err:
            return error(f"method '{method_name}' call failed")
        return mx.MAX_ERR_NONE

    def call(self, str name, *args, parse=False):
        """general call object method function (strongly typed)"""
        if len(args) == 0:
            return self._method_noargs(name)
        elif len(args) == 1:
            if isinstance(args[0], str):
                if parse:
                    return self._method_parsestr(name, args[0])
                else:
                    return self._method_sym(name, args[0])
            elif isinstance(args[0], float):
                return self._method_double(name, args[0])
            elif isinstance(args[0], int):
                return self._method_long(name, args[0])
            elif isinstance(args[0], MaxObject):
                return self._method_obj(name, args[0])
            elif isinstance(args[0], list):
                return self.call(name, *args[0])
            elif isinstance(args[0], tuple):
                return self.call(name, *args[0])
            elif isinstance(args[0], set):
                return self.call(name, *args[0])
        else:
            return self._method_args(name, *args)

    # def call(self, str method, *args):
    #     """Helper wrapper method around object_method* variants"""
    #     cdef Atom atom = Atom(*args)
    #     cdef mx.t_max_err err = mx.MAX_ERR_NONE
    #     cdef mx.t_symbol* meth = str_to_sym(method)

    #     if len(args) == 0:
    #         mx.object_method(<mx.t_object*>self.ptr, str_to_sym(method))
    #     elif len(args) == 1:
    #         if isinstance(args[0], str):
    #             err = mx.object_method_sym(self.ptr, meth, str_to_sym(args[0]), NULL)
    #         elif isinstance(args[0], int):
    #             err = mx.object_method_long(self.ptr, meth, <long>args[0], NULL)
    #         elif isinstance(args[0], float):
    #             err = mx.object_method_float(self.ptr, meth, <float>args[0], NULL)
    #         elif isinstance(args[0], MaxObject):
    #             err = mx.object_method_obj(self.ptr, meth, <mx.t_object*>args[0].ptr, NULL)
    #     else:
    #         err = mx.object_method_typed(<mx.t_object*>self.ptr,
    #             str_to_sym(method), atom.size, atom.ptr, NULL)
    #     if err:
    #         raise ValueError(f"could not apply single arg to method {method}")


    def get_attr_sym(self, str name) -> str:
        """Retrieves the value of an attribute, given its parent object and name."""
        cdef mx.t_symbol *attr_sym = <mx.t_symbol *>mx.object_attr_getsym(
             <mx.t_object *>self.ptr, str_to_sym(name))
        return sym_to_str(attr_sym)

    def set_attr_sym(self, str name, str value):
        """Sets the value of an attribute, given its parent object and name."""
        cdef mx.t_max_err err = mx.object_attr_setsym(<mx.t_object *>self.ptr,
            str_to_sym(name), str_to_sym(value))
        if err:
            return error(f"could not set attr '{name}' value '{value}'")

    def get_attr_long(self, str name) -> int:
        """Retrieves the value of an attribute, given its parent object and name."""
        return mx.object_attr_getlong(<mx.t_object *>self.ptr, str_to_sym(name))

    def set_attr_long(self, str name, int value):
        """Sets the value of an attribute, given its parent object and name."""
        cdef mx.t_max_err err = mx.object_attr_setlong(<mx.t_object *>self.ptr,
            str_to_sym(name), value)
        if err:
            return error(f"could not set attr '{name}' value '{value}'")

    def get_attr_float(self, str name) -> float:
        """Retrieves the value of an attribute, given its parent object and name."""
        return mx.object_attr_getfloat (<mx.t_object *>self.ptr, str_to_sym(name))

    def set_attr_float(self, str name, float value):
        """Sets the value of an attribute, given its parent object and name."""
        cdef mx.t_max_err err = mx.object_attr_setfloat(<mx.t_object *>self.ptr,
            str_to_sym(name), value)
        if err:
            return error(f"could not set attr '{name}' value '{value}'")

    def get_attr_char(self, str name) -> bool:
        """Retrieves the value of an attribute, given its parent and name"""
        return mx.object_attr_getchar(<mx.t_object *>self.ptr, str_to_sym(name))

    def set_attr_char(self, str name, bint value):
        """Sets the value of an attribute, given its parent object and name."""
        cdef mx.t_max_err err = mx.object_attr_setchar(<mx.t_object *>self.ptr,
            str_to_sym(name), value)
        if err:
            return error(f"could not set attr '{name}' value '{value}'")

    def set_attr_from_str(self, str name, str value):
        """Set an attribute value with one or more atoms parsed from a C-string."""
        cdef mx.t_max_err err = mx.object_attr_setparse(<mx.t_object *>self.ptr,
            str_to_sym(name), value.encode())
        if err:
            return error("could not set attr with one or more atoms parsed from a C-string.")

    def clone(self) -> MaxObject:
        """return clone of object"""
        cdef mx.t_object *ptr = <mx.t_object *>mx.object_clone(<mx.t_object *>self.ptr)
        return MaxObject.from_ptr(ptr, owner=True)

    # attach detach / subscribe unsubscribe

    def attach(self, str name, str namespace = "box") -> MaxObject:
        """Attaches a client to a registered object."""
        cdef mx.t_object* registered_obj = <mx.t_object*>mx.object_attach(
            str_to_sym(namespace), str_to_sym(name), <mx.t_object*>self.ptr)
        if registered_obj is NULL:
            raise ValueError(f"could not attach to {namespace} {name} object")
        return MaxObject.from_ptr(registered_obj)

    def detach(self, str name, str namespace = "box"):
        """Detach a client from a registered object."""
        cdef mx.t_max_err err = mx.object_detach(
            str_to_sym(namespace), str_to_sym(name), self.ptr)
        if err:
            raise ValueError(f"could not detach from {namespace} {name} object")

    def subscribe(self, str name, str classname, str namespace = "box") -> MaxObject:
        """Subscribes a client to wait for an object to register."""
        cdef mx.t_object* registered_obj = <mx.t_object*>mx.object_subscribe(
            str_to_sym(namespace), str_to_sym(name), str_to_sym(classname), self.ptr)
        if (registered_obj is NULL):
            raise ValueError(
                f"could not subscribe to {namespace} {classname} {name} object")
        return MaxObject.from_ptr(registered_obj)

    def unsubscribe(self, str name, str classname, str namespace = "box"):
        """Detach a client from a registered object."""
        cdef mx.t_max_err err = mx.object_unsubscribe(
            str_to_sym(namespace), str_to_sym(name), str_to_sym(classname), self.ptr)
        if err:
            raise ValueError(
                f"could not unsubscribe from {namespace} {classname} {name} object")

    def notify(self, str msg, object data = None):
        """Broadcast a message (with an optional argument) from a registered object to any attached client objects."""
        # data may be implemented in another iteration
        cdef mx.t_max_err err = mx.object_notify(self.ptr, str_to_sym(msg), NULL)
        if err:
            raise ValueError("could not notify object(s)")

    # help / open doc functions

    def help(self):
        """Open the help patcher for a given object class name."""
        mx.classname_openhelp(self.classname.encode())

    def open_refpage(self):
        """Open the reference page for a given object class name."""
        mx.classname_openrefpage(self.classname.encode())

    def open_query(self):
        """Open a search in the file browser for files with the name of the given class."""
        mx.classname_openquery(self.classname.encode())


# ----------------------------------------------------------------------------
# api.Atom

cdef class Atom:
    """A wrapper class for a Max t_atom"""

    cdef mx.t_atom *ptr
    cdef bint ptr_owner
    cdef public long size

    def __cinit__(self):
        self.ptr = NULL
        self.ptr_owner = False
        self.size = 0

    def __dealloc__(self):
        # De-allocate if not null and flag is set
        if self.ptr is not NULL and self.ptr_owner is True:
            mx.sysmem_freeptr(self.ptr)
            self.ptr = NULL

    def __init__(self, *args):
        cdef int i = 0

        if len(args) > 0: # otherwise default to __cinit__ values
            if len(args)==1 and isinstance(args[0], (list, tuple)):
                # flatten in case of nesting
                args = self.__flatten(args[0])
            else: # convert to flattened list
                args = self.__flatten(args)
            self.size = len(args)
            self.ptr = <mx.t_atom *>mx.sysmem_newptr(self.size * sizeof(mx.t_atom))
            self.ptr_owner = True
            if self.ptr is NULL:
                raise MemoryError("Atom.__init__ allocation error")
            for i, obj in enumerate(args):
                self[i] = obj

    def __repr__(self) -> str:
        return f"<Atom argc:{self.size}>"

    def __iter__(self):
        return iter(self.to_list())

    def __len__(self):
        return self.size

    def __getitem__(self, int idx) -> object:
        if self.is_symbol(idx):
            return self.get_string(idx)

        elif self.is_float(idx):
            return self.get_float(idx)

        elif self.is_long(idx):
            return self.get_long(idx)

        else:
            raise TypeError

    def __setitem__(self, int idx, object value):
        if isinstance(value, str):
            self.set_symbol(idx, value)
        elif isinstance(value, float):
            self.set_float(idx, value)
        elif isinstance(value, int):
            self.set_long(idx, value)
        elif isinstance(value, bytes):
            self.set_bytes(idx, value)
        else:
            raise TypeError

    def __str__(self) -> str:
        if self.size == 1 and self.is_symbol():
            return self.get_string(0)
        raise TypeError("atom is either wrong length or cannot be converted to str")

    def __int__(self) -> int:
        if self.size == 1 and self.is_long():
            return self.get_long(0)
        raise TypeError("atom is either wrong length or cannot be converted to int")

    def __float__(self) -> float:
        if self.size == 1 and self.is_float():
            return self.get_float(0)
        raise TypeError("atom is either wrong length or cannot be converted to float")

    def __flatten(self, args) -> list[str | float | int | bytes]:
        if not args:
            return []
        _args = []
        for arg in args:
            if isinstance(arg, (list, tuple)):
                _args.extend(arg)
            else:
                _args.append(arg)
        return _args

    @staticmethod
    cdef Atom from_ptr(mx.t_atom *ptr, long size, bint owner=False):
        """Create an Atom instance from an existing pointer."""
        # Call to __new__ bypasses __init__ constructor
        cdef Atom atom = Atom.__new__(Atom)
        atom.ptr = ptr
        atom.ptr_owner = owner
        atom.size = size
        return atom

    @staticmethod
    cdef Atom new(long size):
        """Create an empty Atom instance with an aribitrary length"""
        cdef mx.t_atom *ptr = <mx.t_atom *>mx.sysmem_newptr(size * sizeof(mx.t_atom))
        if ptr is NULL:
            raise MemoryError
        return Atom.from_ptr(ptr, size, owner=True)

    @staticmethod
    def from_str(parsestr: str) -> Atom:
        """Parse a string into an Atom instance."""
        cdef mx.t_atom *ptr = NULL
        cdef long size = 0
        cdef mx.t_max_err err = mx.atom_setparse(&size, &ptr, parsestr.encode())
        if err:
            raise TypeError("unable to parse a string into an Atom instance")
        return Atom.from_ptr(ptr, size, owner=True)

    @staticmethod
    def from_seq(seq: object) -> Atom:
        """Create an Atom instance from a sequence of objects."""
        cdef mx.t_atom *ptr
        cdef long size = len(seq)
        if not size:
            raise IndexError("sequence is empty")
        ptr = <mx.t_atom *>mx.sysmem_newptr(size * sizeof(mx.t_atom))
        if ptr is NULL:
            raise MemoryError

        cdef int i
        for i, obj in enumerate(seq):

            if isinstance(obj, float):
                mx.atom_setfloat(ptr+i, <float>obj)

            elif isinstance(obj, int):
                mx.atom_setlong(ptr+i, <long>obj)

            elif isinstance(obj, bytes):
                mx.atom_setsym(ptr+i, mx.gensym(obj))

            elif isinstance(obj, str):
                mx.atom_setsym(ptr+i, str_to_sym(obj))

            else:
                error(f"cannot convert: {obj}")
                continue

        return Atom.from_ptr(ptr, size, owner=True)

    @property
    def value(self) -> object:
        """Get python value(s) of atom"""
        if self.size == 0:
            return None
        elif self.size == 1:
            return self[0]
        else:
            return self.to_list()

    def set_float(self, int idx, float f):
        """Inserts a float into a t_atom and change its type to A_FLOAT."""
        mx.atom_setfloat(self.ptr + idx, f)

    def get_float(self, int idx=0) -> float:
        """Retrieves a float value from a t_atom."""
        return <float>mx.atom_getfloat(self.ptr + idx)

    def set_long(self, int idx, long x):
        """Inserts an integer into a t_atom and change its type to A_LONG."""
        mx.atom_setlong(self.ptr + idx, x)

    def get_long(self, int idx=0) -> int:
        """Retrieves a long integer value from a t_atom."""
        return <long>mx.atom_getlong(self.ptr + idx)

    def get_int(self, int idx=0) -> int:
        """Retrieves an integer value from a t_atom."""
        return <int>mx.atom_getlong(self.ptr + idx)

    def set_symbol(self, int idx, str symbol):
        """Inserts a t_symbol∗ into a t_atom and change its type to A_SYM."""
        mx.atom_setsym(self.ptr + idx, str_to_sym(symbol))

    cdef mx.t_symbol *get_symbol(self, int idx=0):
        """Retrieves a t_symbol ∗ value from a t_atom."""
        return mx.atom_getsym(self.ptr + idx)

    def get_string(self, int idx=0) -> str:
        """Retrieves a string value from a symbol t_atom."""
        return sym_to_str(self.get_symbol(idx))

    def set_bytes(self, int idx, bytes x):
        """Inserts an integer into a t_atom and change its type to A_LONG."""
        mx.atom_setsym(self.ptr + idx, bytes_to_sym(x))

    def get_bytes(self, int idx=0) -> bytes:
        """Retrieves a long integer value from a t_atom."""
        return sym_to_bytes(mx.atom_getsym(self.ptr + idx))

    def is_symbol(self, int idx=0) -> bool:
        """check if index points to a symbol"""
        if idx > self.size - 1:
            raise IndexError(f"index should be < {self.size - 1}")
        return <bint>((self.ptr + idx).a_type == mx.A_SYM)

    def is_long(self, int idx=0) -> bool:
        """check if index points to a long"""
        if idx > self.size - 1:
            raise IndexError(f"index should be < {self.size - 1}")
        return <bint>((self.ptr + idx).a_type == mx.A_LONG)

    # alias is_int -> is_long
    is_int = is_long

    def is_float(self, int idx=0) -> bool:
        """check if index points to a float"""
        if idx > self.size - 1:
            raise IndexError(f"index should be < {self.size - 1}")
        return <bint>((self.ptr + idx).a_type == mx.A_FLOAT)

    def is_string(self, int idx = 0) -> bool:
        """Determines whether or not an atom represents a t_string object."""
        return bool(mx.atomisstring(self.ptr + idx))

    def is_atomarray(self, int idx = 0) -> bool:
        """Determines whether or not an atom represents a t_atomarray object."""
        return bool(mx.atomisatomarray(self.ptr + idx))

    def is_dictionary(self, int idx = 0) -> bool:
        """Determines whether or not an atom represents a t_dictionary object."""
        return bool(mx.atomisdictionary(self.ptr + idx))

    def to_list(self) -> list:
        """Convert an array of atoms into a list."""
        cdef int i
        _res = []
        for i in range(self.size):
            if self.is_symbol(i):
                _res.append(self.get_string(i))
            elif self.is_long(i):
                _res.append(self.get_long(i))
            elif self.is_float(i):
                _res.append(self.get_float(i))
        return _res

    def to_string(self) -> str:
        """Convert an array of atoms into a C-string."""

        cdef long textsize = 0
        cdef char *text = NULL
        cdef mx.t_max_err err
        cdef bytes result

        err = mx.atom_gettext(self.size, <mx.t_atom *>self.ptr,  &textsize,
            &text, mx.OBEX_UTIL_ATOM_GETTEXT_DEFAULT)
        if (err == mx.MAX_ERR_NONE and textsize and text):
            result = <bytes>text

        if (text):
            mx.sysmem_freeptr(text)

        return result.decode()

    # if INCLUDE_NUMPY:
    #     def to_np_array(self):
    #         return np.array(self.to_list(), dtype=np.float64)

    def display(self):
        """Display the contents of an atom array."""
        cdef int i
        for i in range(self.size):
            if self.is_float(i):
                print("is_float:", i)
            elif self.is_symbol(i):
                print("is_symbol:", i)
                s = self.get_string(i)
                print("string:", type(s))
            else:
                print("other:", i)

    cdef resize_ptr(self, mx.t_ptr_size new_size):
        """Resize an existing pointer."""
        self.ptr = <mx.t_atom *>mx.sysmem_resizeptr(<mx.t_atom *>self.ptr,
            new_size * sizeof(mx.t_atom))

    cdef mx.t_max_err setchar_array(self, long ac, long count, unsigned char *vals):
        """Assign an array of char values to an array of atoms."""
        if ac > self.size:
            self.resize_ptr(ac)
        return mx.atom_setchar_array(ac, self.ptr, count, vals)

    cdef mx.t_max_err getchar_array(self, long count, unsigned char *vals):
        """Fetch an array of char values from an array of atoms."""
        return mx.atom_getchar_array(self.size, self.ptr, count, vals)

    def setlong_array(self, list[int] values):
        """Assign an array of long values to an array of atoms."""
        cdef long count = len(values)
        cdef long ac = count
        if ac > self.size:
            self.resize_ptr(ac)
        cdef mx.t_atom_long *vals = <mx.t_atom_long *>mx.sysmem_newptr(count * sizeof(mx.t_atom_long))
        for i in range(count):
            vals[i] = <long>values[i]
        cdef mx.t_max_err err = mx.atom_setlong_array(ac, self.ptr, count, vals)
        if err:
            raise ValueError("could not set long array")
        mx.sysmem_freeptr(vals)

    def getlong_array(self, long count) -> list[int]:
        """Fetch an array of long values from an array of atoms."""
        cdef mx.t_atom_long *vals = <mx.t_atom_long *>mx.sysmem_newptr(count * sizeof(mx.t_atom_long))
        cdef mx.t_max_err err = mx.atom_getlong_array(self.size, self.ptr, count, vals)
        if err:
            raise ValueError("could not get long array")
        cdef list[int] values = []
        for i in range(count):
            values.append(<int>vals[i])
        mx.sysmem_freeptr(vals)
        return values

    def setfloat_array(self, list[float] values):
        """Assign an array of float values to an array of atoms."""
        cdef long count = len(values)
        cdef long ac = count
        if ac > self.size:
            self.resize_ptr(ac)
        cdef float *vals = <float *>mx.sysmem_newptr(count * sizeof(float))
        for i in range(count):
            vals[i] = <float>values[i]
        cdef mx.t_max_err err = mx.atom_setfloat_array(ac, self.ptr, count, vals)
        if err:
            raise ValueError("could not set float array")
        mx.sysmem_freeptr(vals)

    def getfloat_array(self, long count) -> list[float]:
        """Fetch an array of float values from an array of atoms."""
        cdef float *vals = <float *>mx.sysmem_newptr(count * sizeof(float))
        cdef mx.t_max_err err = mx.atom_getfloat_array(self.size, self.ptr, count, vals)
        if err:
            raise ValueError("could not get float array")
        cdef list[float] values = []
        for i in range(count):
            values.append(<float>vals[i])
        mx.sysmem_freeptr(vals)
        return values

    def setdouble_array(self, list[float] values):
        """Assign an array of double values to an array of atoms."""
        cdef long count = len(values)
        cdef long ac = count
        if ac > self.size:
            self.resize_ptr(ac)
        cdef double *vals = <double *>mx.sysmem_newptr(count * sizeof(double))
        for i in range(count):
            vals[i] = <double>values[i]
        cdef mx.t_max_err err = mx.atom_setdouble_array(ac, self.ptr, count, vals)
        if err:
            raise ValueError("could not set double array")
        mx.sysmem_freeptr(vals)

    def getdouble_array(self, long count) -> list[float]:
        """Fetch an array of double values from an array of atoms."""
        cdef double *vals = <double *>mx.sysmem_newptr(count * sizeof(double))
        cdef mx.t_max_err err = mx.atom_getdouble_array(self.size, self.ptr, count, vals)
        if err:
            raise ValueError("could not get double array")
        cdef list[float] values = []
        for i in range(count):
            values.append(<float>vals[i])
        mx.sysmem_freeptr(vals)
        return values

    cdef mx.t_max_err setsym_array(self, long ac, long count, mx.t_symbol **vals):
        """Assign an array of t_symbol values to an array of atoms."""
        if ac > self.size:
            self.resize_ptr(ac)
        return mx.atom_setsym_array(ac, self.ptr, count, vals)

    cdef mx.t_max_err getsym_array(self, long count, mx.t_symbol **vals):
        """Fetch an array of t_symbol values from an array of atoms."""
        return mx.atom_getsym_array(self.size, self.ptr, count, vals)

    cdef mx.t_max_err setatom_array(self, long ac, long count, mx.t_atom *vals):
        """Assign an array of t_atom values to an array of atoms."""
        if ac > self.size:
            self.resize_ptr(ac)
        return mx.atom_setatom_array(ac, self.ptr, count, vals)

    cdef mx.t_max_err getatom_array(self, long count, mx.t_atom *vals):
        """Fetch an array of t_symbol values from an array of atoms."""
        return mx.atom_getatom_array(self.size, self.ptr, count, vals)

    cdef mx.t_max_err setobj_array(self, long ac, long count, mx.t_object **vals):
        """Assign an array of t_object values to an array of atoms."""
        if ac > self.size:
            self.resize_ptr(ac)
        return mx.atom_setobj_array(ac, self.ptr, count, vals)

    cdef mx.t_max_err getobj_array(self, long count, mx.t_object **vals):
        """Fetch an array of t_object values from an array of atoms."""
        return mx.atom_getobj_array(self.size, self.ptr, count, vals)

    cdef mx.t_max_err setparse(self, const char *parsestr):
        """Parse a C-string into an array of atoms."""
        return mx.atom_setparse(&self.size, <mx.t_atom **>&self.ptr, parsestr)

    cdef mx.t_max_err setbinbuf(self, void *buf):
        """set binbuf content into an array of atoms."""
        return mx.atom_setbinbuf(&self.size, <mx.t_atom **>&self.ptr, buf)

    cdef mx.t_max_err setattrval(self, mx.t_symbol *attrname, mx.t_object *obj):
        """set attribute value into an array of atoms."""
        return mx.atom_setattrval(&self.size, <mx.t_atom **>&self.ptr, attrname, obj)

    cdef mx.t_max_err setobjval(self, mx.t_object *obj):
        """set object value into an array of atoms."""
        return mx.atom_setobjval(&self.size, <mx.t_atom **>&self.ptr, obj)


# ----------------------------------------------------------------------------
# api.Buffer

cdef class Buffer:
    """A wrapper class for a Max buffer~ object"""

    cdef public str name
    cdef mp.t_buffer_obj *obj
    cdef mp.t_buffer_ref *ref
    cdef bint is_locked
    cdef float* samples
    cdef float[:] s_buffer # cython memoryview
    cdef Py_ssize_t shape
    cdef Py_ssize_t strides

    def __cinit__(self):
        self.name = None
        self.obj = NULL
        self.ref = NULL
        self.samples = NULL
        self.is_locked = False

    def __dealloc__(self):
        # De-allocate if not null
        if self.ref is not NULL:
            mx.object_free(self.ref)
            self.ref = NULL

    def __init__(self, str name, str filename, int duration = -1, int channels = 1):
        """Create a buffer from scratch given name and file to load"""
        cdef mx.t_atom[4] argv
        cdef int argc = 4
        mx.atom_setsym(argv, str_to_sym(name))
        mx.atom_setsym(argv+1, str_to_sym(filename))
        mx.atom_setlong(argv+2, duration)
        mx.atom_setlong(argv+3, channels)
        self.obj = <mx.t_object*>mx.object_new_typed(
            mx.CLASS_BOX, mx.gensym("buffer~"), argc, argv)
        self.ref = mp.buffer_ref_new(self.obj, str_to_sym(name))

    def __repr__(self) -> str:
        return f"<Buffer '{self.name}' nframes:{self.framecount()}>"

    def __getbuffer__(self, Py_buffer *buffer, int flags):
        cdef Py_ssize_t itemsize = sizeof(float)
        cdef Py_ssize_t buffersize = <Py_ssize_t>mp.buffer_getframecount(self.obj)

        self.shape = buffersize
        self.strides = itemsize

        self.locksamples()

        buffer.buf = self.samples
        buffer.obj = self
        buffer.len = buffersize * itemsize   # product(shape) * itemsize
        buffer.itemsize = itemsize
        buffer.readonly = 0
        buffer.ndim = 1
        buffer.format = "f"                     # float not double!
        buffer.shape = &self.shape
        buffer.strides = &self.strides
        buffer.suboffsets = NULL                # for pointer arrays only
        buffer.internal = NULL                  # see References

    def __releasebuffer__(self, Py_buffer *buffer):
        self.unlocksamples()

    @staticmethod
    cdef Buffer from_name(mx.t_object *x, str name):
        """Create a reference to a buffer~ object by name."""
        # Call to __new__ bypasses __init__ constructor
        cdef Buffer buffer = Buffer.__new__(Buffer)
        buffer.ref = mp.buffer_ref_new(x, str_to_sym(name))
        assert(mp.buffer_ref_exists(buffer.ref))
        buffer.name = name
        buffer.obj = mp.buffer_ref_getobject(buffer.ref)
        return buffer

    @staticmethod
    cdef Buffer new(mx.t_object *x, str name, str filename, int duration = -1, int channels = 1):
        """Create a buffer from scratch given name and file to load"""
        # create a buffer
        cdef mx.t_atom[4] argv
        cdef int argc = 4
        mx.atom_setsym(argv, str_to_sym(name))
        mx.atom_setsym(argv+1, str_to_sym(filename))
        mx.atom_setlong(argv+2, duration)
        mx.atom_setlong(argv+3, channels)
        cdef mx.t_object *b = <mx.t_object*>mx.object_new_typed(
            mx.CLASS_BOX, mx.gensym("buffer~"), argc, argv)
        argc = 3
        mx.atom_setsym(argv, str_to_sym(filename))
        mx.atom_setlong(argv+1, 0) # starting time
        mx.atom_setlong(argv+2, channels)
        mx.typedmess(b, mx.gensym("replace"), argc, argv)

        # now retrieve buffer by name
        return Buffer.from_name(x, name)

    @staticmethod
    cdef Buffer empty(mx.t_object *x, str name, int duration_ms, int channels=1):
        """Create a new empty buffer"""
        # create a buffer
        cdef mx.t_atom argv[3]

        # check if another buffer exists with same name
        # cdef mp.t_buffer_ref* ref = mp.buffer_ref_new(x, str_to_sym(name))
        # TODO: rethink (overly safe?)
        # if mp.buffer_ref_exists(ref):
        #     return error(f"buffer with name {name} already exists, and was overwritten")

        mx.atom_setsym(argv + 0, str_to_sym(name))
        mx.atom_setlong(argv + 1, duration_ms)
        mx.atom_setlong(argv + 2, channels)
        cdef mx.t_object *_ = <mx.t_object*>mx.object_new_typed(
            mx.CLASS_BOX, mx.gensym("buffer~"), 3, argv)

        # now retrieve buffer by name
        return Buffer.from_name(x, name)

    def refresh(self):
        """reset buffer object from ref"""
        self.obj = mp.buffer_ref_getobject(self.ref)

    def change(self, str msg, *args) -> bool:
        """generic structural change method

        May only be used for structural changes to the buffer.

        >>> buf.change("sizeinsamps", 20000)
        """
        cdef Atom atom = Atom.from_seq(args)

        if (self.obj):
            # mp.buffer_edit_begin(self.obj)
            self.buffer_edit_begin()
            mx.object_method_typed(
                <mx.t_object*>self.obj, str_to_sym(msg), atom.size, atom.ptr, NULL)
            # mp.buffer_edit_end(self.obj, 1)
            self.buffer_edit_end()
            self.setdirty()
            return True

        return False

    def framecount(self) -> int:
        """Get how many frames long the buffer content is in samples."""
        return mp.buffer_getframecount(self.obj)

    def set_framecount(self, int frames):
        """resize buffer by number of samples (frames)"""

        if frames == self.framecount:
            return

        if self.change("sizeinsamps", frames):
            if frames != self.framecount:
                error(f"Could not resize {self.name} "
                      f"frames: {frames} != {self.framecount}")
            assert(frames == self.framecount)
            return

        return error("Resize on null buffer")

    framecount = property(framecount, set_framecount)


    def samplerate(self) -> int:
        """Get the buffer's native sample rate in samples per second."""
        return mp.buffer_getsamplerate(self.obj)

    def set_samplerate(self, int samplerate):
        """change buffer samplerate (Hz)"""
        if samplerate == self.samplerate:
            return

        if self.change("sr", samplerate):
            if samplerate != self.samplerate:
                error(f"Could not set samplerate to buffer {self.name} "
                      f"{samplerate} != {self.samplerate}")
            assert(samplerate == self.samplerate)
            return

        return error("Resize on null buffer")

    samplerate = property(samplerate, set_samplerate)


    def duration(self) -> float:
        """Get the buffer's duration in seconds."""
        return self.n_samples / self.samplerate

    def set_duration(self, float duration):
        """resize buffer duration (seconds)"""
        self.set_duration_ms(1000 * duration)

    duration = property(duration, set_duration)


    def duration_ms(self) -> int:
        """Get the buffer's duration in milliseconds."""
        return self.duration * 1000

    def set_duration_ms(self, int duration):
        """resize buffer duration (milliseconds)"""

        if duration == int(self.duration_ms):
            return

        if self.change("size", duration):
            return

        return error("Resize on null buffer")

    duration_ms = property(duration_ms, set_duration_ms)


    def change_reference(self, str name):
        """Change a buffer reference to refer to a different buffer object by name."""
        mp.buffer_ref_set(self.ref, str_to_sym(name))
        assert(mp.buffer_ref_exists(self.ref))
        self.obj = mp.buffer_ref_getobject(self.ref)

    def view(self):
        """Open a viewer window to display the contents of the buffer."""
        mp.buffer_view(self.obj)

    @property
    def channelcount(self) -> int:
        """Get how many channels are present in the buffer content."""
        return mp.buffer_getchannelcount(self.obj)

    @property
    def millisamplerate(self) -> int:
        """Get the buffer's native sample rate in samples per millisecond."""
        return mp.buffer_getmillisamplerate(self.obj)

    @property
    def n_samples(self) -> int:
        """Get the number of samples in the buffer."""
        return self.framecount

    # @property
    def filename(self) -> str:
        """Retrieve the name of the last file to be read by a buffer~."""
        return sym_to_str(mp.buffer_getfilename(self.obj))

    def set_filename(self, str filename):
        """set filename, uses 'replace <filename>' to set buffer length, contents from file"""
        self.replace(filename)

    filename = property(filename, set_filename)

    def setdirty(self):
        """Set the buffer's dirty flag, indicating that changes have been made."""
        mp.buffer_setdirty(self.obj)

    def setpadding(self, long samplecount):
        """Set the number of samples with which to zero-pad the buffer~'s contents."""
        mp.buffer_setpadding(self.obj, samplecount)

    def locksamples(self):
        """Claim the buffer∼ and get a pointer to the first sample in memory."""
        self.samples = mp.buffer_locksamples(self.obj)
        self.is_locked = True

    def unlocksamples(self):
        """Release claim on buffer's contents so other objects can read/write to it."""
        mp.buffer_unlocksamples(self.obj)
        if self.samples:
            self.samples = NULL
            self.is_locked = False

    def buffer_edit_begin(self):
        """begin buffer_edit block

        Use `buffer_edit` functions to collapse all operations of locking heavy `b_mutex`,
        setting b_valid flag, waiting on lightweight atomic b_inuse, etc.
        """
        mp.buffer_edit_begin(self.obj)

    def buffer_edit_end(self, int valid=1):
        """End a buffer_edit block"""
        mp.buffer_edit_end(self.obj, valid)

    # TODO: add start:end slice
    def get_samples(self):
        """Get samples as a memoryview"""
        cdef int i
        self.s_buffer = cvarray(
            shape=(self.n_samples,), itemsize=sizeof(float), format="f")
        self.locksamples()
        for i in range(self.n_samples):
            self.s_buffer[i] = self.samples[i]
        self.unlocksamples()
        cdef float[:] res = self.s_buffer
        return res

    # float32 is the default in Max/MSP
    def set_samples(self, float[:] samples):
        """Set samples from a memoryview"""
        # assert samples.shape[0] <= self.n_samples
        cdef long n_samples = <Py_ssize_t>samples.shape[0]
        cdef int i
        # resize buffer to samples.shape[0]
        self.set_framecount(n_samples)
        self.locksamples()
        for i in range(n_samples):
            self.samples[i] = samples[i]
        self.unlocksamples()

    def send(self, str msg, *args):
        """Generic message sender

        Used for all message methods except those modifying
        the buffer structure (size, framecount, ..)
        i.e. not for structural changes which require
        re-allocation of memory.

        >>> buf.send("fill", "sin", 24)
        """
        if not args:
            self._send_single(msg)
        else:
            self._send_multi(msg, *args)

    def _send_single(self, str msg):
        """Generic message sender for a one word message"""
        if (self.obj):
            mx.object_method_typed(
                <mx.t_object*>self.obj, str_to_sym(msg), 0, NULL, NULL)

    def _send_multi(self, str msg, *args):
        """Generic message sender for a message with a list of arguments

        >>> buf.send("fill", "sin", 24)
        """
        cdef Atom atom = Atom.from_seq(args)
        if (self.obj):
            self.locksamples()
            mx.object_method_typed(
                <mx.t_object*>self.obj, str_to_sym(msg), atom.size, atom.ptr, NULL)
            self.unlocksamples()
            self.setdirty()

    def bang(self):
        """Redraw the buffer display"""
        self.send("bang")

    def apply(self, *args):
        """Apply a function to buffer contents

        funcs:
            triangle, hamming, hanning, blackman, welch, (kaiser beta) (windowing)
            gain
            offset
            getdelta
        """
        self.send("apply", *args)

    def clear(self):
        """Erase the contents of the buffer"""
        self.send("clear")

    def clearlow(self):
        """Erase the contents of the buffer via a low priority thread"""
        self.send("clearlow")

    def crop(self, int start, int end):
        """Trim audio data in buffer and resize it accordingly"""
        self.change("crop", start, end)

    def duplicate(self, str name):
        """Import the contents of a named buffer"""
        self.change("duplicate", name)

    def enumerate(self):
        """List all objects referencing the buffer"""
        self.send("enumerate")

    def fill(self, *args):
        """Generic fill method

        >>> buf.fill("sin", 24)
        """
        self.send("fill", *args)

    def import_(self, path: str, start: int = 0, duration: int = -1, channels: int = 0):
        """Import a file

        params:
            filename: str
            start: float (ms)
            duration: float (ms) (negative resizes buffer)
            channels
        """
        if channels:
            self.change("import", path, start, duration, channels)
        else:
            self.change("import", path, start, duration)

    def importreplace(self, path: str, start: int = 0, channels: int = 0):
        """Same as import but imports are performed with automatic duration
        and channel resizing enabled by default.
        """
        if channels:
            self.change("importreplace", path, start, channels)
        else:
            self.change("importreplace", path, start)

    def rename(self, str name):
        """Rename the buffer and tell other objects to refer to it by the new name.
        """
        self.send("set", name)
        self.send("name", name)

    def normalize(self, float amount):
        """Normalize the audio in the buffer"""
        self.send("normalize", amount)

    def open(self):
        """Open the sample display"""
        self.send("open")

    def printmodtime(self):
        """Post the last modification time to the console"""
        self.send("printmodtime")

    read = import_ # read is a synonym for import

    replace = importreplace # replace is a synonym for importreplace

    def close(self):
        """Close the view window"""
        self.send("wclose")

    def write(self, str path):
        """Write the contents of the buffer to an audio file"""
        if path.endswith(".wav"):
            self.send("writewave", path)
        elif path.endswith(".aiff"):
            self.send("writeaiff", path)
        elif path.endswith(".raw"):
            self.send("writeraw", path)
        elif path.endswith(".flac"):
            self.send("writeflac", path)

# ----------------------------------------------------------------------------
# api.Dictionary - ext_dictionary.h

cdef class Dictionary:
    """A wrapper class for a Max t_dictionary"""

    cdef mx.t_dictionary *ptr
    cdef bint ptr_owner
    cdef bint to_release
    cdef public str name
    cdef public dict type_map

    def __cinit__(self):
        self.ptr = NULL
        self.ptr_owner = False
        self.to_release = False
        self.name = ""
        self.type_map = {}

    def __init__(self, str name = "", **kwargs):
        cdef mx.t_symbol* name_ptr = str_to_sym(name)
        cdef mx.t_dictionary *d = NULL
        if name:
            # Create (or reference an existing) dictionary by name
            self.ptr = mx.dictobj_findregistered_retain(name_ptr)
            self.to_release = True
            if self.ptr is NULL:
                # create a new dictionary with a registered name
                d = mx.dictionary_new()
                self.ptr = mx.dictobj_register(d, &name_ptr)
                self.to_release = False
        else: # name is empty
            self.ptr = mx.dictionary_new()
            self.to_release = False
        self.name = name
        self.ptr_owner = True
        if kwargs:
            for key, value in kwargs.items():
                self[key] = value

    def __dealloc__(self):
        # De-allocate if not null
        if self.ptr is not NULL and self.ptr_owner:
            if self.to_release:
                mx.dictobj_release(self.ptr)
            else:
                mx.object_free(self.ptr)
            self.ptr = NULL

    def __repr__(self) -> str:
        return f"<Dictionary '{self.name}'>"

    def __contains__(self, str x) -> bool:
        return <bint>self.has_entry(x)

    def __setitem__(self, str key, object value):
        if isinstance(value, float):
            self.type_map[key] = 'float'
            self.set_float(key, <double>value)
        elif isinstance(value, int):
            self.type_map[key] = 'long'
            self.set_long(key, <int>value)
        elif isinstance(value, str):
            self.type_map[key] = 'str'
            self.set_sym(key, value)
        elif isinstance(value, list):
            self.type_map[key] = 'list'
            self.set_atoms(key, value)
        elif isinstance(value, bytes):
            self.type_map[key] = 'bytes'
            self.set_bytes(key, value)
        elif isinstance(value, MaxObject):
            self.type_map[key] = 'object'
            self.set_object(key, value)
        elif isinstance(value, Atom):
            self.type_map[key] = 'atom'
            self.set_atoms(key, value)
        else:
            raise TypeError("unable to recognize type for dict")

    def __getitem__(self, str key):
        return {
            'float': self.get_float,
            'long': self.get_long,
            'str': self.get_sym,
            'bytes': self.get_bytes,
            'list': self.get_atoms,
            'object': self.get_object,
            'atom': self.get_atoms,
        }[self.type_map[key]](key)

    def __delitem__(self, str key):
        self.delete_entry(key)
        # self.chuck_entry(key) # also test chuck_entry
        del self.type_map[key]

    @classmethod
    def from_dict(cls, dict src_dict, str name = "") -> Dictionary:
        """Create an registered or unregistered Dictionary from a python dict"""
        cdef Dictionary _dict = cls(name)
        for key, value in src_dict.items():
            _dict[key] = value
        return _dict

    @classmethod
    def from_kwargs(cls, **kwargs) -> Dictionary:
        """Create an unregistered Dictionary from kwargs entries

        Can be use for object creation from dictionaries.
        In this case, no need to prefix each key with `@`.
        """
        cdef Dictionary _dict = cls()
        for key, value in kwargs.items():
            _dict[key] = value
        return _dict

    @classmethod
    def from_atoms(cls, Atom atoms, str name = "") -> Dictionary:
        """Create an unregistered dictionary from atoms as dict-syntax"""
        cdef Dictionary _dict = cls(name)
        cdef mx.t_max_err err = mx.dictobj_dictionaryfromatoms(&_dict.ptr, atoms.size, atoms.ptr)
        if err:
            raise TypeError("Could not create an unregistered dictionary from atoms as dict-syntax")
        return _dict

    @classmethod
    def from_atoms_extended(cls, Atom atoms, str name = "") -> Dictionary:
        """Create a new t_dictionary from an array of atoms that use Max dictionary syntax, JSON, or compressed JSON."""
        cdef Dictionary _dict = cls(name)
        cdef mx.t_max_err err = mx.dictobj_dictionaryfromatoms_extended(&_dict.ptr, NULL, atoms.size, atoms.ptr)
        if err:
            raise TypeError("could not create dictionary from atoms")
        return _dict

    @classmethod
    def from_string(cls, str dict_string, bint str_is_already_json=False) -> Dictionary:
        """Create a new t_dictionary from Dictionary Syntax which is passed in as a string."""
        cdef Dictionary _dict = cls()
        cdef char * errorstring = NULL
        cdef char* dict_string_ptr = <char *>mx.sysmem_newptr((len(dict_string)+1) * sizeof(char))
        # cdef long n = len(dict_string) + 1
        strcpy(dict_string_ptr, dict_string.encode())
        cdef mx.t_max_err err = mx.dictobj_dictionaryfromstring(&_dict.ptr, dict_string_ptr, <int>str_is_already_json, errorstring)
        if err:
            raise ValueError(f"could not create dictionary from string: {errorstring.decode()}")
        mx.sysmem_freeptr(dict_string_ptr)
        return _dict

    @staticmethod
    cdef Dictionary from_ptr(mx.t_dictionary *ptr, bint owner=False, bint to_release=False, str name=""):
        """Create a Dictionary from an existing pointer."""
        # Call to __new__ bypasses __init__ constructor
        cdef Dictionary _dict = Dictionary.__new__(Dictionary)
        _dict.ptr = ptr
        _dict.ptr_owner = owner
        _dict.type_map = dict()
        _dict.to_release = to_release
        _dict.name = name
        return _dict

    def set_long(self, str key, int value):
        """Add a long integer value to the dictionary."""
        return mx.dictionary_appendlong(self.ptr, str_to_sym(key), value)

    def set_float(self, str key, double value):
        """Add a double-precision float value to the dictionary."""
        return mx.dictionary_appendfloat(self.ptr, str_to_sym(key), value)

    def set_sym(self, str key, str value):
        """Add a t_symbol* value to the dictionary."""
        return mx.dictionary_appendsym(self.ptr, str_to_sym(key), str_to_sym(value))

    def set_atom(self, str key, object obj):
        """Add a t_atom* value to the dictionary."""
        cdef Atom atom = Atom(obj)
        return mx.dictionary_appendatom(self.ptr, str_to_sym(key), atom.ptr)

    def set_atoms(self, str key, *args):
        """Add an array of atoms to the dictionary."""
        cdef Atom atom = Atom(*args)
        return mx.dictionary_appendatoms(self.ptr, str_to_sym(key), atom.size, atom.ptr)

    def set_bytes(self, str key, bytes value):
        """Add a c-string to the dictionary."""
        return mx.dictionary_appendstring(self.ptr, str_to_sym(key), value)

    def set_object(self, str key, MaxObject obj):
        """Add a *t_object to the dictionary."""
        return mx.dictionary_appendobject(self.ptr, str_to_sym(key), <mx.t_object*>obj.ptr)

    def append_atomarray(self, str key, AtomArray atomarray):
        """Add an Atom Array object to the dictionary."""
        cdef mx.t_max_err err = mx.dictionary_appendatomarray(self.ptr,
            str_to_sym(key), <mx.t_object*>atomarray.ptr)
        if err:
            raise ValueError("could not append atomarray to dictionary")

    def append_dictionary(self, str key, Dictionary dict):
        """Add a dictionary object to the dictionary."""
        return mx.dictionary_appenddictionary(self.ptr, str_to_sym(key), <mx.t_object*>dict.ptr)

    def append_object(self, str key, MaxObject obj):
        """Add an object to the dictionary."""
        return mx.dictionary_appendobject(self.ptr, str_to_sym(key), <mx.t_object*>obj.ptr)

    cdef mx.t_max_err appendobject(self, mx.t_symbol* key, mx.t_object* value):
        """Add an object to the dictionary."""
        return mx.dictionary_appendobject(self.ptr, key, value)

    def append_object_flags(self, str key, MaxObject obj, long flags):
        """Add an object to the dictionary with flags."""
        return mx.dictionary_appendobject_flags(self.ptr, str_to_sym(key), <mx.t_object*>obj.ptr, flags)

    def append_binbuf(self, str key, Binbuf binbuf):
        """Add a binary buffer to the dictionary."""
        return mx.dictionary_appendbinbuf(self.ptr, str_to_sym(key), <mx.t_object*>binbuf.ptr)

    def get_long(self, str key) -> int:
        """Retrieve a long integer from the dictionary."""
        cdef mx.t_atom_long value
        cdef mx.t_max_err err = mx.dictionary_getlong(self.ptr, str_to_sym(key), &value)
        if err:
            raise ValueError(f"could not get long value from dict with key {key}")
        return <int>value

    def get_float(self, str key) -> float:
        """Retrieve a double-precision float from the dictionary."""
        cdef double value
        cdef mx.t_max_err err = mx.dictionary_getfloat(self.ptr, str_to_sym(key), &value)
        if err:
            raise ValueError(f"could not get float value from dict with key {key}")
        return <float>value

    def get_sym(self, str key) -> str:
        """Retrieve a t_symbol* as a python string from the dictionary."""
        cdef mx.t_symbol* value
        cdef mx.t_max_err err = mx.dictionary_getsym(self.ptr, str_to_sym(key), &value)
        if err:
            raise ValueError(f"could not get symbol as str from dict with key {key}")
        return sym_to_str(value)

    def get_bytes(self, str key) -> bytes:
        """Retrieve a bytes object from the dictionary."""
        string = self.get_string(key)
        return string.encode()

    def get_atoms(self, str key) -> Atom:
        """Retrieve the address of a t_atom array of in the dictionary."""
        cdef long argc
        cdef mx.t_atom* argv
        cdef Atom atom
        cdef mx.t_max_err err = mx.dictionary_getatoms(self.ptr, str_to_sym(key), &argc, &argv)
        if err:
            raise ValueError(f"could not get atoms from dict with key {key}")
        atom =  Atom.from_ptr(argv, argc)
        return atom

    def get_list(self, str key) -> list:
        """Retrieve the address of a t_atom array of in the dictionary."""
        cdef Atom atom = self.get_atoms(key)
        return atom.to_list()

    def get_string(self, str key) -> str:
        """Retrieve a C-string pointer from the dictionary."""
        cdef const char* value
        cdef mx.t_max_err err = mx.dictionary_getstring(self.ptr, str_to_sym(key), &value)
        if err:
            raise ValueError("could not retrieve string from dictionary")
        return value.decode()

    # FIXME: crashing
    # def get_atom(self, str key) -> Atom:
    #     """Retrieve an Atom instance given a string key from the dictionary"""
    #     cdef mx.t_atom* atom
    #     cdef Atom _atom
    #     cdef mx.t_max_err err = mx.dictionary_getatom(self.d, str_to_sym(key), atom)
    #     if err:
    #         raise ValueError("could not retrieve atom from dictionary")
    #     _atom = Atom.from_ptr(atom, 1)
    #     return _atom

    cdef mx.t_max_err getatom(self, mx.t_symbol* key, mx.t_atom* value):
        """Copy a t_atom from the dictionary."""
        return mx.dictionary_getatom(self.ptr, key, value)

    cdef mx.t_max_err getatoms(self, mx.t_symbol* key, long* argc, mx.t_atom** argv):
        """Retrieve the address of a t_atom array of in the dictionary."""
        return mx.dictionary_getatoms(self.ptr, key, argc, argv)

    cdef mx.t_max_err getatoms_ext(self, mx.t_symbol* key, long stringstosymbols, long* argc, mx.t_atom** argv):
        """Retrieve the address of a t_atom array of in the dictionary."""
        return mx.dictionary_getatoms_ext(self.ptr, key, stringstosymbols, argc, argv)

    cdef mx.t_max_err copyatoms(self, mx.t_symbol* key, long* argc, mx.t_atom** argv):
        """Retrieve copies of a t_atom array in the dictionary."""
        return mx.dictionary_copyatoms(self.ptr, key, argc, argv)

    def get_atomarray(self, str key) -> AtomArray:
        """Retrieve a t_atomarray pointer from the dictionary."""
        cdef mx.t_object* ptr
        cdef mx.t_max_err err = mx.dictionary_getatomarray(self.ptr, str_to_sym(key), &ptr)
        if err:
            raise ValueError("could not get atomarray from dictionary")
        return AtomArray.from_ptr(<mx.t_atomarray*>ptr)

    cdef mx.t_max_err getatomarray(self, mx.t_symbol* key, mx.t_object** value):
        """Retrieve a t_atomarray pointer from the dictionary."""
        return mx.dictionary_getatomarray(self.ptr, key, value)

    def get_dictionary(self, str key) -> Dictionary:
        """Retrieve a t_dictionary pointer from the dictionary."""
        cdef mx.t_object* ptr
        cdef mx.t_max_err err = mx.dictionary_getdictionary(self.ptr, str_to_sym(key), &ptr)
        if err:
            raise ValueError("could not get dictionary from dictionary")
        return Dictionary.from_ptr(<mx.t_dictionary*>ptr)

    cdef mx.t_max_err getdictionary(self, mx.t_symbol* key, mx.t_object** value):
        """Retrieve a t_dictionary pointer from the dictionary."""
        return mx.dictionary_getdictionary(self.ptr, key, value)

    cdef mx.t_max_err get_ex(self, mx.t_symbol* key, long* ac, mx.t_atom** av, char* errstr):
        """Retrieve the address of a t_atom array of in the dictionary within nested dictionaries."""
        return mx.dictionary_get_ex(self.ptr, key, ac, av, errstr)

    def get_object(self, str key) -> MaxObject:
        """Retrieve a t_object pointer from the dictionary."""
        cdef mx.t_object* ptr
        cdef mx.t_max_err err = mx.dictionary_getobject(self.ptr, str_to_sym(key), &ptr)
        if err:
            raise ValueError(f"could not get object '{key}' from dictionary")
        return MaxObject.from_ptr(ptr)

    cdef mx.t_max_err getobject(self, mx.t_symbol* key, mx.t_object** value):
        """Retrieve a t_object pointer from the dictionary."""
        return mx.dictionary_getobject(self.ptr, key, value)

    def has_string_value(self, str key) -> bool:
        """Test a key to set if the data stored with that key contains a t_string object."""
        return <bint>mx.dictionary_entryisstring(self.ptr, str_to_sym(key))

    def has_atomarray_value(self, str key) -> bool:
        """Test a key to set if the data stored with that key contains a t_atomarray object."""
        return <bint>mx.dictionary_entryisatomarray(self.ptr, str_to_sym(key))

    def has_dictionary_value(self, str key) -> bool:
        """Test a key to set if the data stored with that key contains a t_dictionary object."""
        return <bint>mx.dictionary_entryisdictionary(self.ptr, str_to_sym(key))

    def has_entry(self, str key) -> bool:
        """Test a key to set if it exists in the dictionary."""
        return <bint>mx.dictionary_hasentry(self.ptr, str_to_sym(key))

    def getentrycount(self) -> int:
        """Return the number of keys in a dictionary."""
        return mx.dictionary_getentrycount(self.ptr)

    def getkeys(self) -> list[str]:
        """Retrieve all of the key names stored in a dictionary."""
        cdef long numkeys = 0
        cdef mx.t_symbol** keys
        cdef mx.t_max_err err = mx.dictionary_getkeys(self.ptr, &numkeys, &keys)
        if err:
            raise ValueError("could not retrieve keys")
        results = []
        for i in range(numkeys):
            results.append(sym_to_str(keys[i]))
        self.freekeys(numkeys, keys)
        return results

    def getkeys_ordered(self) -> list[str]:
        """Retrieve all of the key names stored in a dictionary in order."""
        cdef long numkeys = 0
        cdef mx.t_symbol** keys
        cdef mx.t_max_err err = mx.dictionary_getkeys_ordered(self.ptr, &numkeys, &keys)
        if err:
            raise ValueError("could not retrieve keys")
        results = []
        for i in range(numkeys):
            results.append(sym_to_str(keys[i]))
        self.freekeys(numkeys, keys)
        return results

    keys = getkeys_ordered

    cdef void freekeys(self, long numkeys, mx.t_symbol** keys):
        """Free memory allocated by the dictionary_getkeys() method."""
        mx.dictionary_freekeys(self.ptr, numkeys, keys)

    def delete_entry(self, str key):
        """Remove a value from the dictionary."""
        cdef mx.t_max_err err = mx.dictionary_deleteentry(self.ptr, str_to_sym(key))
        if err:
            raise ValueError(f"could not delete entry {key} from dictionary")

    def chuck_entry(self, str key):
        """Remove a value from the dictionary without freeing it."""
        cdef mx.t_max_err err = mx.dictionary_chuckentry(self.ptr, str_to_sym(key))
        if err:
            raise ValueError(f"could not chuck entry {key} from dictionary")

    def clear(self):
        """Delete all values from a dictionary."""
        cdef mx.t_max_err err = mx.dictionary_clear(self.ptr)
        if err:
            raise ValueError("could not clear dictionary")

    def clone(self) -> Dictionary:
        """Create a copy of the dictionary."""
        cdef mx.t_dictionary* clone =  mx.dictionary_clone(self.ptr)
        return Dictionary.from_ptr(clone, True)

    def clone_to_self(self, Dictionary dict_to_clone):
        """Create a copy of the dictionary and add it to the current dictionary."""
        cdef mx.t_max_err err = mx.dictionary_clone_to_existing(dict_to_clone.ptr, self.ptr)
        if err:
            raise ValueError("could not clone dictionary to self")

    def clone_to_existing(self, Dictionary existing_dict):
        """Create a copy of the dictionary and add it to an existing dictionary."""
        cdef mx.t_max_err err = mx.dictionary_clone_to_existing(self.ptr, existing_dict.ptr)
        if err:
            raise ValueError("could not clone dictionary to existing dictionary")

    # MAX_SDK BUG
    # cdef mx.t_max_err copy_to_existing(self, mx.t_dictionary* dc):
    #     return mx.dictionary_copy_to_existing(self.d, dc)

    def merge_to_existing(self, Dictionary existing_dict):
        """Merge the contents of the dictionary into an existing dictionary."""
        cdef mx.t_max_err err = mx.dictionary_merge_to_existing(self.ptr, existing_dict.ptr)
        if err:
            raise ValueError("could not merge dictionary to existing dictionary")

    def merge_to_self(self, Dictionary dict_to_merge):
        """Merge the contents of the dictionary into the current dictionary."""
        cdef mx.t_max_err err = mx.dictionary_merge_to_existing(dict_to_merge.ptr, self.ptr)
        if err:
            raise ValueError("could not merge dictionary to self")

    cdef void funall(self, mx.method fun, void* arg):
        """Call the specified function for every entry in the dictionary."""
        mx.dictionary_funall(self.ptr, fun, arg)

    cdef mx.t_symbol* entry_getkey(self, mx.t_dictionary_entry* x):
        """Given a t_dictionary_entry*, return the key associated with that entry."""
        return mx.dictionary_entry_getkey(x)

    cdef void entry_getvalue(self, mx.t_dictionary_entry* x, mx.t_atom* value):
        """Given a t_dictionary_entry*, return the value associated with that entry."""
        mx.dictionary_entry_getvalue(x, value)

    cdef void entry_getvalues(self, mx.t_dictionary_entry* x, long* argc, mx.t_atom** argv):
        """Given a t_dictionary_entry*, return the values associated with that entry."""
        mx.dictionary_entry_getvalues(x, argc, argv)

    def copy_unique(self, Dictionary copyfrom):
        """Given 2 dictionaries, copy the keys unique to one of the dictionaries to the other dictionary."""
        cdef mx.t_max_err err = mx.dictionary_copyunique(self.ptr, copyfrom.ptr)
        if err:
            raise ValueError("could not copy unique keys fromdictionary")

    def get_default_long(self, str key, long default_value) -> int:
        """Retrieve a long integer from the dictionary or a default value if the key is not found."""
        cdef mx.t_atom_long value
        cdef mx.t_max_err err = mx.dictionary_getdeflong(self.ptr, str_to_sym(key), &value, default_value)
        if err:
            raise ValueError(f"could not get long value from dict with key {key}")
        return <int>value

    def get_default_float(self, str key, float default_value) -> float:
        """Retrieve a double-precision float from the dictionary or a default value if the key is not found."""
        cdef double value
        cdef mx.t_max_err err = mx.dictionary_getdeffloat(self.ptr, str_to_sym(key), &value, default_value)
        if err:
            raise ValueError(f"could not get float value from dict with key {key}")
        return <float>value

    def get_default_sym(self, str key, str default_value) -> str:
        """Retrieve a t_symbol* from the dictionary or a default value if the key is not found."""
        cdef mx.t_symbol* value
        cdef mx.t_max_err err = mx.dictionary_getdefsym(self.ptr, str_to_sym(key), &value, str_to_sym(default_value))
        if err:
            raise ValueError(f"could not get symbol as str from dict with key {key}")
        return sym_to_str(value)

    # def get_default_atom(self, str key, Atom default_value) -> Atom:
    #     """Retrieve a t_atom* from the dictionary or a default value if the key is not found."""
    #     cdef mx.t_atom* value = <mx.t_atom*>mx.sysmem_newptr(1 * sizeof(mx.t_atom))
    #     # cdef mx.t_atom* value = NULL
    #     cdef mx.t_max_err err = mx.dictionary_getdefatom(self.ptr, str_to_sym(key), value, default_value.ptr)
    #     if err:
    #         raise ValueError(f"could not get atom from dict with key {key}")
    #     return Atom.from_ptr(value, 1)

    def get_default_string(self, str key, str default_value) -> str:
        """Retrieve a c-string from the dictionary or a default value if the key is not found."""
        cdef const char* value
        cdef mx.t_max_err err = mx.dictionary_getdefstring(self.ptr, str_to_sym(key), &value, default_value.encode())
        if err:
            raise ValueError(f"could not get string from dict with key {key}")
        return value.decode()

    cdef mx.t_max_err getdefatoms(self, mx.t_symbol* key, long* argc, mx.t_atom** argv, mx.t_atom* dfn):
        """Retrieve the address of a t_atom array in the dictionary."""
        return mx.dictionary_getdefatoms(self.ptr, key, argc, argv, dfn)

    cdef mx.t_max_err copydefatoms(self, mx.t_symbol* key, long* argc, mx.t_atom** argv, mx.t_atom* dfn):
        """Retrieve copies of a t_atom array in the dictionary."""
        return mx.dictionary_copydefatoms(self.ptr, key, argc, argv, dfn)

    def dump(self, long recurse=1, long console=0):
        """Print the contents of a dictionary to the Max window.

        @param  recurse If non-zero, the dictionary will be recursively unravelled to the Max window.
                        Otherwise it will only print the top level.
        @param  console If non-zero, the dictionary will be posted to the console rather than the Max window.
                        On the Mac you can view this using Console.app.
                        On Windows you can use the free DbgView program which can be downloaded from Microsoft.
        """
        cdef mx.t_max_err err = mx.dictionary_dump(self.ptr, recurse, console)
        if err:
            raise ValueError("could not dump dictionary")

    def copy_entries(self, Dictionary dst, list[str] keys):
        cdef mx.t_symbol** keys_ptr = <mx.t_symbol**>mx.sysmem_newptr(len(keys) * sizeof(mx.t_symbol*))
        for i, key in enumerate(keys):
            keys_ptr[i] = str_to_sym(key)
        cdef mx.t_max_err err = mx.dictionary_copyentries(self.ptr, dst.ptr, keys_ptr)
        mx.sysmem_freeptr(keys_ptr)
        if err:
            raise ValueError("could not copy entries")

    # cdef mx.t_dictionary* sprintf(self, char* fmt, ...):
    #   """Create a new dictionary populated with values using a combination of attribute and sprintf syntax."""
    #   return mx.dictionary_sprintf(char* fmt, ...)

    def transaction_lock(self):
        """Take a lock on a dictionary.

        For preventing dictionary lock for transactions across multiple calls, or holding
        on to internal dictionary element pointers for complex operations.
        """
        cdef mx.t_max_err err = mx.dictionary_transaction_lock(self.ptr)
        if err:
            raise ValueError("could not lock dictionary")

    def transaction_unlock(self):
        """Release a lock on a dictionary.

        For preventing dictionary lock for transactions across multiple calls, or holding
        on to internal dictionary element pointers for complex operations.
        """
        cdef mx.t_max_err err = mx.dictionary_transaction_unlock(self.ptr)
        if err:
            raise ValueError("could not unlock dictionary")


    # FIXME: add staticmethod as well
    cdef mx.t_max_err read(self, const char* filename, short path, mx.t_dictionary** d):
        """Read the specified JSON file and return a t_dictionary object."""
        return mx.dictionary_read(filename, path, d)

    cdef mx.t_max_err write(self, const char* filename, short path):
        """Serialize the specified t_dictionary object to a JSON file."""
        return mx.dictionary_write(self.ptr, filename, path)

    def post(self):
        """Print the contents of a dictionary to the Max window."""
        mx.postdictionary(<mx.t_object*>self.ptr)

    # ----------------------------------------------------------------------------
    # t_dictionary passing api - ext_dictobj.h

    def register(self, str name) -> Dictionary:
        """Register a #t_dictionary with the dictionary passing system and map it to a unique name.

        @param      d       A valid dictionary object.
        @param      name    The address of a #t_symbol pointer to the name you would like mapped to this dictionary.
                            If the t_symbol pointer has a NULL value then a unique name will be generated and filled-in
                            upon return.
        @return             The dictionary mapped to the specified name.
        """
        cdef mx.t_symbol* name_ptr = str_to_sym(name)
        cdef mx.t_dictionary* registered = mx.dictobj_register(self.ptr, &name_ptr)
        return Dictionary.from_ptr(registered, owner=True, to_release=False, name=name)

    def unregister(self):
        """Unregister a #t_dictionary with the dictionary passing system.

        Generally speaking you should not need to call this method.
        Calling object_free() on the #t_dictionary automatically unregisters it.
        """
        return mx.dictobj_unregister(self.ptr)

    def findregistered_clone(self, str name) -> Dictionary:
        """Find the t_dictionary for a given name, and return a copy of that dictionary.

        When you are done, do not call dictobj_release() on the dictionary, because you
        are working on a copy rather than on a retained pointer.
        """
        cdef mx.t_symbol* name_ptr = str_to_sym(name)
        cdef mx.t_dictionary* registered_clone = mx.dictobj_findregistered_clone(name_ptr)
        return Dictionary.from_ptr(registered_clone, owner=True, to_release=False, name=name)

    def findregistered_retain(self, str name) -> Dictionary:
        """Find the t_dictionary for a given name, return a pointer to that t_dictionary, and increment
        its reference count.

        When you are done you should call dictobj_release() on the dictionary.
        """
        cdef mx.t_symbol* name_ptr = str_to_sym(name)
        cdef mx.t_dictionary* registered_retain = mx.dictobj_findregistered_retain(name_ptr)
        return Dictionary.from_ptr(registered_retain, owner=True, to_release=True, name=name)

    def release(self):
        """Release a t_dictionary/name previously retained with dictobj_findregistered_retain()."""
        cdef mx.t_max_err err = mx.dictobj_release(self.ptr)
        self.name = ""
        if err:
            raise ValueError("could not release dictionary")

    def name_from_ptr(self, Dictionary dict = None) -> str:
        """Find the name associated with a given t_dictionary."""
        cdef mx.t_symbol* name = mx.gensym("")
        if dict:
            name = mx.dictobj_namefromptr(dict.ptr)
        else:
            name = mx.dictobj_namefromptr(self.ptr)
        return sym_to_str(name)

    cdef void dictobj_outlet_atoms(self, void *out, long argc, mx.t_atom *argv):
        """Send atoms to an outlet in your Max object, handling complex datatypes
        that may be present in those atoms.
        """
        mx.dictobj_outlet_atoms(out, argc, argv)

    def ensure_atom_safety(self, Atom atom) -> Atom:
        """Ensure that an atom is safe for passing.

        Atoms are allowed to be A_LONG, A_FLOAT, or A_SYM, but not A_OBJ. If the atom is an A_OBJ,
        it will be converted into something that will be safe to pass.
        """
        cdef long result = mx.dictobj_atom_safety(atom.ptr)
        if result == 1:
            mx.post("atom was changed to ensure safety")
            return atom
        else:
            mx.post("atom was already safe")
            return atom

    # cdef long dictobj_atom_safety_flags(self, mx.t_atom *a, long flags):
    #     """Ensure that an atom is safe for passing.

    #     Atoms are allowed to be A_LONG, A_FLOAT, or A_SYM, but not A_OBJ. If the atom is an A_OBJ,
    #     it will be converted into something that will be safe to pass.

    #     Pass DICTOBJ_ATOM_FLAGS_REGISTER to flaga to have dictionary atoms registered/retained.
    #     """
    #     mx.dictobj_atom_safety_flags(a, flags)

    def validate(self, Dictionary schema, Dictionary candidate) -> bool:
        """Validate the contents of a t_dictionary against a second t_dictionary containing a schema."""
        return <bint>(mx.dictobj_validate(schema.ptr, candidate.ptr) == <long>1)

    def json_from_string(self, str dict_string) -> str:
        """Convert a C-string of Dictionary Syntax into a C-string of JSON."""
        cdef char *json = NULL
        cdef long jsonsize = 0
        cdef mx.t_max_err err = mx.dictobj_jsonfromstring(&jsonsize, &json, dict_string.encode())
        if err:
            raise ValueError("could not convert dictionary to json")
        return json.decode()

    # def from_atoms_extended(self, Atom atoms) -> Dictionary:
    #     """Create a new t_dictionary from an array of atoms that use Max dictionary syntax, JSON, or compressed JSON."""
    #     cdef mx.t_dictionary* d = NULL
    #     cdef mx.t_max_err err = mx.dictobj_dictionaryfromatoms_extended(&d, NULL, atoms.size, atoms.ptr)
    #     if err:
    #         raise ValueError("could not create dictionary from atoms")
    #     return Dictionary.from_ptr(d, owner=True, to_release=False)

    def to_atoms(self, Dictionary dict = None) -> Atom:
        """Serialize the contents of a t_dictionary into array of atoms."""
        cdef long argc = 0
        cdef mx.t_atom *argv = NULL
        cdef mx.t_max_err err = mx.MAX_ERR_NONE
        if dict:
            err = mx.dictobj_dictionarytoatoms(dict.ptr, &argc, &argv)
        else:
            err = mx.dictobj_dictionarytoatoms(self.ptr, &argc, &argv)
        if err:
            raise ValueError("could not serialize dictionary to atoms")
        return Atom.from_ptr(argv, argc, owner=True)

    # cdef t_max_err dictobj_key_parse(t_object *x, t_dictionary *d, t_atom *akey, t_bool create, t_dictionary **targetdict, t_symbol **targetkey, t_int32 *index):
    #     """Given a complex key (one that includes potential heirarchy or array-member access), return the actual key and the dictionary in which the key should be referenced."""

# ----------------------------------------------------------------------------
# api.Database

cdef class DatabaseResult:
    """Wraps the t_db_result object."""

    cdef mx.t_db_result* ptr
    cdef bint ptr_owner

    def __cinit__(self):
        self.ptr = NULL
        self.ptr_owner = True

    def __dealloc__(self):
        if self.ptr and self.ptr_owner:
            mx.object_free(self.ptr)

    def __repr__(self) -> str:
        return f"<DatabaseResult recs:'{self.numrecords}'>"

    @staticmethod
    cdef from_ptr(mx.t_db_result* ptr, bint ptr_owner=False):
        cdef DatabaseResult result = DatabaseResult.__new__(DatabaseResult)
        result.ptr = ptr
        result.ptr_owner = ptr_owner
        return result

    def nextrecord(self) -> list[str]:
        """Get the next record from the result."""
        cdef char** record = mx.db_result_nextrecord(self.ptr)
        return [record[i].decode() for i in range(self.numfields())]

    def reset(self):
        """Reset the result."""
        mx.db_result_reset(self.ptr)

    def clear(self):
        """Clear the result."""
        mx.db_result_clear(self.ptr)

    def numrecords(self) -> int:
        """Get the number of records in the result."""
        cdef int result = <int>mx.db_result_numrecords(self.ptr)
        return result

    def numfields(self) -> int:
        """Get the number of fields in the result."""
        return <int>mx.db_result_numfields(self.ptr)

    def fieldname(self, long fieldindex) -> str:
        """Get the name of a field."""
        cdef char* fieldname = mx.db_result_fieldname(self.ptr, fieldindex)
        return fieldname.decode()

    def string(self, long recordindex, long fieldindex) -> str:
        """Get the string value of a field."""
        cdef char* fieldvalue = mx.db_result_string(self.ptr, recordindex, fieldindex)
        return fieldvalue.decode()

    def long(self, long recordindex, long fieldindex) -> int:
        """Get the long value of a field."""
        cdef long fieldvalue = mx.db_result_long(self.ptr, recordindex, fieldindex)
        return <int>fieldvalue

    def float(self, long recordindex, long fieldindex) -> float:
        """Get the float value of a field."""
        cdef float fieldvalue = mx.db_result_float(self.ptr, recordindex, fieldindex)
        return <float>fieldvalue

    def datetimeinseconds(self, long recordindex, long fieldindex) -> int:
        """Get the datetime in seconds of a field."""
        cdef mx.t_ptr_uint fieldvalue = mx.db_result_datetimeinseconds(self.ptr, recordindex, fieldindex)
        return <int>fieldvalue

    def datatime_as_string(self, long recordindex, long fieldindex) -> str:
        """Get the datetime as a string of a field."""
        cdef mx.t_ptr_uint fieldvalue = mx.db_result_datetimeinseconds(self.ptr, recordindex, fieldindex)
        cdef char* string = <char*>mx.sysmem_newptr(256 * sizeof(char))
        mx.db_util_datetostring(fieldvalue, string)
        cdef str result = string.decode()
        mx.sysmem_freeptr(string)
        return result

cdef class DatabaseView:
    """Wraps the t_db_view object."""
    cdef mx.t_db_view* ptr
    cdef bint ptr_owner
    cdef public str sql
    cdef Database db

    def __cinit__(self):
        self.db = None
        self.ptr = NULL
        self.ptr_owner = True
        self.sql = ""

    def __init__(self, Database db, str sql):
        self.db = db
        self.ptr = NULL
        self.ptr_owner = True
        self.sql = sql
        mx.db_view_create(db.ptr, sql.encode(), &self.ptr)

    def __dealloc__(self):
        if self.ptr is not NULL and self.db and self.ptr_owner:
            mx.db_view_remove(self.db.ptr, &self.ptr)

    def __repr__(self) -> str:
        return f"<DatabaseView '{self.db.name}'>"

    @staticmethod
    cdef DatabaseView from_ptr(Database db, mx.t_db_view* ptr, str sql, bint ptr_owner=False):
        cdef DatabaseView view = DatabaseView.__new__(DatabaseView)
        view.ptr = ptr
        view.db = db
        view.sql = sql
        view.ptr_owner = ptr_owner
        return view

    def getresult(self) -> DatabaseResult:
        """Get the result of a view."""
        cdef mx.t_db_result* result = NULL
        cdef mx.t_max_err err = mx.db_view_getresult(self.ptr, &result)
        if err:
            raise ValueError("could not get result")
        return DatabaseResult.from_ptr(result, True)

    def setquery(self, str sql):
        """Set the query of a view."""
        self.sql = sql
        cdef mx.t_max_err err = mx.db_view_setquery(self.ptr, sql.encode())
        if err:
            raise ValueError("could not set query")

cdef class Database:
    """Wraps the t_database object."""

    cdef mx.t_database *ptr
    cdef public str name
    cdef public str path

    def __cinit__(self):
        self.ptr = NULL
        self.name = ""
        self.path = ""

    def __init__(self, str db_name, str db_path):
        self.name = db_name
        self.path = db_path

    def __dealloc__(self):
        mx.db_close(&self.ptr)
        self.ptr = NULL

    def __repr__(self) -> str:
        return f"<Database '{self.db.name}'>"

    def open(self):
        """Open the database."""
        mx.db_open(str_to_sym(self.name), self.path.encode(), &self.ptr)

    def close(self):
        """Close the database."""
        mx.db_close(&self.ptr)

    def query_table_new(self, str tablename):
        """Create a new table."""
        mx.db_query_table_new(self.ptr, tablename.encode())

    def query_table_addcolumn(self, str tablename, str columnname, str columntype, str flags):
        """Add a column to a table."""
        mx.db_query_table_addcolumn(
            self.ptr,
            tablename.encode(),
            columnname.encode(),
            columntype.encode(),
            flags.encode()
        )

    def transaction_start(self):
        """Start a transaction."""
        mx.db_transaction_start(self.ptr)

    def transaction_end(self):
        """End a transaction."""
        mx.db_transaction_end(self.ptr)

    def transaction_flush(self):
        """Flush a transaction."""
        mx.db_transaction_flush(self.ptr)

    def query_direct(self, str sql) -> DatabaseResult:
        """Execute a direct query."""
        cdef mx.t_db_result* dbresult = NULL
        cdef mx.t_max_err err = mx.db_query_direct(self.ptr, &dbresult, sql.encode())
        if err:
            raise ValueError("could not execute direct query")
        return DatabaseResult.from_ptr(dbresult, True)

    cdef mx.t_max_err query_getlastinsertid(self, long *idx):
        """Get the last insert ID."""
        return mx.db_query_getlastinsertid(self.ptr, idx)

    def query_table_new(self, str tablename) -> int:
        """Create a new table."""
        return mx.db_query_table_new(self.ptr, tablename.encode())

    def query_table_addcolumn(self, str tablename, str columnname, str columntype, str flags) -> int:
        """Add a column to a table."""
        return mx.db_query_table_addcolumn(
            self.ptr,
            tablename.encode(),
            columnname.encode(),
            columntype.encode(),
            flags.encode()
        )

    def view_create(self, str sql) -> DatabaseView:
        """Create a view."""
        cdef DatabaseView view = DatabaseView(self, sql)
        return view

    def view_remove(self, DatabaseView dbview):
        """Remove a view."""
        if not dbview.ptr_owner or dbview.db:
            mx.db_view_remove(self.ptr, &dbview.ptr)
        else:
            del dbview

    # cdef t_max_err db_query(t_database *db, t_db_result **dbresult, const char *sql, ...)
    # cdef t_max_err db_query_silent(t_database *db, t_db_result **dbresult, const char *sql, ...)

# ----------------------------------------------------------------------------
# api.List

cdef class List:
    """Wraps the t_linklist object."""

    cdef mx.t_linklist* ptr


    def __cinit__(self):
        self.ptr = <mx.t_linklist*>mx.linklist_new()

    def __dealloc__(self):
        mx.linklist_chuck(self.ptr)  # will free list only and not contained objects
        #  or
        #  object_free(self.ptr)  # will free all in list

    def __repr__(self) -> str:
        return f"<List size:{self.get_size()}'>"

    def __getitem__(self, long index) -> MaxObject:
        cdef mx.t_object* obj = <mx.t_object*>mx.linklist_getindex(self.ptr, index)
        return MaxObject.from_ptr(obj)

    def __setitem__(self, long index, MaxObject obj):
        self.insert(index, obj)

    def __delitem__(self, long index):
        self.delete_index(index)

    @property
    def size(self) -> int:
        """Get the size of the linklist."""
        return mx.linklist_getsize(self.ptr)

    def get(self, long idx = 0) -> MaxObject:
        """Return a t_object stored in a linklist at a specified index."""
        cdef mx.t_object* obj = NULL
        if not (0 <= idx <= self.size -1):
            raise IndexError("index out of range")
        obj = <mx.t_object*>mx.linklist_getindex(self.ptr, idx)
        if obj is NULL:
            raise ValueError("value could not be retrieved from index {idx}")
        return MaxObject.from_ptr(obj)

    def chuck(self):
        """Free a linklist, but don't free the items it contains.

        The linklist can contain a variety of different types of data.
        By default, the linklist assumes that all items are max objects with a valid
        #t_object header.

        You can alter the linklist's notion of what it contains by using the
        linklist_flags() method.

        When you free the linklist by calling object_free() it then tries to free all of the items it contains.
        If the linklist is storing a custom type of data, or should otherwise not free the data it contains,
        then call linklist_chuck() to free the object instead of object_free().
        """
        mx.linklist_chuck(self.ptr)

    def get_size(self) -> int:
        """Get the size of the linklist."""
        return mx.linklist_getsize(self.ptr)

    def get_index_of_object(self, MaxObject obj) -> int:
        """Return an item's index, given the item itself."""
        cdef long idx = mx.linklist_objptr2index(self.ptr, <mx.t_object*>obj.ptr)
        if idx == mx.MAX_ERR_GENERIC:
            raise IndexError("could not get index from object")
        return idx

    def append(self, MaxObject obj):
        """Add an item to the end of the list."""
        cdef mx.t_atom_long err = mx.linklist_append(self.ptr, obj.ptr)
        if err == -1:
            raise ValueError("append object failed")

    def insert(self, long index, MaxObject obj):
        """Insert an object at a given index."""
        cdef mx.t_atom_long err = mx.linklist_insertindex(self.ptr, obj.ptr, index)
        if err == -1:
            raise ValueError("append object failed")

    def delete_index(self, long index):
        """Remove the item from the list at the specified index and free it.

        The linklist can contain a variety of different types of data.
        By default, the linklist assumes that all items are max objects with a valid
        #t_object header.  Thus by default, it frees items by calling object_free() on them.

        You can alter the linklist's notion of what it contains by using the
        linklist_flags() method.

        If you wish to remove an item from the linklist and free it yourself, then you
        should use linklist_chuckptr().
        """
        cdef mx.t_max_err err = mx.linklist_deleteindex(self.ptr, index)
        if err:
            raise ValueError("could not delete object at index")

    def chuck_index(self, long index):
        """Remove the item from the list at the specified index.

        You are responsible for freeing any memory associated with the item that is
        removed from the linklist.
        """
        cdef mx.t_max_err err = mx.linklist_chuckindex(self.ptr, index)
        if err:
            raise ValueError("could not chuck object at index")

    def chuck_object(self, MaxObject obj) -> int:
        """Remove the specified item from the list.

        You are responsible for freeing any memory associated with the item that is
        removed from the linklist.
        """
        cdef long err = mx.linklist_chuckobject(self.ptr, obj.ptr)
        return err

    def delete_object(self, MaxObject obj) -> int:
        """Delete the specified item from the list.

        The object is removed from the list and deleted.
        The deletion is done with respect to any flags passed to linklist_flags.
        """
        cdef long err = mx.linklist_chuckobject(self.ptr, obj.ptr)
        return err

    def clear(self):
        """Remove and free all items in the list.

        Freeing items in the list is subject to the same rules as linklist_deleteindex().
        You can alter the linklist's notion of what it contains, and thus how items are freed,
        by using the linklist_flags() method.
        """
        mx.linklist_clear(self.ptr)

    cdef mx.t_atom_long makearray(self, void** a, long max):
        """Retrieve linklist items as an array of pointers."""
        return mx.linklist_makearray(self.ptr, a, max)

    def reverse(self):
        """Reverse the order of items in the linked-list."""
        mx.linklist_reverse(self.ptr)

    def rotate(self, long i):
        """Rotate items in the linked list in circular fashion."""
        mx.linklist_rotate(self.ptr, i)

    def shuffle(self):
        """Randomize the order of items in the linked-list."""
        mx.linklist_shuffle(self.ptr)

    def swap(self, long a, long b):
        """Swap the position of two items in the linked-list, specified by index."""
        mx.linklist_swap(self.ptr, a, b)

    cdef void* substitute(self, void* p, void* newp):
        """Substitute an object with a new object."""
        return mx.linklist_substitute(self.ptr, p, newp)

    cdef void* next_obj(self, void* p, void** next):
        """Get the next object."""
        return mx.linklist_next(self.ptr, p, next)

    cdef void* prev_obj(self, void* p, void** prev):
        """Get the previous object."""
        return mx.linklist_prev(self.ptr, p, prev)

    cdef void* last_obj(self, void** item):
        """Get the last object."""
        return mx.linklist_last(self.ptr, item)

    def readonly(self, long readonly=1):
        """Set the readonly flag."""
        mx.linklist_readonly(self.ptr, readonly)

    def flags(self, int flags):
        """Set the flags."""
        mx.linklist_flags(self.ptr, <long>flags)

    def getflags(self) -> int:
        """Get the flags."""
        return <mx.t_atom_long>mx.linklist_getflags(self.ptr)

    cdef long match(self, void* a, void* b):
        """Match two objects."""
        return mx.linklist_match(a, b)

    cdef void funall(self, mx.method fun, void* arg):
        """Call a function on all objects in the linklist."""
        mx.linklist_funall(self.ptr, fun, arg)

# ----------------------------------------------------------------------------
# api.Binbuf

cdef class Binbuf:
    """Wraps the t_binbuf object."""

    cdef mx.t_binbuf* ptr

    def __cinit__(self):
        self.ptr = <mx.t_binbuf*>mx.binbuf_new()
        if not self.ptr:
            raise MemoryError

    def __dealloc__(self):
        mx.object_free(self.ptr)

    def __repr__(self) -> str:
        return "<Binbuf>"

    def append(self, str receiver, *args):
        """Append t_atoms to a Binbuf without modifying them."""
        _args = [receiver] + list(args)
        cdef Atom atom = Atom(*_args)
        mx.binbuf_append(self.ptr, NULL, atom.size, atom.ptr)

    def insert(self, *args):
        """Used if you want to saving your object into a Binbuf.

        It assumes the message is part of a file that will later be evaluated, such as a
        Patcher file. The first argument argv[0] will be the receiver of the
        message and must be a Symbol.
        """
        cdef Atom atom = Atom(*args)
        assert isinstance(args[0], str), "binbuf.insert error: first argument as receiver must be a symbol"
        mx.binbuf_insert(self.ptr, NULL, atom.size, atom.ptr)

    # cdef void vinsert(self, char *fmt, ...):
    #     mx.binbuf_vinsert(self.ptr, fmt, ...)

    def eval(self):
        """Evaluate a Max message in a Binbuf."""
        # mx.critical_enter(<mx.t_critical>0)
        self.eval_msg_to(0, NULL, NULL)
        # mx.critical_exit(<mx.t_critical>0)

    cdef void * eval_msg_to(self, short ac, mx.t_atom *av, void *to):
        """Evaluate a Max message in a Binbuf, passing it arguments.

        Evaluates the message in a Binbuf with arguments in argv, and sends
        it to receiver.
        """
        return mx.binbuf_eval(self.ptr, ac, av, to)

    def add_text(self, str text):
        """Used binbuf_text() to convert a text handle to a Binbuf.

        binbuf_text() parses the text and converts it into binary format.
        Use it to evaluate a text file or text line entry into a  Binbuf.
        """
        cdef char* src_text = <char *>mx.sysmem_newptr((len(text)+1) * sizeof(char))
        cdef long n = len(text) + 1
        cdef short err = 0

        strcpy(src_text, text.encode())
        err = mx.binbuf_text(self.ptr, &src_text, n)
        mx.sysmem_freeptr(src_text)
        if err:
            return error("binbuf.add_text failed")

    def new_from_clipboard(self, str text):
        """Evaluate the text in the binbuf by sending it the Max clipboard

        Thanks to 11OLSEN for the nifty solution
        https://cycling74.com/forums/on-the-current-utility-of-binbufs-and-atombufs
        """
        cdef Max app = Max()
        cdef mx.t_object* clipboard = <mx.t_object*>mx.object_new(
            mx.gensym("nobox"), mx.gensym("clipboard"))
        self.add_text(text)
        mx.object_method(clipboard, mx.gensym("frombinbuf"), self.ptr)
        mx.object_method(app.ptr, mx.gensym("newfromclipboard"))
        mx.object_free(clipboard)

    def to_text(self) -> str:
        """Convert a Binbuf into a text handle.

        Backslashes are added to protect literal commas and semicolons
        contained in symbols. The pseudo-types are converted into commas,
        semicolons, or dollar-sign and number, without backslashes preceding
        them. binbuf_text can read the output of binbuf_totext and
        make the same Binbuf.
        """
        cdef mx.t_handle contents = mx.sysmem_newhandle(0)
        cdef mx.t_ptr_size size = 0
        cdef bytes result
        if mx.binbuf_totext(<mx.t_binbuf *>self.ptr, contents, &size):
            error("could convert binbuf to text")
            raise MemoryError

        if size:
            result = <bytes>contents[0]
        mx.sysmem_freehandle(contents)
        return result.decode()

    cdef short getatom(self, long *p1, long *p2, mx.t_atom *ap):
        """Get an atom from a Binbuf."""
        return mx.binbuf_getatom(self.ptr, p1, p2, ap)

    cdef void set(self, mx.t_symbol *s, short argc, mx.t_atom *argv):
        """Set the contents of a Binbuf."""
        mx.binbuf_set(self.ptr, s, argc, argv)

    cdef void delete(self, long from_type, long to_type, long from_data, long to_data):
        """Delete a range of data from a Binbuf."""
        mx.binbuf_delete(self.ptr, from_type, to_type, from_data, to_data)

    cdef void addtext(self, char **text, long size):
        """Add text to a Binbuf."""
        mx.binbuf_addtext(self.ptr, text, size)

    cdef short readatom(self, char *outstr, char **text, long *n, long e, mx.t_atom *ap):
        """Use readatom() to read a single t_atom from a text buffer."""
        mx.readatom(outstr, text, n, e, ap)

# ---------------------------------------------------------------------------
# api.Atombuf

cdef class Atombuf:
    """Wrapper around the t_atombuf object.

    An alternative to Binbufs for temporary storage of atoms.
    """
    cdef mx.t_atombuf* ptr
    cdef bint ptr_owner

    def __cinit__(self):
        self.ptr = NULL
        self.ptr_owner = False

    def __dealloc__(self):
        # De-allocate if not null and flag is set
        if self.ptr is not NULL and self.ptr_owner is True:
            mx.atombuf_free(self.ptr)
            self.ptr = NULL

    def __init__(self, *args):
        cdef Atom atom = Atom(*args)
        self.ptr = <mx.t_atombuf*>mx.atombuf_new(atom.size, atom.ptr)
        self.ptr_owner = True
        if not self.ptr:
            raise MemoryError

    def __repr__(self) -> str:
        return "<Atombuf>"

    @staticmethod
    cdef Atombuf from_ptr(mx.t_atombuf *ptr, bint owner=False):
        """Create an Atombuf from a pointer."""
        cdef Atombuf atombuf = Atombuf.__new__(Atombuf)
        atombuf.ptr = ptr
        atombuf.ptr_owner = owner
        return atombuf

    @staticmethod
    def new() -> Atombuf:
        """Create an empty Atombuf instance."""
        cdef mx.t_atombuf *ptr = <mx.t_atombuf*>mx.atombuf_new(0, NULL)
        if ptr is NULL:
            raise MemoryError
        return Atombuf.from_ptr(ptr, owner=True)

    def add_text(self, str text):
        """Adds a text -- convert text to a t_atom array in a t_atombuf.

        To use this routine to create a new Atombuf from the text buffer,
        first create a new empty t_atombuf with a call to atombuf_new(0,NULL).
        """
        cdef char* src_text = <char *>mx.sysmem_newptr((len(text)+1) * sizeof(char))
        cdef long n = len(text) + 1
        strcpy(src_text, text.encode())
        mx.atombuf_text(&self.ptr, &src_text, n)
        mx.sysmem_freeptr(src_text)

    def to_text(self) -> str:
        """Convert an atombuf into a text handle."""
        cdef mx.t_handle contents = mx.sysmem_newhandle(0)
        cdef long size = 0
        cdef bytes result
        if mx.atombuf_totext(<mx.t_atombuf *>self.ptr, contents, &size):
            error("could convert atombuf to text")
            raise MemoryError

        if size:
            result = <bytes>contents[0]
        mx.sysmem_freehandle(contents)
        return result.decode()

    def to_list(self) -> list:
        """Convert contents of atombuf to a list."""
        cdef Atom atom = Atom.from_ptr(self.ptr.a_argv, self.ptr.a_argc)
        return atom.to_list()

# ---------------------------------------------------------------------------
# api.Hashtab

cdef class Hashtab:
    """Wrapper around the t_hashtab object."""

    cdef mx.t_hashtab* ptr
    cdef public long slotcount
    cdef public dict type_map

    def __cinit__(self, long slotcount = 59):
        self.ptr = mx.hashtab_new(slotcount)
        self.slotcount = slotcount
        self.type_map = {}

    def __dealloc__(self):
        mx.object_free(self.ptr)

    def __repr__(self) -> str:
        return f"<Hashtab size:{self.size}>"

    def __len__(self) -> int:
        return self.size

    def __contains__(self, str key) -> bool:
        return key in self.type_map

    def __setitem__(self, str key, object value):
        if isinstance(value, int):
            self.type_map[key] = 'long'
            self.store_long(key, <long>value)
        elif isinstance(value, str):
            self.type_map[key] = 'str'
            self.store_sym(key, value)
        elif isinstance(value, MaxObject):
            self.type_map[key] = 'object'
            self.store(key, value)
        else:
            raise TypeError("unable to recognize value type for hashtab")

    def __getitem__(self, str key):
        return {
            'long': self.lookup_long,
            'str': self.lookup_sym,
            'object': self.lookup,
        }[self.type_map[key]](key)

    def __delitem__(self, str key):
        self.delete(key)
        del self.type_map[key]

    def __iter__(self):
        return iter(self.get_keys())

    @property
    def size(self) -> int:
        """Return the number of items stored in a hashtab."""
        return <long>mx.hashtab_getsize(self.ptr)

    def store(self, str key, MaxObject val):
        """Store an object in the hashtab."""
        cdef mx.t_max_err err = mx.hashtab_store(self.ptr, str_to_sym(key), val.ptr)
        if err:
            raise ValueError("could not store object in hashtab")

    def store_long(self, str key, long val):
        """Store a long in the hashtab."""
        cdef mx.t_max_err err = mx.hashtab_storelong(self.ptr, str_to_sym(key), val)
        if err:
            raise ValueError("could not store long in hashtab")

    def store_sym(self, str key, str val):
        """Store a symbol in the hashtab."""
        cdef mx.t_max_err err = mx.hashtab_storesym(self.ptr, str_to_sym(key), str_to_sym(val))
        if err:
            raise ValueError("could not store symbol in hashtab")

    def store_safe(self, str key, MaxObject val):
        """Store an object in the hashtab safely.

        The difference between hashtab_store_safe() and hashtab_store() is what happens
        in the event of a collision in the hash table.

        The normal hashtab_store() function will free the existing value at the collision
        location with sysmem_freeptr() and then replaces it. This version doesn't try to free
        the existing value at the collision location, but instead just over-writes it.
        """
        cdef mx.t_max_err err = mx.hashtab_store_safe(self.ptr, str_to_sym(key), val.ptr)
        if err:
            raise ValueError("could not store_safe object in hashtab")

    def store_with_flags(self, str key, MaxObject val, long flags):
        """Store an item in a hashtab with an associated key and also flags that define
        the behavior of the item.

        The hashtab_store() method is the same as calling this method with the default (0) flags.
        """
        cdef mx.t_max_err err = mx.hashtab_storeflags(self.ptr, str_to_sym(key), val.ptr, flags)
        if err:
            raise ValueError("could not store object in hashtab with flags")

    def lookup(self, str key) -> MaxObject:
        """Lookup an object in the hashtab."""
        cdef mx.t_object* val = NULL
        cdef mx.t_max_err err = mx.hashtab_lookup(self.ptr, str_to_sym(key), &val)
        if err:
            raise ValueError("could not lookup object in hashtab")
        return MaxObject.from_ptr(val)

    def lookup_long(self, str key) -> int:
        """Lookup a long in the hashtab."""
        cdef mx.t_atom_long val = 0
        cdef mx.t_max_err err = mx.hashtab_lookuplong(self.ptr, str_to_sym(key), &val)
        if err:
            raise ValueError("could not lookup long in hashtab")
        return val

    def lookup_sym(self, str key) -> str:
        """Lookup a symbol in the hashtab."""
        cdef mx.t_symbol* val = NULL
        cdef mx.t_max_err err = mx.hashtab_lookupsym(self.ptr, str_to_sym(key), &val)
        if err:
            raise ValueError("could not lookup symbol in hashtab")
        return sym_to_str(val)

    def lookup_with_flags(self, str key) -> tuple[MaxObject, int]:
        """Return an item stored in a hashtab with the specified key,
        also returning the items flags.
        """
        cdef mx.t_object* val = NULL
        cdef long flags = 0
        cdef mx.t_max_err err = mx.hashtab_lookupflags(self.ptr, str_to_sym(key), &val, &flags)
        if err:
            raise ValueError("could not lookup object with flags in hashtab")
        return (MaxObject.from_ptr(val), flags)

    cdef delete(self, str key):
        """Remove an item from a hashtab associated with the specified key and free it.

        The hashtab can contain a variety of different types of data.
        By default, the hashtab assumes that all items are max objects with a valid
        #t_object header.  Thus by default, it frees items by calling object_free() on them.

        You can alter the hashtab's notion of what it contains by using the
        hashtab_flags() method.

        If you wish to remove an item from the hashtab and free it yourself, then you
        should use hashtab_chuckkey().
        """
        cdef mx.t_max_err err = mx.hashtab_delete(self.ptr,  str_to_sym(key))
        if err:
            raise ValueError(f"could not delete object in hashtab with key {key}")

    def clear(self):
        """Delete all items stored in a hashtab."""
        mx.hashtab_clear(self.ptr)
        self.type_map.clear()
        # if err:
        #     raise ValueError(f"could not clear the hashtab")

    cdef chuck_key(self, str key):
        """Remove an item from a hashtab associated with a given key.

        You are responsible for freeing any memory associated with the item
        that is removed from the hashtab.
        """
        cdef mx.t_max_err err = mx.hashtab_chuckkey(self.ptr,  str_to_sym(key))
        if err:
            raise ValueError(f"could not chuck key {key} in hashtab")
        del self.type_map[key]

    def chuck(self):
        """Free a hashtab, but don't free the items it contains.

        The hashtab can contain a variety of different types of data.
        By default, the hashtab assumes that all items are max objects with
        a valid #t_object header.

        You can alter the hashtab's notion of what it contains by using the
        hashtab_flags() method.

        When you free the hashtab by calling object_free() it then tries to free
        all of the items it contains.   If the hashtab is storing a custom type
        of data, or should otherwise not free the data it contains, then call
        hashtab_chuck() to free the object instead of object_free().
        """
        cdef mx.t_max_err err = mx.hashtab_chuck(self.ptr)
        if err:
            raise ValueError("could not chuck the hashtab")
        self.type_map.clear()

    cdef mx.t_max_err funall(self, mx.method fun, void* arg):
        """Call the specified function for every item in the hashtab.


        fun     The function to call, specified as function pointer cast to a Max #method.
        arg     An argument that you would like to pass to the function being called.
        return  A max error code.

        @remark The hashtab_funall() method will call your function for every item in the list.
                It will pass both a pointer to the item in the list, and any argument that you
                provide.
                The following example shows a function that could be called by hashtab_funall().
        @code
        void myFun(t_hashtab_entry *e, void *myArg)
        {
            if (e->key && e->value) {
                // do something with e->key, e->value, and myArg here as appropriate
            }
        }
        """
        return mx.hashtab_funall(self.ptr, fun, arg)

    def print(self):
        """Post a hashtab's statistics to the Max window."""
        mx.hashtab_print(self.ptr)

    def set_readonly(self, bint readonly = False):
        """Set the hashtab's readonly bit.

        By default the readonly bit is 0, indicating that it is threadsafe
        for both reading and writing. setting the readonly bit to 1 will disable
        the hashtab's theadsafety mechanism, increasing  performance but at the
        expense of threadsafe operation.

        Unless you can guarantee the threading context for a hashtab's use, you
        should leave this set to 0.
        """
        mx.hashtab_readonly(self.ptr, readonly)

    def set_flags(self, long flags):
        """Set the hashtab's datastore flags.

        The available flags are enumerated in #e_max_datastore_flags.
        These flags control the behavior of the hashtab, particularly
        when removing items from the list using functions such as
        hashtab_clear(), hashtab_delete(), or when freeing the hashtab itself.
        """
        mx.hashtab_flags(self.ptr, flags)

    def get_flags(self) -> int:
        """Get the hashtab's datastore flags."""
        return <mx.t_atom_long>mx.hashtab_getflags(self.ptr)

    def set_key_flags(self, str key, long flags):
        """Change the flags for an item stored in the hashtab with a given key."""
        cdef mx.t_max_err err = mx.hashtab_keyflags(self.ptr,
            str_to_sym(key), flags)
        if err:
            raise KeyError(f"could not set flags for key: {key}")

    def get_key_flags(self, str key) -> int:
        """Retrieve the flags for an item stored in the hashtab with a given key."""
        return <mx.t_atom_long>mx.hashtab_getkeyflags(self.ptr,
            str_to_sym(key))

    def get_keys(self):
        """Retrieve all of the keys stored in a hashtab.

        If the kc and kv parameters are properly initialized to zero,
        then hashtab_getkeys() will allocate memory for the keys it returns.
        You are then responsible for freeing this memory using sysmem_freeptr().
        """
        cdef long kc = 0
        cdef mx.t_symbol** kv = NULL
        cdef mx.t_max_err err = mx.hashtab_getkeys(self.ptr, &kc, &kv)
        if err:
            raise KeyError("could not get keys from hashtab")
        results = []
        for i in range(kc):
            results.append(sym_to_str(kv[i]))
        if kv:
            mx.sysmem_freeptr(kv)
        return results

# ---------------------------------------------------------------------------
# api.AtomArray

cdef class AtomArray:
    """Wrapper around the t_atomarray object.

    An atomarray is basically just a container for the typical
    pair of long arg and t_atom* argv.

    Note that atoms provided to atomarray_new are copied.
    """

    cdef mx.t_atomarray *ptr
    cdef bint owner

    def __cinit__(self):
        self.ptr = NULL
        self.owner = False

    def __dealloc__(self):
        if self.ptr and self.owner:
            mx.object_free(self.ptr)

    def __init__(self, *args):
        cdef Atom atom = Atom(*args)
        self.ptr = mx.atomarray_new(atom.size, atom.ptr)
        self.owner = True

    def __repr__(self) -> str:
        return f"<AtomArray size:{self.size}>"

    @staticmethod
    cdef AtomArray from_atom(mx.t_atom *av, int ac):
        """Create an AtomArray from an atom array."""
        cdef AtomArray atom_array = AtomArray.__new__(AtomArray)
        atom_array.ptr = mx.atomarray_new(ac, av)
        atom_array.owner = True
        return atom_array

    @staticmethod
    cdef AtomArray from_ptr(mx.t_atomarray *ptr, bint owner=False):
        """Create an AtomArray from a pointer."""
        cdef AtomArray atom_array = AtomArray.__new__(AtomArray)
        atom_array.ptr = ptr
        atom_array.owner = owner
        return atom_array

    @staticmethod
    cdef AtomArray new(int size):
        """Create an AtomArray with a given size."""
        cdef mx.t_atom *ptr = <mx.t_atom *>mx.sysmem_newptr(size * sizeof(mx.t_atom))
        if ptr is NULL:
            raise MemoryError
        return AtomArray.from_atom(ptr, size)

    @property
    def size(self) -> int:
        """get number of atoms in atom array"""
        return self.getsize()

    def set_flags(self, long flags):
        """Set the atomarray flags."""
        mx.atomarray_flags(self.ptr, <long>flags)

    def get_flags(self) -> int:
        """Get the atomarray flags."""
        return <long>mx.atomarray_getflags(self.ptr)

    flags = property(get_flags, set_flags)

    def set_atoms(self, Atom atom):
        """Replace the existing array with a new (copied) set of atoms."""
        cdef mx.t_max_err err = mx.atomarray_setatoms(self.ptr, atom.size, atom.ptr)
        if err:
            raise ValueError("could not replace existing atoms with provided atoms")

    def get_atoms(self) -> Atom:
        """Retrieve a pointer to the first atom in the internal array of atoms."""
        cdef mx.t_atom* av = NULL
        cdef long ac = 0
        cdef mx.t_max_err err = mx.atomarray_getatoms(self.ptr, &ac, &av)
        if err:
            raise ValueError("could not get atoms from atomarray")
        return Atom.from_ptr(av, ac)

    def copy_atoms(self) -> Atom:
        """Retrieve a copy of the atoms in the array."""
        cdef mx.t_atom* av = NULL
        cdef long ac = 0
        cdef mx.t_max_err err = mx.atomarray_copyatoms(self.ptr, &ac, &av)
        if err:
            raise ValueError("could not copy atoms from atomarray")
        return Atom.from_ptr(av, ac)

    def get_atom_from_index(self, long index) -> Atom:
        """Copy an a specific atom from the array."""
        cdef Atom atom = Atom.new(1)
        cdef mx.t_max_err err = mx.atomarray_getindex(self.ptr, index, atom.ptr)
        if err:
            raise ValueError("could not get atom from index from atomarray")
        return atom

    def duplicate(self) -> AtomArray:
        """Create a new atomarray object which is a copy of another atomarray object."""
        cdef mx.t_atomarray* dup = NULL
        if self.ptr is not NULL:
            dup = <mx.t_atomarray*>mx.atomarray_duplicate(self.ptr)
            return AtomArray.from_ptr(dup)
        raise ValueError("could not duplicate an uninitialized atomarray")

    def clone(self) -> AtomArray:
        """Create a new atomarray object which is a full clone of another atomarray object."""
        cdef mx.t_atomarray* _clone = NULL
        if self.ptr is not NULL:
            _clone = <mx.t_atomarray*>mx.atomarray_clone(self.ptr)
            return AtomArray.from_ptr(_clone)
        raise ValueError("could not clone an uninitialized atomarray")

    def append_atom(self, Atom atom):
        """Copy a new atom onto the end of the array."""
        mx.atomarray_appendatom(self.ptr, atom.ptr)

    def append_atoms(self, Atom atom):
        """Copy multiple new atoms onto the end of the array."""
        mx.atomarray_appendatoms(self.ptr, atom.size, atom.ptr)

    def chuck_index(self, long index):
        """Remove an atom from any location within the array.

        The array will be resized and collapsed to fill in the gap.
        """
        mx.atomarray_chuckindex(self.ptr, index)

    def clear(self):
        """Clear the array.

        Frees all of the atoms and sets the size to zero.
        """
        mx.atomarray_clear(self.ptr)

    cdef void funall(self, mx.method fun, void* arg):
        """Call the specified function for every item in the atom array."""
        mx.atomarray_funall(self.ptr, fun, arg)

# ----------------------------------------------------------------------------
# api.Patcher

cdef class Patcher:
    """A wrapper class for a Max patcher."""

    cdef mx.t_object *ptr
    cdef bint owner

    def __cinit__(self):
        self.ptr = NULL
        self.owner = False

    def __repr__(self) -> str:
        return f"<Patcher '{self.name}'>"

    @staticmethod
    cdef Patcher from_object(mx.t_object *x):
        """Create a reference to a patcher object from object."""
        cdef Patcher patcher = Patcher.__new__(Patcher)
        cdef mx.t_max_err err = mx.object_obex_lookup(x, mx.gensym("#P"), &patcher.ptr)
        if err:
            raise TypeError("unable to obtain owning patcher for object")
        if patcher.ptr is NULL:
            raise MemoryError
        return patcher

    @staticmethod
    cdef Patcher from_ptr(mx.t_object *x, bint owner=False):
        """Create a reference to a patcher object from a pointer."""
        cdef Patcher patcher = Patcher.__new__(Patcher)
        patcher.ptr = x
        patcher.owner = owner
        return patcher

    def is_patcher(self) -> bool:
        """determine if a t_object is a patcher"""
        return bool(mx.jpatcher_is_patcher(self.ptr))

    def get_sym_attr(self, name) -> str:
        """Get a symbol attribute."""
        cdef mx.t_symbol *attr_sym = <mx.t_symbol *>mx.object_attr_getsym(
            self.ptr, str_to_sym(name))
        return sym_to_str(attr_sym)

    def get_long_attr(self, name) -> int:
        """Get a long attribute."""
        return mx.object_attr_getlong(self.ptr, str_to_sym(name))

    def get_char_attr(self, name) -> bool:
        """Get a char attribute."""
        return mx.object_attr_getchar(self.ptr, str_to_sym(name))

    def registered_names(self, str namespace = "box") -> list[str]:
        """Returns all registered names in a 'box' or 'nobox' namespace."""
        assert namespace in ['box', 'nobox'], "namespaces can be either 'box' or 'nobox'"
        cdef long namecount
        cdef mx.t_symbol ** names = NULL
        cdef mx.t_symbol * name = NULL
        cdef mx.t_max_err err = mx.object_register_getnames(
            str_to_sym(namespace), &namecount, &names)
        cdef Atom atom = Atom.new(namecount)
        if (err != mx.MAX_ERR_NONE and names is NULL):
            raise ValueError("could not retrieve registered names")
        for i in range(namecount):
            name = names[i]
            mx.atom_setsym(atom.ptr + <int>i, name)
        mx.sysmem_freeptr(names)
        return atom.to_list()

    # array props

    def get_arr_attr(self, str name) -> list:
        """t_max_err object_attr_getvalueof(void *x, t_symbol *s, long *argc, t_atom **argv)"""
        cdef mx.t_atom *argv = NULL
        cdef long argc = 0

        mx.object_attr_getvalueof(self.ptr, str_to_sym(name), &argc, &argv)
        atom = Atom.from_ptr(argv, argc)
        result = atom.to_list()
        mx.sysmem_freeptr(argv)
        return result

    # char/bool props

    @property
    def locked(self) -> bool:
        """Get the locked flag."""
        return self.get_char_attr("locked")

    @property
    def bglocked(self) -> bool:
        """Get the background locked flag."""
        return self.get_char_attr("bglocked")

    @property
    def presentation(self) -> bool:
        """Get the presentation flag."""
        return self.get_char_attr("presentation")

    @property
    def openinpresentation(self) -> bool:
        """Get the open in presentation flag."""
        return self.get_char_attr("openinpresentation")

    @property
    def cansave(self) -> bool:
        """Get the can save flag."""
        return self.get_char_attr("cansave")

    @property
    def dirty(self) -> bool:
        """Get the dirty flag."""
        return self.get_char_attr("dirty")

    @property
    def toolbarvisible(self) -> bool:
        """Get the toolbar visible flag."""
        return self.get_char_attr("toolbarvisible")

    # sym props

    @property
    def title(self) -> str:
        """Get the title."""
        return self.get_sym_attr("title")

    @property
    def fulltitle(self) -> str:
        """Get the full title."""
        return self.get_sym_attr("fulltitle")

    @property
    def name(self) -> str:
        """Get the name."""
        return self.get_sym_attr("name")

    @property
    def filename(self) -> str:
        """Get the filename."""
        return self.get_sym_attr("filename")

    @property
    def filepath(self) -> str:
        """Get the filepath."""
        return self.get_sym_attr("filepath")

    # int/long props

    @property
    def count(self) -> int:
        """Get the count."""
        return self.get_long_attr("count")

    @property
    def fgcount(self) -> int:
        """Get the foreground count."""
        return self.get_long_attr("fgcount")

    @property
    def bgcount(self) -> int:
        """Get the background count."""
        return self.get_long_attr("bgcount")

    @property
    def fileversion(self) -> int:
        """Get the file version."""
        return self.get_long_attr("fileversion")

    @property
    def numviews(self) -> int:
        """Get the number of views."""
        return self.get_long_attr("numviews")

    @property
    def numwindowviews(self) -> int:
        """Get the number of window views."""
        return self.get_long_attr("numwindowviews")

    @property
    def default_fontface(self) -> int:
        """Get the default font face."""
        return self.get_long_attr("default_fontface")

    @property
    def toolbarheight(self) -> int:
        """Get the toolbar height."""
        return self.get_long_attr("toolbarheight")

    # array props

    @property
    def rect(self) -> list:
        """Get the rect coordinates."""
        return self.get_arr_attr("rect")


    # object methods

    def get_named_box(self, str name) -> Box:
        """Get a named box in the patcher."""
        cdef mx.t_object * box_ptr = <mx.t_object *>mx.object_method(
            self.ptr, mx.gensym("getnamedbox"), str_to_sym(name))
        return Box.from_ptr(box_ptr)

    cdef mx.t_object* get_namedbox(self, str name):
        """Get a named box in the patcher."""
        return <mx.t_object *>mx.object_method(
            self.ptr, mx.gensym("getnamedbox"), str_to_sym(name))

    def get_object_in_named_box(self, str name) -> MaxObject:
        """Get the object in a named box in the patcher."""
        cdef mx.t_object * box_ptr = <mx.t_object *>mx.object_method(
            self.ptr, mx.gensym("getnamedbox"), str_to_sym(name))
        cdef mx.t_object * obj_ptr = <mx.t_object*>mx.jbox_get_object(box_ptr)
        return MaxObject.from_ptr(obj_ptr)

    cdef mx.t_object* get_namedbox_object(self, str name):
        """Get object contained in the named box in the patcher. (custom)"""
        # find obj by script name
        cdef mx.t_object * box = <mx.t_object *>mx.object_method(
            self.ptr, mx.gensym("getnamedbox"), str_to_sym(name))
        # now you have a handle for the box, get the contained object
        return <mx.t_object*>mx.jbox_get_object(box)

    cdef mx.t_object *newobject_sprintf(self, str text):
        """Create a new object in a specified patcher with values using a
        combination of attribute and sprintf syntax.
        """
        return <mx.t_object *>mx.newobject_sprintf(
            <mx.t_object *>self.ptr, text.encode())

    def add_box(self, maxclass: str, x: float, y: float) -> Box:
        """Create a new box in the patcher."""
        cdef mx.t_object *obj = self.newobject_sprintf(
            f"@maxclass {maxclass} @patching_position {x} {y}"
        )
        if obj is NULL:
            raise ValueError("could not create a new box in the patcher")
        return Box.from_ptr(obj)

    def add_textbox(self, text: str, x: float, y: float, maxclass='newobj') -> Box:
        """Create a new textbox in the patcher."""
        cdef mx.t_object *obj = self.newobject_sprintf(
           f'@maxclass {maxclass} @text "{text}" @patching_position {x} {y}'
        )
        if obj is NULL:
            raise ValueError("could not create a new textbox in the patcher")
        return Box.from_ptr(obj)

    cdef mx.t_object *newobject_fromboxtext(self, str text):
        """Create an object from the passed in text.

        The passed in text is in the same format as would be typed into an object box.
        It can be used for UI objects or text objects so this is the simplest way to
        create objects from C.
        """
        return mx.newobject_fromboxtext(self.ptr, text.encode())

    def add_tbox(self, str text) -> Box:
        """Create an object from the passed in text."""
        cdef mx.t_object *obj = self.newobject_fromboxtext(text)
        if obj is NULL:
            raise ValueError(f"could not create a newobject from '{text}'")
        return Box.from_ptr(obj)

    def add_box_from_dict(self, Dictionary d) -> Box:
        """Place a new box from a dictionary into a patcher.

        Max attribute syntax is used to define key-value pairs, but no need to
        prefix each key with an `@`. For example:

        >>> p = api.get_patcher()
        >>> d = api.Dictionary(
            maxclass='toggle',
            patching_position=[240.0, 200.0]
        )
        >>> box = p.add_object_from_dict(d)

        """
        cdef mx.t_object *obj = mx.newobject_fromdictionary(self.ptr, d.ptr)
        if obj is NULL:
            raise ValueError("could not create a newobject from a dictionary in the patcher")
        return Box.from_ptr(obj)

    def _method_noargs(self, str name):
        """Call an object method with no arguments."""
        cdef mx.t_max_err err = mx.object_method_typed(
            <mx.t_object *>self.ptr, str_to_sym(name), 0, NULL, NULL)
        if err == mx.MAX_ERR_NONE:
            return
        return error(f"method '{name}' call failed")

    def _method_args(self, str name, *args):
        """Call a strongly typed object method with arguments."""
        cdef Atom atom = Atom(*args)
        cdef mx.t_max_err err = mx.object_method_typed(
            <mx.t_object *>self.ptr, str_to_sym(name), atom.size, atom.ptr, NULL)
        if err == mx.MAX_ERR_NONE:
            return
        return error(f"method '{name}' call failed")

    def _method_parsestr(self, str name, str parsestr):
        """Call a method with a parsed string.

        Combines object_method_typed() + atom_setparse() to define method arguments.
        """
        cdef mx.t_max_err err = mx.object_method_parse(
            <mx.t_object *>self.ptr, str_to_sym(name), parsestr.encode(), NULL)
        if err == mx.MAX_ERR_NONE:
            return
        return error(f"method '{name}' call failed")

    def _method_float(self, str name, float number):
        """Call an object method with a single float argument.

        A wrapper for object_method_typed() that passes a single float as an argument.
        """
        cdef mx.t_max_err err = mx.object_method_float(
            <mx.t_object *>self.ptr, str_to_sym(name), number, NULL)
        if err == mx.MAX_ERR_NONE:
            return
        return error(f"method '{name}' call failed")

    def _method_double(self, str name, double number):
        """Call an object method with a single double argument.

        A wrapper for object_method_typed() that passes a single double as an argument.
        """
        cdef mx.t_max_err err = mx.object_method_double(
            <mx.t_object *>self.ptr, str_to_sym(name), number, NULL)
        if err == mx.MAX_ERR_NONE:
            return
        return error(f"method '{name}' call failed")

    def _method_long(self, str name, long number):
        """Call an object method with a single long argument.

        A wrapper for object_method_typed() that passes a single long as an argument.
        """
        cdef mx.t_max_err err = mx.object_method_long(
            <mx.t_object *>self.ptr, str_to_sym(name), number, NULL)
        if err == mx.MAX_ERR_NONE:
            return
        return error(f"method '{name}' call failed")

    def _method_sym(self, str name, str symbol):
        """Call an object method with a single symbol argument.

        A wrapper for object_method_typed() that passes a single symbol as an argument.
        """
        cdef mx.t_max_err err = mx.object_method_sym(
            <mx.t_object *>self.ptr, str_to_sym(name), str_to_sym(symbol), NULL)
        if err == mx.MAX_ERR_NONE:
            return
        return error(f"method '{name}' call failed")

    def call(self, str name, *args, parse=False):
        """Call a strongly typed object method with arguments.

        If no arguments are provided, call a method with no arguments.
        """
        if len(args) == 0:
            return self._method_noargs(name)
        elif len(args) == 1:
            if isinstance(args[0], str):
                if parse:
                    return self._method_parsestr(name, args[0])
                else:
                    return self._method_sym(name, args[0])
            elif isinstance(args[0], float):
                return self._method_double(name, args[0])
            elif isinstance(args[0], int):
                return self._method_long(name, args[0])
            elif isinstance(args[0], list):
                return self.call(name, *args[0])
            elif isinstance(args[0], tuple):
                return self.call(name, *args[0])
            elif isinstance(args[0], set):
                return self.call(name, *args[0])
        else:
            return self._method_args(name, *args)

    # patcher scripting

    def set_dirty(self):
        """Set the dirty bit in the window (user will be asked to save changes)."""
        self.call("dirty")

    def set_clean(self):
        """Reverse setting the dirty bit."""
        self.call("clean")

    def save(self):
        """Save the current patcher."""
        self.call("write")

    def set_title(self, title: str):
        """Set the current patcher's title."""
        self.call("title", f'"{title}"')

    def front(self):
        """Bring the window to the front, or open it and bring it to the front."""
        self.call("front")

    def set_active_tab(self, name: str):
        """Set a named tab to be active."""
        self.call("setactivetab", name)

    def fullsize(self, on: bool):
        """Switch the window fullsize on and off."""
        self.call("fullsize", on)

    def dispose(self):
        """Closes the patcher or destroy the subpatcher (with thispatcher)."""
        self.script("dispose")

    # TODO patcher window commands


    # box scripting

    def script(self, *args):
        """Call a patcher script command."""
        self.call("script", *args)

    # assigning varnames

    def assign_name_by_index(self, var1: str, maxclass: str, index: int):
        """Name the nth box instance of the class."""
        self.script("nth", var1, maxclass, index)

    def assign_name_by_text(self, var1: str, text: str):
        """Name a box created from the exact provided text."""
        self.script("class", var1, text)

    # object creation / deletion

    def newobject(self, varname: str, x: int, y: int, maxclass: str, *args):
        """Create an object with the given name from the provided arguments."""
        self.script("newobject", maxclass, "@varname", varname, "@patching_position", x, y, *args)

    def newdefault(self, varname: str, x: int, y: int, maxclass: str, *args):
        """Create a new named object with default properties in a patcher window."""
        self.script("newdefault", varname, x, y, maxclass, *args)

    def new(self, varname: str, maxclass: str, x: int, y: int, w: int, h: int = 0, *args):
        """Create a new object in a patcher window and give it a name.

        The format of the arguments (after the class name) are based
        on the legacy Max file format.
        """
        self.script("new", varname, maxclass, x, y, w, h, *args)

    def delete(self, var1: str):
        """Delete a named box."""
        self.script("delete", var1)

    # connecting patchlines

    def connect(self, var1: str, outlet: int, var2: str, inlet: int):
        """Connect two named objects."""
        self.script("connect", var1, outlet, var2, inlet)

    def disconnect(self, var1: str, outlet: int, var2: str, inlet: int):
        """Disconnect an existing connection between two named variables."""
        self.script("disconnect", var1, outlet, var2, inlet)

    def connectcolor(self, var1: str, outlet: int, var2: str, inlet: int, color: int):
        """Modify the color of an existing patch cord, setting it to one of Max's 16 standard colors."""
        assert color < 16, "color index is only from 0 to 15 inclusive"
        self.script("connectcolor", var1, outlet, var2, inlet, color)

    # sending messages

    def send(self, var1: str, *args):
        """Send a message to the object contained by a named box."""
        self.script("send", var1, *args)

    def sendbox(self, var1: str, *args):
        """Send a message to a named box.

        There is currently only one object, bpatcher, in which the object and box
        are different objects.
        """
        self.script("sendbox", var1, *args)

    def sendpatchline(self, from_var: str, outlet: int, to_var: str, inlet: int, *args):
        """Send a message to a patchline specified from the connection."""
        self.script("sendpatchline", from_var, outlet, to_var, inlet, *args)

    # box visibility / responsiveness

    def hide(self, var1: str):
        """Hide a named box."""
        self.script("hide", var1)

    def show(self, var1: str):
        """Show a hidden named box."""
        self.script("show", var1)

    def ignore_click(self, var1: str):
        """Make a named box ignore clicks."""
        self.script("ignoreclick", var1)

    def respond_to_click(self, var1: str):
        """Make a named box respond to clicks."""
        self.script("respondtoclick", var1)

    # moving / resizing boxes

    def move(self, var1: str, x: int, y: int):
        """Move a named box."""
        self.script("move", x, y)

    def offset(self, var1: str, x: int, y: int):
        """Relative 'offset' move of a named box."""
        self.script("offset", x, y)

    def offset_from(self, var2: str, var1: str, from_bottom_right: int,
                    from_top_left: int, x_distance: int, y_distance: int = 0):
        """Relative 'offset' move of a named box relative to another."""
        self.script("offsetfrom", var2, var1,
            from_bottom_right, from_top_left, x_distance, y_distance)

    def size(self, var1: str, width: int, height: int):
        """Resize a named box."""
        self.script("size", var1, width, height)

    def send_to_back(self, var1: str):
        """Send a named box to the back layer."""
        self.script("sendtoback", var1)

    def bring_to_front(self, var1: str):
        """Bring a named box to the front layer."""
        self.script("bringtofront", var1)

    def background(self, var1: str):
        """Bring a named box to the background layer."""
        self.script("background", var1)

    # box related

    def get_count(self) -> int:
        """Determine the number of boxes in a patcher."""
        return mx.jpatcher_get_count(self.ptr)

    def get_presentation(self) -> bool:
        """Determine whether a patcher is currently in presentation mode."""
        return mx.jpatcher_get_presentation(self.ptr)

    cdef mx.t_object* get_first_box(self):
        """Get the first box in a patcher."""
        return mx.jpatcher_get_firstobject(self.ptr)

    cdef mx.t_object* get_last_box(self):
        """Get the last box in a patcher."""
        return mx.jpatcher_get_lastobject(self.ptr)

    cdef mx.t_object* get_first_line(self):
        """Get the first line (patch-cord) in a patcher."""
        return mx.jpatcher_get_firstline(self.ptr)

    cdef mx.t_object* get_first_view(self):
        """Get the first view (jpatcherview) for a given patcher."""
        return mx.jpatcher_get_firstview(self.ptr)

    # def get_view(self) -> MaxObject:
    #     """Get the first view (jpatcherview) for a given patcher."""
    #     cdef mx.t_object* view = mx.jpatcher_get_firstview(self.ptr)
    #     return MaxObject.from_ptr(<mx.t_object*>view)

    def get_title(self) -> str:
        """Retrieve a patcher's title."""
        cdef mx.t_symbol* title = mx.jpatcher_get_name(self.ptr)
        return sym_to_str(title)

    def get_name(self) -> str:
        """Retrieve a patcher's name."""
        cdef mx.t_symbol* name = mx.jpatcher_get_name(self.ptr)
        return sym_to_str(name)

    def get_filepath(self) -> str:
        """get the patchers filepath"""
        cdef mx.t_symbol* fpath = mx.jpatcher_get_filepath(self.ptr)
        return sym_to_str(fpath)

    def get_filename(self) -> str:
        """Retrieve a patcher's file name."""
        cdef mx.t_symbol* fname = mx.jpatcher_get_filename(self.ptr)
        return sym_to_str(fname)

    cdef char get_dirty(self):
        """Determine whether a patcher's dirty bit has been set."""
        return mx.jpatcher_get_dirty(self.ptr)

    # cdef mx.t_max_err set_dirty(self, char c):
    #     """Set a patcher's dirty bit."""
    #     return mx.jpatcher_set_dirty(self.ptr, c)

    def get_bglocked(self) -> bool:
        """Determine whether a patcher's background layer is locked."""
        return mx.jpatcher_get_bglocked(self.ptr)

    def get_bgcolor(self) -> Rgba:
        """Retrieve a patcher's unlocked background color."""
        cdef mx.t_jrgba c
        cdef mx.t_max_err err = mx.jpatcher_get_bgcolor(self.ptr, &c)
        if err == mx.MAX_ERR_NONE:
            return Rgba(c.red, c.green, c.blue, c.alpha)
        return error("could not get patcher bgcolor")

    def set_bgcolor(self, c: Rgba):
        """Set a patcher's unlocked background color."""
        cdef mx.t_jrgba rgba = c
        cdef mx.t_max_err err = mx.jpatcher_set_bgcolor(self.ptr, &rgba)
        if err:
            return error("could not set patcher bgcolor")

    def get_gridsize(self) -> tuple:
        """Retrieve a patcher's grid size."""
        cdef double grid_size_x
        cdef double grid_size_y
        cdef mx.t_max_err err = mx.jpatcher_get_gridsize(
            self.ptr, &grid_size_x, &grid_size_y)
        if err == mx.MAX_ERR_NONE:
            return (grid_size_x, grid_size_y)
        return error("could not get patcher gridsize")

    def set_gridsize(self, grid_size_x: float, grid_size_y: float):
        """Set a patcher's grid size."""
        cdef mx.t_max_err err = mx.jpatcher_set_gridsize(
            self.ptr, grid_size_x, grid_size_y)
        if err:
            return error("could not set patcher gridsize")

    def get_rect(self) -> Rect:
        """Query a patcher to determine its location and size."""
        cdef mx.t_rect r
        cdef mx.t_max_err err = mx.jpatcher_get_rect(self.ptr, &r)
        if err == mx.MAX_ERR_NONE:
            return Rect(r.x, r.y, r.width, r.height)
        return error("could not get patcher rect")

    def set_rect(self, r: Rect):
        """Set a patcher's location and size."""
        cdef mx.t_rect rect = r
        cdef mx.t_max_err err = mx.jpatcher_set_rect(self.ptr, &rect)
        if err:
            return error("could not set patcher rect")

    cdef void deleteobj(self, mx.t_jbox *b):
        """Delete an object that is in a patcher."""
        mx.jpatcher_deleteobj(self.ptr, b)

    def get_fileversion(self) -> int:
        """Return the file version of the patcher."""
        return mx.jpatcher_get_fileversion(self.ptr)

    def get_currentfileversion(self) -> int:
        """Return the file version for any new patchers, e.g. the current version created by Max."""
        return mx.jpatcher_get_currentfileversion()

    cdef mx.t_object* get_parentpatcher(self):
        """Given a patcher, return its parent patcher."""
        return mx.jpatcher_get_parentpatcher(self.ptr)

    cdef mx.t_object* get_toppatcher(self):
        """Given a patcher, return the top-level patcher for the tree in which it exists."""
        return mx.jpatcher_get_toppatcher(self.ptr)

    cdef mx.t_object* get_hubholder(self):
        """Given a patcher, return the patcher that will be responsible for holding the parameter hub."""
        return mx.jpatcher_get_hubholder(self.ptr)

    cdef mx.t_symbol *uniqueboxname(self, mx.t_symbol *classname):
        """Generate a unique name for a box in patcher."""
        return mx.jpatcher_uniqueboxname(self.ptr, classname)

# ----------------------------------------------------------------------------
# api.Box

cdef class Box:
    """Wraps a Max t_jbox user-interface object."""

    cdef mx.t_object *ptr              # typedef t_object t_box
    cdef mx.t_object *patcherview
    cdef bint ptr_owner

    def __cinit__(self):
        self.ptr = NULL
        self.patcherview = NULL
        self.ptr_owner = False

    def __repr__(self) -> str:
        return f"<Box '{self.classname}'>"

    @staticmethod
    cdef Box from_object_ptr(mx.t_object *x):
        """Create a box object from a t_object pointer."""
        cdef mx.t_max_err err
        cdef Box box = Box.__new__(Box)
        box.ptr_owner = True
        err = mx.object_obex_lookup(x, mx.gensym("#B"), &box.ptr)
        if err:
            raise ValueError("could not create a box object from an object pointer")
        if box.ptr is NULL:
            raise MemoryError
        return box

    @staticmethod
    cdef Box from_ptr(mx.t_object *ptr):
        """Create a box object from a box pointer."""
        cdef Box box = Box.__new__(Box)
        box.ptr = ptr
        box.ptr_owner = False
        if box.ptr is NULL:
            raise MemoryError
        return box

    @property
    def id(self) -> str:
        """Retrieve a box's unique id."""
        return self.get_id()

    @property
    def classname(self) -> str:
        """Retrieve the name of the class of the box's object."""
        return self.get_maxclass()

    @property
    def varname(self) -> str:
        """Retrieve a box's scripting name."""
        return self.get_varname()

    @property
    def object(self) -> MaxObject:
        """Retrieve the box's object."""
        return self.get_object()

    @property
    def patcher(self) -> Patcher:
        """Retrieve a box's patcher."""
        return self.get_patcher()

    @property
    def rect(self) -> Rect:
        """Retrieve the patching rect of a box."""
        return self.get_patching_rect()

    def get_rect_for_view(self) -> Rect:
        """Find the rect for a box in a given patcherview."""
        cdef mx.t_rect pr
        cdef mx.t_max_err err = mx.jbox_get_rect_for_view(self.ptr, self.patcherview, &pr)
        if err:
            raise TypeError("could not get rect from patcherview's box")
        return Rect(pr.x, pr.y, pr.width, pr.height)

    def set_rect_for_view(self, rect: Rect):
        """Change the rect for a box in a given patcherview."""
        cdef mx.t_rect pr = rect
        cdef mx.t_max_err err = mx.jbox_set_rect_for_view(self.ptr, self.patcherview, &pr)
        if err:
            raise TypeError("could not set rect for box in patcherview")

    def get_rect_for_sym(self, which: str) -> Rect:
        """Find the rect for a box with a given attribute name."""
        cdef mx.t_rect pr
        cdef mx.t_max_err err = mx.jbox_get_rect_for_sym(self.ptr, str_to_sym(which), &pr)
        if err:
            raise TypeError("could not get rect for box given attribute name")
        return Rect(pr.x, pr.y, pr.width, pr.height)

    def set_rect_for_sym(self, which: str, rect: Rect):
        """Change the rect for a box with a given attribute name."""
        cdef mx.t_rect pr  = rect
        cdef mx.t_max_err err = mx.jbox_set_rect_for_sym(self.ptr, str_to_sym(which), &pr)
        if err:
            raise TypeError("could not set rect for box with given attribute name")

    def set_rect(self, rect: Rect):
        """Set both the presentation rect and the patching rect."""
        cdef mx.t_rect pr  = rect
        cdef mx.t_max_err err = mx.jbox_set_rect(self.ptr, &pr)
        if err:
            raise TypeError("could not set rect for box")

    def get_patching_rect(self) -> Rect:
        """Retrieve the patching rect of a box."""
        cdef mx.t_rect pr
        cdef mx.t_max_err err = mx.jbox_get_patching_rect(self.ptr, &pr)
        if err:
            raise TypeError("could not get patching rect for box")
        return Rect(pr.x, pr.y, pr.width, pr.height)

    def set_patching_rect(self, rect: Rect):
        """Change the patching rect of a box."""
        cdef mx.t_rect pr  = rect
        cdef mx.t_max_err err = mx.jbox_set_patching_rect(self.ptr, &pr)
        if err:
            raise TypeError("could not set patching rect for box")

    def get_presentation_rect(self) -> Rect:
        """Retrieve the presentation rect of a box."""
        cdef mx.t_rect pr
        cdef mx.t_max_err err = mx.jbox_get_presentation_rect(self.ptr, &pr)
        if err:
            raise TypeError("could not get presentation rect for box")
        return Rect(pr.x, pr.y, pr.width, pr.height)

    def set_presentation_rect(self, rect: Rect):
        """Change the presentation rect of a box."""
        cdef mx.t_rect pr  = rect
        cdef mx.t_max_err err = mx.jbox_set_presentation_rect(self.ptr, &pr)
        if err:
            raise TypeError("could not set presentation rect for box")

    def set_position(self, x: float, y: float):
        """Set the position of a box for both presentation and patching views."""
        cdef mx.t_pt pos  = (x, y)
        cdef mx.t_max_err err = mx.jbox_set_position(self.ptr, &pos)
        if err:
            raise TypeError("could not set the position of a box for both views")

    def get_patching_position(self) -> tuple[float, float]:
        """Fetch the position of a box for the patching view."""
        cdef mx.t_pt pos
        cdef mx.t_max_err err = mx.jbox_get_patching_position(self.ptr, &pos)
        if err:
            raise TypeError("could not get patching position for box")
        return (pos.x, pos.y)

    def set_presentation_position(self, x: float, y: float):
        """Set the position of a box for the presentation view."""
        cdef mx.t_pt pos  = (x, y)
        cdef mx.t_max_err err = mx.jbox_set_presentation_position(self.ptr, &pos)
        if err:
            raise TypeError("could not set the position of a box for presentation view")

    def set_size(self, width: float, height: float):
        """Set the size of a box for both the presentation and patching views."""
        cdef mx.t_size size  = (width, height)
        cdef mx.t_max_err err = mx.jbox_set_size(self.ptr, &size)
        if err:
            raise TypeError("could not set the size of a box for both views")

    def get_patching_size(self) -> tuple[float, float]:
        """Fetch the size of a box for the patching view."""
        cdef mx.t_size size
        cdef mx.t_max_err err = mx.jbox_get_patching_size(self.ptr, &size)
        if err:
            raise TypeError("could not get patching size for box")
        return (size.width, size.height)

    def set_patching_size(self, width: float, height: float):
        """Set the size of a box for the patching view."""
        cdef mx.t_size size  = (width, height)
        cdef mx.t_max_err err = mx.jbox_set_patching_size(self.ptr, &size)
        if err:
            raise TypeError("could not set the size of a box for the patching view.")

    def get_presentation_size(self) -> tuple[float, float]:
        """Fetch the size of a box for the presentation view."""
        cdef mx.t_size size
        cdef mx.t_max_err err = mx.jbox_get_presentation_size(self.ptr, &size)
        if err:
            raise TypeError("could not get presentation size for box")
        return (size.width, size.height)

    def set_presentation_size(self, width: float, height: float):
        """Set the size of a box for the presentation view."""
        cdef mx.t_size size  = (width, height)
        cdef mx.t_max_err err = mx.jbox_set_presentation_size(self.ptr, &size)
        if err:
            raise TypeError("could not set the size of a box for the presentation view.")

    def get_maxclass(self) -> str:
        """Retrieve the name of the class of the box's object."""
        cdef mx.t_symbol* name = mx.jbox_get_maxclass(self.ptr)
        return sym_to_str(name)

    def get_object(self) -> MaxObject:
        """Retrieve the box's object."""
        cdef mx.t_object* obj = mx.jbox_get_object(self.ptr)
        return MaxObject.from_ptr(<mx.t_object*>obj)

    def get_patcher(self) -> Patcher:
        """Retrieve a box's patcher."""
        cdef mx.t_object* patcher = mx.jbox_get_patcher(self.ptr)
        if patcher is not NULL:
            return Patcher.from_ptr(<mx.t_object*>patcher)
        raise TypeError("box does not have an associated patcher")

    def get_nextobject(self) -> Box:
        """The next box in the patcher's (linked) list of boxes."""
        cdef mx.t_object* next_box = mx.jbox_get_nextobject(self.ptr)
        return Box.from_ptr(next_box)

    def get_prevobject(self) -> Box:
        """The prior box in the patcher's (linked) list of boxes."""
        cdef mx.t_object* next_box = mx.jbox_get_prevobject(self.ptr)
        return Box.from_ptr(next_box)

    def is_hidden(self) -> bool:
        """Return true if box is hidden"""
        return bool(mx.jbox_get_hidden(self.ptr))

    def set_hidden(self, on: bool):
        """Set a box hidden attribute"""
        cdef mx.t_max_err err = mx.jbox_set_hidden(self.ptr, on)
        if err:
            raise TypeError("could not set box's hidden attribute")

    def get_fontname(self) -> str:
        """Retrieve a box's 'fontname' attribute."""
        cdef mx.t_symbol* name = mx.jbox_get_fontname(self.ptr)
        return sym_to_str(name)

    def set_fontname(self, name: str):
        """Set a box's 'fontname' attribute."""
        cdef mx.t_max_err err = mx.jbox_set_fontname(self.ptr, str_to_sym(name))
        if err:
            raise TypeError("could not set box's fontname attribute")

    def get_fontsize(self) -> float:
        """Retrieve a box's 'fontsize' attribute."""
        return mx.jbox_get_fontsize(self.ptr)

    def set_fontsize(self, size: float):
        """Set a box's 'fontsize' attribute."""
        cdef mx.t_max_err err = mx.jbox_set_fontsize(self.ptr, size)
        if err:
            raise TypeError("could not set box's fontsize attribute")

    def get_color(self) -> Rgba:
        """Retrieve a box's 'color' attribute."""
        cdef mx.t_jrgba c
        cdef mx.t_max_err err = mx.jbox_get_color(self.ptr, &c)
        if err == mx.MAX_ERR_NONE:
            return Rgba(c.red, c.green, c.blue, c.alpha)
        raise TypeError("could not get box's color")

    def set_color(self, color: Rgba):
        """Set a box's 'color' attribute."""
        cdef mx.t_jrgba c = color
        cdef mx.t_max_err err = mx.jbox_set_color(self.ptr, &c)
        if err:
            raise TypeError("could not set box's color")

    def get_varname(self) -> str:
        """Retrieve a box's scripting name."""
        cdef mx.t_symbol* varname = mx.jbox_get_varname(self.ptr)
        return sym_to_str(varname)

    def set_varname(self, varname: str):
        """set a box's scripting name."""
        cdef mx.t_max_err err = mx.jbox_set_varname(self.ptr, str_to_sym(varname))
        if err:
            raise TypeError("could not set box's scripting name")

    def get_id(self) -> str:
        """Retrieve a box's unique id."""
        cdef mx.t_symbol* _id = mx.jbox_get_id(self.ptr)
        return sym_to_str(_id)


# ----------------------------------------------------------------------------
# api.Matrix

cdef class Matrix:
    """Interface to an existing Max jitter matrix."""
    cdef jt.t_object *ptr
    cdef jt.t_jit_matrix_info info
    cdef char* data
    cdef public str name
    cdef Py_ssize_t shape[jt.JIT_MATRIX_MAX_DIMCOUNT]
    cdef Py_ssize_t strides[jt.JIT_MATRIX_MAX_DIMCOUNT]

    def __cinit__(self):
        self.ptr = NULL
        self.data = NULL
        self.name = ""

    # def __dealloc__(self):
    #     if self.ptr:
    #         jt.jit_object_free(self.ptr)

    def __init__(self, str name):
        self.name = name
        self.ptr = <jt.t_object*>jt.jit_object_findregistered(str_to_sym(name))
        if not (self.ptr is not NULL and self.is_matrix()):
            raise ValueError("could not retrieve a matrix object with name '{name}'")
        self.refresh()

    def __getbuffer__(self, Py_buffer *buffer, int flags):
        # Refresh matrix data to ensure we have current info
        self.refresh()
        
        # Validate matrix data
        if self.data is NULL:
            raise ValueError("Matrix data is not available")
        
        if self.ndim <= 0 or self.ndim > jt.JIT_MATRIX_MAX_DIMCOUNT:
            raise ValueError(f"Invalid number of dimensions: {self.ndim}")
        
        # Set shape and strides based on matrix dimensions
        cdef int i
        for i in range(self.ndim):
            if self.info.dim[i] <= 0:
                raise ValueError(f"Invalid dimension size at index {i}: {self.info.dim[i]}")
            self.shape[i] = self.info.dim[i]
            self.strides[i] = self.info.dimstride[i]

        # Set up buffer structure
        buffer.buf = <char *>self.data
        buffer.internal = NULL
        buffer.itemsize = self.itemsize
        buffer.len = product(self.dim) * self.itemsize * self.planecount
        buffer.ndim = self.ndim
        buffer.obj = self
        buffer.readonly = 0
        buffer.shape = self.shape
        buffer.strides = self.strides
        buffer.suboffsets = NULL

        # Set format string based on matrix type
        cdef str matrix_type = self.type
        if matrix_type == 'char':
            buffer.format = 'b'  # signed char
        elif matrix_type == 'long':
            buffer.format = 'l'  # signed long
        elif matrix_type == 'float32':
            buffer.format = 'f'  # float
        elif matrix_type == 'float64':
            buffer.format = 'd'  # double
        else:
            raise ValueError(f"Unsupported matrix type: {matrix_type}")

    def __releasebuffer__(self, Py_buffer *buffer):
        pass

    def __repr__(self) -> str:
        return f"<Matrix '{self.name}'>"

    # matrix_info properties

    @property
    def size(self) -> int:
        """matrix size in bytes"""
        return self.info.size

    @property
    def type(self) -> str:
        """primitifve type (char, long, float32, or float64)"""
        return sym_to_str(self.info.type)

    @property
    def flags(self) -> int:
        """flags to specify data reference, handle, or tightly packed"""
        return self.info.flags

    @property
    def ncols(self) -> int:
        """number of columns in the matrix"""
        return self.dim[0]

    @property
    def shape(self):
        """shape of dimensions"""
        return self.info.dim

    @property
    def strides(self):
        """shape of dimensions"""
        return self.info.dimstride

    @property
    def dimcount(self) -> int:
        """number of dimensions"""
        return self.info.dimcount

    @property
    def dim(self) -> list[int]:
        """dimension sizes"""
        cdef list[int] dim_sizes = []
        for i in range(self.dimcount):
            dim_sizes.append(self.info.dim[i])
        return dim_sizes

    @property
    def dimstride(self) -> list[int]:
        """stride across dimensions in bytes"""
        cdef list[int] dim_stride = []
        for i in range(self.dimcount):
            dim_stride.append(self.info.dimstride[i])
        return dim_stride

    @property
    def planecount(self) -> int:
        """number of planes"""
        return self.info.planecount

    # custom properties

    @property
    def width(self) -> int:
        """width of matrix"""
        return self.dim[0]

    @property
    def height(self) -> int:
        """height of matrix"""
        return self.dim[1]

    @property
    def itemsize(self) -> int:
        """size in bytes of single matrix entry"""
        return {
            'char': 1,
            'long':  4,
            'float32': 4,
            'float64': 8,
        }[self.type]

    @property
    def ndim(self) -> int:
        """number of dimensions"""
        return self.dimcount

    @property
    def plane_len(self) -> int:
        """number of `cells` in a single plane"""
        return product(self.dim)

    @property
    def matrix_len(self) -> int:
        """total number of `cell` in a single matrix"""
        return self.plane_len * self.planecount

    # predicates

    def is_matrix(self) -> bool:
        """Checks if matrix pointer refers to an actual matrix"""
        return <bint>jt.jit_object_method(self.ptr, jt._jit_sym_class_jit_matrix)

    # helper methods

    def call(self, str method, *args) -> Optional[object]:
        """helper wrapper method around jit_object_method_typed"""
        cdef Atom atom = Atom(*args)
        cdef Atom result = Atom.new(1)
        jt.jit_object_method_typed(<jt.t_object*>self.ptr,
            str_to_sym(method), atom.size, atom.ptr, result.ptr
        )
        if result.ptr is not NULL:
            return result.value
        return

    def call_with_atoms(self, str method, Atom atom) -> Optional[object]:
        """helper wrapper method around jit_object_method_typed"""
        cdef Atom result = Atom.new(1)
        jt.jit_object_method_typed(<jt.t_object*>self.ptr,
            str_to_sym(method), atom.size, atom.ptr, result.ptr
        )
        if result.ptr is not NULL:
            return result.value
        return

    # msg methods

    def bang(self):
        """Outputs the currently stored matrix."""
        jt.jit_object_method(self.ptr, mx.gensym("bang"))

    def set_int(self, *values):
        """Set all cells to a value and output the result

        int(list values)

        Sets all cells to the value specified by `value(s)` and output the
        data. Position is specified of a list whose length is equal to the
        number of dimensions (`dimcount`).
        """
        assert 0 < len(values) <= self.planecount, "# of values cannot == 0 or >= # planes"
        self.call("int", values)

    def set_float(self, *values):
        """Set all cells to a value and output the result

        float(list values)

        Sets all cells to the value specified by `value(s)` and output the
        data. Value is specified as a list whose length is equal to the
        number of dimensions (`dimcount`).
        """
        assert 0 < len(values) <= self.planecount, "# of values cannot == 0 or >= # planes"
        self.call("float", values)

    def set_list(self, *values):
        """Set all cells to a value and output the result

        list(list values)

        Sets all cells to the value specified by `value(s)` and output the
        data. Position is specified of a list whose length is equal to the
        number of dimensions (`dimcount`).
        """
        assert 0 < len(values) <= self.planecount, "# of values cannot == 0 or >= # planes"
        self.call("list", values)

    def clear(self):
        """Sets all matrix values to zero."""
        jt.jit_object_method(<jt.t_object*>self.ptr, jt._jit_sym_clear)

    def export_image(self, str filename, str filetype = "png"):
        """Export the current frame as an image file

        exportimage(symbol filename, symbol file-type)

        Export the current frame as an image file with the name specified by
        the first argument. The second argument sets the file type
        (default = png). Available file types are `png`, `tiff`, and
        `jpeg`.
        """
        assert filetype in ['png', 'tiff', 'jpeg'], "incompatible filetype"
        cdef Atom atom = Atom(filename, filetype)
        jt.jit_object_method(self.ptr, mx.gensym("exportimage"), atom.size, atom.ptr)

    def export_movie(self, str filename, float fps, str codec, str quality, int timescale):
        """Export a matrix as a movie

        exportmovie(symbol filename?, float FPS, symbol codec, symbol quality, int timescale)

        Exports a matrix as a movie. The `exportmovie` message takes an
        optional argument to specify a file name. If no filename is
        specified, a file dialog will open to let you choose a file.
        """
        cdef Atom atom = Atom(filename, fps, codec, quality, timescale)
        jt.jit_object_method(self.ptr, mx.gensym("exportmovie"), atom.size, atom.ptr)

    def expr_fill(self, str expr, int plane = 0):
        """Evaluate an expression to fill the matrix

        exprfill(int plane?, symbol expression)

        Evaluates `expression` to fill the matrix. If a `plane` argument is
        provided, the expression is applied to a single plane. Otherwise,
        it is applied to all planes in the matrix. See `jit.expr` for more
        information on expressions. Unlike the `jit.expr` object, there is
        no support for providing multiple expressions to fill multiple
        planes at once with different expressions. Call this method
        multiple times once for each plane you wish to fill.
        """
        cdef Atom atom = Atom(plane, expr)
        jt.jit_object_method(self.ptr, mx.gensym("exprfill"), atom.size, atom.ptr)

    def fill_plane(self, int value = 0, int plane = 0):
        """Fill a plane with a specified value

        fillplane(int plane?, int value?)

        The word `fillplane`, followed by an integer that specifies a plane
        number and a value, will fill the specified plane with the single
        value.
        """
        cdef Atom atom = Atom(plane, value)
        jt.jit_object_method(self.ptr, mx.gensym("fillplane"), atom.size, atom.ptr)

    def get_cell(self, *positions):
        """Report cell values

        getcell(list position)

        Sends the value(s) in the cell specified by `position` out the right
        outlet of the object as a list in the form

            cell pos1... posN val plane0-value... planeN-value

        where pos1 and pos2 would correspond to x and y in a 2d matrix
        """
        cdef Atom atom = Atom(*positions)
        jt.jit_object_method(self.ptr, mx.gensym("getcell"), atom.size, atom.ptr)

    def import_movie(self, str filename, int timeoffset = 0):
        """Import a movie into the matrix

        importmovie(symbol filename?, int time-offset)

        Imports a movie into the matrix. If no filename is specified, a file
        dialog will open to let you choose a file. The `time-offset`
        argument may be used to set a time offset for the movie being
        imported (the default is 0).
        """
        cdef Atom atom = Atom(filename, timeoffset)
        jt.jit_object_method(self.ptr, mx.gensym("importmovie"), atom.size, atom.ptr)

    def add_gl_texture(self, str texture_name):
        """Copy a texture to the matrix

        jit_gl_texture(symbol texture-name)

        Copies the texture specified by `texture-name` to the matrix.
        """
        cdef Atom atom = Atom(texture_name)
        jt.jit_object_method(self.ptr, mx.gensym("jit_gl_texture"), atom.size, atom.ptr)

    def op(self, *args):
        """Perform `jit.op` operations on the matrix

        The word `op`, followed by the name of a `jit.op` object operator and
        a set of values, is equivalent to including a `jit.op` object with
        the specified operator set as an attribute and this `jit.matrix`
        object specified as the output matrix. The additional `value`
        arguments may either be a matrix name or a constant. If only one
        value argument is provided, this matrix is considered both the
        output and the left operand.

        For example
            `op + foo bar` is equivalent to the operation `thismatrix = foo + bar`,
                and
            `op * 0.5` is equivalent to the operation `thismatrix = thismatrix * 0.5`
        """
        self.call("op", args)

    def read(self, str filename):
        """Read Jitter binary data files (.jxf)

        read(symbol filename?)

        Reads Jitter binary data files (.jxf) into a matrix set. If no
        filename is specified, a file dialog will open to let you choose a
        file.
        """
        cdef Atom atom = Atom(filename)
        jt.jit_object_method(self.ptr, mx.gensym("read"), atom.size, atom.ptr)

    def set_all(self, *args):
        """Set all cells to a value (_jit_sym_setall)

        setall(list values)

        Sets all cells to the value specified by `value(s)`. Position is
        specified of a list whose length is equal to the number of
        dimensions (`dimcount`).

        >>> matrix.set_all(10, 20)
        # sets all cells in: plane0 to 10, plane1 to 20
        """
        cdef Atom atom = Atom(*args)
        jt.jit_object_method_typed(<jt.t_object*>self.ptr,
            str_to_sym("setall"), atom.size, atom.ptr, NULL # <- NULL or raises TypError
        )

    def set_cell(self, list[int] positions,  list[object] values, int plane=-1):
        """Set a cell to a specified value

        setcell(list position, literal plane?, int plane-number?, literal val, list values)

        Sets the cell specified by `position` to the value specified by
        `value`. Position is specified of a list whose length is equal to
        the number of dimensions (`dimcount`). The optional arguments
        `plane`  `plane-number` can be used to specify a plane. If a plane
        is specified, `value` should be a single number, otherwise it
        should be a list of numbers of size `planecount - 1`.

        For eg, for a char 3 plane 2d matrix

        >>> self.set_cell(positions=[0,0], values=[10, 8, 5]/)
        """
        assert len(positions) == self.dimcount, "len(positions) must equal # of dimensions"
        assert len(values) < self.planecount, "len(values) must be less than planecount"
        args = positions[:]
        if plane >= 0:
            args.extend(['plane', plane])
        args.append('val')
        args.extend(values)
        self.call("setcell", args)

    def set_cell1d(self, int x, list[object] values):
        """Set a 1-dimensional cell to a specified value

        The word `setcell1d`, followed by a number specifying an `x`
        coordinate and a list of values, is similar to the `setcell`
        message but without the need to use a `val` token to separate the
        coordinates from the value since the dimension count (1) is fixed.
        """
        self.call("setcell1d", x, values)

    def set_cell2d(self, int x, int y, list[object] values):
        """Set a 2-dimensional cell to specified values

        The word `setcell2d`, followed by a pair of numbers specifying `x` and
        `y` coordinates and a list of values, is similar to the `setcell`
        message but without the need to use a `val` token to separate the
        coordinates from the value since the dimension count (2) is fixed.
        """
        assert len(values) < self.planecount, "len(values) must be less than planecount"
        self.call("set_cell2d", x, y, values)

    def set_cell3d(self, int x, int y, int z, list[object] values):
        """Set a 3-dimensional cell to specified values

        The word `setcell3d`, followed by three numbers specifying `x`, `y`,
        and `z` coordinates and a list of values, is similar to the
        `setcell` message but without the need to use a `val` token to
        separate the coordinates from the value since the dimension count
        (3) is fixed.
        """
        assert len(values) < self.planecount, "len(values) must be less than planecount"
        self.call("set_cell2d", x, y, z, values)

    def set_plane1d(self, int x, int plane, object value):
        """Set a cell in a plane to a value (1d, no val token)

        The word `setplane1d`, followed by a number specifying an `x`
        coordinate, a number specifying a plane, and a value, is similar
        to the `setcell` message but without the need to use a `val` token
        to separate the coordinates from the value since the dimension
        count (1) is fixed, or use the `plane` token to specify which
        plane to set.
        """
        self.call("setplane1d", x, plane, value)

    def set_plane2d(self, int x, int y, int plane, object value):
        """Set a cell in a plane to a value (2d, no val token)

        The word `setplane2d`, followed by a pair of numbers specifying `x`
        and `y` coordinates, a number specifying a plane, and a value, is
        similar to the `setcell` message but without the need to use a
        `val` token to separate the coordinates from the value since the
        dimension count (2) is fixed, or use the `plane` token to specify
        which plane to set.
        """
        self.call("setplane2d", x, y, plane, value)

    def set_plane3d(self, int x, int y, int z, int plane, object value):
        """Set a cell in a plane to a value (3d, no val token)

        The word `setplane3d`, followed by three numbers specifying `x`, `y`,
        and `z` coordinates, a number specifying a plane, and a value, is
        similar to the `setcell` message but without the need to use a
        `val` token to separate the coordinates from the value since the
        dimension count (1) is fixed, or use the `plane` token to specify
        which plane to set.
        """
        self.call("setplane2d", x, y, z, plane, value)

    def set_val(self, *values):
        """Set all cells to a value and output the result

        val(list values)

        Sets all cells to the value specified by `value(s)`. Position is
        specified of a list whose length is equal to the number of
        dimensions (`dimcount`) and outputs the data.

        >>> matrix.set_val(10, 20)
        # sets all cells in: plane0 to 10, plane1 to 20 and outputs the data
        """
        assert 0 < len(values) <= self.planecount, "# of values cannot == 0 or >= # planes"
        # using `setall` because `val` cannot be found
        # (suspect it's need to be an attr to be called as in max-sdk/matrix/jit.op)
        self.call("setall", values)
        self.bang()

    def write(self, str filename):
        """Write matrix set as a Jitter binary data file (.jxf)

        write(symbol filename?)

        Writes matrix set as a Jitter binary data file (.jxf). If no filename
        is specified, a file dialog will open to let you choose a file.
        """
        self.call("write", filename)

    # other methods

    def lock(self) -> int:
        """lock matrix and return savelock id"""
        return <long>jt.jit_object_method(self.ptr, jt._jit_sym_lock, 1)

    def unlock(self, int savelock):
        """unlock matrix using prior savelock"""
        jt.jit_object_method(self.ptr, jt._jit_sym_lock, savelock)

    def refresh(self):
        """updates matrix info and data"""
        jt.jit_object_method(<jt.t_object*>self.ptr, jt._jit_sym_getinfo, &self.info)

        # self.info.flags = jt.JIT_MATRIX_DATA_REFERENCE | jt.JIT_MATRIX_DATA_PACK_TIGHT
        # self.info.flags = jt.JIT_MATRIX_DATA_HANDLE | jt.JIT_MATRIX_DATA_PACK_TIGHT
        self.info.flags = jt.JIT_MATRIX_DATA_HANDLE | jt.JIT_MATRIX_DATA_PACK_TIGHT | jt.JIT_MATRIX_DATA_FLAGS_USE

        jt.jit_object_method(<jt.t_object*>self.ptr, jt._jit_sym_getdata, &self.data)

    def get_data(self) -> list[object]:
        """retrieve data from matrix as contiguous array."""
        if self.type == "char":
            return self.get_char_data()
        elif self.type == "long":
            return self.get_long_data()
        elif self.type == "float32":
            return self.get_float_data()
        elif self.type == "float64":
            return self.get_double_data()
        else:
            raise TypeError("could not process this type")

    def get_char_data(self) -> list[int]:
        """retrieve char data from matrix as contiguous array."""
        cdef list[int] results = []
        cdef int i, j, p
        cdef char *m_ptr = NULL

        for i in range(self.height):
            m_ptr = self.data + i * self.info.dimstride[1]
            for j in range(self.width):
                for p in range(self.planecount):
                    results.append(<jt.uchar>m_ptr[p])
        return results

    def get_long_data(self) -> list[long]:
        """retrieve long data from matrix as contiguous array."""
        cdef list[long] results = []
        cdef int i, j, p
        cdef char *m_ptr = NULL

        for i in range(self.height):
            m_ptr = self.data + i * self.info.dimstride[1]
            for j in range(self.width):
                for p in range(self.planecount):
                    results.append(<long>m_ptr[p])
        return results

    def get_float_data(self) -> list[float]:
        """retrieve float data from matrix as contiguous array."""
        cdef list[float] results = []
        cdef int i, j, p
        cdef char *m_ptr = NULL

        for i in range(self.height):
            m_ptr = self.data + i * self.info.dimstride[1]
            for j in range(self.width):
                for p in range(self.planecount):
                    results.append(<float>m_ptr[p])
        return results

    def get_double_data(self) -> list[double]:
        """retrieve double data from matrix as contiguous array."""
        cdef list[double] results = []
        cdef int i, j, p
        cdef char *m_ptr = NULL

        for i in range(self.height):
            m_ptr = self.data + i * self.info.dimstride[1]
            for j in range(self.width):
                for p in range(self.planecount):
                    results.append(<double>m_ptr[p])
        return results

    # def set_char_data(self, list[int] data):
    #     """set data to whole matrix"""
    #     cdef int k = 0
    #     cdef int i, j, p
    #     cdef char *m_ptr = NULL

    #     for i in range(self.height):
    #         m_ptr = self.data + i * self.info.dimstride[1]
    #         for j in range(self.width):
    #             for p in range(self.planecount):
    #                 post(f"(i, j, p, k) = ({i}, {j}, {p}, {k})")
    #                 # (m_ptr+p)[0] = 2 # doesn't work!
    #                 m_ptr[0] = <jt.uchar>clamp(data[k], 0, 255)
    #                 k += 1
    #                 m_ptr += 1


    def set_data(self, list[object] data):
        """retrieve data from matrix as contiguous array."""
        if self.type == "char":
            self.set_char_data(data)
        elif self.type == "long":
            self.set_long_data(data)
        elif self.type == "float32":
            self.set_float_data(data)
        elif self.type == "float64":
            self.set_double_data(data)
        else:
            raise TypeError("could not process this type")

    def set_char_data(self, list[int] data):
        """set char data to whole matrix"""
        cdef int i, j, p, k = 0
        cdef char *m_ptr = <char *>self.data

        for i in range(self.height):
            for j in range(self.width):
                for p in range(self.planecount):
                    post(f"(i, j, p, k) = ({i}, {j}, {p}, {k}) = {data[k]}")
                    m_ptr[k] = <jt.uchar>clamp(data[k], 0, 255)
                    k += 1

    def set_long_data(self, list[int] data):
        """set long data to whole matrix"""
        cdef int i, j, p, k = 0
        cdef long *m_ptr = <long*>self.data

        for i in range(self.height):
            for j in range(self.width):
                for p in range(self.planecount):
                    post(f"(i, j, p, k) = ({i}, {j}, {p}, {k}) = {data[k]}")
                    m_ptr[k] = <long>data[k]
                    k += 1

    def set_float_data(self, list[float] data):
        """set float data to whole matrix"""
        cdef int i, j, p, k = 0
        cdef float *m_ptr =  <float*>self.data

        for i in range(self.height):
            for j in range(self.width):
                for p in range(self.planecount):
                    post(f"(i, j, p, k) = ({i}, {j}, {p}, {k}) = {data[k]}")
                    m_ptr[k] = <float>data[k]
                    k += 1

    def set_double_data(self, list[double] data):
        """set long data to whole matrix"""
        cdef int i, j, p, k = 0
        cdef double *m_ptr =<double*> self.data

        for i in range(self.height):
            for j in range(self.width):
                for p in range(self.planecount):
                    post(f"(i, j, p, k) = ({i}, {j}, {p}, {k}) = {data[k]}")
                    m_ptr[k] = <double>data[k]
                    k += 1

    def fill(self, Atom atom, int plane = 0, int offsetcount = 0):
        """fill a matrix plane with an atom's values

        basically is an initial cython translation of the fill method in `jit.fill`
        """
        cdef long offset[jt.JIT_MATRIX_MAX_DIMCOUNT]
        cdef long argc, i, j
        cdef long savelock, offset0, offset1
        cdef char *p = NULL

        if atom.size and atom.ptr:
            savelock = <long>self.lock()
            self.refresh()

            if ((self.data is NULL) or (plane >= self.info.planecount) or (plane < 0)):
                # jt.jit_error_sym(x, jt._jit_sym_err_calculate)
                error("cannot set data to matrix")
                jt.jit_object_method(self.ptr, jt._jit_sym_lock, savelock)
                return

            # limited to filling at most into 2 dimensions per list
            offset0 = offset[0] if offsetcount > 0 else 0
            offset1 = offset[1] if offsetcount > 1 else 0
            offset0 = clamp(offset0, 0, self.info.dim[0] - 1)
            offset1 = clamp(offset1, 0, self.info.dim[1] - 1)
            argc = clamp(atom.size, 0, (self.info.dim[0] * (self.info.dim[1] - offset1)) - offset0)

            j = offset0 + offset1 * self.dim[0]

            if (self.info.type == jt._jit_sym_char):
                self.data += plane
                for i in range(argc):
                    p = self.data + (j // self.info.dim[0]) * self.info.dimstride[1] + (j % self.info.dim[0]) * self.info.dimstride[0]
                    (<jt.uchar*>p)[0] = jt.jit_atom_getcharfix(atom.ptr + i)
                    j += 1

            elif (self.info.type == jt._jit_sym_long):
                self.data += plane * 4
                for i in range(argc):
                    p = self.data + (j // self.info.dim[0]) * self.info.dimstride[1] + (j % self.info.dim[0]) * self.info.dimstride[0]
                    (<mx.t_int32*>p)[0] = <mx.t_int32>jt.jit_atom_getlong(atom.ptr + i)
                    j += 1

            elif (self.info.type == jt._jit_sym_float32):
                self.data += plane * 4
                for i in range(argc):
                    p = self.data + (j // self.info.dim[0]) * self.info.dimstride[1] + (j % self.info.dim[0]) * self.info.dimstride[0]
                    (<float*>p)[0] = <float>jt.jit_atom_getfloat(atom.ptr + i)
                    j += 1

            elif (self.info.type == jt._jit_sym_float64):
                self.data += plane * 8
                for i in range(argc):
                    p = self.data + (j // self.info.dim[0]) * self.info.dimstride[1] + (j % self.info.dim[0]) * self.info.dimstride[0]
                    (<double*>p)[0] = <double>jt.jit_atom_getfloat(atom.ptr + i)
                    j += 1

            self.unlock(savelock)


    # def myfill2d(self):
    #     cdef int i, j = 0
    #     cdef char* p = NULL
    #     cdef char* bp = self.data

    #     for i in range(self.info.dim[0]):
    #         for j in range(self.info.dim[1]):
    #             p = bp + (j * self.info.dimstride[1]) + (i * self.info.dimstride[0])

    #             p[0] = 1
    #             p += 1

    #             p[0] = 2
    #             p += 1

    # def myfill2d(self):
    #     cdef int i, j = 0
    #     cdef char* p = NULL
    #     cdef char* bp = self.data

    #     for i in range(self.info.dim[0]):
    #         for j in range(self.info.dim[1]):
    #             p = bp + (j * self.info.dimstride[1]) + (i * self.info.dimstride[0])
    #             for k in range(self.planecount):
    #                 p[0] = k
    #                 p += 1

    def myfill2d(self):
        cdef int i, j, z = 0
        cdef char* p = NULL
        cdef char* bp = self.data

        xs = list(range(self.height * self.width * self.planecount))

        for i in range(self.info.dim[0]):
            for j in range(self.info.dim[1]):
                p = bp + (j * self.info.dimstride[1]) + (i * self.info.dimstride[0])
                for k in range(self.planecount):
                    p[0] = <long>xs[z]
                    p += 1
                    z += 1

    # Typed memoryview methods
    
    def as_float32_memoryview(self):
        """Return a typed memoryview for float32 matrices.
        
        Returns:
            memoryview: Typed memoryview with appropriate shape and strides
        
        Raises:
            ValueError: If matrix type is not float32 or if matrix is not available
        """
        if self.type != 'float32':
            raise ValueError(f"Matrix type is '{self.type}', expected 'float32'")
        
        self.refresh()
        if self.data is NULL:
            raise ValueError("Matrix data is not available")
        
        # Create a memoryview from the buffer protocol
        return memoryview(self)
    
    def as_float64_memoryview(self):
        """Return a typed memoryview for float64 matrices.
        
        Returns:
            memoryview: Typed memoryview with appropriate shape and strides
        
        Raises:
            ValueError: If matrix type is not float64 or if matrix is not available
        """
        if self.type != 'float64':
            raise ValueError(f"Matrix type is '{self.type}', expected 'float64'")
        
        self.refresh()
        if self.data is NULL:
            raise ValueError("Matrix data is not available")
        
        # Create a memoryview from the buffer protocol
        return memoryview(self)
    
    def as_char_memoryview(self):
        """Return a typed memoryview for char matrices.
        
        Returns:
            memoryview: Typed memoryview with appropriate shape and strides
        
        Raises:
            ValueError: If matrix type is not char or if matrix is not available
        """
        if self.type != 'char':
            raise ValueError(f"Matrix type is '{self.type}', expected 'char'")
        
        self.refresh()
        if self.data is NULL:
            raise ValueError("Matrix data is not available")
        
        # Create a memoryview from the buffer protocol
        return memoryview(self)
    
    def as_long_memoryview(self):
        """Return a typed memoryview for long matrices.
        
        Returns:
            memoryview: Typed memoryview with appropriate shape and strides
        
        Raises:
            ValueError: If matrix type is not long or if matrix is not available
        """
        if self.type != 'long':
            raise ValueError(f"Matrix type is '{self.type}', expected 'long'")
        
        self.refresh()
        if self.data is NULL:
            raise ValueError("Matrix data is not available")
        
        # Create a memoryview from the buffer protocol
        return memoryview(self)
    
    def as_memoryview(self):
        """Return a generic typed memoryview for any matrix type.
        
        Returns:
            memoryview: Typed memoryview with appropriate shape and strides for the matrix's data type
        
        Raises:
            ValueError: If matrix data is not available
        """
        self.refresh()
        if self.data is NULL:
            raise ValueError("Matrix data is not available")
        
        # Create a memoryview from the buffer protocol
        # The __getbuffer__ method will set the appropriate format string
        return memoryview(self)

# def demo(height=6, width=8, planes=2):
#     z = 0
#     for i in range(height):
#         for j in range(width):
#             for p in range(planes):
#                 print(z)
#                 z += 1

# ----------------------------------------------------------------------------
# api.Path

# NOTE: this class is an untested unholy collection of ext_path.h and
# ext_sysfile.h functions that have been added to this placeholder class only
# because they compile  without errors. As these are tested, it is likely that
# the api will change considerably to remove redundancy, with functions moved
# into separate classes and also add python-friendly featurs such as convertsion
# to `pathlib.Path` instances + add __enter__, __exit__ methods as necessary.

cdef class Path:
    """A wrapper for Max friendly paths"""

    cdef public str filename    # name used to search for file
    cdef short path_id          # max short code for parent folder
    cdef mx.t_fourcc ftype      # fourchar ftype of file default to 'TEXT'
    cdef bint is_directory      # true if path is a directory
    cdef mx.t_fileinfo info     # instance of file metadata struct
    cdef mx.t_filehandle fh     # file handle
    cdef int size               # size of file handle file
    cdef mx.t_ptr_uint moddate  # modificiation date

    def __cinit__(self):
        self.path_id = 0
        self.moddate = 0
        self.fh = NULL

    def __init__(self, str filename = "", int path_id = 0, str ftype = 'TEXT'):
        self.filename = filename
        self.path_id = <short>path_id
        self.ftype = fourchar_to_int(ftype)
        if filename and not path_id:
            try: # assume pathname exists
                self.locatefile_extended(filename, ftype)
            except IOError:
                # assume pathname does not exist (for .create/.write/etc.)
                self.update_from_pathname(filename)
            except IOError:
                # raise
                pass
        self._get_info() # populate info attribute

    def __repr__(self):
        f"<Path id={self.path_id}>"

    def __enter__(self):
        return self

    def __exit__(self):
        self.close_sysfile()

    @property
    def is_directory(self) -> bool:
        """return true is path is a directory"""
        if self.path_id and not self.filename:
            return True
        return False

    @property
    def type(self):
        """type (four-char-code)"""
        return int_to_fourchar(self.info.type)

    @property
    def creator(self):
        """Mac-only creator (four-char-code)"""
        return int_to_fourchar(self.info.creator)

    @property
    def flags(self):
        """One of the values defined in e_max_fileinfo_flags"""
        return self.info.flags

    @property
    def pathname(self) -> str:
        """Returns absolute path of filename/path_id combo"""
        return self.to_absolute_path(self.filename, self.path_id)

    @property
    def absolute(self) -> pathlib.Path:
        """Returns pathlib.Path wrapped absolute path of filename/path_id combo"""
        return pathlib.Path(
            self.to_absolute_path(self.filename, self.path_id))

    @classmethod
    def maxapp_dir(cls) -> Path:
        """Retrieve the Path ID of the Max application.

        >>> p = Path.maxapp_dir()
        """
        return cls(path_id=mx.path_getapppath())

    @classmethod
    def temp_dir(cls) -> Path:
        """Retrieve the Path ID of a temp folder.

        >>> p = Path.temp_dir()
        """
        return cls(path_id=mx.path_tempfolder())

    @classmethod
    def desktop_dir(cls) -> Path:
        """Retrieve the Path ID of the desktop."""
        return cls(path_id=mx.path_desktopfolder())

    @classmethod
    def userdoc_dir(cls) -> Path:
        """Retrieve the Path ID of the user documents folder."""
        return cls(path_id=mx.path_userdocfolder())

    @classmethod
    def usermax_dir(cls) -> Path:
        """Retrieve the Path ID of the user max folder."""
        return cls(path_id=mx.path_usermaxfolder())

    @classmethod
    def support_dir(cls) -> Path:
        """Retrieve the Path ID of the support folder"""
        return cls(path_id=mx.path_getsupportpath())

    @classmethod
    def default_dir(cls) -> Path:
        """Retrieve the Path ID of the default search path."""
        return cls(path_id=mx.path_getdefault())

    def set_default_path(self, short path_id, bint recursive = False):
        """Install a path as the default search path.

        The default path is searched before the Max search path. For instance,
        when loading a patcher from a directory outside the search path, the
        patcher's directory is searched for files before the search path.
        path_setdefault() allows you to set a path as the default.
        """
        mx.path_setdefault(path_id, <short>recursive)

    @property
    def mod_date(self) -> str:
        """get formatted modification datatime for files and dirs"""
        if self.is_directory:
            self.get_moddate()
        else:
            self.get_file_moddate()
        return date_to_string(<mx.t_ptr_uint>self.moddate)

    def get_moddate(self):
        """Determine the modification date of the selected path."""
        cdef short err = mx.path_getmoddate(self.path_id, &self.moddate)
        if err:
            raise ValueError("could not get modification date of active path")

    def get_file_moddate(self):
        """Determine the modification date of the selected path."""
        cdef short err = mx.path_getfilemoddate(self.filename.encode(),
            self.path_id, &self.moddate)
        if err:
            raise ValueError("could not get modification date of active file")

    def to_absolute_path(self, str filename, short path_id) -> str:
        """Translates a Max path+filename combo into a correct POSIX absolute path

        The resulting absolute path can be used to pass to libraries
        and also handles multiple volumes correctly.
        """
        cdef char pathname[MAX_PATH_CHARS] # absolute path result of search
        cdef mx.t_max_err err = mx.path_toabsolutesystempath(path_id,
            filename.encode(), pathname)
        if err:
            raise IOError(f"can't convert {filename} to absolute path")
        return pathname.decode()

    def to_pathname(self, str filename, int path_id):
        """Create a fully qualified file name from a Path ID/file name combination.

        This routine will only convert a pathname pair to a valid path
        string if the path exists.
        """
        cdef char pathname[MAX_PATH_CHARS]
        cdef short err = mx.path_topathname(path_id, filename.encode(), pathname)
        if err:
            raise IOError("could not get pathname from filename and path_id")
        return pathname.decode()

    def from_pathname(self, str pathname) -> tuple[str, int]:
        """Create and return a filename and path_id combination from a fully
        qualified file name.

        Note that this function does not require that the file actually exist.
        In this way you can use it to convert a full path you may have received as an
        argument to a file writing message to a form appropriate to provide to
        a routine such as path_createfile().
        """
        cdef char filename[MAX_PATH_CHARS]
        cdef short path_id = 0
        cdef short err = mx.path_frompathname(pathname.encode(), &path_id, filename)
        # cdef short err = mx.path_frompotentialpathname(pathname.encode(), &path_id, filename)
        if err:
            raise IOError("could not get filename and path_id from pathname")
        return (filename.decode(), path_id)

    def from_potential_pathname(self, str pathname) -> tuple[str, int]:
        """Create and return a filename and path_id combination from a fully
        qualified file name.

        Note that this function does not require that the file actually exist.
        In this way you can use it to convert a full path you may have received as an
        argument to a file writing message to a form appropriate to provide to
        a routine such as path_createfile().
        """
        cdef char filename[MAX_PATH_CHARS]
        cdef short path_id = 0
        cdef short err = mx.path_frompotentialpathname(pathname.encode(), &path_id, filename)
        if err:
            raise IOError("could not get filename and path_id from pathname")
        return (filename.decode(), path_id)

    def update_from_pathname(self, str pathname):
        """Create a filename and update path's path_id and filename from a fully
        qualified pathname.

        Note that in this case, pathname does not have to exist.
        """
        norm_pathname = self.nameconform(pathname)
        filename, path_id = self.from_pathname(norm_pathname)
        self.path_id = <short>path_id
        self.filename = filename

    def nameconform(self, str src_pathname) -> str:
        """Convert a source path string to destination path string using the
        specified style and type.

        params:
            src     A pointer to source character string to be converted.
            dst     A pointer to destination character string.
            style   The destination filepath style, as defined in #e_max_path_styles
            type    The destination filepath type, as defined in #e_max_path_types

        returns An error code.
        """
        cdef char dst_pathname[MAX_PATH_CHARS]
        cdef short err = mx.path_nameconform(src_pathname.encode(), dst_pathname,
            mx.PATH_STYLE_MAX, mx.PATH_TYPE_BOOT)
        if err:
            raise IOError(f"cant't nameconform '{src_pathname}'")
        return dst_pathname.decode()

    def to_potential_name(self, str filename = "", int path_id = 0) -> str:
        """Create a fully qualified file name from a Path ID/file name combination,
        regardless of whether or not the file exists on disk.
        """
        cdef char pathname[MAX_PATH_CHARS]
        cdef short check = 0
        cdef short err = 0
        if not filename and not path_id:
            err = mx.path_topotentialname(self.path_id, self.filename.encode(), pathname, check)
        else:
            err = mx.path_topotentialname(path_id, filename.encode(), pathname, check)
        if err:
            raise IOError("could not get pathname from filename and path_id")
        return pathname.decode()

    def locatefile(self, str name) -> int:
        """Find a Max document by name in the search path.

        Searches through the directories specified by the user for
        Patcher files and tables current default path and the directory
        containing the Max application.

        Returns the path code if found else 0
        """
        cdef short outvol = 0
        cdef short binflag = 0
        cdef short err = mx.locatefile(name.encode(), &outvol, &binflag)
        if err:
            return 0
        else:
            return outvol

    def locatefile_extended(self, str filename, str code = 'TEXT'):
        """Find a file by name.

        If a complete path is not specified, search for the name in the search path.
        """
        # cdef char pathname[MAX_PATH_CHARS] # absolute path result of search
        cdef mx.t_fourcc ftype  = <mx.t_fourcc>fourchar_to_int(code)
        cdef mx.t_fourcc outtype  = 0
        cdef short err = 0

        err = mx.locatefile_extended(filename.encode(), &self.path_id,
            &outtype, &ftype, 1)
        if err:
            raise IOError(f"can't find file '{filename}'")
        if ftype == outtype: # request and response match
            self.ftype = ftype
        else: # should not get here
            error("reqested {}, got {}".format(
                int_to_fourchar(ftype),
                int_to_fourchar(outtype)))

    def resolve_file(self, str name, short path_id) -> tuple[str, int]:
        """Resolve a Path ID plus a (possibly extended) file name
        into a path that identifies the file's directory and a filename.

        This routine converts a name and Path ID to a standard form in which
        the name has no path information and does not refer to an aliased file.
        """
        cdef char _name[MAX_PATH_CHARS]
        cdef short outpath
        cdef short err = 0

        _name = <bytes>name
        err = mx.path_resolvefile(_name, path_id, &outpath)
        if err:
            raise IOError(f"could not resolvefile {name}")
        norm = _name.decode()
        return (norm, outpath)

    def create_folder(self, str folder_name, short path_id) -> int:
        """Create folder given a folder name and path_id"""
        cdef short new_path_id = 0
        cdef short err = mx.path_createfolder(path_id, folder_name.encode(), &new_path_id)
        if err:
            raise IOError("could not create folder from folder name and path_id")
        return new_path_id

    def _get_info(self):
        """Populate t_fileinfo struct file metatadata  instance"""
        cdef short err = mx.path_fileinfo(self.filename.encode(), self.path_id, &self.info)
        if err:
            raise IOError(f"couldn't retrieve file info for {self.filename}")

    def copy_file(self, str dstfilename, short dstpath):
        """Copy file given destination filename and path id"""
        cdef short err = mx.path_copyfile(self.path_id, self.filename.encode(),
            dstpath, dstfilename.encode())
        if err:
            raise IOError(f"could not copy filename to destination filename {dstfilename} "
                          f"with path id {dstpath}")

    def copy_folder(self, str dstfilename, short dstpath, bint recurse = False) -> int:
        """Copy folder given destination filename and path id"""
        cdef short newpath = 0
        cdef short err = mx.path_copyfolder(self.path_id, dstpath, dstfilename.encode(),
            <long>recurse, &newpath)
        if err:
            raise IOError(f"could not copy src folder {self.path_id} to destination  "
                          f"with path id {dstpath}")
        return <int>newpath

    def copy_to_tempfile(self, str dst_filename, short dst_path_id) -> Path:
        """Copy file given destination filename and path id to a tempile"""
        cdef short outpath = 0
        cdef char * outtempfile = NULL
        cdef short err = mx.path_copytotempfile(self.path_id, self.filename.encode(),
            &outpath, outtempfile)
        if err:
            raise IOError("could not copy filename to destination tempfile")
        return Path(outtempfile, outpath)

    def rename(self, str new_name):
        """Rename the file"""
        cdef short err = mx.path_renamefile(self.filename.encode, self.path_id, new_name.encode())
        if err:
            raise IOError(f"couldn't rename {self.filename}")

    def delete(self):
        """Delete located file."""
        cdef short err = mx.path_deletefile(self.filename.encode(), self.path_id)
        if err:
            raise IOError(f"couldn't delete {self.filename}")

    def open(self, str perm = 'r') -> Path:
        """Open the active file given permission ('w', 'r', 'rw'), and returns self

        >>> with Path('/tmp/demo.txt').open('w') as f:
            f.write('hello')
        """
        self.open_sysfile(self.filename.encode(), self.path_id, perm)
        return self

    def open_sysfile(self, str filename, short path_id, str perm = 'w'):
        """Open a file given a filename and Path ID.

        Will update the t_filehandle reference in the object to point to the open file

        permission modes are:
            'r': 1 read
            'w': 2 write
           'rw': 3 read/write
        """
        assert not self.is_directory, "can only open a file"
        cdef short _perm = <short>dict(r=1, w=2, rw=3)[perm]
        cdef short err = mx.path_opensysfile(filename.encode(), path_id, &self.fh, _perm)
        if err:
            raise IOError(f"could not open sysfile {filename} with path_id={path_id}")

    def new(self, ftype = 'TEXT'):
        """Create a new file given fourchar file ftype, and returns self

        >>> with Path('/tmp/demo.txt').new() as f:
            f.write('hello')
        """
        self.create_sysfile(self.filename.encode(), self.path_id, ftype)
        return self

    def create_sysfile(self, str filename, short path_id, str ftype = 'TEXT'):
        """Create a file given a type code, a filename, and a Path ID.

        Will update the t_filehandle reference in the object to point to the created file

        permission modes are:
            'r': 1 read
            'w': 2 write
           'rw': 3 read/write
        """
        assert not self.is_directory, "can only create a file"
        cdef mx.t_fourcc _ftype = <mx.t_fourcc>fourchar_to_int(ftype)
        cdef short err = mx.path_createsysfile(filename.encode(), path_id, _ftype, &self.fh)
        if err:
            raise IOError(f"could not create sysfile {filename} "
                          f"with path_id={path_id}, ftype={ftype}")

    def close_sysfile(self):
        """Close a file opened with sysfile_open().

        This function is similar to FSClose() or fclose().
        It should be used instead of system-specific file closing routines in order to make max external
        code that will compile cross-platform.
        """
        cdef mx.t_max_err err = mx.sysfile_close(self.fh)
        if err:
            raise IOError("can't close open filehandle")

    def sysfile_read(self, mx.t_ptr_size count) -> str:
        """Read a file from disk.

        This function is similar to FSRead() or fread(). It should be used instead of
        these functions (or other system-specific file reading routines) in order
        to make max external code that will compile cross-platform. It reads
        "count" bytes from file handle at current file position into "bufptr".
        The byte count actually read is set in "count", and the file position is
        updated by the actual byte count read.
        """
        cdef char * bufptr = <char *>mx.sysmem_newptr(count * sizeof(char))
        cdef mx.t_max_err err = mx.sysfile_read(self.fh, &count, <char *>bufptr)
        if err:
            raise IOError("could not read contents of from from disk")
        cdef str result = bufptr.decode()
        mx.sysmem_freeptr(bufptr)
        return result

    def sysfile_readtohandle(self, int size) -> list[str]:
        """Read the contents of a file into a handle."""
        cdef int i = 0
        cdef char** fh = <mx.t_handle>mx.sysmem_newhandleclear(<mx.t_ptr_size>size)
        cdef mx.t_max_err err = mx.sysfile_readtohandle(self.fh, &fh)
        if err:
            raise IOError("could not read contents of filehandle into a handle")
        lines = []
        for i in range(size):
            line = fh[i].decode()
            # line = fh[i][0].decode()
            lines.append(line)
        mx.sysmem_freehandle(fh)
        return lines

    def sysfile_readtoptr(self, int bufsize) -> str:
        """Read the contents of a file into a pointer."""
        cdef char* bufptr = <char *>mx.sysmem_newptr(bufsize * sizeof(char))
        cdef mx.t_max_err err = mx.sysfile_readtoptr(self.fh, &bufptr)
        if err:
            raise IOError("could not read contents of a file into a pointer")
        cdef str result = bufptr.decode()
        mx.sysmem_freeptr(bufptr)
        return result

    def sysfile_write(self, object contents):
        """Write part of a file to disk.

        This function is similar to FSWrite() or fwrite(). It should be used instead
        of these functions (or other system-specific file reading routines) in
        order to make max external code that will compile cross-platform. The
        function writes "count" bytes from "bufptr" into file handle at current
        file position. The byte count actually written is set in "count", and the
        file position is updated by the actual byte count written.
        """
        cdef mx.t_ptr_size count = <mx.t_ptr_size>len(contents)
        cdef char* bufptr = <char *>mx.sysmem_newptr(count * sizeof(char))
        cdef mx.t_max_err err = mx.MAX_ERR_NONE
        if isinstance(contents, bytes):
            bufptr = contents
        elif isinstance(contents, str):
            bufptr = <bytes>contents
        else:
            raise TypeError("could not write this type file handler")
        err = mx.sysfile_write(self.fh, &count, <char*>bufptr)
        if err:
            raise IOError("could not write contents file handler")
        mx.sysmem_freeptr(bufptr)
        # mx.post("wrote %d bytes to file handler", count)

    write = sysfile_write

    def sysfile_seteof(self, int nbytes):
        """Set the size of the file handle in bytes."""
        cdef mx.t_max_err err = mx.sysfile_seteof(self.fh, <mx.t_ptr_size>nbytes)
        if err:
            raise ValueError("could not set the size of the file handle")

    def sysfile_geteof(self) -> int:
        """Get the size of a file handle."""
        cdef mx.t_ptr_size nbytes = 0
        cdef mx.t_max_err err = mx.sysfile_geteof(self.fh, &nbytes)
        if err:
            raise ValueError("could not get the size of the file handle")
        return <int>nbytes

    def sysfile_getpos(self) -> int:
        """Get the current file position of a file handle."""
        cdef mx.t_ptr_size filepos = 0
        cdef mx.t_max_err err = mx.sysfile_getpos(self.fh, &filepos)
        if err:
            raise ValueError("could not get the position of the file handle")
        return <int>filepos

    cdef mx.t_max_err sysfile_spoolcopy(self, mx.t_filehandle src, mx.t_filehandle dst, mx.t_ptr_size size):
        """Copy the contents of one file handle to another file handle.

        @param  src   The file handle from which to copy.
        @param  dst   The file handle to which the copy is written.
        @param  size  The number of bytes to copy.  If 0 the size of src will be used.
        @return     An error code.
        """
        return mx.sysfile_spoolcopy(src, dst, size)

    def sysfile_readtextfile(self, int maxlen = 0, int flags = 0) -> str:
        """Read a text file from disk.

        This function reads up to the maximum number of bytes given by
        maxlen from file handle at current file position into the htext
        handle, performing linebreak translation if set in flags.
        """
        cdef int size = self.sysfile_geteof()
        cdef mx.t_handle htext = <mx.t_handle>mx.sysmem_newhandleclear(<mx.t_ptr_size>size)
        cdef mx.t_max_err err = mx.MAX_ERR_NONE
        err = mx.sysfile_readtextfile(self.fh,
            htext, <mx.t_ptr_size>maxlen, mx.TEXT_LB_NATIVE)
        if err:
            raise ValueError("could not read text from the file handle")
        lines = []
        for i in range(size):
            line = htext[i].decode()
            lines.append(line)
        mx.sysmem_freehandle(htext)
        return "".join(lines)

    def sysfile_writetextfile(self, str text):
        """Write a text file to disk.

        This function writes a text handle to a text file performing linebreak
        translation if set in flags.

        see: https://cycling74.com/forums/problem-with-sysfile_writetextfile
        """
        cdef char* buf = <bytes>text
        cdef mx.t_handle htext = mx.sysmem_newhandle(0)
        cdef mx.t_max_err err = mx.MAX_ERR_NONE

        mx.sysmem_ptrandhand(buf, htext, strlen(buf))
        err = mx.sysfile_writetextfile(self.fh, htext, mx.TEXT_LB_NATIVE)
        if err:
            raise IOError("could not write text to handler")
        mx.sysfile_close(self.fh)
        mx.sysmem_freehandle(htext)

    cdef mx.t_max_err sysfile_openhandle(self, char **h, mx.t_sysfile_flags flags, mx.t_filehandle *fh):
        """Create a #t_filehandle from a pre-existing handle.

        @param    h   A handle for some data, data is *not* copied and *not* freed on file close.
        @param    flags Pass 0 (additional flags are private).
        @param    fh    The address of a #t_filehandle which will be allocated.
        @return       An error code.
        """
        return mx.sysfile_openhandle(h, flags, fh)

    cdef mx.t_max_err sysfile_openptrsize(self, char *p, mx.t_ptr_size length, mx.t_sysfile_flags flags, mx.t_filehandle *fh):
        """Create a #t_filehandle from a pre-existing pointer.

        @param    p   A pointer to some data. Data is *not* copied and *not* freed on file close.
        @param    length  The size of p.
        @param    flags Pass 0 (additional flags are private).
        @param    fh    The address of a #t_filehandle which will be allocated.
        @return       An error code.
        """
        return mx.sysfile_openptrsize(p, length, flags, fh)

# ----------------------------------------------------------------------------
# api.PyExternal

cdef class PyExternal:
    """Wraps the `py` external object and its methods.

    Should expose as much functionality as possible.
    """
    cdef px.t_py *ptr
    cdef public str name

    def __cinit__(self):
        """Retrieves the py object name and reference.

        PY_OBJ_NAME is set to __builtins__ at object creation
        making it available to all modules.

        Since all py objects are registered, knowing the name
        allows any module in the namespace to get a reference
        (as below) to its parent object.
        """
        self.name = getattr(__builtins__, 'PY_OBJ_NAME')
        self.ptr = <px.t_py *>mx.object_findregistered(
            mx.CLASS_BOX, mx.gensym(self.name.encode()))

    def __repr__(self) -> str:
        return f"<PyExternal '{self.name}'>"

    def bang(self):
        """Send bang out of left (default) outlet"""
        px.py_bang(self.ptr)

    def bang_success(self):
        """Signal success by banging out of right outlet."""
        px.py_bang_success(self.ptr)

    def bang_failure(self):
        """Signal failure by banging out of middle outlet."""
        px.py_bang_failure(self.ptr)

    def log_info(self, str msg):
        """Log info using object_post."""
        px.py_info(self.ptr, msg.encode())

    def log_debug(self, str msg):
        """Log debug using object_post."""
        px.py_debug(self.ptr, msg.encode())

    def log_error(self, str s):
        """Log error using object_error."""
        px.py_error(self.ptr, s.encode())

    def scan(self):
        """Scan patcher for named objects."""
        px.py_scan(self.ptr)

    def lookup(self, str name) -> bool:
        """Lookup a variable name in the object registry."""
        cdef mx.t_hashtab* registry = px.py_get_global_registry()
        cdef mx.t_object* obj = NULL
        cdef mx.t_max_err err

        if (mx.hashtab_getsize(registry) == 0):
            self.log_error("registry not populated")
            return

        err = mx.hashtab_lookup(registry, str_to_sym(name), &obj)

        if ((err != mx.MAX_ERR_NONE) or (obj == NULL)):
            self.log_error("no object found with name")
            return False
        else:
            self.log_debug("found object")
            return True

    def get_patcher(self) -> Patcher:
        """Get the containing patcher."""
        patcher = Patcher.from_object(<mx.t_object*>self.ptr)
        return patcher

    def get_box(self) -> Box:
        """Get the external's box."""
        box = Box.from_object_ptr(<mx.t_object*>self.ptr)
        return box

    def get_buffer(self, name: str) -> Buffer:
        """Retrieve a buffer by name."""
        buf = Buffer.from_name(<mx.t_object*>self.ptr, name)
        return buf

    def create_buffer(self, name: str, sample_file: str) -> Buffer:
        """Create a buffer with a given name from a sample file."""
        buf = Buffer.new(<mx.t_object*>self.ptr, name, sample_file)
        return buf

    def create_empty_buffer(self, str name, int duration_ms):
        """Create an empty buffer with a given name and duration in milliseconds."""
        buf = Buffer.empty(<mx.t_object*>self.ptr, name, duration_ms)
        return buf

    def send(self, str name, *args):
        """general message send to receiver"""
        _args = [name] + list(args)
        cdef Atom atom = Atom(*_args)
        assert isinstance(_args[0], str), "send first arg must be str name of receiver"
        px.py_send(self.ptr, mx.gensym(""), atom.size, atom.ptr)

    cdef out_sym(self, str arg):
        """Send a symbol to the outlet."""
        mx.outlet_anything(<void*>px.get_outlet(self.ptr), str_to_sym(arg), 0, NULL)

    cdef out_float(self, float arg):
        """Send a float to the outlet."""
        mx.outlet_float(<void*>px.get_outlet(self.ptr), <double>arg)

    cdef out_int(self, int arg):
        """Send an integer to the outlet."""
        mx.outlet_int(<void*>px.get_outlet(self.ptr), <long>arg)

    cdef out_list(self, list arg):
        """Send a list to the outlet.

        Note: not recursive...(yet) still cannot deal with list in list
        """
        cdef Atom atom = Atom.from_seq(arg)
        cdef int i

        for i, elem in enumerate(arg):
            if type(elem) is float:
                atom.set_float(i, <double>elem)
            elif type(elem) is int:
                atom.set_long(i, <long>elem)
            elif type(elem) is str:
                atom.set_symbol(i, elem)
            else:
                continue
        mx.outlet_list(<void*>px.get_outlet(self.ptr),
            mx.gensym("list"), atom.size, atom.ptr)

    cdef out_dict(self, dict arg):
        """Send a dictionary to the outlet.

        Note: not recursive...(yet) still cannot deal with dict in dict
        """
        res = []
        for k, v in arg.items():
            res.append(k)
            res.append(':')
            if type(v) in [list, set, tuple]:
                for i in v:
                    res.append(i)
            else:
                res.append(v)
        self.out_list(res)

    def out_atoms(self, Atom atoms):
        """Send atoms to an outlet in your Max object, handling complex datatypes
        that may be present in those atoms.
        """
        mx.dictobj_outlet_atoms(<void*>px.get_outlet(self.ptr), atoms.size, atoms.ptr)

    def out(self, arg: object):
        """Send an object to the outlet."""
        if isinstance(arg, float):
            self.out_float(arg)
        elif isinstance(arg, int):
            self.out_int(arg)
        elif isinstance(arg, str):
            self.out_sym(arg)
        elif isinstance(arg, list):
            self.out_list(arg)
        elif isinstance(arg, dict):
            self.out_dict(<dict>arg)
        elif isinstance(arg, Atom):
            self.out_atoms(arg)
        else:
            return

    cdef mx.t_max_err method_binbuf(self, mx.t_symbol* s, void* buf, mx.t_atom* rv):
        """Call a method with a binary buffer."""
        return mx.object_method_binbuf(<mx.t_object*>self.ptr, s, buf, rv)

# ----------------------------------------------------------------------------
# Alternative external extension type (obj pointer retrieved via uintptr_t

cdef class PyMxObject:
    """Alternative external extension type (obj pointer retrieved via uintptr_t)."""

    cdef px.t_py *x

    def __cinit__(self):
        self.x = <px.t_py*>px.py_get_object_ref()

    def __repr__(self) -> str:
        return f"<PyMxObject '{self.name}'>"

    cpdef bang(self):
        """send a bang"""
        px.py_bang(self.x)

def test_ref():
    """Test the reference to the py object."""
    ext = PyMxObject()
    ext.bang()

# ----------------------------------------------------------------------------
# api.Object
#
# to be subclassed by max object wrapper with special methods in the Max SDK

cdef class Object(MaxObject):
    """An abstract wrapper for a Max t_object
    """

    def __init__(self, name: str, *args, **kwds):
        self.name = name
        self._namespace = kwds.get('namespace', 'box')
        self._classname = kwds.get('classname',
            (lambda: (self.__class__.__name__.lower()))()
        ) # deferred execution to capture subclass class name

        self.ptr = self._object_ptr_from_existing(self._classname, self.name)
        self.ptr_owner = False
        if self.ptr is NULL:
            self.ptr = self._object_ptr_from_new(
                self._classname, self.name, self._namespace, args)
            self.ptr_owner = True
        if self.ptr is NULL:
            raise ValueError("could not retrieve nor create the "
                            f"{self._classname} object with name '{name}'")

    def __repr__(self) -> str:
        return f"<{self.__class__.__name__} '{self.name}'>"

    # helper methods

    # def call(self, str method, *args):
    #     """Helper wrapper method around object_method* variants"""
    #     cdef Atom atom = Atom(*args)
    #     cdef mx.t_max_err err = mx.MAX_ERR_NONE
    #     cdef mx.t_symbol* meth = str_to_sym(method)

    #     if len(args) == 0:
    #         mx.object_method(<mx.t_object*>self.ptr, str_to_sym(method))
    #     elif len(args) == 1:
    #         if isinstance(args[0], str):
    #             err = mx.object_method_sym(self.ptr, meth, str_to_sym(args[0]), NULL)
    #         elif isinstance(args[0], int):
    #             err = mx.object_method_long(self.ptr, meth, <long>args[0], NULL)
    #         elif isinstance(args[0], float):
    #             err = mx.object_method_float(self.ptr, meth, <float>args[0], NULL)
    #         elif isinstance(args[0], MaxObject):
    #             err = mx.object_method_obj(self.ptr, meth, <mx.t_object*>args[0].ptr, NULL)
    #     else:
    #         err = mx.object_method_typed(<mx.t_object*>self.ptr,
    #             str_to_sym(method), atom.size, atom.ptr, NULL)
    #     if err:
    #         raise ValueError(f"could not apply single arg to method {method}")

# ----------------------------------------------------------------------------
# api.Max

cdef class Max(Object):
    """A class to enable messages to the 'max' application."""

    def __init__(self):
        self.ptr = <mx.t_object*>mx.object_new(
            mx.gensym("nobox"), mx.gensym("max"))
        self.ptr_owner = False

    def __repr__(self) -> str:
        return "<Max>"

    def clean(self):
        """Call the 'clean' method on the application object."""
        self.call("clean")

    def clearmaxwindow(self):
        """Call the 'clearmaxwindow' method on the application object."""
        self.call("clearmaxwindow")

    def htmlref(self, name: str):
        """Call the 'htmlref' method on the application object."""
        self.call("htmlref", name)

    def openfile(self, name: str):
        """Call the 'openfile' method on the application object."""
        self.call("openfile", name)

    def closefile(self, name: str):
        """Call the 'closefile' method on the application object."""
        self.call("closefile", name)

    def midilist(self):
        """Call the 'midilist' method on the application object."""
        self.call("midilist")

    def maxwindow(self):
        """Call the 'maxwindow' method on the application object."""
        self.call("maxwindow")

    def paths(self):
        """Call the 'paths' method on the application object."""
        self.call("paths")

    def externaleditor(self, name: str):
        """Call the 'externaleditor' method on the application object."""
        self.call("externaleditor", name)

    def useexternaleditor(self, on: bool = True):
        """Call the 'useexternaleditor' method on the application object."""
        self.call("useexternaleditor", on)

    def hidemenubar(self):
        """Call the 'hidemenubar' method on the application object."""
        self.call("hidemenubar")

    def showmenubar(self):
        """Call the 'showmenubar' method on the application object."""
        self.call("showmenubar")

    def externs(self):
        """Call the 'externs' method on the application object."""
        self.call("externs")

    def getsystem(self, receiver: str):
        """Call the 'getsystem' method on the application object."""
        self.call("getsystem", receiver)

    def getversion(self, receiver: str):
        """Call the 'getversion' method on the application object."""
        self.call("getversion", receiver)

    def sendapppath(self, receiver: str):
        """Call the 'sendapppath' method on the application object."""
        self.call("sendapppath", receiver)

    def launchbrowser(self, url: str):
        """Call the 'launchbrowser' method on the application object."""
        self.call("launchbrowser", url)

    def preempt(self, on: bool = True):
        """Call the 'preempt' method on the application object."""
        self.call("preempt", on)

    def quit(self):
        """Call the 'quit' method on the application object."""
        self.call("quit")


# ----------------------------------------------------------------------------
# api.Table

cdef class Table(Object):
    """A wrapper class to acess a pre-existing Max table"""

    cdef long **storage
    cdef readonly long size

    def __cinit__(self):
        self.ptr = NULL
        self.ptr_owner = False
        self.name = ""
        self._classname = ""
        self._namespace = 'box'
        self.type_map = {}
        self.storage = NULL
        self.size = 0

    def __init__(self, name: str, *args, **kwds):
        self.name = name
        self._namespace = kwds.get('namespace', 'box')
        self._classname = "table"

        self.ptr = self._object_ptr_from_existing(self._classname, name)
        self.ptr_owner = False
        if self.ptr is not NULL:
            check = mx.table_get(str_to_sym(name), &self.storage, &self.size)
            if check == 0: # i.e is found
                return

        self.ptr = self._object_ptr_from_new(
            self._classname, self.name, self._namespace, args)
        self.ptr_owner = True
        if self.ptr is NULL:
            raise ValueError("could not retrieve nor create the "
                            f"{self._classname} object with name '{name}'")

    def __repr__(self) -> str:
        return f"<Table '{self.name}' size:{self.size}>"

    def __len__(self):
        return self.size

    def __getitem__(self, int idx):
        return self.get_int(idx)

    def __setitem__(self, int idx, int value):
        self.set_int(idx, value)

    def __iter__(self):
        return iter(self.as_list())

    # helper methods

    def populate(self, list[int] xs):
        """Populate a table from a python list of ints"""
        if len(xs) <= self.size:
            for i, x in enumerate(xs):
                self.storage[0][i] = <long>x
        else:
            for i in range(self.size):
                self.storage[0][i] = <long>xs[i]
        self.set_dirty() # makes ui updates faster!

    def to_list(self):
        """Convert a table to a python list of ints"""
        cdef long value
        cdef list[int] xs = []
        for i in range(self.size):
            value = self.storage[0][i]
            xs.append(<int>value)
        return xs

    def set_dirty(self):
        """Mark a table object as having changed data."""
        cdef short res = mx.table_dirty(str_to_sym(self.name))
        if res != 0:
            raise TypeError(f"no table is associated with tableName {self.name}")

    # msg methods

    def bang(self):
        """Output a random quantile

        Same as a `quantile` message with a random number between 0 and 32,768
        as an argument. See the `quantile` message for more details.
        """
        self.call("bang")

    def get_int(self, int index):
        """Retrieve a number by index

        int(int index?)

        Retrieves the number by address from the `table`, and sends it out the
        left outlet.
        """
        self.call("int", index)

    def set_int(self, int index, int value):
        """Store a value at an index

        list(int index?, int value?)

        The second number is stored in at the address (index) specified by the
        first number.
        """
        self.call("list", index, value)

    def cancel(self):
        """Ignore value received

        Causes table to ignore a number received in the right inlet, so that
        the next number received in the left inlet will output a number,
        rather than storing a number at that address.
        """
        self.call("cancel")

    def clear(self):
        """Set all values to 0"""
        self.call("clear")

    def const(self, int value):
        """Fill the table with a number"""
        self.call("const", value)

    def dump(self):
        """Output all numbers

        Sends all the numbers stored in the table out the left outlet in
        immediate succession, beginning with address 0.
        """
        self.call("dump")

    def fquantile(self, float multiplier = 0.5):
        """Return quantile address from float

        fquantile(float multiplier?)

        Given a number between zero and one, multiplies the number by the sum
        of all the numbers in the table. Then, table sends out the address
        at which the sum of the all values up to that address is greater
        than or equal to the result.
        """
        self.call("fquantile", multiplier)

    def getbits(self, int address, int start, int bits):
        """Get bit values from an index

        getbits(int address?, int start?, int bits?)

        Gets the value of one or more specific bits of a number stored in the
        table, and sends that value out the left outlet. The first
        argument is the address to query; the second argument is the
        starting bit location in the number stored at that address (the
        bit locations are numbered 0 to 31, from the least significant bit
        to the most significant bit); and the third argument specifies how
        many bits to the right of the starting bit location should be sent
        out. The specified bits are sent out the outlet as a single
        decimal integer.
        """
        self.call("getbits", address, start, bits)

    def goto(self, int index):
        """Set the pointer location

        goto(int index?)

        Sets a pointer to the address specified by the number. The pointer is
        set at the beginning of the table initially.
        """
        self.call("goto", index)

    def in1(self, int value):
        """Store a value

        in1(int value?)

        Stores the value at the next index number received at the left inlet.
        """
        self.call("in1", value)

    def inv(self, int value):
        """Find the index of a value

        inv(int value?)

        Finds the first value which is greater than or equal to that number,
        and sends the address of that value out the left outlet.
        """
        self.call("inv", value)

    def length(self):
        """Output the table size"""
        self.call("length")

    def load(self):
        """Fill a table with a stream of data

        Places the table in load mode. In load mode, every number received in
        the left inlet gets stored in the table, beginning at address 0
        and continuing until the table is filled (or until the table is
        taken out of load mode by a `normal` message). If more numbers are
        received than will fit in the size of the table, additional
        numbers are ignored.
        """
        self.call("load")

    def max(self):
        """Retrieve the maximum stored value"""
        self.call("max")

    def min(self):
        """Retrieve the minimum stored value"""
        self.call("min")

    def next(self):
        """Output value, then move the pointer

        Sends the value stored in the address pointed at by the pointer out
        the left outlet, then sets the pointer to the next address. If the
        pointer is currently at the last address in the table, it wraps
        around to the first address.
        """
        self.call("next")

    def normal(self):
        """Exit load mode

        Takes the table out of load mode and reverts it to normal operation.
        See the `load` message for more details.
        """
        self.call("normal")

    def open(self):
        """Open the graphic editor

        Opens the object’s graphic editor window and brings it to the
        foreground. Double-clicking on the `table` object in a locked
        patcher has the same effect.
        """
        self.call("open")

    def prev(self):
        """Output value, then move the pointer

        Causes the same output as the `next` message, but the pointer is then
        decremented rather than incremented. If the pointer is currently
        at the first address in the table, it wraps around to the last
        address.
        """
        self.call("prev")

    def quantile(self, int number):
        """Return quantile address

        quantile(int number?)

        Multiplies the incoming number by the sum of all the numbers in the
        table. This result is then divided by 2^15 (32,768). Then, table
        sends out the address at which the sum of all values up to that
        address is greater than or equal to the result.
        """
        self.call("quantile", number)

    def read(self, str filename):
        """Read a data file from disk

        read(symbol filename?)

        Opens and reads data values from a file in Text or Max binary format.
        Without an argument, `read` opens a standard Open Document dialog
        to choose a file. If the file contains valid data, the entire
        contents of the existing table are replaced with the file
        contents.
        """
        self.call("read", filename)

    def refer(self, str name):
        """Change table data context

        refer(symbol name?)

        Sets the receiving `table` object to read its data values from the
        named table.
        """
        self.call("refer", name)

    def send(self, str receiver_name, int address):
        """Send a value to a receive object

        send(symbol receive-name?, int address?)

        Sends the value stored at the incoming address to all `receive`
        objects with that name.
        """
        self.call("send", receiver_name, address)

    def set(self, int start, list[int] values):
        """Store a list of values

        set(int start?, list values?)

        Stores values in certain addresses. The first argument specifies an
        address. The next number is the value to be stored in that
        address, and each number after that is stored in a successive
        address.
        """
        # args = [start]
        # args.extend(values)
        self.call("set", start, values)

    def setbits(self, int address, int start, int count, int value):
        """Change the bit values of an address

        setbits(int address?, int start?, int count?, int value?)

        Changes the value of one or more specific bits of a number stored in
        the table. The first argument is the address being referred to;
        the second argument is the starting bit location in the number
        stored at that address (the bit locations are numbered 0 to 31,
        from the least significant bit to the most significant bit); the
        third argument specifies how many bits to the right of the
        starting bit location should be modified, and the fourth argument
        is the value (stated in decimal or hexadecimal form) to which
        those bits should be set.
        """
        self.call("setbits", address, start, count, value)

    def sum(self):
        """Output the sum of all values"""
        self.call("sum")

    def wclose(self):
        """Close the graphic editing window"""
        self.call("wclose")

    def write(self, path: str):
        """Write data to disk

        Opens a standard save file dialog for choosing a name to write data
        values from the table. The file can be saved in Text or Max binary
        format.
        """
        self.call("write", path)


    # table attributes

    def embed(self, bint value = True):
        """Embed table with patcher or not.


        This is a an attribute so need special methods
        """
        cdef mx.t_max_err err = mx.object_attr_setlong(<mx.t_object*>self.ptr,
            mx.gensym("embed"), <long>value)
        if err:
            raise TypeError("could not set embed attribute")


# ----------------------------------------------------------------------------
# api.Coll

cdef class Coll(Object):
    """Store and edit a collection of data

    Allows for the storage, organization, editing, and retrieval of
    different messages.
    """

    # msg methods

    def bang(self):
        """Retrieve the next data set

        See the `next` listing.
        """
        self.call("bang")

    def get(self, object index):
        """Retrieve data by index

        The number refers to the address of a message stored in `coll`. If a
        message is stored at that address, the stored message is
        output. If the stored message is a single symbol, it is always
        prepended with the word `symbol` when output.
        """
        if isinstance(index, int):
            self.call("int", index)
        elif isinstance(index, float):
            self.call("float", index)
        elif isinstance(index, str):
            self.call("symbol", index)
        else:
            raise TypeError(f"type {type(object)} not supported as index")

    def set(self, int index, *args):
        """Store index and data

        list(int index?, list data?)

        The first value is used as the address (the storage location within
        `coll`) at which to store the remaining items in the list. The
        address will always be stored as an int.
        """
        self.call("list", index, args)

    def append(self, *args):
        """Add item associated with an index

        append(list data?)

        The `append` message creates a new item associated with an index that
        is one larger than the highest current index. For example, if
        the `coll` is empty, `append xyz` will add an item `xyz`
        associated with the index 0. `append xyz` a second time will
        add another item `xyz` associated with the index 1.
        """
        self.call("append", *args)

    def assoc(self, str address_name, int data_index):
        """Associate a name with an index

        assoc(symbol address name?, int data index?)

        Associates a symbol with the numeric address, provided that the number
        address already exists. After association, any reference to
        that symbol will be interpreted as a reference to the number
        address. Each number address can have only one symbol
        associated with it.
        """
        self.call("assoc", address_name, data_index)

    def clear(self):
        """Clear all data"""
        self.call("clear")

    def deassoc(self, str address_name, int data_index):
        """De-associate a name with an index

        deassoc(symbol address name?, int data index?)

        Removes the association between a symbol and the number address. The
        symbol will no longer have any meaning to `coll`.
        """
        self.call("deassoc", address_name, data_index)

    def delete(self, object index):
        """Remove data and renumber

        delete(any index?)

        Removes the data at the address provided. If the specified address is
        numeric, all higher numbered addresses are decremented by 1.
        """
        self.call("delete", index)

    def dump(self):
        """Output all data

        Sends all of the stored addresses out the 2nd outlet and all of the
        stored messages out the 1st outlet, in the order in which they
        are stored. A `bang` is sent out the 4th outlet when the dump
        is completed.
        """
        self.call("dump")

    def end(self):
        """Move to last address

        Sets the pointer (as used by the `goto`, `next`, and `prev` messages)
        to the last address.
        """
        self.call("end")

    def filetype(self, str filetype = ""):
        """Set the recognized file types

        filetype(symbol filetype?)

        Sets the file types which can be read and written into the `coll`
        object. The message `filetype` with no arguments restores the
        default file behavior.
        """
        if not filetype:
            self.call("filetype")
        else:
            self.call("filetype", filetype)

    def embed(self, int save_setting, int unused = 0):
        """Set the file-save flag (renamed for consistency)

        flags(int save-setting?, int unused?)

        Sets the flags used to save its contents within the patch that
        contains it. The message `flags 1 0` notifies the object to
        save its contents as part of the patcher file. The message
        `flags 0 0` causes the contents not to be saved.
        """
        self.call("flags", save_setting, unused)

    def goto(self, object index):
        """Move to an index

        goto(list index?)

        Sets the pointer (as used by the `goto`, `next`, and `prev` messages)
        at a specific address, but does not trigger output. If the
        specified address does not exist, the pointer is set at the
        beginning of the collection. Data will be output in response
        to a subsequent `bang`, `next`, or `prev` message.
        """
        self.call("goto", index)

    def insert(self, int index, *args):
        """Insert data at a specific address

        insert(int index?, list data?)

        Inserts the message at the address specified by the number,
        incrementing all equal or greater addresses by 1 if necessary.
        """
        self.call("insert", args)

    def insert2(self, int index, *args):
        """Insert data at a specific address

        insert2(int index?, list data?)

        See the `insert` listing.
        """
        self.call("insert2", args)

    def length(self):
        """Retrieve the number of entries

        Counts the number of entries contained in the `coll` and sends the
        number out the 1st outlet.
        """
        self.call("length")

    def max(self, int element = 1):
        """Return the highest numeric value

        max(int element?)

        Gets the highest value in any entry. An optional integer argument
        (defaults to '1') specifies an element position to use.
        """
        self.call("max", element)

    def merge(self, int index, *args):
        """Merge data at an existing address

        merge(int index?, list data?)

        Appends data at the end of the data found at the specified index. If
        the address does not yet exist, it is created.
        """
        self.call("merge", index, args)

    def min(self, int element = 1):
        """Return the lowest numeric value

        min(int element?)

        Gets the lowest value in any entry. An optional integer argument
        (defaults to '1') specifies an element position to use.
        """
        self.call("min", element)

    def next(self):
        """Move to the next address

        Sends the address and data stored at the current address, then sets
        the pointer to the next address. If the pointer is currently
        at the last address in the collection, it wraps around to the
        first address. If the address is a symbol rather than a
        number, `0` is sent out the second outlet.
        """
        self.call("next")

    def nstore(self, int index, str association, *args):
        """Store data with both number and symbol index

        nstore(int index?, symbol association?, list data?)

        Stores the message at the specified number address, with the specified
        symbol associated. This has the same effect as storing the
        message at an int address, then using the `assoc` message to
        associate a symbol with that number.
        """
        self.call("nstore", index, association, *args)

    def nsub(self, int index, int position, object data):
        """Replace a single data element

        nsub(int index?, int position?, any data?)

        Replaces a data element with a new value. As an example, `nsub 2 4 7`
        replaces the fourth element of address 2 with the value 7.
        Number values and symbols can both be substituted in this
        manner.
        """
        self.call("nsub", index, position, data)

    def nth(self, int index, int position):
        """Return a single data element

        nth(int index?, int position?)

        Returns the data element found at a specific position in the stored
        list and send it out the first outlet. As an example, `nth 75
        2` will output the second item in the list stored at address
        75.
        """
        self.call("nth", index, position)

    def open(self):
        """Open a data editing window

        Opens a data editing window for the current data and bring it into
        focus.
        """
        self.call("end")

    def prev(self):
        """Move to the previous address

        Sends the address and data stored at the current address, then sets
        the pointer to the previous address. If the pointer is
        currently at the first address in the collection, it wraps
        around to the last address. If the address is a symbol rather
        than a number, `0` is sent out the second outlet.
        """
        self.call("prev")

    def read(self, str filename):
        """Choose a file to load

        read(symbol filename?)

        With no arguments, `read` puts up a standard Open Document dialog box
        to choose a file to load. If an argument is provided, the
        named file is loaded.
        """
        self.call("read", filename)

    def readagain(self):
        """Reload a file

        Loads the contents of the most recently read file. If no prior file
        load has occurred, the request is treated like a `read`
        message.
        """
        self.call("readagain")

    def refer(self, str object_name):
        """Change data reference

        refer(symbol object name?)

        Changes the reference to the data in another named `coll` object.
        Changes to the data stored in any referenced `coll` will be
        shared by all other objects with the same name.
        """
        self.call("refer", object_name)

    def remove(self, object index):
        """Remove an entry

        remove(any index?)

        Removes that address and its contents from the collection.
        """
        self.call("remove", index)

    def renumber(self, int data_index=0):
        """Renumber entries

        renumber(int data index?)

        Renumbers data entries as consecutive and in increasing order. The
        optional argument specifies the starting number address for
        the data.
        """
        self.call("renumber", data_index)

    def renumber2(self, int data_index):
        """Increment indices by one

        renumber2(int data index?)
        """
        self.call("renumber2", data_index)

    def separate(self, int data_index):
        """Creates an open entry index

        separate(int data index?)

        Increments the numerical indices for all data whose index is greater
        than the provided. This creates an open 'slot' for a
        subsequent add.
        """
        self.call("separate", data_index)

    def sort(self, int sort_order = -1, int entry = -1):
        """Sort the data

        sort(int sort order (-1 or 1)?, int entry (-1, 0, or 1)?)

        Sorts the data into a specified order. If the first argument is `-1`,
        the items are sorted in ascending order. If the first argument
        is `1`, the items are sorted in descending order.
        """
        self.call("sort", sort_order, entry)

    def start(self):
        """Move to the first entry

        Sets the pointer (used by the `goto`, `next`, and `prev` messages) to
        the first address in the `coll`.
        """
        self.call("start")

    def store(self, str index, *args):
        """Store data at a symbolic index

        store(symbol index?, list data?)

        Stores the message at an address named by the provided symbol. As an
        example, `store triad 0 4 7` will store `0 4 7` at an address
        named `triad`.
        """
        self.call("store", index, args)

    def sub(self, int index, int position, *args):
        """Replace a data element, output data

        sub(int index?, int position?, list data?)

        Same as `nsub`, except that the message stored at the specified
        address is sent out after the item has been substituted.
        """
        self.call("sub", index, position, args)

    def subsym(self, str new_name, str old_name):
        """Changes an index symbol

        subsym(symbol new name?, symbol old name?)

        Changes the symbol associated with data. The first argument is the new
        symbol to use, the second argument is the symbol associator to
        replace.
        """
        self.call("subsym", new_name, old_name)

    def swap(self, int index1, int index2):
        """Swap two indices

        swap(int index?, int index?)

        Exchanges the indices associated with two addresses. The data is
        unchanged, but the indexes that they use are swapped.
        """
        self.call("swap", index1, index2)

    def wclose(self):
        """Close the data editing window
        """
        self.call("wclose")

    def write(self, str filename):
        """Write the data to a disk file

        write(symbol filename?)

        With no arguments, `write` puts up a standard Open Document dialog box
        to choose a filename to write. If an argument is provided, the
        name is used as a filename for storage.
        """
        self.call("write", filename)

    def writeagain(self):
        """Rewrite a file

        Saves the contents to the most recently written file. If no prior file
        write has occurred, the request is treated like a `write`
        message.
        """
        self.call("writeagain")

# ----------------------------------------------------------------------------
# api.Array

cdef class Array(Object):
    """Create or duplicate an array object

    Create or duplicate a named array object.
    """

    def bang(self):
        """Trigger output

        Output the current array.
        """
        self.call("bang")

    def int(self, int value):
        """Convert an integer to an array

        int(int value?)

        Convert an integer to an array. The integer will be placed inside of
        an array, which will be sent to the outlet.
        """
        self.call("int", value)

    def float(self, float value):
        """Convert a floating-point number to an array

        float(float value?)

        Convert a floating-point number to an array. The floating-point number
        will be placed inside of an array, which will be sent to the
        outlet.
        """
        self.call("float", value)

    def list(self, *list_value):
        """Convert a list to an array

        list(list list-value?)

        Convert a list to an array. The contents of the list will be placed
        inside the array, which will be sent to the outlet.
        """
        self.call("list", *list_value)

    def anything(self, *list_value):
        """Convert a list to an array

        anything(list list-value?)

        Convert a list to an array. The contents of the list will be placed
        inside the array, which will be sent to the outlet.
        """
        self.call("anything", list_value)

    def append(self, *value):
        """Append a value to the end of the current array

        append(list value?)

        Append a value to the end of the current array. The array will not be
        output in response to this message. Use `bang` to force
        output.
        """
        self.call("append", value)

    def array(self, *args):
        """Make a copy of an array

        array(list ARG_NAME_0?)

        Make a copy of an array. The incoming array will be cloned and passed
        to the outlet.
        """
        self.call("array", args)

    def atoms(self):
        """Output the current array as a list

        Output the current array as a list out of the `array` objects middle
        outlet. The elements of the array will be output as a Max
        list.
        """
        self.call("atoms")

    def clear(self):
        """Clear the current array

        Clear the current array. The (now empty) array will not be output in
        response to this message. Use `bang` to force output.
        """
        self.call("clear")

    def delete(self, int index):
        """Delete an entry in the array

        delete(int index?)

        Delete an entry in the array. The indexed element will be removed from
        the array (indices are 0-based). The array will not be output
        in response to this message. Use `bang` to force output.
        """
        self.call("delete", index)

    def dictionary(self, *dictionary_value):
        """Wrap a dictionary in an array

        dictionary(list dictionary-value?)

        Wrap a dictionary in an array. The dictionary will be placed inside of
        an array, which will be sent to the outlet.
        """
        self.call("dictionary", dictionary_value)

    def get(self, int index):
        """Get an array element

        get(int index?)

        Get an array element. The element will be passed to the rightmost
        outlet in the form `get [index] [value]`.
        """
        self.call("get", index)

    def insert(self, int index, *value):
        """Insert a value into the current array

        insert(int index?, list value?)

        Insert a value into the current array. A new array element will be
        created at the index provided, with the supplied value. Any
        existing array elements will be shifted to make room for the
        new element. The array will not be output in response to this
        message. Use `bang` to force output.
        """
        self.call("insert", index, value)

    def prepend(self, *value):
        """Place a new entry at the start of the current array

        prepend(list value?)

        Place a new entry at the start of the current array. The array will
        not be output in response to this message. Use `bang` to force
        output.
        """
        self.call("prepend", value)

    def replace(self, int index, *value):
        """Replace a value in the current array

        replace(int index?, list value?)

        Replace a value in the current array at an existing index. The array
        will not be output in response to this message. Use `bang` to
        force output.
        """
        self.call("replace", index, value)

    def reserve(self, int number_of_entries):
        """Reserve memory for a provided number of entries (doesn't resize array)

        reserve(int number-of-entries?)

        Reserve memory for a provided number of entries (doesn't resize
        array). This is rarely needed, as the object manages its own
        memory and grows as necessary. If the desired array size is
        known, and re-allocation of the array needs to be avoided,
        this message can be used to ensure that the `array` object is
        pre-allocated to the desired size.
        """
        self.call("reserve", number_of_entries)

    def shrink(self):
        """Reduce memory usage to the current array object length

        Reduce memory usage to the current array object length. This is rarely
        needed. The `array` object does not automatically shrink if
        its contents are removed or cleared, this message can be used
        to ensure that the object doesn't use more resources than
        necessary.
        """
        self.call("shrink")

    def string(self, *string_value):
        """Wrap a string in an array

        string(list string-value?)

        Wrap a string in an array. The string will be placed inside of an
        array, which will be sent to the outlet.
        """
        self.call("string", string_value)

# ----------------------------------------------------------------------------
# numpy c-api import example

if INCLUDE_NUMPY:

    # @cython.boundscheck(False)
    def zadd(in1, in2):
        """Add two numpy arrays of complex numbers."""
        cdef double complex[:] a = in1.ravel()
        cdef double complex[:] b = in2.ravel()

        out = np.empty(a.shape[0], np.complex64)
        cdef double complex[:] c = out.ravel()

        for i in range(c.shape[0]):
            c[i].real = a[i].real + b[i].real
            c[i].imag = a[i].imag + b[i].imag

        return out

# ----------------------------------------------------------------------------
# c-level helper functions for the `py` external
#
# This section makes available c types, variables and functions defined here
# to the py.c file that is linked together with the cython-generated api.c file


cdef public mx.t_max_err py_hello(px.t_py* x, mx.t_symbol* s, long argc, mx.t_atom* argv):
    """A demo of a Max method in cython!

    This works exactly as a A_GIMME method
    """
    cdef mx.t_symbol* name
    if argc > 0:
        name = mx.atom_getsym(argv)
        if name:
            mx.post("hello %s: a method defined in api.pyx", name.s_name)
            return mx.MAX_ERR_NONE
    return mx.MAX_ERR_GENERIC


# ----------------------------------------------------------------------------
# python-level helper functions

# obligatory hello world demo

def hello():
    """Send 'Hello World' to the outlet."""
    ext = PyExternal()
    ext.out("Hello World")

# general helpers

def get_globals():
    """Get the global variables."""
    return list(globals().keys())

def bang():
    """Send a bang to the outlet."""
    ext = PyExternal()
    ext.bang()

def bang_success():
    """Send a success signal by banging out of right outlet."""
    ext = PyExternal()
    ext.bang_success()

def bang_failure():
    """Send a failure signal by banging out of middle outlet."""
    ext = PyExternal()
    ext.bang_failure()

def out(object obj):
    """Send an object to the outlet."""
    ext = PyExternal()
    ext.out(obj)

def out2(object obj):
    """Send an object to the outlet."""
    cdef px.t_py* x = <px.t_py*>px.py_get_object_ref()
    px.py_handle_output(x, <PyObject *>obj)

def send(name, *args):
    """Send a message to a receiver."""
    ext = PyExternal()
    ext.send(name, *args)

def lookup(name):
    """Lookup a variable name in the object registry."""
    ext = PyExternal()
    ext.lookup(name)

def post(str s):
    """Post a message to the console."""
    mx.post(s.encode())

def error(str s):
    """Post an error message to the console."""
    mx.error(s.encode())


# key directories

def resources_dir() -> str:
    """Return the path to the `Resources` dir in the external bundle"""
    cdef mx.t_string* path_str = px.py_get_path_to_external(px.py_class, "/Contents/Resources")
    cdef const char* path_cstr = mx.string_getptr(path_str)
    return path_cstr.decode()

def support_dir() -> str:
    """Return the path of the `support` dir in the package"""
    cdef mx.t_string* sdir_str = px.py_get_path_to_package(px.py_class, "/support")
    cdef const char* sdir_cstr = mx.string_getptr(sdir_str)
    return sdir_cstr.decode()


# get object helpers

def get_patcher() -> Patcher:
    """Get the containing patcher."""
    ext = PyExternal()
    patcher = ext.get_patcher()
    return patcher

def get_buffer(name: str) -> Buffer:
    """Retrieve a buffer by name."""
    ext = PyExternal()
    buf = ext.get_buffer(name)
    return buf

def get_max():
    """Get the Max object."""
    cdef mx.t_object *maxobj = <mx.t_object*>mx.object_new(
            mx.gensym("nobox"), mx.gensym("max"))
    if maxobj is NULL:
        error("could not get max object")


# buffer helpers

def create_buffer(name: str, sample_file: str) -> Buffer:
    """Create a buffer with a given name from a sample file."""
    ext = PyExternal()
    buf = ext.create_buffer(name, sample_file)
    return buf

def create_empty_buffer(name: str, duration_ms: int) -> Buffer:
    """Create an empty buffer with a given name and duration in milliseconds."""
    ext = PyExternal()
    buf = ext.create_empty_buffer(name, duration_ms)
    return buf


# patcher utils

def scan_objects() -> dict[str, MaxObject]:
    """populates a dict of all max objects in a patcher"""
    cdef mx.t_object *patcher = NULL
    cdef mx.t_object *box = NULL
    cdef mx.t_object *obj = NULL
    cdef px.t_py *x = <px.t_py*>px.py_get_object_ref()
    cdef int i = 0
    cdef dict objdict = {}
    cdef MaxObject mxo

    mx.object_obex_lookup(x, mx.gensym("#P"), &patcher)
    box = mx.jpatcher_get_firstobject(patcher)
    while box is not NULL:
        obj = mx.jbox_get_object(box)
        if obj:
            mxo = MaxObject.from_ptr(obj)
            if mxo.box.varname:
                oid = "{}-{}".format(mxo.box.varname, sym_to_str(mx.object_classname(obj)))
            else:
                oid = "{}-{}".format(i, sym_to_str(mx.object_classname(obj)))
            objdict[oid] = mxo
            post(oid)
            i += 1
        box = mx.jbox_get_nextobject(box)
    return objdict

