/**
 * @file py_function_cache.h
 * @brief Header-only Python function code object cache for C extensions
 * @version 1.0
 * @author Production Function Cache System
 * 
 * This header-only library provides a thread-safe, high-performance cache
 * for compiled Python functions that can be integrated into any Python C extension.
 * 
 * Features:
 * - Thread-safe hash table with rwlock synchronization
 * - Automatic function compilation and caching
 * - Support for positional and keyword arguments
 * - Memory leak prevention with proper reference counting
 * - Performance statistics and monitoring
 * - Easy integration into existing C extensions
 * 
 * Usage:
 * 1. Include this header in your C extension
 * 2. Call pyfc_init() during module initialization
 * 3. Use pyfc_add_function() to cache functions
 * 4. Call pyfc_call_function() to execute with different parameters
 * 5. Call pyfc_cleanup() during module cleanup
 * 
 * Example:
 *   pyfc_init();
 *   pyfc_add_function("def square(x): return x*x", "square.py");
 *   PyObject* result = pyfc_call_function("square", args, NULL);
 *   pyfc_cleanup();
 */

#ifndef PY_FUNCTION_CACHE_H
#define PY_FUNCTION_CACHE_H

#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Thread support - detect if threading is available
#ifdef _WIN32
    #include <windows.h>
    typedef SRWLOCK pyfc_rwlock_t;
    #define PYFC_RWLOCK_INIT SRWLOCK_INIT
    #define pyfc_rwlock_init(lock) InitializeSRWLock(lock)
    #define pyfc_rwlock_rdlock(lock) AcquireSRWLockShared(lock)
    #define pyfc_rwlock_wrlock(lock) AcquireSRWLockExclusive(lock)
    #define pyfc_rwlock_unlock_rd(lock) ReleaseSRWLockShared(lock)
    #define pyfc_rwlock_unlock_wr(lock) ReleaseSRWLockExclusive(lock)
    #define pyfc_rwlock_destroy(lock) (void)0
#elif defined(__unix__) || defined(__APPLE__)
    #include <pthread.h>
    typedef pthread_rwlock_t pyfc_rwlock_t;
    #define PYFC_RWLOCK_INIT PTHREAD_RWLOCK_INITIALIZER
    #define pyfc_rwlock_init(lock) pthread_rwlock_init(lock, NULL)
    #define pyfc_rwlock_rdlock(lock) pthread_rwlock_rdlock(lock)
    #define pyfc_rwlock_wrlock(lock) pthread_rwlock_wrlock(lock)
    #define pyfc_rwlock_unlock_rd(lock) pthread_rwlock_unlock(lock)
    #define pyfc_rwlock_unlock_wr(lock) pthread_rwlock_unlock(lock)
    #define pyfc_rwlock_destroy(lock) pthread_rwlock_destroy(lock)
#else
    // No threading support - use dummy macros
    typedef int pyfc_rwlock_t;
    #define PYFC_RWLOCK_INIT 0
    #define pyfc_rwlock_init(lock) (void)0
    #define pyfc_rwlock_rdlock(lock) (void)0
    #define pyfc_rwlock_wrlock(lock) (void)0
    #define pyfc_rwlock_unlock_rd(lock) (void)0
    #define pyfc_rwlock_unlock_wr(lock) (void)0
    #define pyfc_rwlock_destroy(lock) (void)0
#endif

// Configuration constants
#ifndef PYFC_CACHE_SIZE
#define PYFC_CACHE_SIZE 1024
#endif

#ifndef PYFC_MAX_FUNCTION_NAME
#define PYFC_MAX_FUNCTION_NAME 256
#endif

#ifndef PYFC_MAX_SOURCE_LENGTH
#define PYFC_MAX_SOURCE_LENGTH 8192
#endif

