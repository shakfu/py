#define PY_SSIZE_T_CLEAN
#include <Python.h>

// Global dicts
static PyObject *code_cache = NULL;  // maps expr string -> code object
static PyObject *memo_dict  = NULL;  // maps (expr, args...) -> result

// Get or compile code object for an expression string
static PyObject *
get_code_object(const char *expr) {
    PyObject *key = PyUnicode_FromString(expr);
    if (!key) return NULL;

    PyObject *code = PyDict_GetItemWithError(code_cache, key);
    if (code) {
        Py_INCREF(code);
        Py_DECREF(key);
        return code;
    }
    if (PyErr_Occurred()) {
        Py_DECREF(key);
        return NULL;
    }

    code = Py_CompileString(expr, "<expr>", Py_eval_input);
    if (!code) {
        Py_DECREF(key);
        return NULL;
    }

    if (PyDict_SetItem(code_cache, key, code) < 0) {
        Py_DECREF(code);
        Py_DECREF(key);
        return NULL;
    }

    Py_DECREF(key);
    return code;
}

// run_cached(expr: str, locals: dict) -> result
static PyObject *
run_cached(PyObject *self, PyObject *args) {
    const char *expr;
    PyObject *locals;

    if (!PyArg_ParseTuple(args, "sO!", &expr, &PyDict_Type, &locals)) {
        return NULL;
    }

    // Memoization key = (expr, frozenset(locals.items()))
    PyObject *expr_key = PyUnicode_FromString(expr);
    if (!expr_key) return NULL;

    PyObject *items = PyDict_Items(locals);  // list of (k,v)
    if (!items) {
        Py_DECREF(expr_key);
        return NULL;
    }
    PyObject *frozen = PyFrozenSet_New(items);
    Py_DECREF(items);
    if (!frozen) {
        Py_DECREF(expr_key);
        return NULL;
    }

    PyObject *memo_key = PyTuple_Pack(2, expr_key, frozen);
    Py_DECREF(expr_key);
    Py_DECREF(frozen);
    if (!memo_key) return NULL;

    PyObject *result = PyDict_GetItemWithError(memo_dict, memo_key);
    if (result) {
        Py_INCREF(result);
        Py_DECREF(memo_key);
        return result;  // memoized result
    }
    if (PyErr_Occurred()) {
        Py_DECREF(memo_key);
        return NULL;
    }

    // Compile or fetch cached code object
    PyObject *code = get_code_object(expr);
    if (!code) {
        Py_DECREF(memo_key);
        return NULL;
    }

    // Execute code with globals=None, given locals
    result = PyEval_EvalCode((PyCodeObject *)code, PyEval_GetGlobals(), locals);
    Py_DECREF(code);

    if (!result) {
        Py_DECREF(memo_key);
        return NULL;
    }

    // Store in memo_dict
    if (PyDict_SetItem(memo_dict, memo_key, result) < 0) {
        Py_DECREF(result);
        Py_DECREF(memo_key);
        return NULL;
    }

    Py_DECREF(memo_key);
    return result;
}

// Module definition
static PyMethodDef CacheMethods[] = {
    {"run_cached", run_cached, METH_VARARGS,
     "Run and memoize compiled expression with given locals dict."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef cachemodule = {
    PyModuleDef_HEAD_INIT,
    "codecaching",
    "Multi-expression code caching with memoization",
    -1,
    CacheMethods
};

PyMODINIT_FUNC
PyInit_codecaching(void) {
    PyObject *m = PyModule_Create(&cachemodule);
    if (m == NULL)
        return NULL;

    code_cache = PyDict_New();
    memo_dict  = PyDict_New();
    if (!code_cache || !memo_dict) {
        return NULL;
    }

    return m;
}

