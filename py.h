#ifndef PY_H
#define PY_H

/* py.h */

/*--------------------------------------------------------------------
 * Includes
 */

/* max api */
#include "ext.h"
#include "ext_obex.h"

/* python */
#define PY_SSIZE_T_CLEAN
#include <Python.h>

/* constants */
#define PY_MAX_ATOMS 128
#define PY_MAX_NAME  "PY_NAME"
//#define PY_NAMESPACE "PY_SPACE"
#define MAX_IMPORTS 3

/*--------------------------------------------------------------------
 * Global variables
 */

static int py_global_obj_count;

/*--------------------------------------------------------------------
 * Object Types
 */

/* [py] external type */
typedef struct _py {
    /* object header */
    t_object p_ob;

    /* object attributes */
    t_symbol *p_name;               /* unique name */

    /* default python file to 'exec' into the global namespace to be exposed in editor */
    t_symbol *p_code_filepath;      /* the path itself */

    /* infra objects */
    // t_patcher *p_patcher; /* to send msgs to objects */
    // t_box *p_box;         /* the ui box of the py instance? */
    // t_object *registry;   /* to keep a local (or global?) registry of objects? */

    /* text editor attrs */
    t_object *p_code_editor;
    char **p_code;
    long p_code_size;

    /* outlet creation */
    void *p_outlet;

    /* python-related */
    PyObject *p_globals;    /* global python namespace (new ref) */

} t_py;


/*--------------------------------------------------------------------
 * Enums
 */

/* python execution mode */
typedef enum {
    PY_EVAL,
    PY_EXEC,
    PY_EXECFILE
} py_mode;


/*--------------------------------------------------------------------
 * Methods
 */

/* python methods */
void py_import(t_py *x, t_symbol *s);
void py_eval(t_py *x, t_symbol *s, long argc, t_atom *argv);
void py_exec(t_py *x, t_symbol *s, long argc, t_atom *argv);
void py_execfile(t_py* x, t_symbol* s);
void py_load(t_py* x, t_symbol* s); // combo of read -> execfile

/* used for meta info */
void py_count(t_py *x);

/* used for testing */
void py_bang(t_py *x);
void py_sym(t_py* x, t_symbol* s);


/* code editor */
void py_read(t_py *x, t_symbol *s);
void py_doread(t_py *x, t_symbol *s, long argc, t_atom *argv);
void py_dblclick(t_py *x);
void py_edclose(t_py *x, char **text, long size);
void py_edsave(t_py *x, char **text, long size);

/* help */
void py_assist(t_py *x, void *b, long m, long a, char *s);

/* object creation and destruction */
void *py_new(t_symbol *s, long argc, t_atom *argv);
void py_free(t_py *x);




/*--------------------------------------------------------------------
 * Helper Functions
 */

void py_init(t_py *x);

#endif // PY_H