// Error codes
typedef enum {
    PYFC_SUCCESS = 0,
    PYFC_ERROR_INIT = -1,
    PYFC_ERROR_COMPILE = -2,
    PYFC_ERROR_MEMORY = -3,
    PYFC_ERROR_NOT_FOUND = -4,
    PYFC_ERROR_INVALID_ARGS = -5,
    PYFC_ERROR_NOT_INITIALIZED = -6
} pyfc_result_t;

// Cache entry structure
typedef struct pyfc_entry {
    char function_name[PYFC_MAX_FUNCTION_NAME];
    char *source_code;
    PyObject *code_object;
    PyObject *function_object;
    PyObject *globals_dict;
    size_t source_hash;
    time_t created_time;
    time_t last_access;
    int access_count;
    struct pyfc_entry *next;
} pyfc_entry_t;

// Cache statistics
typedef struct {
    size_t total_entries;
    size_t cache_hits;
    size_t cache_misses;
    double hit_ratio;
} pyfc_stats_t;

// Main cache structure
typedef struct {
    pyfc_entry_t *table[PYFC_CACHE_SIZE];
    pyfc_rwlock_t lock;
    size_t total_entries;
    size_t cache_hits;
    size_t cache_misses;
    int initialized;
} pyfc_cache_t;

// Global cache instance
static pyfc_cache_t g_pyfc_cache = {0};

// Hash function (djb2 algorithm)
static inline size_t pyfc_hash_string(const char *str) {
    size_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % PYFC_CACHE_SIZE;
}

// Extract function name from source code
static inline int pyfc_extract_function_name(const char *source, char *name_buffer, size_t buffer_size) {
    const char *def_pos = strstr(source, "def ");
    if (!def_pos) return -1;
    
    def_pos += 4; // Skip "def "
    
    // Skip whitespace
    while (*def_pos && (*def_pos == ' ' || *def_pos == '\t')) {
        def_pos++;
    }
    
    // Extract function name
    size_t i = 0;
    while (*def_pos && *def_pos != '(' && *def_pos != ' ' && 
           *def_pos != '\t' && i < buffer_size - 1) {
        name_buffer[i++] = *def_pos++;
    }
    name_buffer[i] = '\0';
    
    return (i > 0) ? 0 : -1;
}

// Create a new cache entry
static inline pyfc_entry_t* pyfc_create_entry(const char *function_name, 
                                             const char *source_code,
                                             PyObject *code_object,
                                             PyObject *function_object,
                                             PyObject *globals_dict) {
    pyfc_entry_t *entry = (pyfc_entry_t*)malloc(sizeof(pyfc_entry_t));
    if (!entry) return NULL;
    
    memset(entry, 0, sizeof(pyfc_entry_t));
    
    strncpy(entry->function_name, function_name, PYFC_MAX_FUNCTION_NAME - 1);
    entry->source_code = strdup(source_code);
    entry->code_object = code_object;
    entry->function_object = function_object;
    entry->globals_dict = globals_dict;
    entry->source_hash = pyfc_hash_string(source_code);
    entry->created_time = time(NULL);
    entry->last_access = entry->created_time;
    entry->access_count = 0;
    entry->next = NULL;
    
    // Increment reference counts
    Py_INCREF(code_object);
    Py_INCREF(function_object);
    Py_INCREF(globals_dict);
    
    return entry;
}

// Free a cache entry
static inline void pyfc_free_entry(pyfc_entry_t *entry) {
    if (!entry) return;
    
    free(entry->source_code);
    Py_XDECREF(entry->code_object);
    Py_XDECREF(entry->function_object);
    Py_XDECREF(entry->globals_dict);
    free(entry);
}

