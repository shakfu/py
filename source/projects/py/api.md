# apy.pyx

`api.pyx` is a cython 'builtin' module which wraps parts of the Max/MSP `c-api`
for the `py` external.

The `api` module consists of:
    - api.pyx: main [cython](https://cython.org) wrapper
    - a number of cython declaration files (`.pxd`) which expose c headers
        - `api_max.pxd`: exposes Max api headers
        - `api_msp.pxd`: exposes MSP api headers
        - `api_jit.pxd`: exposes Jitter api headers
        - `api_py.pxd`: exposes the `py` external's headers

Cython classes, functions, and constants defined here are optionally
available for use by python code running in a `py` external instance.

## Wrapper Classes

There are several class types:

1. Generic classes which are meant to be used as superclasses, wrapping `t_object*` pointers

    - `AbstractMaxObject`: abstract class for basic t_object* based object

    - `MaxObject`: Base wrapper for Max t_object* objects

    ```python

    # generic objet class usage

    # has signature

    o = api.Object(classname, name, *args, **kwds)

    # for example

    c = api.Object("coll", "mycol", embed=True)

    # Another variant is to be used as superclass

    # has signature

    o = api.AbstractObject(name, *args, **kwds)

    class Coll(AbstractObject):
        # ...

    ```

2. Specialized classes with their own c-api methods and type-specific pointer types. These are documented in the Max-api docs:

    - `Atom`: Wrapper for Max mx.t_atom* atoms/messages
    - `Table`: Interface to Max tables
    - `Buffer`: Interface to MSP buffers
    - `Dictionary`: Interface to Max dictionaries
    - `DatabaseView`: Interface to Max database views
    - `DatabaseResult`: Interface to Max database results
    - `Database`: Interface to Max databases
    - `List`: Interface to Max linked lists
    - `Binbuf`: Interface to Max binbufs
    - `Atombuf`: Interface to Max atom buffers
    - `Hashtab`: Interface to Max hash tables
    - `AtomArray`: Interface to Max atom arrays
    - `Patcher`: Interface to Max patchers
    - `Box`: Interface to Max boxes/objects
    - `Matrix`: Interface to Max jit matrices
    - `Path`: Interface to Max path handling
    - `MaxApp`: Interface to the Max application

3. High-level wrapper: these can only be invoked in the max c-apu using generic object methods and subclass (1) generic classes. An example of this is the `api.Coll` class

4. External wrappers: these wrap the `py` external itself:

    - `PyExternal`: Main interface for the `py` external
    - `PyMxObject`: Alternative external extension type (obj pointer retrieved via uintptr_t)

## Features

- Provides access to max c-api functionality in an interpreted language.

- Wraps a decent subset of the max c-api for python code running in a `py` external instance.

- Write max methods in cython which can be called in c

- All c is exposed via cython to python

- Wrap max types as cython extension classes, e.g. `api.Atom`, `api.Patcher`, ..

- Errors in python don't crash Max (or crash max less!)

## Design Notes

- [ ] to refactor the object_method_typed calls which are being duplicated in `Patcher`, `MaxObect`, `MaxApp`, etc..

- [ ] There should be sufficient number of wrapper classes (which wrap pointers)  to enable productive scripting. So instead of returning a `mx.t_object*` one returns a `MaxObject`

## Usage

Either

1. Send the `py` object an `import api` message or

2. Import a module on the pythonpath which imports the `api` module or

3. Send a `(load <file.py)` message to load a file on the Max search path which contains python code that imports the `api` module. Then;

4. Call one of the functions or classes in this file making sure to prefix it with `api.`

```text
        1.
   ( import api )
        |
        |                      2.
      [ py ] ------ ( api.post('hello world') )
```

## Development

A lot of the painful laborious work of creating header mappings has been
done and can be reviewed (and corrected) in
the `api_max.pxd` and `api_msp.pxd` files.

This provides the benefit that we can import the max api via its header
declarations as follows:

```python
    cimport api_max as mx
```

Then you can start using Max api symbols or functions in the cython code
by prefixing with `mx.` For example

```text
    gensym()    -> mx.gensym()
    post()      -> mx.post()
    ...
```

In addition the `py` external api is also mapped for use by cython
(see below) in api_py.pxd file:

```python
    cimport api_py as px
```

Again please note any function exposed from the `py` external must
be prefixed as `px`:

```text
    py_scan()   -> px.py_scan()
```

This separation of namespaces is clearly very useful when you are
wrapping code.

## Extension Types

We use cython extension types to wrap related C data structures and functions
in the Max api. This them accessible to python code and gives them a python-friendly interface.

So far the following extension types are implemented (partial or otherwise)

- [x] MaxObject
- [x] Atom
- [x] Table
- [x] Buffer
- [x] Dictionary
- [x] Database
- [x] DatabaseResult
- [x] DatabaseView
- [x] Linklist
- [x] Binbuf
- [x] Atombuf
- [x] Hashtab
- [x] AtomArray
- [x] Patcher
- [x] Box
- [x] Buffer
- [x] PyExternal
- [x] MaxApp

Workarounds for max types which are not exposed in the c-api:

- `coll`: import and export the contents of a `coll` into a `dict` by
  sending a message to the `dict` object.

- `jit.cellblock`: link a `coll` to a `cellblock` and data sent to the
  `coll` will be sent to the `cellblock`.

- `jit.matrix`: can be populated via `jit.fill` from a `coll`


## Matrix Methods

Note the following

> These and other jit_matrix methods are not exported. They are listed
> in the HTML reference as prototypes to inform how you would call
> `jit_object_method()` for these `A_CANT` methods. as mentioned in the API
> listing with the text "Warning: This function is not exported, but is
> provided for reference when calling via `jit_object_method` on an
> intance of `t_jit_matrix`. 
> 
> If you search on `getinfo` or `setinfo` in the SDK folder you'll see
> examples of how this is used. [jkclayton](https://cycling74.com/forums/jit_matrix_setinfo-not-available-on-windows#reply-58ed1f5043f50b22d4ba9e7d)


```c
// Frees instance of t_jit_linklist.
void jit_linklist_free (t_jit_linklist *x)

// Constructs instance of t_jit_matrix.
void *jit_matrix_new (t_jit_matrix_info *info)

// Constructs instance of t_jit_matrix, copying from input.
void *jit_matrix_newcopy (t_jit_matrix *copyme)

// Frees instance of t_jit_matrix.
t_jit_err jit_matrix_free (t_jit_matrix *x)

// Sets all attributes according to the t_jit_matrix_info struct provided.
t_jit_err jit_matrix_setinfo (t_jit_matrix *x, t_jit_matrix_info *info)

// Sets all attributes according to the t_jit_matrix_info struct provided (including data flags).
t_jit_err jit_matrix_setinfo_ex (t_jit_matrix *x, t_jit_matrix_info *info)

// Retrieves all attributes, copying into the t_jit_matrix_info struct provided.
t_jit_err jit_matrix_getinfo (t_jit_matrix *x, t_jit_matrix_info *info)

// Retrieves matrix data pointer.
t_jit_err jit_matrix_getdata (t_jit_matrix *x, void *data)

// Sets matrix data pointer.
t_jit_err jit_matrix_data (t_jit_matrix *x, void *data)

// Frees matrix's internal data pointer if an internal reference and sets to NULL.
t_jit_err jit_matrix_freedata (t_jit_matrix *x)

// Initializes matrix info struct to default values.
t_jit_err jit_matrix_info_default (t_jit_matrix_info *info)

// Sets all cells in matrix to the zero.
t_jit_err jit_matrix_clear (t_jit_matrix *x)

// Sets cell at index to the value provided.
t_jit_err jit_matrix_setcell1d (t_jit_matrix *x, t_symbol *s, long argc, t_atom *argv)

// Sets cell at index to the value provided.
t_jit_err jit_matrix_setcell2d (t_jit_matrix *x, t_symbol *s, long argc, t_atom *argv)

// Sets cell at index to the value provided.
t_jit_err jit_matrix_setcell3d (t_jit_matrix *x, t_symbol *s, long argc, t_atom *argv)

// Sets plane of cell at index to the value provided.
t_jit_err jit_matrix_setplane1d (t_jit_matrix *x, t_symbol *s, long argc, t_atom *argv)

// Sets plane of cell at index to the value provided.
t_jit_err jit_matrix_setplane2d (t_jit_matrix *x, t_symbol *s, long argc, t_atom *argv)

// Sets plane of cell at index to the value provided.
t_jit_err jit_matrix_setplane3d (t_jit_matrix *x, t_symbol *s, long argc, t_atom *argv)

// Sets cell at index to the value provided.
t_jit_err jit_matrix_setcell (t_jit_matrix *x, t_symbol *s, long argc, t_atom *argv)

Gets cell at index to the value provided.
t_jit_err jit_matrix_getcell (t_jit_matrix *x, t_symbol *s, long argc, t_atom *argv, long *rac, t_atom *rav)

// Sets all cells to the value provided.
t_jit_err jit_matrix_setall (t_jit_matrix *x, t_symbol *s, long argc, t_atom *argv)

// Sets the plane specified in all cells to the value provided.
t_jit_err jit_matrix_fillplane (t_jit_matrix *x, t_symbol *s, long argc, t_atom *argv)

// Copies Jitter matrix data from another matrix.
t_jit_err jit_matrix_frommatrix (t_jit_matrix *dst_matrix, t_jit_matrix *src_matrix, t_matrix_conv_info *mcinfo)

// Applies unary or binary operator to matrix See Jitter user documentation for more information.
t_jit_err jit_matrix_op (t_jit_matrix *x, t_symbol *s, long argc, t_atom *argv)

// Fills cells according to the jit.expr expression provided.
t_jit_err jit_matrix_exprfill (t_jit_matrix *x, t_symbol *s, long argc, t_atom *argv)

// Copies texture information to matrix.
t_jit_err jit_matrix_jit_gl_texture (t_jit_matrix *x, t_symbol *s, long argc, t_atom *argv)
```


```python
# exposed jit syms in jit.symbols.h

jit_syms = [
    '_jit_sym_adapt',
    '_jit_sym_add',
    '_jit_sym_anim',
    '_jit_sym_append',
    '_jit_sym_atom',
    '_jit_sym_attach',
    '_jit_sym_block',
    '_jit_sym_boundcalc',
    '_jit_sym_bounds',
    '_jit_sym_calcbounds',
    '_jit_sym_cell',
    '_jit_sym_char',
    '_jit_sym_chuck',
    '_jit_sym_chuckindex',
    '_jit_sym_class_jit_attribute',
    '_jit_sym_class_jit_matrix',
    '_jit_sym_class_jit_namespace',
    '_jit_sym_classname',
    '_jit_sym_clear',
    '_jit_sym_clear_custom',
    '_jit_sym_data',
    '_jit_sym_decorator',
    '_jit_sym_deleteindex',
    '_jit_sym_detach',
    '_jit_sym_dim',
    '_jit_sym_dimlink',
    '_jit_sym_direction',
    '_jit_sym_err_calculate',
    '_jit_sym_err_lockout_stack',
    '_jit_sym_filter',
    '_jit_sym_findall',
    '_jit_sym_findfirst',
    '_jit_sym_findsize',
    '_jit_sym_float32',
    '_jit_sym_float64',
    '_jit_sym_free',
    '_jit_sym_fromgworld',
    '_jit_sym_frommatrix',
    '_jit_sym_frommatrix_trunc',
    '_jit_sym_genframe',
    '_jit_sym_get',
    '_jit_sym_getdata',
    '_jit_sym_getindex',
    '_jit_sym_getinfo',
    '_jit_sym_getinput',
    '_jit_sym_getinputlist',
    '_jit_sym_getioproc',
    '_jit_sym_getmatrix',
    '_jit_sym_getmethod',
    '_jit_sym_getname',
    '_jit_sym_getoutput',
    '_jit_sym_getoutputlist',
    '_jit_sym_getsize',
    '_jit_sym_getspecial',
    '_jit_sym_gettype',
    '_jit_sym_gl_line_loop',
    '_jit_sym_gl_line_strip',
    '_jit_sym_gl_lines',
    '_jit_sym_gl_point_sprite',
    '_jit_sym_gl_points',
    '_jit_sym_gl_polygon',
    '_jit_sym_gl_quad_grid',
    '_jit_sym_gl_quad_strip',
    '_jit_sym_gl_quads',
    '_jit_sym_gl_tri_fan',
    '_jit_sym_gl_tri_grid',
    '_jit_sym_gl_tri_strip',
    '_jit_sym_gl_triangles',
    '_jit_sym_inputcount',
    '_jit_sym_insertindex',
    '_jit_sym_ioname',
    '_jit_sym_ioproc',
    '_jit_sym_jit_attr_offset',
    '_jit_sym_jit_attr_offset_array',
    '_jit_sym_jit_attribute',
    '_jit_sym_jit_linklist',
    '_jit_sym_jit_matrix',
    '_jit_sym_jit_mop',
    '_jit_sym_jit_namespace',
    '_jit_sym_list',
    '_jit_sym_lock',
    '_jit_sym_long',
    '_jit_sym_lookat',
    '_jit_sym_makearray',
    '_jit_sym_matrix',
    '_jit_sym_matrix_calc',
    '_jit_sym_matrixname',
    '_jit_sym_max_jit_classex',
    '_jit_sym_maxdim',
    '_jit_sym_maxdimcount',
    '_jit_sym_maxplanecount',
    '_jit_sym_methodall',
    '_jit_sym_methodindex',
    '_jit_sym_mindim',
    '_jit_sym_mindimcount',
    '_jit_sym_minplanecount',
    '_jit_sym_modified',
    '_jit_sym_name',
    '_jit_sym_new',
    '_jit_sym_newcopy',
    '_jit_sym_nothing',
    '_jit_sym_notifyall',
    '_jit_sym_ob_sym',
    '_jit_sym_object',
    '_jit_sym_objptr2index',
    '_jit_sym_outputcount',
    '_jit_sym_outputmatrix',
    '_jit_sym_outputmode',
    '_jit_sym_plane',
    '_jit_sym_planecount',
    '_jit_sym_planelink',
    '_jit_sym_pointer',
    '_jit_sym_position',
    '_jit_sym_quat',
    '_jit_sym_rebuilding',
    '_jit_sym_register',
    '_jit_sym_replace',
    '_jit_sym_resolve_name',
    '_jit_sym_resolve_raw',
    '_jit_sym_restrict_dim',
    '_jit_sym_restrict_planecount',
    '_jit_sym_restrict_type',
    '_jit_sym_reverse',
    '_jit_sym_rotate',
    '_jit_sym_rotatexyz',
    '_jit_sym_scale',
    '_jit_sym_set',
    '_jit_sym_setall',
    '_jit_sym_setinfo',
    '_jit_sym_setinfo_ex',
    '_jit_sym_shuffle',
    '_jit_sym_sort',
    '_jit_sym_special',
    '_jit_sym_swap',
    '_jit_sym_symbol',
    '_jit_sym_togworld',
    '_jit_sym_type',
    '_jit_sym_typelink',
    '_jit_sym_types',
    '_jit_sym_unblock',
    '_jit_sym_val',
]

```