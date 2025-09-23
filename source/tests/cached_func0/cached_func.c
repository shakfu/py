#include <Python.h>
#include <stdio.h>

// Structure to hold cached code objects
typedef struct {
    PyObject *code_object;
    char *source_hash;  // Simple hash of source for cache validation
} CodeCache;

// Simple cache with fixed size for demonstration
#define CACHE_SIZE 10
static CodeCache cache[CACHE_SIZE];
static int cache_initialized = 0;

// Initialize the cache
void init_cache() {
    if (!cache_initialized) {
        for (int i = 0; i < CACHE_SIZE; i++) {
            cache[i].code_object = NULL;
            cache[i].source_hash = NULL;
        }
        cache_initialized = 1;
    }
}

// Simple hash function for source code
unsigned int simple_hash(const char *str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

// Find cached code object
PyObject* find_cached_code(const char *source) {
    char hash_str[32];
    snprintf(hash_str, sizeof(hash_str), "%u", simple_hash(source));
    
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].source_hash && 
            strcmp(cache[i].source_hash, hash_str) == 0) {
            Py_INCREF(cache[i].code_object);
            return cache[i].code_object;
        }
    }
    return NULL;
}

// Cache a code object
void cache_code_object(const char *source, PyObject *code_obj) {
    char hash_str[32];
    snprintf(hash_str, sizeof(hash_str), "%u", simple_hash(source));
    
    // Find empty slot or replace oldest (simple strategy)
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].code_object == NULL) {
            cache[i].source_hash = strdup(hash_str);
            cache[i].code_object = code_obj;
            Py_INCREF(code_obj);
            return;
        }
    }
    
    // Replace first entry if cache is full (simple eviction)
    if (cache[0].source_hash) free(cache[0].source_hash);
    if (cache[0].code_object) Py_DECREF(cache[0].code_object);
    
    cache[0].source_hash = strdup(hash_str);
    cache[0].code_object = code_obj;
    Py_INCREF(code_obj);
}

// Compile source code to code object with caching
PyObject* compile_with_cache(const char *source, const char *filename, int mode) {
    // Check cache first
    PyObject *cached = find_cached_code(source);
    if (cached) {
        printf("Cache hit! Returning cached code object.\n");
        return cached;
    }
    
    printf("Cache miss. Compiling source code.\n");
    
    // Compile the source code
    PyObject *code_obj = Py_CompileString(source, filename, mode);
    if (!code_obj) {
        PyErr_Print();
        return NULL;
    }
    
    // Cache the compiled code object
    cache_code_object(source, code_obj);
    
    return code_obj;
}

// Execute code object with given globals and locals
PyObject* execute_code_object(PyObject *code_obj, PyObject *globals, PyObject *locals) {
    if (!PyCode_Check(code_obj)) {
        PyErr_SetString(PyExc_TypeError, "Expected code object");
        return NULL;
    }
    
    return PyEval_EvalCode(code_obj, globals, locals);
}