// Compile function source code
static inline pyfc_result_t pyfc_compile_function(const char *source_code, 
                                                 const char *filename,
                                                 PyObject **code_obj,
                                                 PyObject **func_obj,
                                                 PyObject **globals_dict) {
    
    // Compile the source code
    *code_obj = Py_CompileString(source_code, filename, Py_file_input);
    if (!*code_obj) {
        return PYFC_ERROR_COMPILE;
    }
    
    // Create globals dictionary with isolated namespace
    *globals_dict = PyDict_New();
    if (!*globals_dict) {
        Py_DECREF(*code_obj);
        return PYFC_ERROR_MEMORY;
    }
    
    // Set up builtins
    PyDict_SetItemString(*globals_dict, "__builtins__", PyEval_GetBuiltins());
    
    // Execute the module to define the function
    PyObject *locals_dict = PyDict_New();
    if (!locals_dict) {
        Py_DECREF(*code_obj);
        Py_DECREF(*globals_dict);
        return PYFC_ERROR_MEMORY;
    }
    
    PyObject *result = PyEval_EvalCode(*code_obj, *globals_dict, locals_dict);
    if (!result) {
        Py_DECREF(*code_obj);
        Py_DECREF(*globals_dict);
        Py_DECREF(locals_dict);
        return PYFC_ERROR_COMPILE;
    }
    Py_DECREF(result);
    
    // Extract function name
    char func_name[PYFC_MAX_FUNCTION_NAME];
    if (pyfc_extract_function_name(source_code, func_name, sizeof(func_name)) != 0) {
        Py_DECREF(*code_obj);
        Py_DECREF(*globals_dict);
        Py_DECREF(locals_dict);
        return PYFC_ERROR_COMPILE;
    }
    
    // Get the function object
    *func_obj = PyDict_GetItemString(locals_dict, func_name);
    if (!*func_obj || !PyFunction_Check(*func_obj)) {
        Py_DECREF(*code_obj);
        Py_DECREF(*globals_dict);
        Py_DECREF(locals_dict);
        return PYFC_ERROR_COMPILE;
    }
    
    Py_INCREF(*func_obj);
    Py_DECREF(locals_dict);
    
    return PYFC_SUCCESS;
}

