/** \file py.h
    \brief A single-header python3 library for Max externals.

    This library is supposed to enable nested py objects in a given
    Max external. This library is in the public domain.

    The latest code can be found in [py-js](https://github.com/shakfu/py-js)

    If PY_IMPLEMENTATION isdefined before including the header,
    it will activate the implementation, otherwise the implementation
    will not be included.

    Usage example:

        #define PY_IMPLEMENTATION // <-- activate the implementation
        #include "py.h"

        typedef struct myobj {
            t_object obj;
            t_py* py; // <-- this is the key opaque type and instance
            void* outlet;
        } t_myobj;

*/
// ---------------------------------------------------------------------------------------
// HEADER

#ifndef PY_H
#define PY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ext.h"
#include "ext_obex.h"

#define PY_SSIZE_T_CLEAN
#include <Python.h>


// data structure declaration
typedef struct t_py t_py;

// init / free methods
t_py* py_init(t_class* c);
void py_free(t_py* x);

// core methods
t_max_err py_import(t_py* x, t_symbol* s);
t_max_err py_eval(t_py* x, t_symbol* s, void* outlet);
t_max_err py_exec(t_py* x, t_symbol* s);
t_max_err py_execfile(t_py* x, t_symbol* s);

// extra methods
t_max_err py_call(t_py* x, t_symbol* s, long argc, t_atom* argv, void* outlet);
t_max_err py_assign(t_py* x, t_symbol* s, long argc, t_atom* argv);
t_max_err py_code(t_py* x, t_symbol* s, long argc, t_atom* argv, void* outlet);
t_max_err py_anything(t_py* x, t_symbol* s, long argc, t_atom* argv,void* outlet);
t_max_err py_pipe(t_py* x, t_symbol* s, long argc, t_atom* argv, void* outlet);

// code execution methods
t_max_err py_exec_file_input(t_py* x, const char* code);
t_max_err py_exec_single_input(t_py* x, const char* code);

// code execution methods with output capture
char* py_exec_file_input_with_output(t_py* x, const char* code);
char* py_exec_single_input_with_output(t_py* x, const char* code);

// get runtime python version
#define _STR(x) #x
#define STR(x) _STR(x)
#define PY_VER STR(PY_MAJOR_VERSION) "." STR(PY_MINOR_VERSION)

#ifdef __cplusplus
}
#endif
#endif /* PY_H */

// ---------------------------------------------------------------------------------------
// END HEADER


// ---------------------------------------------------------------------------------------
// IMPLEMENTATION

#ifdef PY_IMPLEMENTATION

/*
    py.h -- single-header library providing minimal python3 services for Max
   externals.
*/

#define PY_MAX_ELEMS 1024
#define PY_DEBUG 1
#define PY_MAX_CODE_SIZE 1048576  // 1MB maximum code size for exec/eval

// Sandboxing configuration
// Set PY_ENABLE_SANDBOX to 0 to disable Python code sandboxing (not recommended)
// Sandboxing restricts dangerous builtins and limits module imports
#ifndef PY_ENABLE_SANDBOX
#define PY_ENABLE_SANDBOX 1  // Enabled by default for security
#endif

// ---------------------------------------------------------------------------------------
// Datastructure
// clang-format off

struct t_py
{
    t_symbol* p_name;                       /*!< unique python object name */
    t_symbol* p_pythonpath;                 /*!< path to python directory */
    t_bool p_debug;                         /*!< bool to switch per-object debug state */
    t_fourcc p_code_filetype;               /*!< filetype four char code of 'TEXT' */
    t_fourcc p_code_outtype;                /*!< filetype four char code of 'TEXT' */
    char p_code_filename[MAX_PATH_CHARS];   /*!< file name field */
    char p_code_pathname[MAX_PATH_CHARS];   /*!< file path field */
    short p_code_path;                      /*!< short code for max file system */
    t_symbol* p_code_filepath;              /*!< filepath to python file to execfile */
    PyObject* p_globals;                    /*!< per object 'globals' python namespace */
};

// clang-format on
// ---------------------------------------------------------------------------------------
// Forward Declarations not in header

void py_log(t_py* x, char* fmt, ...);
void py_error(t_py* x, char* fmt, ...);
void py_handle_error(t_py* x, char* fmt, ...);

t_string* py_get_path_to_package(t_class* c, char* subpath);
t_string* py_get_path_to_external(t_class* c, char* subpath);
t_max_err py_locate_path_from_symbol(t_py* x, t_symbol* s);

t_max_err py_handle_float_output(t_py* x, void* outlet, PyObject* pval);
t_max_err py_handle_long_output(t_py* x, void* outlet, PyObject* pval);
t_max_err py_handle_string_output(t_py* x, void* outlet, PyObject* pval);
t_max_err py_handle_list_output(t_py* x, void* outlet, PyObject* pval);
t_max_err py_handle_dict_output(t_py* x, void* outlet, PyObject* pval);
t_max_err py_handle_output(t_py* x, void* outlet, PyObject* pval);
t_max_err py_eval_text(t_py* x, long argc, t_atom* argv, int offset, void* outlet);

PyObject* py_atoms_to_list(t_py* x, long argc, t_atom* argv, int start_from);


// ---------------------------------------------------------------------------------------
// HELPERS

/**
 * @brief Post msg to Max console.
 *
 * @param x pointer to object struct
 * @param fmt character string with format codes
 * @param ... other arguments
 *
 * This log function is a variadic function which does not `post` its message
 * if the object struct member `x->p_debug` is 0.
 *
 * SECURITY: Messages exceeding PY_MAX_ELEMS are truncated with warning.
 */
void py_log(t_py* x, char* fmt, ...)
{
    if (x->p_debug) {
        char msg[PY_MAX_ELEMS];

        va_list va;
        va_start(va, fmt);
        int written = vsnprintf(msg, PY_MAX_ELEMS, fmt, va);
        va_end(va);

        // Check for buffer truncation
        if (written >= PY_MAX_ELEMS) {
            error("[py %s]: Log message truncated (%d bytes, max %d)",
                  x->p_name->s_name, written, PY_MAX_ELEMS - 1);
            return;
        }

        post("[py %s]: %s", x->p_name->s_name, msg);
    }
}

/**
 * @brief Post error message to Max console.
 *
 * @param x pointer to object struct
 * @param fmt character string with format codes
 * @param ... other arguments
 *
 * SECURITY: Messages exceeding PY_MAX_ELEMS are truncated safely.
 */
void py_error(t_py* x, char* fmt, ...)
{
    char msg[PY_MAX_ELEMS];

    va_list va;
    va_start(va, fmt);
    int written = vsnprintf(msg, PY_MAX_ELEMS, fmt, va);
    va_end(va);

    // Check for buffer truncation
    if (written >= PY_MAX_ELEMS) {
        error("[py %s]: Error message truncated (%d bytes, max %d)",
              x->p_name->s_name, written, PY_MAX_ELEMS - 1);
        // Still show the truncated message
        error("[py %s]: %s [TRUNCATED]", x->p_name->s_name, msg);
        return;
    }

    error("[py %s]: %s", x->p_name->s_name, msg);
}

// ---------------------------------------------------------------------------------------
// SANDBOXING

#if PY_ENABLE_SANDBOX

/**
 * @brief Module import whitelist for sandboxed environment
 * Only these modules can be imported when sandboxing is enabled
 */
static const char* py_allowed_modules[] = {
    // Safe standard library modules
    "math",
    "random",
    "collections",
    "itertools",
    "functools",
    "operator",
    "string",
    "re",
    "datetime",
    "time",
    "json",
    "decimal",
    "fractions",
    // Required for internal output capture
    "sys",       // Needed for stdout/stderr redirection
    "io",        // Needed for StringIO
    // Max-specific modules (if needed)
    NULL  // Sentinel
};

