/**
 * @file py_cache_signature_example.c
 * @brief Example demonstrating function signature introspection in py_cache.h
 *
 * This example shows how to extract and use parameter information from cached functions.
 */

#include <Python.h>
#include <stdio.h>
#include "py_cache.h"

void print_function_signature(psc_instance_t *cache, const char *func_name) {
    int arg_count = 0;
    int kwonly_arg_count = 0;
    int has_varargs = 0;
    int has_varkwargs = 0;

    psc_result_t result = psc_get_function_signature(cache, func_name,
                                                     &arg_count,
                                                     &kwonly_arg_count,
                                                     &has_varargs,
                                                     &has_varkwargs);

    if (result != PSC_SUCCESS) {
        printf("Function '%s' not found in cache\n", func_name);
        return;
    }

    printf("\n=== Function: %s ===\n", func_name);
    printf("Positional arguments: %d\n", arg_count);
    printf("Keyword-only arguments: %d\n", kwonly_arg_count);
    printf("Has *args: %s\n", has_varargs ? "yes" : "no");
    printf("Has **kwargs: %s\n", has_varkwargs ? "yes" : "no");

    // Get parameter names
    const char* const* param_names = psc_get_function_param_names(cache, func_name);
    if (param_names) {
        printf("Parameters: ");
        for (int i = 0; param_names[i] != NULL; i++) {
            printf("%s", param_names[i]);
            if (param_names[i + 1] != NULL) printf(", ");
        }
        printf("\n");
    }

    // Get type annotations
    PyObject *annotations = psc_get_function_annotations(cache, func_name);
    if (annotations && PyDict_Size(annotations) > 0) {
        printf("Type annotations: ");
        PyObject *key, *value;
        Py_ssize_t pos = 0;
        while (PyDict_Next(annotations, &pos, &key, &value)) {
            const char *key_str = PyUnicode_AsUTF8(key);
            PyObject *value_repr = PyObject_Repr(value);
            const char *value_str = value_repr ? PyUnicode_AsUTF8(value_repr) : "?";
            printf("%s: %s  ", key_str, value_str);
            Py_XDECREF(value_repr);
        }
        printf("\n");
    }
}

int main(void) {
    // Initialize Python
    Py_Initialize();

    // Create and initialize cache instance
    psc_instance_t *cache = psc_create_instance("example");
    psc_init(cache);
    psc_configure(cache, 25, 1); // Enable debug mode

    printf("=== Function Signature Introspection Demo ===\n");

    // Example 1: Simple function
    const char *simple_func = "def add(x, y):\n    return x + y";
    psc_add_function(cache, simple_func, "simple.py");
    print_function_signature(cache, "add");

    // Example 2: Function with type hints
    const char *typed_func =
        "def multiply(a: int, b: int) -> int:\n"
        "    return a * b";
    psc_add_function(cache, typed_func, "typed.py");
    print_function_signature(cache, "multiply");

    // Example 3: Function with default values
    const char *default_func =
        "def greet(name, greeting='Hello'):\n"
        "    return f'{greeting}, {name}!'";
    psc_add_function(cache, default_func, "default.py");
    print_function_signature(cache, "greet");

    // Example 4: Function with *args
    const char *varargs_func =
        "def sum_all(*numbers):\n"
        "    return sum(numbers)";
    psc_add_function(cache, varargs_func, "varargs.py");
    print_function_signature(cache, "sum_all");

    // Example 5: Function with **kwargs
    const char *kwargs_func =
        "def configure(**options):\n"
        "    return dict(options)";
    psc_add_function(cache, kwargs_func, "kwargs.py");
    print_function_signature(cache, "configure");

    // Example 6: Function with keyword-only arguments
    const char *kwonly_func =
        "def process(data, *, verbose=False, mode='fast'):\n"
        "    return f'Processing {data} in {mode} mode'";
    psc_add_function(cache, kwonly_func, "kwonly.py");
    print_function_signature(cache, "process");

    // Example 7: Complex function with everything
    const char *complex_func =
        "def complex_func(a, b, c=10, *args, d, e=20, **kwargs) -> dict:\n"
        "    return {'result': 'complex'}";
    psc_add_function(cache, complex_func, "complex.py");
    print_function_signature(cache, "complex_func");

    // Cleanup
    psc_destroy_instance(cache);
    Py_Finalize();

    return 0;
}

/**
 * Expected output:
 *
 * === Function Signature Introspection Demo ===
 *
 * === Function: add ===
 * Positional arguments: 2
 * Keyword-only arguments: 0
 * Has *args: no
 * Has **kwargs: no
 * Parameters: x, y
 *
 * === Function: multiply ===
 * Positional arguments: 2
 * Keyword-only arguments: 0
 * Has *args: no
 * Has **kwargs: no
 * Parameters: a, b
 * Type annotations: a: <class 'int'>  b: <class 'int'>  return: <class 'int'>
 *
 * === Function: greet ===
 * Positional arguments: 2
 * Keyword-only arguments: 0
 * Has *args: no
 * Has **kwargs: no
 * Parameters: name, greeting
 *
 * === Function: sum_all ===
 * Positional arguments: 0
 * Keyword-only arguments: 0
 * Has *args: yes
 * Has **kwargs: no
 * Parameters: numbers
 *
 * === Function: configure ===
 * Positional arguments: 0
 * Keyword-only arguments: 0
 * Has *args: no
 * Has **kwargs: yes
 * Parameters: options
 *
 * === Function: process ===
 * Positional arguments: 1
 * Keyword-only arguments: 2
 * Has *args: no
 * Has **kwargs: no
 * Parameters: data, verbose, mode
 *
 * === Function: complex_func ===
 * Positional arguments: 3
 * Keyword-only arguments: 2
 * Has *args: yes
 * Has **kwargs: yes
 * Parameters: a, b, c, args, d, e, kwargs
 * Type annotations: return: <class 'dict'>
 */