// Find function in cache (assumes lock is held)
static inline pyfc_entry_t* pyfc_find_function_unlocked(const char *function_name) {
    size_t hash_idx = pyfc_hash_string(function_name);
    pyfc_entry_t *current = g_pyfc_cache.table[hash_idx];
    
    while (current) {
        if (strcmp(current->function_name, function_name) == 0) {
            current->last_access = time(NULL);
            current->access_count++;
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

/**
 * @brief Initialize the function cache system
 * @return PYFC_SUCCESS on success, error code on failure
 * 
 * This function must be called before any other cache operations.
 * Typically called during Python module initialization.
 */
static inline pyfc_result_t pyfc_init(void) {
    if (g_pyfc_cache.initialized) {
        return PYFC_SUCCESS;
    }
    
    memset(&g_pyfc_cache, 0, sizeof(pyfc_cache_t));
    
    if (pyfc_rwlock_init(&g_pyfc_cache.lock) != 0) {
        return PYFC_ERROR_INIT;
    }
    
    g_pyfc_cache.initialized = 1;
    return PYFC_SUCCESS;
}

/**
 * @brief Add a function to the cache
 * @param source_code Python source code containing function definition
 * @param filename Filename for debugging (can be descriptive string)
 * @return PYFC_SUCCESS on success, error code on failure
 * 
 * Compiles the Python source code and caches the resulting function.
 * If a function with the same name already exists, it will be replaced.
 * 
 * Example:
 *   pyfc_add_function("def square(x): return x*x", "math_funcs.py");
 */
static inline pyfc_result_t pyfc_add_function(const char *source_code, const char *filename) {
    if (!g_pyfc_cache.initialized) {
        return PYFC_ERROR_NOT_INITIALIZED;
    }
    
    // Extract function name
    char function_name[PYFC_MAX_FUNCTION_NAME];
    if (pyfc_extract_function_name(source_code, function_name, sizeof(function_name)) != 0) {
        return PYFC_ERROR_COMPILE;
    }
    
    // Compile the function
    PyObject *code_obj, *func_obj, *globals_dict;
    pyfc_result_t compile_result = pyfc_compile_function(source_code, filename, 
                                                        &code_obj, &func_obj, &globals_dict);
    if (compile_result != PYFC_SUCCESS) {
        return compile_result;
    }
    
    // Create cache entry
    pyfc_entry_t *entry = pyfc_create_entry(function_name, source_code, 
                                           code_obj, func_obj, globals_dict);
    if (!entry) {
        Py_DECREF(code_obj);
        Py_DECREF(func_obj);
        Py_DECREF(globals_dict);
        return PYFC_ERROR_MEMORY;
    }
    
    // Insert into cache with write lock
    pyfc_rwlock_wrlock(&g_pyfc_cache.lock);
    
    size_t hash_idx = pyfc_hash_string(function_name);
    
    // Check if function already exists and replace it
    pyfc_entry_t *current = g_pyfc_cache.table[hash_idx];
    pyfc_entry_t *prev = NULL;
    
    while (current) {
        if (strcmp(current->function_name, function_name) == 0) {
            // Replace existing entry
            if (prev) {
                prev->next = entry;
            } else {
                g_pyfc_cache.table[hash_idx] = entry;
            }
            entry->next = current->next;
            pyfc_free_entry(current);
            pyfc_rwlock_unlock_wr(&g_pyfc_cache.lock);
            
            // Clean up temporary references
            Py_DECREF(code_obj);
            Py_DECREF(func_obj);
            Py_DECREF(globals_dict);
            return PYFC_SUCCESS;
        }
        prev = current;
        current = current->next;
    }
    
    // Add new entry
    entry->next = g_pyfc_cache.table[hash_idx];
    g_pyfc_cache.table[hash_idx] = entry;
    g_pyfc_cache.total_entries++;
    
    pyfc_rwlock_unlock_wr(&g_pyfc_cache.lock);
    
    // Clean up temporary references
    Py_DECREF(code_obj);
    Py_DECREF(func_obj);
    Py_DECREF(globals_dict);
    
    return PYFC_SUCCESS;
}

/**
 * @brief Call a cached function with arguments
 * @param function_name Name of the cached function
 * @param args Tuple of positional arguments (can be NULL for no args)
 * @param kwargs Dictionary of keyword arguments (can be NULL)
 * @return PyObject* result of function call, or NULL on error
 * 
 * Calls the cached function with the provided arguments.
 * The caller is responsible for decrementing the returned object's reference count.
 * 
 * Example:
 *   PyObject* args = PyTuple_Pack(1, PyLong_FromLong(5));
 *   PyObject* result = pyfc_call_function("square", args, NULL);
 *   // result now contains 25
 *   Py_DECREF(args);
 *   Py_XDECREF(result);
 */
static inline PyObject* pyfc_call_function(const char *function_name, PyObject *args, PyObject *kwargs) {
    if (!g_pyfc_cache.initialized) {
        PyErr_SetString(PyExc_RuntimeError, "Function cache not initialized");
        return NULL;
    }
    
    pyfc_rwlock_rdlock(&g_pyfc_cache.lock);
    
    pyfc_entry_t *entry = pyfc_find_function_unlocked(function_name);
    if (!entry) {
        g_pyfc_cache.cache_misses++;
        pyfc_rwlock_unlock_rd(&g_pyfc_cache.lock);
        PyErr_Format(PyExc_KeyError, "Function '%s' not found in cache", function_name);
        return NULL;
    }
    
    g_pyfc_cache.cache_hits++;
    
    // Call the cached function
    PyObject *result;
    if (kwargs && PyDict_Size(kwargs) > 0) {
        result = PyObject_Call(entry->function_object, args ? args : PyTuple_New(0), kwargs);
    } else {
        result = PyObject_CallObject(entry->function_object, args);
    }
    
    pyfc_rwlock_unlock_rd(&g_pyfc_cache.lock);
    
    return result;
}

/**
 * @brief Check if a function exists in the cache
 * @param function_name Name of the function to check
 * @return 1 if function exists, 0 otherwise
 */
static inline int pyfc_has_function(const char *function_name) {
    if (!g_pyfc_cache.initialized) return 0;
    
    pyfc_rwlock_rdlock(&g_pyfc_cache.lock);
    pyfc_entry_t *entry = pyfc_find_function_unlocked(function_name);
    int exists = (entry != NULL);
    pyfc_rwlock_unlock_rd(&g_pyfc_cache.lock);
    
    return exists;
}

/**
 * @brief Get cache statistics
 * @param stats Pointer to pyfc_stats_t structure to fill
 */
static inline void pyfc_get_stats(pyfc_stats_t *stats) {
    if (!g_pyfc_cache.initialized || !stats) {
        if (stats) memset(stats, 0, sizeof(pyfc_stats_t));
        return;
    }
    
    pyfc_rwlock_rdlock(&g_pyfc_cache.lock);
    stats->total_entries = g_pyfc_cache.total_entries;
    stats->cache_hits = g_pyfc_cache.cache_hits;
    stats->cache_misses = g_pyfc_cache.cache_misses;
    
    size_t total_accesses = stats->cache_hits + stats->cache_misses;
    stats->hit_ratio = total_accesses > 0 ? (double)stats->cache_hits / total_accesses : 0.0;
    
    pyfc_rwlock_unlock_rd(&g_pyfc_cache.lock);
}

/**
 * @brief Clean up the cache system
 * 
 * Frees all cached functions and cleans up resources.
 * Should be called during module cleanup.
 */
static inline void pyfc_cleanup(void) {
    if (!g_pyfc_cache.initialized) return;
    
    pyfc_rwlock_wrlock(&g_pyfc_cache.lock);
    
    for (int i = 0; i < PYFC_CACHE_SIZE; i++) {
        pyfc_entry_t *current = g_pyfc_cache.table[i];
        while (current) {
            pyfc_entry_t *next = current->next;
            pyfc_free_entry(current);
            current = next;
        }
        g_pyfc_cache.table[i] = NULL;
    }
    
    g_pyfc_cache.total_entries = 0;
    g_pyfc_cache.cache_hits = 0;
    g_pyfc_cache.cache_misses = 0;
    
    pyfc_rwlock_unlock_wr(&g_pyfc_cache.lock);
    pyfc_rwlock_destroy(&g_pyfc_cache.lock);
    
    g_pyfc_cache.initialized = 0;
}

// Convenience macros for common operations
#define PYFC_CALL_NOARGS(func_name) \
    pyfc_call_function(func_name, NULL, NULL)

#define PYFC_CALL_1ARG(func_name, arg1) do { \
    PyObject *_args = PyTuple_Pack(1, arg1); \
    PyObject *_result = pyfc_call_function(func_name, _args, NULL); \
    Py_XDECREF(_args); \
    return _result; \
} while(0)

#define PYFC_CALL_2ARGS(func_name, arg1, arg2) do { \
    PyObject *_args = PyTuple_Pack(2, arg1, arg2); \
    PyObject *_result = pyfc_call_function(func_name, _args, NULL); \
    Py_XDECREF(_args); \
    return _result; \
} while(0)

// Helper functions for creating common argument types
static inline PyObject* pyfc_make_args_1(PyObject *arg1) {
    PyObject *args = PyTuple_New(1);
    PyTuple_SetItem(args, 0, arg1);
    return args;
}

static inline PyObject* pyfc_make_args_2(PyObject *arg1, PyObject *arg2) {
    PyObject *args = PyTuple_New(2);
    PyTuple_SetItem(args, 0, arg1);
    PyTuple_SetItem(args, 1, arg2);
    return args;
}

static inline PyObject* pyfc_make_args_3(PyObject *arg1, PyObject *arg2, PyObject *arg3) {
    PyObject *args = PyTuple_New(3);
    PyTuple_SetItem(args, 0, arg1);
    PyTuple_SetItem(args, 1, arg2);
    PyTuple_SetItem(args, 2, arg3);
    return args;
}

#endif // PY_FUNCTION_CACHE_H

