#ifndef FUNCTION_CACHE_H
#define FUNCTION_CACHE_H

#include <Python.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Error codes
typedef enum {
    CACHE_SUCCESS = 0,
    CACHE_ERROR_INIT = -1,
    CACHE_ERROR_COMPILE = -2,
    CACHE_ERROR_MEMORY = -3,
    CACHE_ERROR_NOT_FOUND = -4,
    CACHE_ERROR_INVALID_ARGS = -5
} CacheResult;

// Core cache functions
CacheResult cache_init(void);
void cache_cleanup(void);

// Function management
CacheResult cache_add_function(const char *source_code, const char *filename);
PyObject* cache_call_function(const char *function_name, PyObject *args, PyObject *kwargs);
int cache_has_function(const char *function_name);

// Cache information and statistics
void cache_get_stats(size_t *total_entries, size_t *cache_hits, 
                    size_t *cache_misses, double *hit_ratio);
void cache_print_stats(void);
void cache_list_functions(void);

// Convenience macros for common operations
#define CACHE_CALL_SIMPLE(func_name, ...) do { \
    PyObject *_args = PyTuple_Pack(__VA_ARGS__); \
    PyObject *_result = cache_call_function(func_name, _args, NULL); \
    Py_XDECREF(_args); \
    Py_XDECREF(_result); \
} while(0)

#define CACHE_CALL_WITH_RESULT(result, func_name, ...) do { \
    PyObject *_args = PyTuple_Pack(__VA_ARGS__); \
    result = cache_call_function(func_name, _args, NULL); \
    Py_XDECREF(_args); \
} while(0)

// Helper function to create common argument types
static inline PyObject* make_args_1(PyObject *arg1) {
    PyObject *args = PyTuple_New(1);
    PyTuple_SetItem(args, 0, arg1);
    return args;
}

static inline PyObject* make_args_2(PyObject *arg1, PyObject *arg2) {
    PyObject *args = PyTuple_New(2);
    PyTuple_SetItem(args, 0, arg1);
    PyTuple_SetItem(args, 1, arg2);
    return args;
}

static inline PyObject* make_args_3(PyObject *arg1, PyObject *arg2, PyObject *arg3) {
    PyObject *args = PyTuple_New(3);
    PyTuple_SetItem(args, 0, arg1);
    PyTuple_SetItem(args, 1, arg2);
    PyTuple_SetItem(args, 2, arg3);
    return args;
}

#ifdef __cplusplus
}
#endif

#endif // FUNCTION_CACHE_H
