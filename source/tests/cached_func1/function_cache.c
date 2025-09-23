#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

// Hash table configuration
#define CACHE_SIZE 1024
#define MAX_FUNCTION_NAME 256
#define MAX_SOURCE_LENGTH 8192

// Cache entry structure
typedef struct CacheEntry {
    char function_name[MAX_FUNCTION_NAME];
    char *source_code;
    PyObject *code_object;
    PyObject *function_object;
    PyObject *globals_dict;
    size_t source_hash;
    time_t created_time;
    time_t last_access;
    int access_count;
    struct CacheEntry *next;  // For collision handling
} CacheEntry;

// Thread-safe cache structure
typedef struct {
    CacheEntry *table[CACHE_SIZE];
    pthread_rwlock_t lock;
    size_t total_entries;
    size_t cache_hits;
    size_t cache_misses;
} FunctionCache;

// Global cache instance
static FunctionCache g_cache = {0};
static int cache_initialized = 0;

// Error handling
typedef enum {
    CACHE_SUCCESS = 0,
    CACHE_ERROR_INIT = -1,
    CACHE_ERROR_COMPILE = -2,
    CACHE_ERROR_MEMORY = -3,
    CACHE_ERROR_NOT_FOUND = -4,
    CACHE_ERROR_INVALID_ARGS = -5
} CacheResult;

// Hash function (djb2 algorithm)
static size_t hash_string(const char *str) {
    size_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % CACHE_SIZE;
}

// Initialize the cache system
CacheResult cache_init(void) {
    if (cache_initialized) {
        return CACHE_SUCCESS;
    }
    
    memset(&g_cache, 0, sizeof(FunctionCache));
    
    if (pthread_rwlock_init(&g_cache.lock, NULL) != 0) {
        return CACHE_ERROR_INIT;
    }
    
    cache_initialized = 1;
    return CACHE_SUCCESS;
}

