/* py.h */

#ifndef PY_H
#define PY_H


/**
 * @file py.h
 * 
 * @brief Python scripting external for Max/MSP
 * 
 *  This is the main header file for the `py` external. Note that the external
 *  structure is not directly exposed at the header level.
 * 
 * The py external provides Python scripting capabilities within Max/MSP. It allows:
 * - Running Python code directly in Max via messages
 * - Loading Python scripts from files
 * - Importing Python modules
 * - Bidirectional communication between Python and Max
 * - Access to Max objects and methods from Python code
 * - Support for Python packages via PYTHONPATH
 * 
 * The external manages Python interpreter initialization/finalization and provides
 * a sandboxed environment for each py object instance. It includes a built-in API
 * module for Max integration.
 */


/*--------------------------------------------------------------------------*/
/* Includes */

/* max api */
#include "ext.h"
#include "ext_obex.h"

/* optional */
#if defined(INCLUDE_COMMONSYMS)
#include "commonsyms.h"
#endif

/* python */
#define PY_SSIZE_T_CLEAN
#include <Python.h>

/* py default embedded module */
#include "py_prelude.h"

/*--------------------------------------------------------------------------*/
/* Constants */

#define PY_MAX_ERROR 4096
#define PY_MAX_ELEMS 1024

/*--------------------------------------------------------------------------*/
/* Compile-time Options */

#define PY_WITH_API 1
#define PY_CFG_ISOLATED 0
#define PY_ATTRS_WITH_DEFAULTS 0
#define PY_CHECK_REFS 0

/*--------------------------------------------------------------------------*/
/* Enums */

enum ARGUMENTS { A_NAME, NUM_ARGUMENTS };
enum INLETS { I_INPUT, NUM_INLETS };
enum OUTLETS { O_OUTPUT, O_FAILURE, O_SUCCESS, NUM_OUTLETS };

/*--------------------------------------------------------------------------*/
/* Globals */

t_class* py_class;                    // global pointer to object class
static int py_global_obj_count;       // when 0 then free interpreter
static t_hashtab* py_global_registry; // global object lookups

/*--------------------------------------------------------------------------*/
/* Datastructures */

typedef struct t_py t_py;

/*--------------------------------------------------------------------------*/
/* Object creation and destruction Methods */

void* py_new(t_symbol* s, long argc, t_atom* argv);
void py_free(t_py* x);
void py_init(t_py* x);

/*--------------------------------------------------------------------------*/
/* Attribute Getters / Setters and Helpers */

t_max_err py_pythonpath_attr_get(t_py *x, t_object *attr, long *argc, t_atom **argv);
t_max_err py_pythonpath_attr_set(t_py *x, t_object *attr, long argc, t_atom *argv);
t_max_err py_pythonpath_add(t_py* x, t_symbol* path);
t_max_err py_get(t_py* x, t_symbol* s);

/*--------------------------------------------------------------------------*/
/* Logging */

void py_info(t_py* x, char* fmt, ...);
void py_debug(t_py* x, char* fmt, ...);
void py_error(t_py* x, char* fmt, ...);
void py_postargs(t_symbol *s, long argc, t_atom *argv);

/*--------------------------------------------------------------------------*/
/* Helpers */

void py_init_builtins(t_py* x);
t_max_err py_eval_text(t_py* x, long argc, t_atom* argv);

/* api module helpers */

t_hashtab* py_get_global_registry(void);
uintptr_t py_get_object_ref(void);

/*--------------------------------------------------------------------------*/
/* Path helpers */

t_string* py_get_path_to_external(t_class* c, const char* subpath);
t_string* py_get_path_to_package(t_class* c, const char* subpath);
t_max_err py_locate_path_from_symbol(t_py* x, t_symbol* s);
void path_join(char* destination, const char* path1, const char* path2);

/*--------------------------------------------------------------------------*/
/* Side-effect helpers */

void py_bang(t_py* x);
void py_bang_success(t_py* x);
void py_bang_failure(t_py* x);
void* get_outlet(t_py* x);

/*--------------------------------------------------------------------------*/
/* Common handlers */

void py_handle_error(t_py* x, char* fmt, ...);
t_max_err py_handle_float_output(t_py* x, PyObject* pfloat);
t_max_err py_handle_long_output(t_py* x, PyObject* plong);
t_max_err py_handle_string_output(t_py* x, PyObject* pstring);
t_max_err py_handle_list_output(t_py* x, PyObject* plist);
t_max_err py_handle_dict_output(t_py* x, PyObject* pdict);
t_max_err py_handle_output(t_py* x, PyObject* pval);

/*--------------------------------------------------------------------------*/
/* Core Python Methods */

t_max_err py_import(t_py* x, t_symbol* s);
t_max_err py_eval(t_py* x, t_symbol* s, long argc, t_atom* argv);
t_max_err py_exec(t_py* x, t_symbol* s, long argc, t_atom* argv);
t_max_err py_execfile(t_py* x, t_symbol* s);

