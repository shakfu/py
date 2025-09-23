/**
 * @file math_extension.c
 * @brief Example Python C extension using the function cache header
 * 
 * This demonstrates how to integrate the py_function_cache.h header
 * into a real Python C extension module.
 * 
 * The extension provides:
 * - Mathematical computation functions with caching
 * - Dynamic function loading from strings
 * - Performance monitoring
 * - Cache management
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "py_function_cache.h"  // Include our header-only cache

// Module state structure (for Python 3.5+)
typedef struct {
    PyObject *error;
} math_extension_state;

#define GET_STATE(m) ((math_extension_state*)PyModule_GetState(m))

// Forward declarations
static PyObject* math_add_function(PyObject *self, PyObject *args, PyObject *kwargs);
static PyObject* math_call_function(PyObject *self, PyObject *args, PyObject *kwargs);
static PyObject* math_has_function(PyObject *self, PyObject *args);
static PyObject* math_get_stats(PyObject *self, PyObject *args);
static PyObject* math_list_functions(PyObject *self, PyObject *args);
static PyObject* math_benchmark(PyObject *self, PyObject *args);

/**
 * Extension method: add_function(source_code, filename=None)
 * Adds a Python function to the cache from source code
 */
static PyObject* math_add_function(PyObject *self, PyObject *args, PyObject *kwargs) {
    const char *source_code;
    const char *filename = "<dynamic>";
    
    static char *kwlist[] = {"source_code", "filename", NULL};
    
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|s", kwlist, 
                                    &source_code, &filename)) {
        return NULL;
    }
    
    pyfc_result_t result = pyfc_add_function(source_code, filename);
    
    switch (result) {
        case PYFC_SUCCESS:
            Py_RETURN_TRUE;
        case PYFC_ERROR_NOT_INITIALIZED:
            PyErr_SetString(PyExc_RuntimeError, "Cache not initialized");
            return NULL;
        case PYFC_ERROR_COMPILE:
            PyErr_SetString(PyExc_SyntaxError, "Failed to compile function");
            return NULL;
        case PYFC_ERROR_MEMORY:
            PyErr_SetString(PyExc_MemoryError, "Out of memory");
            return NULL;
        default:
            PyErr_SetString(PyExc_RuntimeError, "Unknown error");
            return NULL;
    }
}

/**
 * Extension method: call_function(name, *args, **kwargs)
 * Calls a cached function with the provided arguments
 */
static PyObject* math_call_function(PyObject *self, PyObject *args, PyObject *kwargs) {
    const char *function_name;
    PyObject *func_args = NULL;
    PyObject *func_kwargs = NULL;
    
    // First argument is the function name
    if (PyTuple_Size(args) < 1) {
        PyErr_SetString(PyExc_TypeError, "Function name required");
        return NULL;
    }
    
    PyObject *name_obj = PyTuple_GetItem(args, 0);
    if (!PyUnicode_Check(name_obj)) {
        PyErr_SetString(PyExc_TypeError, "Function name must be a string");
        return NULL;
    }
    
    function_name = PyUnicode_AsUTF8(name_obj);
    if (!function_name) return NULL;
    
    // Extract remaining arguments
    Py_ssize_t total_args = PyTuple_Size(args);
    if (total_args > 1) {
        func_args = PyTuple_GetSlice(args, 1, total_args);
        if (!func_args) return NULL;
    }
    
    func_kwargs = kwargs; // Pass through kwargs directly
    
    PyObject *result = pyfc_call_function(function_name, func_args, func_kwargs);
    
    Py_XDECREF(func_args);
    return result;
}

/**
 * Extension method: has_function(name)
 * Check if a function exists in the cache
 */