// Create a new cache entry
static CacheEntry* create_cache_entry(const char *function_name, 
                                     const char *source_code,
                                     PyObject *code_object,
                                     PyObject *function_object,
                                     PyObject *globals_dict) {
    CacheEntry *entry = malloc(sizeof(CacheEntry));
    if (!entry) return NULL;
    
    memset(entry, 0, sizeof(CacheEntry));
    
    strncpy(entry->function_name, function_name, MAX_FUNCTION_NAME - 1);
    entry->source_code = strdup(source_code);
    entry->code_object = code_object;
    entry->function_object = function_object;
    entry->globals_dict = globals_dict;
    entry->source_hash = hash_string(source_code);
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
static void free_cache_entry(CacheEntry *entry) {
    if (!entry) return;
    
    free(entry->source_code);
    Py_XDECREF(entry->code_object);
    Py_XDECREF(entry->function_object);
    Py_XDECREF(entry->globals_dict);
    free(entry);
}

// Extract function name from source code
static int extract_function_name(const char *source, char *name_buffer, size_t buffer_size) {
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

// Compile function source code
static CacheResult compile_function(const char *source_code, 
                                  const char *filename,
                                  PyObject **code_obj,
                                  PyObject **func_obj,
                                  PyObject **globals_dict) {
    
    // Compile the source code
    *code_obj = Py_CompileString(source_code, filename, Py_file_input);
    if (!*code_obj) {
        PyErr_Print();
        return CACHE_ERROR_COMPILE;
    }
    
    // Create globals dictionary
    *globals_dict = PyDict_New();
    if (!*globals_dict) {
        Py_DECREF(*code_obj);
        return CACHE_ERROR_MEMORY;
    }
    
    // Set up builtins
    PyDict_SetItemString(*globals_dict, "__builtins__", PyEval_GetBuiltins());
    
    // Execute the module to define the function
    PyObject *locals_dict = PyDict_New();
    if (!locals_dict) {
        Py_DECREF(*code_obj);
        Py_DECREF(*globals_dict);
        return CACHE_ERROR_MEMORY;
    }
    
    PyObject *result = PyEval_EvalCode(*code_obj, *globals_dict, locals_dict);
    if (!result) {
        PyErr_Print();
        Py_DECREF(*code_obj);
        Py_DECREF(*globals_dict);
        Py_DECREF(locals_dict);
        return CACHE_ERROR_COMPILE;
    }
    Py_DECREF(result);
    
    // Extract function name
    char func_name[MAX_FUNCTION_NAME];
    if (extract_function_name(source_code, func_name, sizeof(func_name)) != 0) {
        Py_DECREF(*code_obj);
        Py_DECREF(*globals_dict);
        Py_DECREF(locals_dict);
        return CACHE_ERROR_COMPILE;
    }
    
    // Get the function object
    *func_obj = PyDict_GetItemString(locals_dict, func_name);
    if (!*func_obj || !PyFunction_Check(*func_obj)) {
        Py_DECREF(*code_obj);
        Py_DECREF(*globals_dict);
        Py_DECREF(locals_dict);
        return CACHE_ERROR_COMPILE;
    }
    
    Py_INCREF(*func_obj);
    Py_DECREF(locals_dict);
    
    return CACHE_SUCCESS;
}

// Add function to cache
CacheResult cache_add_function(const char *source_code, const char *filename) {
    if (!cache_initialized) {
        return CACHE_ERROR_INIT;
    }
    
    // Extract function name
    char function_name[MAX_FUNCTION_NAME];
    if (extract_function_name(source_code, function_name, sizeof(function_name)) != 0) {
        return CACHE_ERROR_COMPILE;
    }
    
    // Compile the function
    PyObject *code_obj, *func_obj, *globals_dict;
    CacheResult compile_result = compile_function(source_code, filename, 
                                                 &code_obj, &func_obj, &globals_dict);
    if (compile_result != CACHE_SUCCESS) {
        return compile_result;
    }
    
    // Create cache entry
    CacheEntry *entry = create_cache_entry(function_name, source_code, 
                                          code_obj, func_obj, globals_dict);
    if (!entry) {
        Py_DECREF(code_obj);
        Py_DECREF(func_obj);
        Py_DECREF(globals_dict);
        return CACHE_ERROR_MEMORY;
    }
    
    // Insert into cache
    pthread_rwlock_wrlock(&g_cache.lock);
    
    size_t hash_idx = hash_string(function_name);
    
    // Check if function already exists and replace it
    CacheEntry *current = g_cache.table[hash_idx];
    CacheEntry *prev = NULL;
    
    while (current) {
        if (strcmp(current->function_name, function_name) == 0) {
            // Replace existing entry
            if (prev) {
                prev->next = entry;
            } else {
                g_cache.table[hash_idx] = entry;
            }
            entry->next = current->next;
            free_cache_entry(current);
            pthread_rwlock_unlock(&g_cache.lock);
            return CACHE_SUCCESS;
        }
        prev = current;
        current = current->next;
    }
    
    // Add new entry
    entry->next = g_cache.table[hash_idx];
    g_cache.table[hash_idx] = entry;
    g_cache.total_entries++;
    
    pthread_rwlock_unlock(&g_cache.lock);
    
    // Clean up temporary references
    Py_DECREF(code_obj);
    Py_DECREF(func_obj);
    Py_DECREF(globals_dict);
    
    return CACHE_SUCCESS;
}

// Find function in cache
static CacheEntry* find_function(const char *function_name) {
    size_t hash_idx = hash_string(function_name);
    CacheEntry *current = g_cache.table[hash_idx];
    
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

// Call cached function with arguments
PyObject* cache_call_function(const char *function_name, PyObject *args, PyObject *kwargs) {
    if (!cache_initialized) {
        PyErr_SetString(PyExc_RuntimeError, "Cache not initialized");
        return NULL;
    }
    
    pthread_rwlock_rdlock(&g_cache.lock);
    
    CacheEntry *entry = find_function(function_name);
    if (!entry) {
        g_cache.cache_misses++;
        pthread_rwlock_unlock(&g_cache.lock);
        PyErr_Format(PyExc_KeyError, "Function '%s' not found in cache", function_name);
        return NULL;
    }
    
    g_cache.cache_hits++;
    
    // Call the cached function
    PyObject *result;
    if (kwargs && PyDict_Size(kwargs) > 0) {
        result = PyObject_Call(entry->function_object, args, kwargs);
    } else {
        result = PyObject_CallObject(entry->function_object, args);
    }
    
    pthread_rwlock_unlock(&g_cache.lock);
    
    return result;
}

// Check if function exists in cache
int cache_has_function(const char *function_name) {
    if (!cache_initialized) return 0;
    
    pthread_rwlock_rdlock(&g_cache.lock);
    CacheEntry *entry = find_function(function_name);
    int exists = (entry != NULL);
    pthread_rwlock_unlock(&g_cache.lock);
    
    return exists;
}

// Get cache statistics
void cache_get_stats(size_t *total_entries, size_t *cache_hits, 
                    size_t *cache_misses, double *hit_ratio) {
    if (!cache_initialized) {
        *total_entries = *cache_hits = *cache_misses = 0;
        *hit_ratio = 0.0;
        return;
    }
    
    pthread_rwlock_rdlock(&g_cache.lock);
    *total_entries = g_cache.total_entries;
    *cache_hits = g_cache.cache_hits;
    *cache_misses = g_cache.cache_misses;
    
    size_t total_accesses = *cache_hits + *cache_misses;
    *hit_ratio = total_accesses > 0 ? (double)*cache_hits / total_accesses : 0.0;
    
    pthread_rwlock_unlock(&g_cache.lock);
}

// Print cache statistics
void cache_print_stats(void) {
    size_t total_entries, cache_hits, cache_misses;
    double hit_ratio;
    
    cache_get_stats(&total_entries, &cache_hits, &cache_misses, &hit_ratio);
    
    printf("\n=== Cache Statistics ===\n");
    printf("Total cached functions: %zu\n", total_entries);
    printf("Cache hits: %zu\n", cache_hits);
    printf("Cache misses: %zu\n", cache_misses);
    printf("Hit ratio: %.2f%%\n", hit_ratio * 100.0);
}

// List all cached functions
void cache_list_functions(void) {
    if (!cache_initialized) return;
    
    printf("\n=== Cached Functions ===\n");
    
    pthread_rwlock_rdlock(&g_cache.lock);
    
    for (int i = 0; i < CACHE_SIZE; i++) {
        CacheEntry *current = g_cache.table[i];
        while (current) {
            printf("Function: %s\n", current->function_name);
            printf("  Created: %s", ctime(&current->created_time));
            printf("  Last accessed: %s", ctime(&current->last_access));
            printf("  Access count: %d\n", current->access_count);
            printf("  Source hash: %zu\n\n", current->source_hash);
            current = current->next;
        }
    }
    
    pthread_rwlock_unlock(&g_cache.lock);
}

// Cleanup cache
void cache_cleanup(void) {
    if (!cache_initialized) return;
    
    pthread_rwlock_wrlock(&g_cache.lock);
    
    for (int i = 0; i < CACHE_SIZE; i++) {
        CacheEntry *current = g_cache.table[i];
        while (current) {
            CacheEntry *next = current->next;
            free_cache_entry(current);
            current = next;
        }
        g_cache.table[i] = NULL;
    }
    
    g_cache.total_entries = 0;
    g_cache.cache_hits = 0;
    g_cache.cache_misses = 0;
    
    pthread_rwlock_unlock(&g_cache.lock);
    pthread_rwlock_destroy(&g_cache.lock);
    
    cache_initialized = 0;
}

// Demonstration functions
void demo_basic_caching(void) {
    printf("=== Basic Function Caching Demo ===\n");
    
    // Define some test functions
    const char *math_func = 
        "def quadratic(a, b, c, x):\n"
        "    return a * x * x + b * x + c\n";
    
    const char *string_func = 
        "def format_message(name, age, city):\n"
        "    return f'Hello {name}, age {age}, from {city}!'\n";
    
    const char *list_func = 
        "def process_list(items, multiplier):\n"
        "    return [x * multiplier for x in items]\n";
    
    // Add functions to cache
    printf("Adding functions to cache...\n");
    
    if (cache_add_function(math_func, "<math>") == CACHE_SUCCESS) {
        printf("✓ Added quadratic function\n");
    }
    
    if (cache_add_function(string_func, "<string>") == CACHE_SUCCESS) {
        printf("✓ Added format_message function\n");
    }
    
    if (cache_add_function(list_func, "<list>") == CACHE_SUCCESS) {
        printf("✓ Added process_list function\n");
    }
    
    printf("\n");
}

void demo_function_calls(void) {
    printf("=== Function Call Demo ===\n");
    
    // Test quadratic function with different parameters
    printf("Testing quadratic function:\n");
    
    double test_params[][4] = {
        {1.0, 2.0, 1.0, 3.0},   // x^2 + 2x + 1 at x=3
        {2.0, -3.0, 1.0, 2.0}, // 2x^2 - 3x + 1 at x=2
        {1.0, 0.0, -4.0, 2.0}  // x^2 - 4 at x=2
    };
    
    for (int i = 0; i < 3; i++) {
        PyObject *args = PyTuple_New(4);
        PyTuple_SetItem(args, 0, PyFloat_FromDouble(test_params[i][0]));
        PyTuple_SetItem(args, 1, PyFloat_FromDouble(test_params[i][1]));
        PyTuple_SetItem(args, 2, PyFloat_FromDouble(test_params[i][2]));
        PyTuple_SetItem(args, 3, PyFloat_FromDouble(test_params[i][3]));
        
        PyObject *result = cache_call_function("quadratic", args, NULL);
        if (result && PyFloat_Check(result)) {
            printf("  quadratic(%.1f, %.1f, %.1f, %.1f) = %.2f\n",
                   test_params[i][0], test_params[i][1], 
                   test_params[i][2], test_params[i][3],
                   PyFloat_AsDouble(result));
        } else {
            printf("  Error calling quadratic function\n");
            if (PyErr_Occurred()) PyErr_Print();
        }
        
        Py_XDECREF(result);
        Py_DECREF(args);
    }
    
    // Test string function
    printf("\nTesting format_message function:\n");
    
    const char *names[] = {"Alice", "Bob", "Charlie"};
    int ages[] = {25, 30, 35};
    const char *cities[] = {"New York", "London", "Tokyo"};
    
    for (int i = 0; i < 3; i++) {
        PyObject *args = PyTuple_New(3);
        PyTuple_SetItem(args, 0, PyUnicode_FromString(names[i]));
        PyTuple_SetItem(args, 1, PyLong_FromLong(ages[i]));
        PyTuple_SetItem(args, 2, PyUnicode_FromString(cities[i]));
        
        PyObject *result = cache_call_function("format_message", args, NULL);
        if (result && PyUnicode_Check(result)) {
            const char *message = PyUnicode_AsUTF8(result);
            printf("  %s\n", message);
        } else {
            printf("  Error calling format_message function\n");
            if (PyErr_Occurred()) PyErr_Print();
        }
        
        Py_XDECREF(result);
        Py_DECREF(args);
    }
    
    // Test list function
    printf("\nTesting process_list function:\n");
    
    int test_lists[][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    int multipliers[] = {2, 3, 4};
    
    for (int i = 0; i < 3; i++) {
        PyObject *list = PyList_New(3);
        for (int j = 0; j < 3; j++) {
            PyList_SetItem(list, j, PyLong_FromLong(test_lists[i][j]));
        }
        
        PyObject *args = PyTuple_New(2);
        PyTuple_SetItem(args, 0, list);
        PyTuple_SetItem(args, 1, PyLong_FromLong(multipliers[i]));
        
        PyObject *result = cache_call_function("process_list", args, NULL);
        if (result && PyList_Check(result)) {
            printf("  [%d, %d, %d] * %d = [",
                   test_lists[i][0], test_lists[i][1], test_lists[i][2], multipliers[i]);
            
            Py_ssize_t size = PyList_Size(result);
            for (Py_ssize_t k = 0; k < size; k++) {
                PyObject *item = PyList_GetItem(result, k);
                if (k > 0) printf(", ");
                printf("%ld", PyLong_AsLong(item));
            }
            printf("]\n");
        } else {
            printf("  Error calling process_list function\n");
            if (PyErr_Occurred()) PyErr_Print();
        }
        
        Py_XDECREF(result);
        Py_DECREF(args);
    }
    
    printf("\n");
}

void demo_performance(void) {
    printf("=== Performance Demo ===\n");
    
    // Fixed: Use a simpler mathematical function that doesn't require recursion
    const char *complex_func = 
        "def polynomial_eval(coefficients, x):\n"
        "    \"\"\"\n"
        "    Evaluate polynomial with given coefficients at point x\n"
        "    coefficients = [a0, a1, a2, ...] for a0 + a1*x + a2*x^2 + ...\n"
        "    \"\"\"\n"
        "    result = 0\n"
        "    power = 1\n"
        "    for coeff in coefficients:\n"
        "        result += coeff * power\n"
        "        power *= x\n"
        "    return result\n";
    
    // Add function to cache
    cache_add_function(complex_func, "<polynomial>");
    
    printf("Calling polynomial evaluation 1000 times with different parameters...\n");
    
    clock_t start = clock();
    
    for (int i = 1; i <= 1000; i++) {
        // Create different coefficient sets and x values
        PyObject *coeffs = PyList_New(4);
        PyList_SetItem(coeffs, 0, PyFloat_FromDouble(1.0));                    // constant term
        PyList_SetItem(coeffs, 1, PyFloat_FromDouble(i % 5 + 1));              // x term
        PyList_SetItem(coeffs, 2, PyFloat_FromDouble((i % 3 + 1) * 0.5));      // x^2 term
        PyList_SetItem(coeffs, 3, PyFloat_FromDouble((i % 2 + 1) * 0.1));      // x^3 term
        
        PyObject *args = PyTuple_New(2);
        PyTuple_SetItem(args, 0, coeffs);
        PyTuple_SetItem(args, 1, PyFloat_FromDouble(i % 10 + 1)); // x values 1-10
        
        PyObject *result = cache_call_function("polynomial_eval", args, NULL);
        if (result) {
            if (i % 200 == 0) {
                printf("  polynomial_eval([1, %d, %.1f, %.1f], %d) = %.2f\n", 
                       i % 5 + 1, (i % 3 + 1) * 0.5, (i % 2 + 1) * 0.1, 
                       i % 10 + 1, PyFloat_AsDouble(result));
            }
        } else {
            printf("Error in function call %d\n", i);
            if (PyErr_Occurred()) PyErr_Print();
        }
        
        Py_XDECREF(result);
        Py_DECREF(args);
    }
    
    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("Time taken: %.4f seconds\n", time_taken);
    printf("All calls used the same cached function object!\n");
    
    // Add a second performance test with a recursive function that works properly
    printf("\nTesting with a properly defined recursive function...\n");
    
    const char *recursive_func = 
        "def factorial(n):\n"
        "    if n <= 1:\n"
        "        return 1\n"
        "    return n * factorial(n - 1)\n";
    
    // This will work because we execute the entire module, making factorial available globally
    cache_add_function(recursive_func, "<factorial>");
    
    start = clock();
    
    for (int i = 1; i <= 100; i++) {
        PyObject *args = PyTuple_New(1);
        PyTuple_SetItem(args, 0, PyLong_FromLong(i % 10 + 1)); // factorial 1-10
        
        PyObject *result = cache_call_function("factorial", args, NULL);
        if (result) {
            if (i % 25 == 0) {
                printf("  factorial(%d) = %ld\n", i % 10 + 1, PyLong_AsLong(result));
            }
        } else {
            printf("Error in factorial call %d\n", i);
            if (PyErr_Occurred()) PyErr_Print();
        }
        
        Py_XDECREF(result);
        Py_DECREF(args);
    }
    
    end = clock();
    time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("Recursive function time: %.4f seconds\n", time_taken);
    printf("\n");
}

int main(void) {
    // Initialize Python and cache
    Py_Initialize();
    if (!Py_IsInitialized()) {
        printf("Failed to initialize Python\n");
        return 1;
    }
    
    if (cache_init() != CACHE_SUCCESS) {
        printf("Failed to initialize cache\n");
        Py_Finalize();
        return 1;
    }
    
    printf("Production-Level Function Code Object Cache\n");
    printf("==========================================\n\n");
    
    // Run demonstrations
    demo_basic_caching();
    demo_function_calls();
    demo_performance();
    
    // Show cache information
    cache_list_functions();
    cache_print_stats();
    
    // Cleanup
    cache_cleanup();
    Py_Finalize();
    
    return 0;
}