/*--------------------------------------------------------------------------*/
/* Extra Python Methods */

t_max_err py_apply(t_py* x, t_symbol* s, long argc, t_atom* argv);
t_max_err py_assign(t_py* x, t_symbol* s, long argc, t_atom* argv);
t_max_err py_call(t_py* x, t_symbol* s, long argc, t_atom* argv);
t_max_err py_code(t_py* x, t_symbol* s, long argc, t_atom* argv);
t_max_err py_pipe(t_py* x, t_symbol* s, long argc, t_atom* argv);
t_max_err py_product(t_py* x, t_symbol* s, long argc, t_atom* argv);
t_max_err py_fold(t_py* x, t_symbol* s, long argc, t_atom* argv);
t_max_err py_shell(t_py* x, t_symbol* s, long argc, t_atom* argv);
t_max_err py_anything(t_py* x, t_symbol* s, long argc, t_atom* argv);

/*--------------------------------------------------------------------------*/
/* Generic Python function wrappers */

t_max_err py_func_to_atoms(t_py* x, const char* pyfunc_name, t_symbol* s, long argc, t_atom* argv);
t_max_err py_func_to_list(t_py* x, const char* pyfunc_name, t_symbol* s, long argc, t_atom* argv);
t_max_err py_func_to_text(t_py* x, const char* pyfunc_name, t_symbol* s, long argc, t_atom* argv);
t_max_err py_func_to_pyobj(t_py* x, const char* pyfunc_name, PyObject* obj);

/*--------------------------------------------------------------------------*/
/* Information Methods */

void py_count(t_py* x);
void py_metadata(t_py* x);
void py_assist(t_py* x, void* b, long m, long a, char* s);

/*--------------------------------------------------------------------------*/
/* Time-based Methods */

t_max_err py_task(t_py* x);
t_max_err py_sched(t_py* x, t_symbol* s, long argc, t_atom* argv);

/*--------------------------------------------------------------------------*/
/* Interobject Methods */

t_max_err py_send(t_py* x, t_symbol* s, long argc, t_atom* argv);
void py_scan(t_py* x);
long py_scan_callback(t_py* x, t_object* box);

/*--------------------------------------------------------------------------*/
/* Code editor Methods */

void py_read(t_py* x, t_symbol* s);
void py_load(t_py* x, t_symbol* s); // read(f) -> execfile(f)
void py_doread(t_py* x, t_symbol* s, long argc, t_atom* argv);
void py_dblclick(t_py* x);
void py_run(t_py* x);
void py_edclose(t_py* x, char** text, long size);
t_max_err py_edsave(t_py* x, char** text, long size);
void py_okclose(t_py* x, char *s, short *result);

/*--------------------------------------------------------------------------*/
/* max datastructure support methods */

// dict
void py_appendtodict(t_py* x, t_dictionary* dict);


/*--------------------------------------------------------------------------*/
/* Python Compatibility Section */

/*
 * This section is for backwards compatibility code as the python c-api
 * evolves and some apis are deprecated and removed.
 * 
 * Python >= 3.7 is supported
 *
 */


// to enable tracebacks in py_handle_error
// for versions of python < 3.11
#if PY_VERSION_HEX < 0x030900B1
#include <frameobject.h>
static inline PyCodeObject* PyFrame_GetCode(PyFrameObject* frame)
{
    Py_INCREF(frame->f_code);
    return frame->f_code;
}
#endif

#if PY_VERSION_HEX < 0x030A00B1 && !defined(Py_IsNone)
#  define Py_Is(x, y) ((x) == (y))
#  define Py_IsNone(x) Py_Is(x, Py_None)
#endif


/*--------------------------------------------------------------------------*/
/* Util Macros */

#define _STR(x) #x
#define STR(x) _STR(x)
#define _CONCAT(a, b) a##b
#define CONCAT(a, b) _CONCAT(a, b)
#define _PY_VER CONCAT(PY_MAJOR_VERSION, CONCAT(., PY_MINOR_VERSION))
#define PY_VER STR(_PY_VER)
// PY_VERSION is already defined as Major.Minor.Patch by patchlevel.h

// utility macros to check refcounts of python objects
#if PY_CHECK_REFS == 1
#define PY_REF(p,v) \
    do { \
        int n = (int)Py_REFCNT((v)); \
        if ((n) > 1) \
            error("%s %s.%s: %d", (p), __func__, (#v), (n)); \
        else \
            post("%s %s.%s: %d", (p), __func__, (#v), (n)); \
    } while (0)
#define PY_REF_PRE(v) PY_REF("PRE",(v))
#define PY_REF_PST(v) PY_REF("PST",(v))
#define PY_REF_FIN(v) PY_REF("FIN",(v))
#define PY_REF_ERR(v) PY_REF("ERR",(v))
#else
#undef PY_REF
#undef PY_REF_PRE
#undef PY_REF_PST
#undef PY_REF_FIN
#undef PY_REF_ERR
#endif


#endif // PY_H
