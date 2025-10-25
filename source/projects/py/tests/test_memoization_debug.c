#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <stdio.h>

void print_error() {
    if (PyErr_Occurred()) {
        printf("ERROR DETAILS:\n");
        PyErr_Print();
        PyErr_Clear();
    }
}

int main() {
    Py_Initialize();
    
    const char *source = "def fib(n):\n    if n <= 1:\n        return n\n    else:\n        return fib(n-1) + fib(n-2)\n";
    
    PyObject *code = Py_CompileString(source, "test.py", Py_file_input);
    if (!code) { print_error(); return 1; }
    
    PyObject *globals = PyDict_New();
    PyDict_SetItemString(globals, "__builtins__", PyEval_GetBuiltins());
    PyObject *locals = PyDict_New();
    
    PyObject *result = PyEval_EvalCode(code, globals, locals);
    if (!result) { print_error(); return 1; }
    Py_DECREF(result);
    
    PyObject *func_obj = PyDict_GetItemString(locals, "fib");
    if (!func_obj) { printf("No func\n"); return 1; }
    Py_INCREF(func_obj);
    
    // Add to globals FIRST (for recursion)
    PyDict_SetItemString(globals, "fib", func_obj);
    printf("✓ Added to globals (refcnt=%ld)\n", Py_REFCNT(func_obj));
    
    // Now apply memoization
    PyObject *functools = PyImport_ImportModule("functools");
    if (!functools) { print_error(); return 1; }
    
    PyObject *lru_cache = PyObject_GetAttrString(functools, "lru_cache");
    if (!lru_cache) { print_error(); return 1; }
    
    PyObject *kwargs = PyDict_New();
    PyObject *maxsize = PyLong_FromLong(128);
    PyDict_SetItemString(kwargs, "maxsize", maxsize);
    Py_DECREF(maxsize);
    
    PyObject *empty_tuple = PyTuple_New(0);
    printf("✓ Created empty tuple (refcnt=%ld)\n", Py_REFCNT(empty_tuple));
    
    PyObject *decorator = PyObject_Call(lru_cache, empty_tuple, kwargs);
    Py_DECREF(empty_tuple);
    Py_DECREF(kwargs);
    
    if (!decorator) {
        printf("✗ Decorator creation failed\n");
        print_error();
        return 1;
    }
    printf("✓ Created decorator (refcnt=%ld)\n", Py_REFCNT(decorator));
    
    // THE CRITICAL PART: Apply decorator
    printf("Before decoration: func_obj refcnt=%ld\n", Py_REFCNT(func_obj));
    
    PyObject *args = PyTuple_Pack(1, func_obj);
    printf("Created args tuple (refcnt=%ld), func in tuple refcnt=%ld\n", 
           Py_REFCNT(args), Py_REFCNT(func_obj));
    
    PyObject *memoized_func = PyObject_CallObject(decorator, args);
    
    if (!memoized_func) {
        printf("✗ Decoration failed!\n");
        print_error();
        Py_DECREF(args);
        return 1;
    }
    
    printf("✓ Decorated successfully! (refcnt=%ld)\n", Py_REFCNT(memoized_func));
    Py_DECREF(args);
    Py_DECREF(decorator);
    
    // Test it
    PyObject *test_args = PyTuple_Pack(1, PyLong_FromLong(10));
    PyObject *fib_result = PyObject_CallObject(memoized_func, test_args);
    Py_DECREF(test_args);
    
    if (!fib_result) {
        printf("✗ Call failed\n");
        print_error();
        return 1;
    }
    
    printf("✓ fib(10) = %ld\n", PyLong_AsLong(fib_result));
    Py_DECREF(fib_result);
    
    Py_DECREF(memoized_func);
    Py_DECREF(func_obj);
    Py_DECREF(lru_cache);
    Py_DECREF(functools);
    Py_DECREF(locals);
    Py_DECREF(globals);
    Py_DECREF(code);
    
    Py_Finalize();
    return 0;
}