/**
 * @brief Check if a module is in the import whitelist
 *
 * @param module_name The name of the module to check
 * @return 1 if allowed, 0 if not allowed
 */
int py_is_module_allowed(const char* module_name)
{
    if (module_name == NULL) {
        return 0;
    }

    for (int i = 0; py_allowed_modules[i] != NULL; i++) {
        if (strcmp(module_name, py_allowed_modules[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Custom __import__ function that enforces whitelist
 */
static PyObject* py_restricted_import(PyObject* self, PyObject* args, PyObject* kwargs)
{
    const char* name = NULL;

    // Extract module name from arguments
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return NULL;
    }

    // Check whitelist
    if (!py_is_module_allowed(name)) {
        PyErr_Format(PyExc_ImportError,
                     "Module '%s' is not allowed in sandboxed environment", name);
        return NULL;
    }

    // Use the original __import__ for allowed modules
    PyObject* builtins = PyEval_GetBuiltins();
    PyObject* import_func = PyDict_GetItemString(builtins, "__import__");

    if (import_func == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Cannot find __import__");
        return NULL;
    }

    return PyObject_Call(import_func, args, kwargs);
}

/**
 * @brief Set up restricted builtins for sandboxed environment
 *
 * @param globals The globals dictionary to restrict
 * @return 0 on success, -1 on error
 */
int py_setup_sandbox(PyObject* globals)
{
    if (globals == NULL) {
        return -1;
    }

    PyObject* builtins = PyEval_GetBuiltins();
    if (builtins == NULL) {
        return -1;
    }

    // Create a copy of builtins to modify
    PyObject* restricted_builtins = PyDict_Copy(builtins);
    if (restricted_builtins == NULL) {
        return -1;
    }

    // Remove dangerous builtins
    const char* dangerous_builtins[] = {
        "eval",       // Allows arbitrary code execution
        "exec",       // Allows arbitrary code execution
        "compile",    // Can bypass restrictions
        "open",       // File system access
        "__import__", // We'll replace with restricted version
        "input",      // User input can be dangerous
        "help",       // Can reveal system info
        "breakpoint", // Debugging
        "exit",       // Process control
        "quit",       // Process control
        "copyright",  // System info
        "credits",    // System info
        "license",    // System info
        NULL
    };

    for (int i = 0; dangerous_builtins[i] != NULL; i++) {
        if (PyDict_DelItemString(restricted_builtins, dangerous_builtins[i]) < 0) {
            // Ignore if item doesn't exist
            PyErr_Clear();
        }
    }

    // Add restricted __import__
    static PyMethodDef import_method = {
        "__import__",
        (PyCFunction)py_restricted_import,
        METH_VARARGS | METH_KEYWORDS,
        "Restricted import"
    };

    PyObject* import_func = PyCFunction_New(&import_method, NULL);

    if (import_func != NULL) {
        PyDict_SetItemString(restricted_builtins, "__import__", import_func);
        Py_DECREF(import_func);
    }

    // Set the restricted builtins in the globals
    PyDict_SetItemString(globals, "__builtins__", restricted_builtins);
    Py_DECREF(restricted_builtins);

    return 0;
}

#endif  // PY_ENABLE_SANDBOX

// ---------------------------------------------------------------------------------------
// OBJECT INIT / FREE

/**
 * @brief Initializes and allocate t_py object instance
 *
 * @return t_py*  pointer to object structure
 */
t_py* py_init(t_class* c)
{
    wchar_t* python_home = NULL;

    if (c) { // special-case pythonhome config, only makes sense if c not NULL

#if defined(__APPLE__) && defined(BUILD_STATIC)
    const char* resources_path = string_getptr(
        py_get_path_to_external(c, "/Contents/Resources"));
    python_home = Py_DecodeLocale(resources_path, NULL);
#endif

#if defined(__APPLE__) && defined(BUILD_SHARED_PKG)
    const char* package_path = string_getptr(
        py_get_path_to_package(c, "/support/python" PY_VER));
    python_home = Py_DecodeLocale(package_path, NULL);
#endif

    } // end special-case python-home config

    t_py* x = (t_py*)malloc(sizeof(struct t_py));
    if (x == NULL) {
        error("py_init: malloc failed - out of memory");
        if (python_home != NULL) {
            PyMem_RawFree(python_home);
        }
        return NULL;
    }

    x->p_name = symbol_unique();
    x->p_pythonpath = gensym("");
    x->p_debug = PY_DEBUG;

    x->p_code_filetype = FOUR_CHAR_CODE('TEXT');
    x->p_code_outtype = 0;
    x->p_code_filename[0] = 0;
    x->p_code_pathname[0] = 0;
    x->p_code_path = 0;
    x->p_code_filepath = gensym("");

#if PY_VERSION_HEX < 0x0308000
    if (python_home != NULL) {
        Py_SetPythonHome(python_home);
        PyMem_RawFree(python_home);
    }
    Py_Initialize();
#else
    PyConfig config;
    PyConfig_InitPythonConfig(&config);
    config.parse_argv = 0; // Disable parsing command line arguments
    config.isolated = 0;   // default is disabled
    config.home = python_home;

    PyStatus status = Py_InitializeFromConfig(&config);
    if (PyStatus_Exception(status)) {
        PyConfig_Clear(&config);
        py_error(x, "could not initialize python");
    }
    PyConfig_Clear(&config);
#endif

    // python init
    PyObject* main_mod = PyImport_AddModule(x->p_name->s_name); // borrowed
    x->p_globals = PyModule_GetDict(main_mod); // borrowed reference

    PyObject* p_name = NULL;
    PyObject* builtins = NULL;

    p_name = PyUnicode_FromString(x->p_name->s_name);
    builtins = PyEval_GetBuiltins();
    PyDict_SetItemString(builtins, "PY_OBJ_NAME", p_name);

#if PY_ENABLE_SANDBOX
    // Set up sandboxed environment with restricted builtins
    if (py_setup_sandbox(x->p_globals) < 0) {
        py_error(x, "Failed to set up sandbox environment");
        // Continue anyway - sandboxing is a security enhancement, not required
    }
    post("[py %s]: Sandboxing ENABLED - restricted builtins and import whitelist active",
         x->p_name->s_name);
#else
    // Normal (unsandboxed) environment
    PyDict_SetItemString(x->p_globals, "__builtins__", builtins);
#endif

    Py_XDECREF(p_name);

    return x;
}

/**
 * @brief Free t_py object
 *
 * @param x  pointer to object structure
 */
void py_free(t_py* x)
{
    py_log(x, (char*)"deleting object %s", x->p_name->s_name);
    Py_XDECREF(x->p_globals);
    // Note: Do NOT call Py_FinalizeEx() here - it shuts down the entire
    // Python interpreter, not just this object's namespace. This would
    // corrupt the interpreter if multiple py objects exist.
    free(x);
}


// ---------------------------------------------------------------------------------------
// HELPERS


/**
 * @brief  Return path to external with optional subpath
 *
 * @param  c        t_class instance
 * @param  subpath  The subpath or NULL (if not)
 *
 * @return path to external + (optional subpath)
 */
t_string* py_get_path_to_external(t_class* c, char* subpath)
{
    char external_path[MAX_PATH_CHARS];
    char external_name[MAX_PATH_CHARS];
    short path_id = class_getpath(c);
    t_string* result;

#ifdef __APPLE__
    const char* ext_filename = "%s.mxo";
#else
    const char* ext_filename = "%s.mxe64";
#endif
    snprintf_zero(external_name, MAX_FILENAME_CHARS, ext_filename,
                  c->c_sym->s_name);
    path_toabsolutesystempath(path_id, external_name, external_path);
    result = string_new(external_path);
    if (subpath != NULL) {
        string_append(result, subpath);
    }
    return result;
}

/**
 * @brief  Return path to package with optional subpath
 *
 * @param  c        t_class instance
 * @param  subpath  The subpath or NULL (if not)
 *
 * @return path to package + (optional subpath)
 */
t_string* py_get_path_to_package(t_class* c, char* subpath)
{
    char _dummy[MAX_PATH_CHARS];
    char externals_folder[MAX_PATH_CHARS];
    char package_folder[MAX_PATH_CHARS];

    t_string* result;
    t_string* external_path = py_get_path_to_external(c, NULL);

    const char* ext_path_c = string_getptr(external_path);

    path_splitnames(ext_path_c, externals_folder, _dummy); // ignore filename
    path_splitnames(externals_folder, package_folder,
                    _dummy); // ignore filename

    result = string_new((char*)package_folder);

    if (subpath != NULL) {
        string_append(result, subpath);
    }

    return result;
}


/**
 * @brief Validates file path to prevent path traversal attacks
 *
 * @param path The file path to validate
 * @return 1 if path is safe, 0 if path is potentially malicious
 */
int py_validate_path(const char* path)
{
    if (path == NULL) {
        return 0;
    }

    // Check for path traversal patterns
    // Note: ".." is dangerous in paths like "../../../etc/passwd"
    const char* pos = path;
    while (*pos != '\0') {
        if (pos[0] == '.' && pos[1] == '.') {
            // Found ".." - check if it's a directory traversal
            // Allow ".." only if preceded by path separator and followed by path separator or end
            if ((pos == path || pos[-1] == '/' || pos[-1] == '\\') &&
                (pos[2] == '\0' || pos[2] == '/' || pos[2] == '\\')) {
                return 0;  // Reject path traversal
            }
        }
        pos++;
    }

    return 1;  // Path is safe
}

/**
 * @brief Searches the Max filesystem context for a file given by a symbol
 *
 * @param x pointer to object struct
 * @param s symbol to be searched
 * @return t_max_err
 *
 * If successful, this function will set `x->p_code_filepath` with
 * the Max readable path of the found file.
 */
t_max_err py_locate_path_from_symbol(t_py* x, t_symbol* s)
{
    t_max_err ret = 0;

    if (s == gensym("")) { // if no arg supplied ask for file
        x->p_code_filename[0] = 0;

        if (open_dialog(x->p_code_filename, &x->p_code_path,
                        &x->p_code_outtype, &x->p_code_filetype, 1))
            // non-zero: cancelled
            ret = MAX_ERR_GENERIC;
        goto finally;

    } else {
        // must copy symbol before calling locatefile_extended
        strncpy_zero(x->p_code_filename, s->s_name, MAX_PATH_CHARS);
        if (locatefile_extended(x->p_code_filename, &x->p_code_path,
                                &x->p_code_outtype, &x->p_code_filetype, 1)) {
            // nozero: not found
            py_error(x, (char*)"can't find file %s", s->s_name);
            ret = MAX_ERR_GENERIC;
            goto finally;
        } else {
            x->p_code_pathname[0] = 0;
            ret = path_toabsolutesystempath(x->p_code_path, x->p_code_filename,
                                            x->p_code_pathname);
            if (ret != MAX_ERR_NONE) {
                py_error(x, (char*)"can't convert %s to absolutepath",
                         s->s_name);
                goto finally;
            }

            // Validate path for security (check for path traversal)
            if (!py_validate_path(x->p_code_pathname)) {
                py_error(x, (char*)"invalid path: potential path traversal detected");
                ret = MAX_ERR_GENERIC;
                goto finally;
            }
        }

        // success
        // set attribute from pathname symbol
        x->p_code_filepath = gensym(x->p_code_pathname);
        assert(ret == MAX_ERR_NONE);
    }

finally:
    return ret;
}


// ---------------------------------------------------------------------------------------
// TYPE HANDLERS

/**
 * @brief Generic python error handler
 *
 * @param x pointer to object struct
 * @param fmt format string
 * @param ... other args
 *
 * SECURITY: Messages exceeding PY_MAX_ELEMS are truncated safely.
 */
void py_handle_error(t_py* x, char* fmt, ...)
{
    if (PyErr_Occurred()) {

        // build custom msg
        char msg[PY_MAX_ELEMS];

        va_list va;
        va_start(va, fmt);
        int written = vsnprintf(msg, PY_MAX_ELEMS, fmt, va);
        va_end(va);

        // Check for buffer truncation
        if (written >= PY_MAX_ELEMS) {
            error("[py %s]: Error context truncated (%d bytes, max %d)",
                  x->p_name->s_name, written, PY_MAX_ELEMS - 1);
            // Continue with truncated message
        }

        // get error info
        PyObject *ptype, *pvalue, *ptraceback;
        PyErr_Fetch(&ptype, &pvalue, &ptraceback);
        PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);
        Py_XDECREF(ptype);

        PyObject* pvalue_pstr = PyObject_Repr(pvalue);
        const char* pvalue_str = PyUnicode_AsUTF8(pvalue_pstr);
        Py_XDECREF(pvalue);
        Py_XDECREF(pvalue_pstr);

        Py_XDECREF(ptraceback);

        error("[py %s] %s: %s", x->p_name->s_name, msg, pvalue_str ? pvalue_str : "(unknown)");
    }
}


/**
 * @brief Handler to output python float as max float
 *
 * @param x pointer to object struct
 * @param pfloat python float
 * @return t_max_err error code
 */
t_max_err py_handle_float_output(t_py* x, void* outlet, PyObject* pfloat)
{
    if (pfloat == NULL) {
        goto error;
    }

    if (PyFloat_Check(pfloat)) {
        float float_result = (float)PyFloat_AsDouble(pfloat);
        if (float_result == -1.0 && PyErr_Occurred()) {
            goto error;
        }
        outlet_float(outlet, float_result);
    }
    Py_XDECREF(pfloat);
    return MAX_ERR_NONE;

error:
    py_handle_error(x, (char*)"py_handle_float_output failed");
    Py_XDECREF(pfloat);
    return MAX_ERR_GENERIC;
}

/**
 * @brief Handler to output python long as max int
 *
 * @param x pointer to object struct
 * @param plong python long
 * @return t_max_err error code
 */
t_max_err py_handle_long_output(t_py* x, void* outlet, PyObject* plong)
{
    if (plong == NULL) {
        goto error;
    }

    if (PyLong_Check(plong)) {
        long long_result = PyLong_AsLong(plong);
        if (long_result == -1 && PyErr_Occurred()) {
            goto error;
        }
        outlet_int(outlet, long_result);
    }

    Py_XDECREF(plong);
    return MAX_ERR_NONE;

error:
    py_handle_error(x, (char*)"py_handle_long_output failed");
    Py_XDECREF(plong);
    return MAX_ERR_GENERIC;
}

/**
 * @brief Handler to output python string as max symbol
 *
 * @param x pointer to object struct
 * @param pstring python string
 * @return t_max_err error code
 */
t_max_err py_handle_string_output(t_py* x, void* outlet, PyObject* pstring)
{
    if (pstring == NULL) {
        goto error;
    }

    if (PyUnicode_Check(pstring)) {
        const char* unicode_result = PyUnicode_AsUTF8(pstring);
        if (unicode_result == NULL) {
            goto error;
        }
        outlet_anything(outlet, gensym(unicode_result), 0, (t_atom*)NIL);
    }

    Py_XDECREF(pstring);
    return MAX_ERR_NONE;

error:
    py_handle_error(x, (char*)"py_handle_string_output failed");
    Py_XDECREF(pstring);
    return MAX_ERR_GENERIC;
}

/**
 * @brief Handler to output python list as max list
 *
 * @param x pointer to object struct
 * @param plist python list
 * @return t_max_err error code
 */
t_max_err py_handle_list_output(t_py* x, void* outlet, PyObject* plist)
{
    if (plist == NULL) {
        goto error;
    }

    if (PySequence_Check(plist) && !PyUnicode_Check(plist)
        && !PyBytes_Check(plist) && !PyByteArray_Check(plist)) {
        PyObject* iter = NULL;
        PyObject* item = NULL;
        int i = 0;

        t_atom atoms_static[PY_MAX_ELEMS];
        t_atom* atoms = NULL;
        int is_dynamic = 0;

        Py_ssize_t seq_size = PySequence_Length(plist);
        py_log(x, (char*)"seq_size: %d", seq_size);

        if (seq_size == 0) {
            py_error(x, (char*)"cannot convert py list of length 0 to atoms");
            goto error;
        }

        // Check for excessive sequence size to prevent resource exhaustion
        #define PY_MAX_SEQUENCE_SIZE 1048576  // 1M elements maximum
        if (seq_size > PY_MAX_SEQUENCE_SIZE) {
            py_error(x, (char*)"sequence too large (%d elements, max %d)",
                     seq_size, PY_MAX_SEQUENCE_SIZE);
            goto error;
        }

        if (seq_size > PY_MAX_ELEMS) {
            py_log(x, (char*)"dynamically increasing size of atom array");
            atoms = atom_dynamic_start(atoms_static, PY_MAX_ELEMS,
                                       seq_size + 1);
            if (atoms == NULL) {
                py_error(x, (char*)"failed to allocate memory for %d atoms", seq_size);
                goto error;
            }
            is_dynamic = 1;

        } else {
            atoms = atoms_static;
        }

        if ((iter = PyObject_GetIter(plist)) == NULL) {
            if (is_dynamic && atoms != NULL) {
                atom_dynamic_end(atoms_static, atoms);
            }
            goto error;
        }
        py_log(x, (char*)"seq_size2: %d", seq_size);

        while ((item = PyIter_Next(iter)) != NULL) {
            // Bounds check to prevent overflow if sequence size changed
            if (i >= seq_size) {
                py_error(x, (char*)"sequence size changed during iteration (i=%d, seq_size=%d)", i, seq_size);
                Py_DECREF(item);
                Py_DECREF(iter);
                if (is_dynamic && atoms != NULL) {
                    atom_dynamic_end(atoms_static, atoms);
                }
                goto error;
            }

            // Use else-if to prevent type confusion and ensure each item is processed once
            if (PyLong_Check(item)) {
                long long_item = PyLong_AsLong(item);
                if (long_item == -1 && PyErr_Occurred()) {
                    Py_DECREF(item);
                    Py_DECREF(iter);
                    if (is_dynamic && atoms != NULL) {
                        atom_dynamic_end(atoms_static, atoms);
                    }
                    goto error;
                }
                atom_setlong(atoms + i, long_item);
                py_log(x, (char*)"%d long: %ld\n", i, long_item);
                i++;
            } else if (PyFloat_Check(item)) {
                float float_item = PyFloat_AsDouble(item);
                if (float_item == -1.0 && PyErr_Occurred()) {
                    Py_DECREF(item);
                    Py_DECREF(iter);
                    if (is_dynamic && atoms != NULL) {
                        atom_dynamic_end(atoms_static, atoms);
                    }
                    goto error;
                }
                atom_setfloat(atoms + i, float_item);
                py_log(x, (char*)"%d float: %f\n", i, float_item);
                i++;
            } else if (PyUnicode_Check(item)) {
                const char* unicode_item = PyUnicode_AsUTF8(item);
                if (unicode_item == NULL) {
                    Py_DECREF(item);
                    Py_DECREF(iter);
                    if (is_dynamic && atoms != NULL) {
                        atom_dynamic_end(atoms_static, atoms);
                    }
                    goto error;
                }
                atom_setsym(atoms + i, gensym(unicode_item));
                py_log(x, (char*)"%d unicode: %s\n", i, unicode_item);
                i++;
            } else {
                // Unrecognized type - log warning and skip
                py_log(x, (char*)"warning: skipping unsupported type at index %d", i);
                // Don't increment i since we're not adding this item
            }
            Py_DECREF(item);
        }

        // Clean up iterator after loop completion
        Py_XDECREF(iter);

        outlet_list(outlet, NULL, i, atoms);
        py_log(x, (char*)"end iter op: %d", i);

        if (is_dynamic) {
            py_log(x, (char*)"restoring to static atom array");
            atom_dynamic_end(atoms_static, atoms);
        }
    }

    Py_XDECREF(plist);
    return MAX_ERR_NONE;

error:
    py_handle_error(x, (char*)"py_handle_list_output failed");
    Py_XDECREF(plist);
    return MAX_ERR_GENERIC;
}

/**
 * @brief Handler to output python dict as max list
 *
 * @param x pointer to object struct
 * @param pdict python dict
 * @return t_max_err error code
 */
t_max_err py_handle_dict_output(t_py* x, void* outlet, PyObject* pdict)
{
    PyObject* pfun_co = NULL;
    PyObject* pfun = NULL;
    PyObject* pval = NULL;

    if (pdict == NULL) {
        goto error;
    }

    if (PyDict_Check(pdict)) {

        pfun_co = PyRun_String("def __py_maxmsp_out_dict(arg):\n"
                               "\tres = []\n"
                               "\tfor k,v in arg.items():\n"
                               "\t\tres.append(k)\n"
                               "\t\tres.append(':')\n"
                               "\t\tif type(v) in [list, set, tuple]:\n"
                               "\t\t\tfor i in v:\n"
                               "\t\t\t\tres.append(i)\n"
                               "\t\telse:\n"
                               "\t\t\tres.append(v)\n"
                               "\treturn res\n",
                               Py_single_input, x->p_globals, x->p_globals);

        if (pfun_co == NULL) {
            py_error(x, (char*)"out_dict function code object is NULL");
            goto error;
        }

        pfun = PyDict_GetItemString(x->p_globals, "__py_maxmsp_out_dict");
        if (pfun == NULL) {
            py_error(x, (char*)"retrieving out_dict func from globals failed");
            goto error;
        }

        pval = PyObject_CallFunctionObjArgs(pfun, pdict, NULL);
        if (pval == NULL) {
            py_error(x, (char*)"out_dict call failed to retrieve result");
            goto error;
        }

        if (PyList_Check(pval)) {                   // expecting a python list
            py_handle_list_output(x, outlet, pval); // this decrefs pval
            Py_XDECREF(pfun_co);
            return MAX_ERR_NONE;
        } else {
            py_error(x, (char*)"expected list output got something else");
            goto error;
        }
    }

error:
    py_handle_error(x, (char*)"py_handle_dict_output failed");
    Py_XDECREF(pfun_co);
    Py_XDECREF(pval);
    return MAX_ERR_GENERIC;
}

/**
 * @brief Generic handler to output arbitrarily-typed python object as max
 * object
 *
 * @param x pointer to object struct
 * @param pval python object
 * @return t_max_err error code
 */
t_max_err py_handle_output(t_py* x, void* outlet, PyObject* pval)
{
    if (pval == NULL) {
        py_error(x, (char*)"cannot handle NULL value");
        return MAX_ERR_GENERIC;
    }

    if (PyFloat_Check(pval)) {
        return py_handle_float_output(x, outlet, pval);
    }

    else if (PyLong_Check(pval)) {
        return py_handle_long_output(x, outlet, pval);
    }

    else if (PyUnicode_Check(pval)) {
        return py_handle_string_output(x, outlet, pval);
    }

    else if (PySequence_Check(pval) && !PyBytes_Check(pval)
             && !PyByteArray_Check(pval)) {
        return py_handle_list_output(x, outlet, pval);
    }

    else if (PyDict_Check(pval)) {
        return py_handle_dict_output(x, outlet, pval);
    }

    else if (pval == Py_None) {
        return MAX_ERR_GENERIC;
    }

    else {
        py_error(x, (char*)"cannot handle his type of value");
        return MAX_ERR_GENERIC;
    }
}


// ---------------------------------------------------------------------------------------
// CORE METHODS


/**
 * @brief Import a python module
 *
 * @param x pointer to object structure
 * @param s symbol of module to be imported
 * @return t_max_err error code
 */
t_max_err py_import(t_py* x, t_symbol* s)
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    PyObject* x_module = NULL;

    if (s != gensym("")) {

#if PY_ENABLE_SANDBOX
        // Validate module against whitelist when sandboxing is enabled
        if (!py_is_module_allowed(s->s_name)) {
            py_error(x, (char*)"Module '%s' not allowed in sandboxed environment", s->s_name);
            PyGILState_Release(gstate);
            return MAX_ERR_GENERIC;
        }
#endif

        x_module = PyImport_ImportModule(s->s_name);
        // x_module borrrowed ref
        if (x_module == NULL) {
            goto error;
        }
        PyDict_SetItemString(x->p_globals, s->s_name, x_module);
        PyGILState_Release(gstate);
        py_log(x, (char*)"imported: %s", s->s_name);
    }
    return MAX_ERR_NONE;

error:
    py_handle_error(x, (char*)"import %s", s->s_name);
    PyGILState_Release(gstate);
    return MAX_ERR_GENERIC;
}

/**
 * @brief Evaluate a max symbol as a python expression
 *
 * @param x pointer to object structure
 * @param s symbol of code to be evaluated
 * @param outlet object outlet
 *
 * @return t_max_err error code
 */
t_max_err py_eval(t_py* x, t_symbol* s, void* outlet)
{
    // Validate input size
    size_t code_len = strlen(s->s_name);
    if (code_len > PY_MAX_CODE_SIZE) {
        py_error(x, "code too large (%zu bytes, max %d)", code_len, PY_MAX_CODE_SIZE);
        return MAX_ERR_GENERIC;
    }

    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    py_log(x, (char*)"eval %s", s->s_name);

    PyObject* pval = PyRun_String(s->s_name, Py_eval_input, x->p_globals,
                                  x->p_globals);

    if (pval != NULL) {
        py_handle_output(x, outlet, pval);
        PyGILState_Release(gstate);
        return MAX_ERR_NONE;
    } else {
        py_handle_error(x, (char*)"eval %s", s->s_name);
        PyGILState_Release(gstate);
        return MAX_ERR_GENERIC;
    }
}


/**
 * @brief Execute a c string as a file-length block of python code
 *
 * @param x pointer to object structure
 * @param code char* python code
 * @return t_max_err error code
 */
t_max_err py_exec_file_input(t_py* x, const char* code)
{
    // Validate input size
    size_t code_len = strlen(code);
    if (code_len > PY_MAX_CODE_SIZE) {
        py_error(x, "code too large (%zu bytes, max %d)", code_len, PY_MAX_CODE_SIZE);
        return MAX_ERR_GENERIC;
    }

    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    PyObject* pval = NULL;

    pval = PyRun_String(code, Py_file_input, x->p_globals, x->p_globals);
    if (pval == NULL) {
        goto error;
    }
    Py_DECREF(pval);
    PyGILState_Release(gstate);

    py_log(x, (char*)"py_exec_file_input");
    return MAX_ERR_NONE;

error:
    py_handle_error(x, (char*)"py_exec_file_input");
    Py_XDECREF(pval);
    PyGILState_Release(gstate);
    return MAX_ERR_GENERIC;
}


/**
 * @brief Execute a c string as a statement-length block of python code
 *
 * @param x pointer to object structure
 * @param code char* python code
 * @return t_max_err error code
 */
t_max_err py_exec_single_input(t_py* x, const char* code)
{
    // Validate input size
    size_t code_len = strlen(code);
    if (code_len > PY_MAX_CODE_SIZE) {
        py_error(x, "code too large (%zu bytes, max %d)", code_len, PY_MAX_CODE_SIZE);
        return MAX_ERR_GENERIC;
    }

    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    PyObject* pval = NULL;

    pval = PyRun_String(code, Py_single_input, x->p_globals, x->p_globals);
    if (pval == NULL) {
        goto error;
    }
    Py_DECREF(pval);
    PyGILState_Release(gstate);

    py_log(x, (char*)"py_exec_single_input");
    return MAX_ERR_NONE;

error:
    py_handle_error(x, (char*)"py_exec_single_input");
    Py_XDECREF(pval);
    PyGILState_Release(gstate);
    return MAX_ERR_GENERIC;
}


/**
 * @brief Execute a c string as a file-length block of python code with output capture
 *
 * @param x pointer to object structure
 * @param code char* python code
 * @return char* captured output (caller must free), or NULL on error
 */
char* py_exec_file_input_with_output(t_py* x, const char* code)
{
    // Validate input size
    size_t code_len = strlen(code);
    if (code_len > PY_MAX_CODE_SIZE) {
        py_error(x, "code too large (%zu bytes, max %d)", code_len, PY_MAX_CODE_SIZE);
        return NULL;
    }

    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    PyObject* pval = NULL;
    PyObject* sys_module = NULL;
    PyObject* io_module = NULL;
    PyObject* string_io = NULL;
    PyObject* old_stdout = NULL;
    PyObject* old_stderr = NULL;
    PyObject* getvalue_method = NULL;
    PyObject* output_obj = NULL;
    char* output_str = NULL;

    // Import sys and io modules
    sys_module = PyImport_ImportModule("sys");
    if (sys_module == NULL) {
        py_error(x, "failed to import sys module");
        goto error;
    }

    io_module = PyImport_ImportModule("io");
    if (io_module == NULL) {
        py_error(x, "failed to import io module");
        goto error;
    }

    // Create StringIO object
    PyObject* string_io_class = PyObject_GetAttrString(io_module, "StringIO");
    if (string_io_class == NULL) {
        py_error(x, "failed to get StringIO class");
        goto error;
    }
    string_io = PyObject_CallObject(string_io_class, NULL);
    Py_DECREF(string_io_class);
    if (string_io == NULL) {
        py_error(x, "failed to create StringIO object");
        goto error;
    }

    // Save old stdout and stderr
    old_stdout = PyObject_GetAttrString(sys_module, "stdout");
    old_stderr = PyObject_GetAttrString(sys_module, "stderr");

    // Redirect stdout and stderr to StringIO
    if (PyObject_SetAttrString(sys_module, "stdout", string_io) < 0) {
        py_error(x, "failed to redirect stdout");
        goto error;
    }
    if (PyObject_SetAttrString(sys_module, "stderr", string_io) < 0) {
        py_error(x, "failed to redirect stderr");
        goto error;
    }

    // Execute code
    pval = PyRun_String(code, Py_file_input, x->p_globals, x->p_globals);

    // Restore stdout and stderr (even if execution failed)
    if (old_stdout) {
        PyObject_SetAttrString(sys_module, "stdout", old_stdout);
    }
    if (old_stderr) {
        PyObject_SetAttrString(sys_module, "stderr", old_stderr);
    }

    if (pval == NULL) {
        // Capture error output from StringIO before reporting error
        getvalue_method = PyObject_GetAttrString(string_io, "getvalue");
        if (getvalue_method != NULL) {
            output_obj = PyObject_CallObject(getvalue_method, NULL);
            if (output_obj != NULL && PyUnicode_Check(output_obj)) {
                const char* temp_str = PyUnicode_AsUTF8(output_obj);
                if (temp_str != NULL) {
                    output_str = strdup(temp_str);
                }
            }
        }
        goto error;
    }

    // Get captured output
    getvalue_method = PyObject_GetAttrString(string_io, "getvalue");
    if (getvalue_method == NULL) {
        py_error(x, "failed to get getvalue method");
        goto error;
    }

    output_obj = PyObject_CallObject(getvalue_method, NULL);
    if (output_obj == NULL) {
        py_error(x, "failed to call getvalue");
        goto error;
    }

    if (PyUnicode_Check(output_obj)) {
        const char* temp_str = PyUnicode_AsUTF8(output_obj);
        if (temp_str != NULL) {
            output_str = strdup(temp_str);
        }
    }

    // Cleanup
    Py_XDECREF(pval);
    Py_XDECREF(sys_module);
    Py_XDECREF(io_module);
    Py_XDECREF(string_io);
    Py_XDECREF(old_stdout);
    Py_XDECREF(old_stderr);
    Py_XDECREF(getvalue_method);
    Py_XDECREF(output_obj);
    PyGILState_Release(gstate);

    py_log(x, (char*)"py_exec_file_input_with_output");
    return output_str ? output_str : strdup("");

error:
    py_handle_error(x, (char*)"py_exec_file_input_with_output");
    Py_XDECREF(pval);
    Py_XDECREF(sys_module);
    Py_XDECREF(io_module);
    Py_XDECREF(string_io);
    Py_XDECREF(old_stdout);
    Py_XDECREF(old_stderr);
    Py_XDECREF(getvalue_method);
    Py_XDECREF(output_obj);
    PyGILState_Release(gstate);
    return output_str;  // May be NULL or partial output
}


/**
 * @brief Execute a c string as a statement-length block of python code with output capture
 *
 * @param x pointer to object structure
 * @param code char* python code
 * @return char* captured output (caller must free), or NULL on error
 */
char* py_exec_single_input_with_output(t_py* x, const char* code)
{
    // Validate input size
    size_t code_len = strlen(code);
    if (code_len > PY_MAX_CODE_SIZE) {
        py_error(x, "code too large (%zu bytes, max %d)", code_len, PY_MAX_CODE_SIZE);
        return NULL;
    }

    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    PyObject* pval = NULL;
    PyObject* sys_module = NULL;
    PyObject* io_module = NULL;
    PyObject* string_io = NULL;
    PyObject* old_stdout = NULL;
    PyObject* old_stderr = NULL;
    PyObject* getvalue_method = NULL;
    PyObject* output_obj = NULL;
    char* output_str = NULL;

    // Import sys and io modules
    sys_module = PyImport_ImportModule("sys");
    if (sys_module == NULL) {
        py_error(x, "failed to import sys module");
        goto error;
    }

    io_module = PyImport_ImportModule("io");
    if (io_module == NULL) {
        py_error(x, "failed to import io module");
        goto error;
    }

    // Create StringIO object
    PyObject* string_io_class = PyObject_GetAttrString(io_module, "StringIO");
    if (string_io_class == NULL) {
        py_error(x, "failed to get StringIO class");
        goto error;
    }
    string_io = PyObject_CallObject(string_io_class, NULL);
    Py_DECREF(string_io_class);
    if (string_io == NULL) {
        py_error(x, "failed to create StringIO object");
        goto error;
    }

    // Save old stdout and stderr
    old_stdout = PyObject_GetAttrString(sys_module, "stdout");
    old_stderr = PyObject_GetAttrString(sys_module, "stderr");

    // Redirect stdout and stderr to StringIO
    if (PyObject_SetAttrString(sys_module, "stdout", string_io) < 0) {
        py_error(x, "failed to redirect stdout");
        goto error;
    }
    if (PyObject_SetAttrString(sys_module, "stderr", string_io) < 0) {
        py_error(x, "failed to redirect stderr");
        goto error;
    }

    // Execute code
    pval = PyRun_String(code, Py_single_input, x->p_globals, x->p_globals);

    // Restore stdout and stderr (even if execution failed)
    if (old_stdout) {
        PyObject_SetAttrString(sys_module, "stdout", old_stdout);
    }
    if (old_stderr) {
        PyObject_SetAttrString(sys_module, "stderr", old_stderr);
    }

    if (pval == NULL) {
        // Capture error output from StringIO before reporting error
        getvalue_method = PyObject_GetAttrString(string_io, "getvalue");
        if (getvalue_method != NULL) {
            output_obj = PyObject_CallObject(getvalue_method, NULL);
            if (output_obj != NULL && PyUnicode_Check(output_obj)) {
                const char* temp_str = PyUnicode_AsUTF8(output_obj);
                if (temp_str != NULL) {
                    output_str = strdup(temp_str);
                }
            }
        }
        goto error;
    }

    // Get captured output
    getvalue_method = PyObject_GetAttrString(string_io, "getvalue");
    if (getvalue_method == NULL) {
        py_error(x, "failed to get getvalue method");
        goto error;
    }

    output_obj = PyObject_CallObject(getvalue_method, NULL);
    if (output_obj == NULL) {
        py_error(x, "failed to call getvalue");
        goto error;
    }

    if (PyUnicode_Check(output_obj)) {
        const char* temp_str = PyUnicode_AsUTF8(output_obj);
        if (temp_str != NULL) {
            output_str = strdup(temp_str);
        }
    }

    // Cleanup
    Py_XDECREF(pval);
    Py_XDECREF(sys_module);
    Py_XDECREF(io_module);
    Py_XDECREF(string_io);
    Py_XDECREF(old_stdout);
    Py_XDECREF(old_stderr);
    Py_XDECREF(getvalue_method);
    Py_XDECREF(output_obj);
    PyGILState_Release(gstate);

    py_log(x, (char*)"py_exec_single_input_with_output");
    return output_str ? output_str : strdup("");

error:
    py_handle_error(x, (char*)"py_exec_single_input_with_output");
    Py_XDECREF(pval);
    Py_XDECREF(sys_module);
    Py_XDECREF(io_module);
    Py_XDECREF(string_io);
    Py_XDECREF(old_stdout);
    Py_XDECREF(old_stderr);
    Py_XDECREF(getvalue_method);
    Py_XDECREF(output_obj);
    PyGILState_Release(gstate);
    return output_str;  // May be NULL or partial output
}


/**
 * @brief Execute a max symbol as a line of python code
 *
 * @param x pointer to object structure
 * @param s symbol of code string to exec
 * @return t_max_err error code
 */
t_max_err py_exec(t_py* x, t_symbol* s)
{
    // Validate input size
    size_t code_len = strlen(s->s_name);
    if (code_len > PY_MAX_CODE_SIZE) {
        py_error(x, "code too large (%zu bytes, max %d)", code_len, PY_MAX_CODE_SIZE);
        return MAX_ERR_GENERIC;
    }

    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    PyObject* pval = NULL;

    pval = PyRun_String(s->s_name, Py_single_input, x->p_globals, x->p_globals);
    if (pval == NULL) {
        goto error;
    }
    Py_DECREF(pval);
    PyGILState_Release(gstate);

    py_log(x, (char*)"exec %s", s->s_name);
    return MAX_ERR_NONE;

error:
    py_handle_error(x, (char*)"exec %s", s->s_name);
    Py_XDECREF(pval);
    PyGILState_Release(gstate);
    return MAX_ERR_GENERIC;
}

/**
 * @brief Execute contents of a file (obtained from symbol) as python code
 *
 * @param x pointer to object structure
 * @param s symbol
 * @return t_max_err error code
 */
t_max_err py_execfile(t_py* x, t_symbol* s)
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    PyObject* pval = NULL;
    FILE* fhandle = NULL;

    if (s != gensym("")) {
        // set x->p_code_filepath
        t_max_err err = py_locate_path_from_symbol(x, s);
        if (err != MAX_ERR_NONE) {
            py_error(x, (char*)"could not locate path from symbol");
            goto error;
        }
    }

    if (s == gensym("") || x->p_code_filepath == gensym("")) {
        py_error(x, (char*)"could not set filepath");
        goto error;
    }

    // assume x->p_code_filepath has be been set without errors

    py_log(x, (char*)"pathname: %s", x->p_code_filepath->s_name);
    // Use read-only mode for security - no need to write when executing
    fhandle = fopen(x->p_code_filepath->s_name, "r");

    if (fhandle == NULL) {
        py_error(x, (char*)"could not open file");
        goto error;
    }

    pval = PyRun_File(fhandle, x->p_code_filepath->s_name, Py_file_input,
                      x->p_globals, x->p_globals);
    if (pval == NULL) {
        fclose(fhandle);
        goto error;
    }

    // success cleanup
    fclose(fhandle);
    Py_DECREF(pval);
    PyGILState_Release(gstate);
    return MAX_ERR_NONE;

error:
    py_handle_error(x, (char*)"execfile");
    Py_XDECREF(pval);
    PyGILState_Release(gstate);
    return MAX_ERR_GENERIC;
}

// ---------------------------------------------------------------------------------------
// EXTRA METHODS HELPERS

/**
 * @brief Translates atom vector to python list
 *
 * @param x pointer to object struct
 * @param argc atom argument count
 * @param argv atom argument vector
 * @param start_from index of vector to start from
 * @return PyObject* python list
 */
PyObject* py_atoms_to_list(t_py* x, long argc, t_atom* argv, int start_from)
{

    PyObject* plist = NULL; // python list

    if ((plist = PyList_New(0)) == NULL) {
        py_error(x, (char*)"could not create an empty python list");
        goto error;
    }

    for (int i = start_from; i < argc; i++) {
        switch ((argv + i)->a_type) {
        case A_FLOAT: {
            double c_float = atom_getfloat(argv + i);
            PyObject* p_float = PyFloat_FromDouble(c_float);
            if (p_float == NULL) {
                goto error;
            }
            PyList_Append(plist, p_float);
            Py_DECREF(p_float);
            break;
        }
        case A_LONG: {
            PyObject* p_long = PyLong_FromLong(atom_getlong(argv + i));
            if (p_long == NULL) {
                goto error;
            }
            PyList_Append(plist, p_long);
            Py_DECREF(p_long);
            break;
        }
        case A_SYM: {
            PyObject* p_str = PyUnicode_FromString(
                atom_getsym(argv + i)->s_name);
            if (p_str == NULL) {
                goto error;
            }
            PyList_Append(plist, p_str);
            Py_DECREF(p_str);
            break;
        }
        default:
            py_log(x, (char*)"cannot process unknown type");
            break;
        }
    }
    return plist;

error:
    py_error(x, (char*)"atom to list conversion failed");
    return NULL;
}


/**
 * @brief A helper function to evaluate Max text as a Python expression.
 *
 * @param x pointer to object structure
 * @param argc atom argument count
 * @param argv atom argument vector
 * @param offset offset of atom vector from which to evaluate
 * @param outlet object outlet
 *
 * @return t_max_err error code
 */
t_max_err py_eval_text(t_py* x, long argc, t_atom* argv, int offset,
                       void* outlet)
{
    PyGILState_STATE gstate = PyGILState_Ensure();

    long textsize = 0;
    char* text = NULL;
    int is_eval = 1;
    PyObject* co = NULL;
    PyObject* pval = NULL;


    t_max_err err = atom_gettext(argc + offset, argv, &textsize, &text,
                                 OBEX_UTIL_ATOM_GETTEXT_DEFAULT);
    if (err == MAX_ERR_NONE && textsize && text) {
        py_log(x, (char*)">>> %s", text);
    } else {
        goto error;
    }

    co = Py_CompileString(text, x->p_name->s_name, Py_eval_input);

    if (PyErr_ExceptionMatches(PyExc_SyntaxError)) {
        PyErr_Clear();
        co = Py_CompileString(text, x->p_name->s_name, Py_single_input);
        is_eval = 0;
    }

    if (co == NULL) { // can be eval-co or exec-co or NULL here
        sysmem_freeptr(text);
        goto error;
    }
    sysmem_freeptr(text);

    pval = PyEval_EvalCode(co, x->p_globals, x->p_globals);
    if (pval == NULL) {
        goto error;
    }
    Py_DECREF(co);

    if (!is_eval) {
        // bang for exec-type op
        PyGILState_Release(gstate);
    } else {
        py_handle_output(x, outlet, pval);
    }
    return MAX_ERR_NONE;

error:
    py_handle_error(x, (char*)"python code evaluation failed");
    PyGILState_Release(gstate);
    return MAX_ERR_GENERIC;
}


// ---------------------------------------------------------------------------------------
// EXTRA METHODS

/**
 * @brief Converts a Max list to call a python function with arguments
 *
 * @param x pointer to object structure
 * @param s symbol
 * @param argc atom argument count
 * @param argv atom argument vector
 * @param outlet object outlet
 *
 * @return t_max_err error code
 */
t_max_err py_call(t_py* x, t_symbol* s, long argc, t_atom* argv, void* outlet)
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    char* callable_name = NULL;
    PyObject* py_argslist = NULL;
    PyObject* pval = NULL;
    PyObject* py_callable = NULL;
    // python list
    PyObject* py_args = NULL; // python tuple

    // first atom in argv must be a symbol
    if (argv->a_type != A_SYM) {
        py_error(x, (char*)"first atom must be a symbol!");
        goto error;

    } else {
        callable_name = atom_getsym(argv)->s_name;
        py_log(x, (char*)"callable_name: %s", callable_name);
    }

    py_callable = PyRun_String(callable_name, Py_eval_input, x->p_globals,
                               x->p_globals);
    if (py_callable == NULL) {
        py_error(x, (char*)"could not evaluate %s", callable_name);
        goto error;
    }

    py_argslist = py_atoms_to_list(x, argc, argv, 1);
    if (py_argslist == NULL) {
        py_error(x, (char*)"atom to py list conversion failed");
        goto error;
    }

    py_log(x, (char*)"length of argc:%ld list: %d", argc,
           PyList_Size(py_argslist));

    // convert py_args to tuple
    py_args = PyList_AsTuple(py_argslist);
    if (py_args == NULL) {
        py_error(x, (char*)"unable to convert args list to tuple");
        goto error;
    }

    pval = PyObject_CallObject(py_callable, py_args);
    if (!PyErr_ExceptionMatches(PyExc_TypeError)) {
        if (pval == NULL) {
            py_error(x, (char*)"unable to apply callable(*args)");
            goto error;
        }
        goto handle_output;
    }
    PyErr_Clear();

    pval = PyObject_CallFunctionObjArgs(py_callable, py_argslist, NULL);
    if (pval == NULL) {
        py_error(x, (char*)"could not retrieve result of callable(list)");
        goto error;
    }
    goto handle_output; // this is redundant but safer in case code is added

handle_output:
    py_handle_output(x, outlet, pval);
    // success cleanup
    Py_XDECREF(py_callable);
    Py_XDECREF(py_argslist);
    PyGILState_Release(gstate);
    return MAX_ERR_NONE;

error:
    py_handle_error(x, (char*)"method %s", s->s_name);
    // cleanup
    Py_XDECREF(py_callable);
    Py_XDECREF(py_argslist);
    Py_XDECREF(pval);
    PyGILState_Release(gstate);
    return MAX_ERR_GENERIC;
}