static PyObject* math_has_function(PyObject *self, PyObject *args) {
    const char *function_name;
    
    if (!PyArg_ParseTuple(args, "s", &function_name)) {
        return NULL;
    }
    
    if (pyfc_has_function(function_name)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

/**
 * Extension method: get_stats()
 * Returns cache statistics as a dictionary
 */
static PyObject* math_get_stats(PyObject *self, PyObject *args) {
    pyfc_stats_t stats;
    pyfc_get_stats(&stats);
    
    PyObject *dict = PyDict_New();
    if (!dict) return NULL;
    
    PyDict_SetItemString(dict, "total_entries", PyLong_FromSize_t(stats.total_entries));
    PyDict_SetItemString(dict, "cache_hits", PyLong_FromSize_t(stats.cache_hits));
    PyDict_SetItemString(dict, "cache_misses", PyLong_FromSize_t(stats.cache_misses));
    PyDict_SetItemString(dict, "hit_ratio", PyFloat_FromDouble(stats.hit_ratio));
    
    return dict;
}

/**
 * Extension method: benchmark(function_name, args, iterations=1000)
 * Benchmark a cached function call
 */
static PyObject* math_benchmark(PyObject *self, PyObject *args) {
    const char *function_name;
    PyObject *func_args;
    int iterations = 1000;
    
    if (!PyArg_ParseTuple(args, "sO|i", &function_name, &func_args, &iterations)) {
        return NULL;
    }
    
    if (!pyfc_has_function(function_name)) {
        PyErr_Format(PyExc_KeyError, "Function '%s' not found", function_name);
        return NULL;
    }
    
    clock_t start = clock();
    
    for (int i = 0; i < iterations; i++) {
        PyObject *result = pyfc_call_function(function_name, func_args, NULL);
        if (!result) {
            return NULL; // Error in function call
        }
        Py_DECREF(result);
    }
    
    clock_t end = clock();
    double total_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    double avg_time = total_time / iterations;
    
    PyObject *result_dict = PyDict_New();
    PyDict_SetItemString(result_dict, "total_time", PyFloat_FromDouble(total_time));
    PyDict_SetItemString(result_dict, "average_time", PyFloat_FromDouble(avg_time));
    PyDict_SetItemString(result_dict, "iterations", PyLong_FromLong(iterations));
    PyDict_SetItemString(result_dict, "calls_per_second", PyFloat_FromDouble(iterations / total_time));
    
    return result_dict;
}

// Method definitions
static PyMethodDef math_extension_methods[] = {
    {"add_function", (PyCFunction)math_add_function, METH_VARARGS | METH_KEYWORDS,
     "Add a function to the cache from source code\n\n"
     "Args:\n"
     "    source_code (str): Python source code containing function definition\n"
     "    filename (str, optional): Filename for debugging. Defaults to '<dynamic>'\n\n"
     "Returns:\n"
     "    bool: True if function was added successfully\n\n"
     "Example:\n"
     "    add_function('def square(x): return x*x', 'math.py')"},
    
    {"call_function", (PyCFunction)math_call_function, METH_VARARGS | METH_KEYWORDS,
     "Call a cached function with arguments\n\n"
     "Args:\n"
     "    name (str): Name of the cached function\n"
     "    *args: Positional arguments to pass to the function\n"
     "    **kwargs: Keyword arguments to pass to the function\n\n"
     "Returns:\n"
     "    Any: Result of the function call\n\n"
     "Example:\n"
     "    result = call_function('square', 5)"},
    
    {"has_function", math_has_function, METH_VARARGS,
     "Check if a function exists in the cache\n\n"
     "Args:\n"
     "    name (str): Function name to check\n\n"
     "Returns:\n"
     "    bool: True if function exists in cache"},
    
    {"get_stats", math_get_stats, METH_NOARGS,
     "Get cache statistics\n\n"
     "Returns:\n"
     "    dict: Dictionary with cache statistics\n"
     "        - total_entries: Number of cached functions\n"
     "        - cache_hits: Number of cache hits\n"
     "        - cache_misses: Number of cache misses\n"
     "        - hit_ratio: Cache hit ratio (0.0 to 1.0)"},
    
    {"benchmark", math_benchmark, METH_VARARGS,
     "Benchmark a cached function\n\n"
     "Args:\n"
     "    function_name (str): Name of function to benchmark\n"
     "    args (tuple): Arguments to pass to function\n"
     "    iterations (int, optional): Number of iterations. Defaults to 1000\n\n"
     "Returns:\n"
     "    dict: Benchmark results"},
    
    {NULL, NULL, 0, NULL} // Sentinel
};

// Module initialization for Python 3
static int math_extension_traverse(PyObject *m, visitproc visit, void *arg) {
    Py_VISIT(GET_STATE(m)->error);
    return 0;
}

static int math_extension_clear(PyObject *m) {
    Py_CLEAR(GET_STATE(m)->error);
    return 0;
}

static void math_extension_free(void *m) {
    math_extension_clear((PyObject*)m);
    pyfc_cleanup(); // Clean up the function cache
}

static PyModuleDef math_extension_module = {
    PyModuleDef_HEAD_INIT,
    "math_extension",                           // Module name
    "Example extension using function cache",   // Module docstring
    sizeof(math_extension_state),               // Per-module state size
    math_extension_methods,                     // Method definitions
    NULL,                                       // Reload function
    math_extension_traverse,                    // Traverse function
    math_extension_clear,                       // Clear function
    math_extension_free                         // Free function
};

// Module initialization function
PyMODINIT_FUNC PyInit_math_extension(void) {
    PyObject *module;
    math_extension_state *state;
    
    // Create the module
    module = PyModule_Create(&math_extension_module);
    if (module == NULL) {
        return NULL;
    }
    
    // Initialize module state
    state = GET_STATE(module);
    state->error = PyErr_NewException("math_extension.Error", NULL, NULL);
    if (state->error == NULL) {
        Py_DECREF(module);
        return NULL;
    }
    
    // Add error exception to module
    Py_INCREF(state->error);
    PyModule_AddObject(module, "Error", state->error);
    
    // Initialize the function cache
    pyfc_result_t cache_result = pyfc_init();
    if (cache_result != PYFC_SUCCESS) {
        PyErr_SetString(state->error, "Failed to initialize function cache");
        Py_DECREF(module);
        return NULL;
    }
    
    // Pre-load some common mathematical functions
    const char *common_functions[] = {
        "def add(x, y): return x + y",
        "def multiply(x, y): return x * y",
        "def power(base, exp): return base ** exp",
        "def factorial(n):\n    return 1 if n <= 1 else n * factorial(n - 1)",
        "def fibonacci(n):\n    return n if n <= 1 else fibonacci(n-1) + fibonacci(n-2)",
        NULL
    };
    
    for (int i = 0; common_functions[i] != NULL; i++) {
        pyfc_add_function(common_functions[i], "<builtin>");
    }
    
    // Add module constants
    PyModule_AddStringConstant(module, "__version__", "1.0.0");
    PyModule_AddStringConstant(module, "__author__", "Function Cache System");
    
    return module;
}

