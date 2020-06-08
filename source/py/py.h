#ifndef PY_H
#define PY_H

/* py.h */

/*--------------------------------------------------------------------------*/
// INCLUDES

/* max api */
#include "ext.h"
#include "ext_obex.h"

/* python */
#define PY_SSIZE_T_CLEAN
#include <Python.h>
/*--------------------------------------------------------------------------*/
// CONSTANTS

#define PY_MAX_ATOMS 128
#define MAX_IMPORTS 3
/*--------------------------------------------------------------------------*/
// GLOBALS

t_class* py_class; // global pointer to object class
static int py_global_obj_count; // when 0 then free interpreter
static t_hashtab* py_global_registry; // global object lookups
/*--------------------------------------------------------------------------*/
// OBJECT TYPES

typedef struct _py {
    /* object header */
    t_object p_ob;

    /* object attributes */
    t_symbol* p_name; /* unique object name (not scripting name) */

    /* python-related */
    t_symbol* p_pythonpath; /* path to python directory */
    t_bool p_debug;         /* bool to switch per-object debug state */
    // int p_debug;
    PyObject* p_globals; /* global python namespace (new ref) */

    /* infra objects */
    t_patcher* p_patcher; /* to send msgs to objects */
    t_box* p_box;         /* the ui box of the py instance? */

    /* text editor attrs */
    t_object* p_code_editor;
    char** p_code;
    long p_code_size;
    t_symbol* p_code_filepath; /* default python filepath to load into
                                  the code editor and global namespace */
    /* outlet creation */
    void* p_outlet_right;  // right outlet to bang success
    void* p_outlet_middle; // middle outleet to bang error
    void* p_outlet_left;   // left outleet for msg output

} t_py;

/*--------------------------------------------------------------------------*/
// FUNCTION TYPES
/*--------------------------------------------------------------------------*/
// ENUMS

/* python execution mode */
typedef enum { PY_EVAL, PY_EXEC, PY_EXECFILE } py_mode;
/*--------------------------------------------------------------------------*/
// MACROS

#define foreach(i, n)                                                         \
    int i;                                                                    \
    for (i = 0; i < n; i++)
/*--------------------------------------------------------------------------*/
// METHODS

/* object creation and destruction */
void* py_new(t_symbol* s, long argc, t_atom* argv);
void py_free(t_py* x);
void py_init(t_py* x);

/* helpers */
void py_log(t_py* x, char* fmt, ...);
void py_error(t_py* x, char* fmt, ...);

/* common handlers */
void py_handle_error(t_py* x, char* fmt, ...);
void py_handle_output(t_py* x, PyObject* pval);

/* core python methods */
void py_import(t_py* x, t_symbol* s);
void py_eval(t_py* x, t_symbol* s, long argc, t_atom* argv);
void py_eval2(t_py* x, t_symbol* s, long argc, t_atom* argv);
void py_exec(t_py* x, t_symbol* s, long argc, t_atom* argv);
void py_exec2(t_py* x, t_symbol* s, long argc, t_atom* argv);
void py_execfile(t_py* x, t_symbol* s);

/* extra python methods */
void py_assign(t_py* x, t_symbol* s, long argc, t_atom* argv);
void py_call(t_py* x, t_symbol* s, long argc, t_atom* argv);
void py_code(t_py* x, t_symbol* s, long argc, t_atom* argv);
void py_globex(t_py* x, long n);

/* informational */
void py_count(t_py* x);
void py_assist(t_py* x, void* b, long m, long a, char* s);

/* testing */
void py_bang(t_py* x);

/* interobject communications */
void py_scan(t_py* x);
long py_scan_callback(t_py* x, t_object* obj);
void py_send(t_py* x, t_symbol* s, long argc, t_atom* argv);
void py_lookup(t_py* x, t_symbol* s);

/* code editor */
void py_read(t_py* x, t_symbol* s);
void py_doread(t_py* x, t_symbol* s, long argc, t_atom* argv);
void py_dblclick(t_py* x);
void py_edclose(t_py* x, char** text, long size);
void py_edsave(t_py* x, char** text, long size);
void py_locatefile(t_py* x, char* filename);
void py_load(t_py* x, t_symbol* s); // combo of read -> execfile



#endif // PY_H