/**
 * @brief Converts an atom list to a python assignment
 *
 * @param x pointer to object structure
 * @param s symbol
 * @param argc atom argument count
 * @param argv atom argument vector
 * @return t_max_err error code
 *
 * The first item of the Max list must be a symbol. This is converted into a
 * python variable and the rest of the list is assignment to this variable in
 * the object's python namespace.
 */
t_max_err py_assign(t_py* x, t_symbol* s, long argc, t_atom* argv)
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    char* varname = NULL;
    PyObject* list = NULL;
    int res;

    if (s != gensym(""))
        py_log(x, (char*)"s: %s", s->s_name);

    // first atom in argv must be a symbol
    if (argv->a_type != A_SYM) {
        py_error(x, (char*)"first atom must be a symbol!");
        goto error;

    } else {
        varname = atom_getsym(argv)->s_name;
        py_log(x, (char*)"varname: %s", varname);
    }

    list = py_atoms_to_list(x, argc, argv, 1);
    if (list == NULL) {
        py_error(x, (char*)"atom to py list conversion failed");
        goto error;
    }

    if (PyList_Size(list) != argc - 1) {
        py_error(x, (char*)"PyList_Size(list) != argc - 1");
        goto error;
    } else {
        py_log(x, (char*)"length of list: %d", PyList_Size(list));
    }

    // finally, assign list to varname in object namespace
    py_log(x, (char*)"setting %s to list in namespace", varname);
    // following does not steal ref to list
    res = PyDict_SetItemString(x->p_globals, varname, list);
    if (res != 0) {
        py_error(x, (char*)"assign varname to list failed");
        goto error;
    }
    PyGILState_Release(gstate);
    return MAX_ERR_NONE;