// Demonstration function
void demonstrate_caching() {
    printf("=== Python C API Code Object Caching Demo ===\n\n");
    
    init_cache();
    
    // Example 1: Mathematical function
    const char *math_source = 
        "def calculate(x, y):\n"
        "    return x * x + y * y + x * y\n"
        "result = calculate(a, b)";
    
    // Example 2: String processing function  
    const char *string_source = 
        "def process_text(text, multiplier):\n"
        "    return (text.upper() + '!') * multiplier\n"
        "result = process_text(text, count)";
    
    // Compile both functions (cache miss expected)
    PyObject *math_code = compile_with_cache(math_source, "<math>", Py_file_input);
    PyObject *string_code = compile_with_cache(string_source, "<string>", Py_file_input);
    
    if (!math_code || !string_code) {
        printf("Compilation failed!\n");
        return;
    }
    
    // Create different parameter sets for testing
    PyObject *main_module = PyImport_AddModule("__main__");
    PyObject *globals = PyModule_GetDict(main_module);
    
    printf("\n--- Testing Math Function with Different Parameters ---\n");
    
    // Test math function with parameters (3, 4)
    PyObject *locals1 = PyDict_New();
    PyDict_SetItemString(locals1, "a", PyLong_FromLong(3));
    PyDict_SetItemString(locals1, "b", PyLong_FromLong(4));
    
    // This should use cached code object
    PyObject *cached_math_code = compile_with_cache(math_source, "<math>", Py_file_input);
    PyObject *result1 = execute_code_object(cached_math_code, globals, locals1);
    
    if (result1) {
        PyObject *calc_result = PyDict_GetItemString(locals1, "result");
        if (calc_result) {
            printf("calculate(3, 4) = %ld\n", PyLong_AsLong(calc_result));
        }
    }
    
    // Test math function with parameters (5, 6)
    PyObject *locals2 = PyDict_New();
    PyDict_SetItemString(locals2, "a", PyLong_FromLong(5));
    PyDict_SetItemString(locals2, "b", PyLong_FromLong(6));
    
    // This should also use cached code object
    PyObject *cached_math_code2 = compile_with_cache(math_source, "<math>", Py_file_input);
    PyObject *result2 = execute_code_object(cached_math_code2, globals, locals2);
    
    if (result2) {
        PyObject *calc_result = PyDict_GetItemString(locals2, "result");
        if (calc_result) {
            printf("calculate(5, 6) = %ld\n", PyLong_AsLong(calc_result));
        }
    }
    
    printf("\n--- Testing String Function with Different Parameters ---\n");
    
    // Test string function with different parameters
    PyObject *locals3 = PyDict_New();
    PyDict_SetItemString(locals3, "text", PyUnicode_FromString("hello"));
    PyDict_SetItemString(locals3, "count", PyLong_FromLong(3));
    
    PyObject *cached_string_code = compile_with_cache(string_source, "<string>", Py_file_input);
    PyObject *result3 = execute_code_object(cached_string_code, globals, locals3);
    
    if (result3) {
        PyObject *str_result = PyDict_GetItemString(locals3, "result");
        if (str_result) {
            const char *result_str = PyUnicode_AsUTF8(str_result);
            printf("process_text('hello', 3) = %s\n", result_str);
        }
    }
    
    // Test with different parameters
    PyObject *locals4 = PyDict_New();
    PyDict_SetItemString(locals4, "text", PyUnicode_FromString("world"));
    PyDict_SetItemString(locals4, "count", PyLong_FromLong(2));
    
    PyObject *cached_string_code2 = compile_with_cache(string_source, "<string>", Py_file_input);
    PyObject *result4 = execute_code_object(cached_string_code2, globals, locals4);
    
    if (result4) {
        PyObject *str_result = PyDict_GetItemString(locals4, "result");
        if (str_result) {
            const char *result_str = PyUnicode_AsUTF8(str_result);
            printf("process_text('world', 2) = %s\n", result_str);
        }
    }
    
    // Cleanup
    Py_DECREF(math_code);
    Py_DECREF(string_code);
    Py_DECREF(cached_math_code);
    Py_DECREF(cached_math_code2);
    Py_DECREF(cached_string_code);
    Py_DECREF(cached_string_code2);
    Py_DECREF(locals1);
    Py_DECREF(locals2);
    Py_DECREF(locals3);
    Py_DECREF(locals4);
}

// Performance comparison function
void performance_comparison() {
    printf("\n=== Performance Comparison ===\n");
    
    const char *source = 
        "def fibonacci(n):\n"
        "    if n <= 1: return n\n"
        "    return fibonacci(n-1) + fibonacci(n-2)\n"
        "result = fibonacci(num)";
    
    clock_t start, end;
    int iterations = 1000;
    
    // Test without caching (compile every time)
    printf("Testing without caching (%d iterations)...\n", iterations);
    start = clock();
    
    for (int i = 0; i < iterations; i++) {
        PyObject *code = Py_CompileString(source, "<test>", Py_file_input);
        if (code) {
            Py_DECREF(code);
        }
    }
    
    end = clock();
    double time_without_cache = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    // Test with caching
    printf("Testing with caching (%d iterations)...\n", iterations);
    start = clock();
    
    for (int i = 0; i < iterations; i++) {
        PyObject *code = compile_with_cache(source, "<test>", Py_file_input);
        if (code) {
            Py_DECREF(code);
        }
    }
    
    end = clock();
    double time_with_cache = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("Time without caching: %.4f seconds\n", time_without_cache);
    printf("Time with caching: %.4f seconds\n", time_with_cache);
    printf("Speedup: %.2fx\n", time_without_cache / time_with_cache);
}

// Cleanup function
void cleanup_cache() {
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (cache[i].source_hash) {
            free(cache[i].source_hash);
            cache[i].source_hash = NULL;
        }
        if (cache[i].code_object) {
            Py_DECREF(cache[i].code_object);
            cache[i].code_object = NULL;
        }
    }
}

int main() {
    // Initialize Python
    Py_Initialize();
    if (!Py_IsInitialized()) {
        printf("Failed to initialize Python\n");
        return 1;
    }
    
    // Run demonstrations
    demonstrate_caching();
    performance_comparison();
    
    // Cleanup
    cleanup_cache();
    Py_Finalize();
    
    return 0;
}
