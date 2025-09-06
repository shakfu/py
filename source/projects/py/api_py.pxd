# api_py.pxd

from cpython.ref cimport PyObject
cimport api_max as mx

from libc.stdint cimport uintptr_t


cdef extern from "py.h":

    cdef int PY_MAX_ERROR
    cdef int PY_MAX_ELEMS
    cdef int PY_ATTRS_WITH_DEFAULTS
    cdef int PY_CHECK_REFS

    # Globals

    cdef mx.t_class* py_class                # global pointer to object class
    cdef int py_global_obj_count             # when 0 then free interpreter
    cdef mx.t_hashtab* py_global_registry    # global object lookups

    # Datastructures

    ctypedef struct t_py

    # Object creation and destruction Methods

    void* py_new(mx.t_symbol* s, long argc, mx.t_atom* argv)
    void py_free(t_py* x)
    void py_init(t_py* x)

    # Attribute Getters / Setters and Helpers

    mx.t_max_err py_pythonpath_attr_get(t_py *x, mx.t_object *attr, long *argc, mx.t_atom **argv)
    mx.t_max_err py_pythonpath_attr_set(t_py *x, mx.t_object *attr, long argc, mx.t_atom *argv)
    mx.t_max_err py_pythonpath_add(t_py* x, mx.t_symbol* path)
    mx.t_max_err py_get(t_py* x, mx.t_symbol* s)

    # Logging

    void py_info(t_py* x, char* fmt, ...)
    void py_debug(t_py* x, char* fmt, ...)
    void py_error(t_py* x, char* fmt, ...)
    void py_postargs(mx.t_symbol *s, long argc, mx.t_atom *argv)

    # Helpers

    void py_init_builtins(t_py* x)
    mx.t_max_err py_eval_text(t_py* x, long argc, mx.t_atom* argv)

    # api module helpers 

    mx.t_hashtab* py_get_global_registry()
    uintptr_t py_get_object_ref()

    # Path helpers

    mx.t_string* py_get_path_to_external(mx.t_class* c, char* subpath)
    mx.t_string* py_get_path_to_package(mx.t_class* c, char* subpath)
    mx.t_max_err py_locate_path_from_symbol(t_py* x, mx.t_symbol* s)
    void path_join(char* destination, const char* path1, const char* path2)

    # Side-effect helpers

    void py_bang(t_py* x)
    void py_bang_success(t_py* x)
    void py_bang_failure(t_py* x)
    void* get_outlet(t_py* x)

    # Common handlers

    void py_handle_error(t_py* x, char* fmt, ...)
    mx.t_max_err py_handle_float_output(t_py* x, PyObject* pval)
    mx.t_max_err py_handle_long_output(t_py* x, PyObject* pval)
    mx.t_max_err py_handle_string_output(t_py* x, PyObject* pval)
    mx.t_max_err py_handle_list_output(t_py* x, PyObject* pval)
    mx.t_max_err py_handle_dict_output(t_py* x, PyObject* pval)
    mx.t_max_err py_handle_output(t_py* x, PyObject* pval)

    # Core Python Methods

    mx.t_max_err py_import(t_py* x, mx.t_symbol* s)
    mx.t_max_err py_eval(t_py* x, mx.t_symbol* s, long argc, mx.t_atom* argv)
    mx.t_max_err py_exec(t_py* x, mx.t_symbol* s, long argc, mx.t_atom* argv)
    mx.t_max_err py_execfile(t_py* x, mx.t_symbol* s)

    # Extra Python Methods

    mx.t_max_err py_assign(t_py* x, mx.t_symbol* s, long argc, mx.t_atom* argv)
    mx.t_max_err py_call(t_py* x, mx.t_symbol* s, long argc, mx.t_atom* argv)
    mx.t_max_err py_code(t_py* x, mx.t_symbol* s, long argc, mx.t_atom* argv)
    mx.t_max_err py_pipe(t_py* x, mx.t_symbol* s, long argc, mx.t_atom* argv)
    mx.t_max_err py_fold(t_py* x, mx.t_symbol* s, long argc, mx.t_atom* argv)
    mx.t_max_err py_shell(t_py* x, mx.t_symbol* s, long argc, mx.t_atom* argv)
    mx.t_max_err py_anything(t_py* x, mx.t_symbol* s, long argc, mx.t_atom* argv)

    # Information Methods 

    void py_count(t_py* x)
    void py_metadata(t_py* x)
    void py_assist(t_py* x, void* b, long m, long a, char* s)

    # Time-based Methods

    mx.t_max_err py_task(t_py* x)
    mx.t_max_err py_sched(t_py* x, mx.t_symbol* s, long argc, mx.t_atom* argv)

    # Interobject Methods

    mx.t_max_err py_send(t_py* x, mx.t_symbol* s, long argc, mx.t_atom* argv)
    void py_scan(t_py* x)
    long py_scan_callback(t_py* x, mx.t_object* obj)

    # Code editor Methods

    void py_read(t_py* x, mx.t_symbol* s)
    void py_load(t_py* x, mx.t_symbol* s) # read(f) -> execfile(f)
    void py_doread(t_py* x, mx.t_symbol* s, long argc, mx.t_atom* argv)
    void py_dblclick(t_py* x)
    void py_run(t_py* x)
    void py_edclose(t_py* x, char** text, long size)
    mx.t_max_err py_edsave(t_py* x, char** text, long size)
    void py_okclose(t_py* x, char *s, short *result)

    # max datastructure support methods

    # dict
    void py_appendtodict(t_py* x, mx.t_dictionary* dict)