error:
    py_handle_error(x, (char*)"assign %s", s->s_name);
    Py_XDECREF(list);
    PyGILState_Release(gstate);
    return MAX_ERR_GENERIC;
}


/**
 * @brief Converts all of the atom to text and evaluate as python code.
 *
 * @param x pointer to object structure
 * @param s symbol
 * @param argc atom argument count
 * @param argv atom argument vector
 * @param outlet object outlet
 *
 * @return t_max_err error code
 */
t_max_err py_code(t_py* x, t_symbol* s, long argc, t_atom* argv, void* outlet)
{
    return py_eval_text(x, argc, argv, 0, outlet);
}


/**
 * @brief Anything method converting all of the atom to text and evaluate as
 * python code.
 *
 * @param x pointer to object structure
 * @param s symbol
 * @param argc atom argument count
 * @param argv atom argument vector
 * @param outlet object outlet
 *
 * @return t_max_err error code
 */
t_max_err py_anything(t_py* x, t_symbol* s, long argc, t_atom* argv,
                      void* outlet)
{
    t_atom atoms[PY_MAX_ELEMS];

    if (s == gensym("")) {
        return MAX_ERR_GENERIC;
    }

    // set '=' as shorthand for assign method
    if (s == gensym("=")) {
        py_assign(x, gensym(""), argc, argv);
        return MAX_ERR_NONE;
    }

    // Check bounds to prevent buffer overflow
    // We need space for the symbol (1 element) + all args
    if (argc >= PY_MAX_ELEMS) {
        py_error(x, (char*)"too many arguments (%d args, max %d)",
                 argc, PY_MAX_ELEMS - 1);
        return MAX_ERR_GENERIC;
    }

    // set symbol as first atom in new atoms array
    atom_setsym(atoms, s);

    for (int i = 0; i < argc; i++) {
        switch ((argv + i)->a_type) {
        case A_FLOAT: {
            atom_setfloat((atoms + (i + 1)), atom_getfloat(argv + i));
            break;
        }
        case A_LONG: {
            atom_setlong((atoms + (i + 1)), atom_getlong(argv + i));
            break;
        }
        case A_SYM: {
            atom_setsym((atoms + (i + 1)), atom_getsym(argv + i));
            break;
        }
        default:
            py_log(x, (char*)"cannot process unknown type");
            break;
        }
    }

    return py_eval_text(x, argc, atoms, 1, outlet);
}

/**
 * @brief Create a function python pipeline from a Max list
 *
 * @param x pointer to object structure
 * @param s symbol
 * @param argc atom argument count
 * @param argv atom argument vector
 * @param outlet object outlet
 *
 * @return t_max_err error code
 */
t_max_err py_pipe(t_py* x, t_symbol* s, long argc, t_atom* argv, void* outlet)
{
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();

    long textsize = 0;
    char* text = NULL;
    t_max_err err;
    PyObject* pipe_pre = NULL;
    PyObject* pipe_fun = NULL;
    PyObject* pval = NULL;
    PyObject* pstr = NULL;

    err = atom_gettext(argc, argv, &textsize, &text,
                       OBEX_UTIL_ATOM_GETTEXT_DEFAULT);
    if (err != MAX_ERR_NONE || !textsize || !text) {
        py_error(x, (char*)"atom -> text conversion failed");
        goto error;
    }

    pipe_pre = PyRun_String(
        "def __py_maxmsp_pipe(arg):\n"
        "\targs = arg.split()\n"
        "\tval = eval(args[0], locals(), globals())\n"
        "\tfuncs = [eval(f, locals(), globals()) for f in args[1:]]\n"
        "\tfor f in funcs:\n"
        "\t\tval = f(val)\n"
        "\treturn val\n",
        Py_single_input, x->p_globals, x->p_globals);

    if (pipe_pre == NULL) {
        py_error(x, (char*)"pipe func is NULL");
        goto error;
    }

    pstr = PyUnicode_FromString(text);
    if (pstr == NULL) {
        py_error(x, (char*)"cstr -> pyunicode conversion failed");
        goto error;
    }

    sysmem_freeptr(text);

    pipe_fun = PyDict_GetItemString(x->p_globals, "__py_maxmsp_pipe");
    if (pipe_fun == NULL) {
        py_error(x, (char*)"retrieving pipe func from globals failed");
        goto error;
    }

    pval = PyObject_CallFunctionObjArgs(pipe_fun, pstr, NULL);

    if (pval != NULL) {

        if (!PyUnicode_Check(pval)) {
            py_handle_output(x, outlet, pval); // this decrefs pval
        } else {
            // special case strings, which will cause crash if handled
            // out of this methods's scope. (huge PITA to debug!)
            const char* unicode_result = PyUnicode_AsUTF8(pval);
            if (unicode_result == NULL) {
                goto error;
            }
            outlet_anything(outlet, gensym(unicode_result), 0, (t_atom*)NIL);
            Py_XDECREF(pval);
        }

        Py_XDECREF(pipe_pre);
        Py_XDECREF(pstr);
        PyGILState_Release(gstate);
        return MAX_ERR_NONE;
    } else {
        goto error;
    }

error:
    py_handle_error(x, (char*)"pipe failed");
    Py_XDECREF(pipe_pre);
    Py_XDECREF(pstr);
    Py_XDECREF(pval);
    PyGILState_Release(gstate);
    return MAX_ERR_GENERIC;
}

#endif

// ---------------------------------------------------------------------------------------
// END IMPLEMENTATION
