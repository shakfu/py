/**
 * @file py_cache.h
 * @brief Instance-based Python function cache with smart caching decisions
 * @version 4.0
 * @author Smart Function Cache System
 * 
 * A complete, production-ready, thread-safe function caching system that uses
 * instance-based design instead of global state. Each cache instance is independent
 * and must be passed as the first parameter to all functions.
 * 
 * Features:
 * - Instance-based design - no global state
 * - Multiple independent cache instances supported
 * - Automatic smart caching decisions based on source code analysis
 * - Thread-safe with cross-platform locking
 * - Unified data structure for all cache state
 * - Memory leak prevention with proper reference counting
 * - Performance statistics and monitoring
 * - Configurable complexity thresholds
 * - Comprehensive error handling
 * 
 * Usage:
 *   #include "py_cache.h"
 * 
 *   // Create instance
 *   psc_instance_t* cache = psc_create_instance();
 * 
 *   // Initialize
 *   psc_init(cache);
 * 
 *   // Add functions (automatically decides whether to cache)
 *   psc_add_function(cache, "def square(x): return x*x", "math.py");
 * 
 *   // Call functions
 *   PyObject* result = psc_call_function(cache, "square", args, NULL);
 *   
 *   // Reset and re-use cache
 *   psc_reset(cache);
 * 
 *   // Cleanup and destroy
 *   psc_destroy_instance(cache);
 */

#ifndef PY_CACHE_H
#define PY_CACHE_H

#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "ext.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CONFIGURATION AND PLATFORM DETECTION
// ============================================================================

// Configuration constants - override before including this header
#ifndef PSC_CACHE_SIZE
#define PSC_CACHE_SIZE 1024
#endif

#ifndef PSC_MAX_FUNCTION_NAME
#define PSC_MAX_FUNCTION_NAME 256
#endif

#ifndef PSC_MAX_SOURCE_LENGTH
#define PSC_MAX_SOURCE_LENGTH 16384
#endif

#ifndef PSC_CACHE_THRESHOLD_SCORE
#define PSC_CACHE_THRESHOLD_SCORE 25
#endif

#ifndef PSC_MIN_LINES_FOR_CACHE
#define PSC_MIN_LINES_FOR_CACHE 3
#endif

#ifndef PSC_MAX_SIMPLE_LENGTH
#define PSC_MAX_SIMPLE_LENGTH 80
#endif

#ifndef PSC_MAX_EXPLANATION_LENGTH
#define PSC_MAX_EXPLANATION_LENGTH 2048
#endif

#ifndef PSC_MAX_DECISON_REASON_LENGTH
#define PSC_MAX_DECISON_REASON_LENGTH 256
#endif

// Thread support detection and abstraction
#ifdef _WIN32
    #include <windows.h>
    typedef SRWLOCK psc_rwlock_t;
    #define PSC_RWLOCK_INIT SRWLOCK_INIT
    #define psc_rwlock_init(lock) InitializeSRWLock(lock)
    #define psc_rwlock_rdlock(lock) AcquireSRWLockShared(lock)
    #define psc_rwlock_wrlock(lock) AcquireSRWLockExclusive(lock)
    #define psc_rwlock_unlock_rd(lock) ReleaseSRWLockShared(lock)
    #define psc_rwlock_unlock_wr(lock) ReleaseSRWLockExclusive(lock)
    #define psc_rwlock_destroy(lock) (void)0
#elif defined(__unix__) || defined(__APPLE__)
    #include <pthread.h>
    typedef pthread_rwlock_t psc_rwlock_t;
    #define PSC_RWLOCK_INIT PTHREAD_RWLOCK_INITIALIZER
    #define psc_rwlock_init(lock) pthread_rwlock_init(lock, NULL)
    #define psc_rwlock_rdlock(lock) pthread_rwlock_rdlock(lock)
    #define psc_rwlock_wrlock(lock) pthread_rwlock_wrlock(lock)
    #define psc_rwlock_unlock_rd(lock) pthread_rwlock_unlock(lock)
    #define psc_rwlock_unlock_wr(lock) pthread_rwlock_unlock(lock)
    #define psc_rwlock_destroy(lock) pthread_rwlock_destroy(lock)
#else
    // No threading support - use dummy macros
    typedef int psc_rwlock_t;
    #define PSC_RWLOCK_INIT 0
    #define psc_rwlock_init(lock) (void)0
    #define psc_rwlock_rdlock(lock) (void)0
    #define psc_rwlock_wrlock(lock) (void)0
    #define psc_rwlock_unlock_rd(lock) (void)0
    #define psc_rwlock_unlock_wr(lock) (void)0
    #define psc_rwlock_destroy(lock) (void)0
#endif

// ============================================================================
// UNIFIED DATA STRUCTURE
// ============================================================================

/**
 * @brief Cache instance structure - contains all data for one cache instance
 * 
 * This structure contains all data needed for the smart caching system:
 * - Cache entries and hash table
 * - Performance statistics  
 * - Complexity analysis results
 * - Configuration and state
 * - Thread synchronization
 */
typedef struct psc_instance {
    // === CORE CACHE DATA ===
    struct psc_cache_entry {
        char function_name[PSC_MAX_FUNCTION_NAME];
        char *source_code;
        PyObject *code_object;
        PyObject *function_object;
        PyObject *globals_dict;
        size_t source_hash;
        time_t created_time;
        time_t last_access;
        int access_count;
        struct psc_cache_entry *next;
        
        // Embedded complexity analysis for this entry
        struct {
            int line_count;
            int char_count;
            int import_statements;
            int decorator_count;
            int loop_constructs;
            int comprehensions;
            int lambda_functions;
            int complex_operations;
            int nested_functions;
            int exception_handling;
            int string_operations;
            int mathematical_operations;
            int builtin_function_calls;
            int complexity_score;
            int was_cached;
            char decision_reason[PSC_MAX_DECISON_REASON_LENGTH];
        } analysis;
    } *hash_table[PSC_CACHE_SIZE];
    
    // === PERFORMANCE STATISTICS ===
    struct {
        size_t total_entries;
        size_t cache_hits;
        size_t cache_misses;
        size_t functions_analyzed;
        size_t functions_cached;
        size_t functions_skipped;
        double total_analysis_time;
        double total_compilation_time;
        double hit_ratio;
        double cache_efficiency;
    } stats;
    
    // === CONFIGURATION ===
    struct {
        int cache_threshold_score;
        int min_lines_for_cache;
        int max_simple_length;
        int adaptive_threshold;
        int debug_mode;
    } config;
    
    // === SYSTEM STATE ===
    struct {
        int initialized;
        psc_rwlock_t lock;
        time_t system_start_time;
        char last_error[512];
        char instance_name[64];
    } state;
    
    // === COMPLEXITY ANALYSIS WORKSPACE ===
    struct {
        char explanation_buffer[PSC_MAX_EXPLANATION_LENGTH];
        char temp_function_name[PSC_MAX_FUNCTION_NAME];
        int pattern_counts[32]; // Reusable array for pattern counting
    } workspace;
    
} psc_instance_t;

// ============================================================================
// ERROR CODES
// ============================================================================

typedef enum {
    PSC_SUCCESS = 0,
    PSC_ERROR_INIT = -1,
    PSC_ERROR_COMPILE = -2,
    PSC_ERROR_MEMORY = -3,
    PSC_ERROR_NOT_FOUND = -4,
    PSC_ERROR_INVALID_ARGS = -5,
    PSC_ERROR_NOT_INITIALIZED = -6,
    PSC_ERROR_FUNCTION_TOO_SIMPLE = -7,
    PSC_ERROR_NULL_INSTANCE = -8
} psc_result_t;


// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Hash function (djb2 algorithm)
 */
static inline size_t psc_hash_string(const char *str) {
    size_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % PSC_CACHE_SIZE;
}

/**
 * Count occurrences of a pattern in source code
 */
static inline int psc_count_pattern(const char *source, const char *pattern) {
    int count = 0;
    const char *p = source;
    size_t pattern_len = strlen(pattern);
    
    while ((p = strstr(p, pattern)) != NULL) {
        count++;
        p += pattern_len;
    }
    
    return count;
}

/**
 * Count lines in source code
 */
static inline int psc_count_lines(const char *source) {
    int lines = 1;
    const char *p = source;
    
    while (*p) {
        if (*p == '\n') {
            lines++;
        }
        p++;
    }
    
    return lines;
}

/**
 * Extract function name from source code
 */
static inline int psc_extract_function_name(psc_instance_t *instance, const char *source) {
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
           *def_pos != '\t' && i < PSC_MAX_FUNCTION_NAME - 1) {
        instance->workspace.temp_function_name[i++] = *def_pos++;
    }
    instance->workspace.temp_function_name[i] = '\0';
    
    return (i > 0) ? 0 : -1;
}

// ============================================================================
// COMPLEXITY ANALYSIS ENGINE
// ============================================================================

/**
 * Perform comprehensive complexity analysis of source code
 */
static inline void psc_analyze_complexity(psc_instance_t *instance, const char *source_code, 
                                         struct psc_cache_entry *entry) {
    // Reset analysis
    memset(&entry->analysis, 0, sizeof(entry->analysis));
    
    // Basic metrics
    entry->analysis.line_count = psc_count_lines(source_code);
    entry->analysis.char_count = (int)strlen(source_code);
    
    // Import statements (very expensive compilation cost)
    entry->analysis.import_statements = psc_count_pattern(source_code, "import ") +
                                       psc_count_pattern(source_code, "from ");
    
    // Decorators (moderate compilation cost)
    entry->analysis.decorator_count = psc_count_pattern(source_code, "@");
    
    // Control flow constructs
    entry->analysis.loop_constructs = psc_count_pattern(source_code, " for ") +
                                     psc_count_pattern(source_code, " while ") +
                                     psc_count_pattern(source_code, "\nfor ") +
                                     psc_count_pattern(source_code, "\nwhile ");
    
    // Comprehensions (moderate cost)
    int total_for = psc_count_pattern(source_code, " for ");
    entry->analysis.comprehensions = total_for - entry->analysis.loop_constructs;
    if (entry->analysis.comprehensions < 0) entry->analysis.comprehensions = 0;
    
    // Lambda functions
    entry->analysis.lambda_functions = psc_count_pattern(source_code, "lambda ");
    
    // Complex language features
    entry->analysis.complex_operations = 
        (strstr(source_code, "yield") != NULL) +
        (strstr(source_code, "async") != NULL) +
        (strstr(source_code, "await") != NULL) +
        (strstr(source_code, "with ") != NULL) +
        (strstr(source_code, "class ") != NULL);
    
    // Nested functions
    entry->analysis.nested_functions = psc_count_pattern(source_code, "\n    def ") +
                                      psc_count_pattern(source_code, "\n        def ");
    
    // Exception handling
    entry->analysis.exception_handling = psc_count_pattern(source_code, "try:") +
                                        psc_count_pattern(source_code, "except") +
                                        psc_count_pattern(source_code, "finally") +
                                        psc_count_pattern(source_code, "raise ");
    
    // String operations
    entry->analysis.string_operations = psc_count_pattern(source_code, ".format(") +
                                       psc_count_pattern(source_code, "f\"") +
                                       psc_count_pattern(source_code, "f'") +
                                       psc_count_pattern(source_code, ".join(") +
                                       psc_count_pattern(source_code, ".split(");
    
    // Mathematical operations
    entry->analysis.mathematical_operations = 
        psc_count_pattern(source_code, "math.") +
        psc_count_pattern(source_code, "numpy.") +
        psc_count_pattern(source_code, "scipy.") +
        psc_count_pattern(source_code, "**") +
        psc_count_pattern(source_code, "sqrt") +
        psc_count_pattern(source_code, "sin") +
        psc_count_pattern(source_code, "cos") +
        psc_count_pattern(source_code, "log");
    
    // Builtin function calls
    entry->analysis.builtin_function_calls = 
        psc_count_pattern(source_code, "len(") +
        psc_count_pattern(source_code, "range(") +
        psc_count_pattern(source_code, "sum(") +
        psc_count_pattern(source_code, "max(") +
        psc_count_pattern(source_code, "min(") +
        psc_count_pattern(source_code, "sorted(") +
        psc_count_pattern(source_code, "enumerate(");
    
    // Calculate complexity score
    int score = 0;
    
    // Line count contribution
    if (entry->analysis.line_count > 5) {
        score += (entry->analysis.line_count - 5) * 2;
    }
    
    // Weighted feature contributions
    score += entry->analysis.import_statements * 50;        // Very expensive
    score += entry->analysis.decorator_count * 20;          // Expensive
    score += entry->analysis.complex_operations * 25;       // Very expensive
    score += entry->analysis.nested_functions * 15;         // Expensive
    score += entry->analysis.exception_handling * 12;       // Expensive
    score += entry->analysis.lambda_functions * 10;         // Moderate
    score += entry->analysis.loop_constructs * 8;           // Moderate
    score += entry->analysis.comprehensions * 6;            // Moderate
    score += entry->analysis.string_operations * 4;         // Light
    score += entry->analysis.mathematical_operations * 3;   // Light
    score += entry->analysis.builtin_function_calls * 2;    // Very light
    
    entry->analysis.complexity_score = score;
}

/**
 * Generate detailed explanation of caching decision
 */
static inline void psc_generate_explanation(struct psc_cache_entry *entry) {
    snprintf_zero(entry->analysis.decision_reason, sizeof(entry->analysis.decision_reason),
        "Score:%d Lines:%d Imports:%d Decorators:%d Complex:%d Loops:%d",
        entry->analysis.complexity_score,
        entry->analysis.line_count,
        entry->analysis.import_statements,
        entry->analysis.decorator_count,
        entry->analysis.complex_operations,
        entry->analysis.loop_constructs
    );
}

/**
 * Smart caching decision engine
 */
static inline int psc_should_cache_function(psc_instance_t *instance, const char *source_code, 
                                           struct psc_cache_entry *entry) {
    if (!source_code || strlen(source_code) == 0) {
        strncpy_zero(entry->analysis.decision_reason, "Empty source code",
            PSC_MAX_DECISON_REASON_LENGTH);         
        return 0;
    }
    
    // Perform complexity analysis
    psc_analyze_complexity(instance, source_code, entry);
    
    // Quick filters for obviously simple functions
    size_t length = strlen(source_code);
    
    // Very short functions without imports or decorators
    if ((int)length < instance->config.max_simple_length && 
        entry->analysis.import_statements == 0 && 
        entry->analysis.decorator_count == 0) {
        strncpy_zero(entry->analysis.decision_reason, "Too short and simple",
            PSC_MAX_DECISON_REASON_LENGTH);
        return 0;
    }
    
    // Single-line functions without complexity
    if (entry->analysis.line_count < instance->config.min_lines_for_cache && 
        entry->analysis.import_statements == 0 &&
        entry->analysis.lambda_functions == 0 &&
        entry->analysis.complex_operations == 0) {
        strncpy_zero(entry->analysis.decision_reason, "Too few lines, no complexity",
            PSC_MAX_DECISON_REASON_LENGTH);
        return 0;
    }
    
    // Always cache functions with high-cost features
    if (entry->analysis.import_statements > 0) {
        strncpy_zero(entry->analysis.decision_reason, "Has import statements",
            PSC_MAX_DECISON_REASON_LENGTH);        
        return 1;
    }
    
    if (entry->analysis.decorator_count > 0) {
        strncpy_zero(entry->analysis.decision_reason, "Has decorators",
            PSC_MAX_DECISON_REASON_LENGTH);
        return 1;
    }
    
    if (entry->analysis.complex_operations > 0) {
        strncpy_zero(entry->analysis.decision_reason, "Has complex operations",
            PSC_MAX_DECISON_REASON_LENGTH);            
        return 1;
    }
    
    if (entry->analysis.nested_functions > 0) {
        strncpy_zero(entry->analysis.decision_reason, "Has nested functions",
            PSC_MAX_DECISON_REASON_LENGTH);
        return 1;
    }
    
    // Use threshold for borderline cases
    int threshold = instance->config.adaptive_threshold ? 
                   instance->config.cache_threshold_score : 
                   PSC_CACHE_THRESHOLD_SCORE;
    
    if (entry->analysis.complexity_score >= threshold) {
        psc_generate_explanation(entry);
        return 1;
    } else {
        psc_generate_explanation(entry);
        return 0;
    }
}

// ============================================================================
// MEMORY MANAGEMENT
// ============================================================================

/**
 * Create a new cache entry
 */
static inline struct psc_cache_entry* psc_create_entry(const char *function_name, 
                                                      const char *source_code,
                                                      PyObject *code_object,
                                                      PyObject *function_object,
                                                      PyObject *globals_dict) {
    struct psc_cache_entry *entry = malloc(sizeof(struct psc_cache_entry));
    if (!entry) return NULL;
    
    memset(entry, 0, sizeof(struct psc_cache_entry));
    
    strncpy_zero(entry->function_name, function_name, PSC_MAX_FUNCTION_NAME - 1);
    entry->source_code = strdup(source_code);
    entry->code_object = code_object;
    entry->function_object = function_object;
    entry->globals_dict = globals_dict;
    entry->source_hash = psc_hash_string(source_code);
    entry->created_time = time(NULL);
    entry->last_access = entry->created_time;
    entry->access_count = 0;
    entry->next = NULL;
    
    // Increment Python reference counts
    Py_INCREF(code_object);
    Py_INCREF(function_object);
    Py_INCREF(globals_dict);
    
    return entry;
}

/**
 * Free a cache entry and all associated memory
 */
static inline void psc_free_entry(struct psc_cache_entry *entry) {
    if (!entry) return;
    
    free(entry->source_code);
    Py_XDECREF(entry->code_object);
    Py_XDECREF(entry->function_object);
    Py_XDECREF(entry->globals_dict);
    free(entry);
}

// ============================================================================
// INSTANCE MANAGEMENT
// ============================================================================

/**
 * @brief Create a new cache instance
 * @param instance_name Optional name for the instance (can be NULL)
 * @return Pointer to new instance or NULL on failure
 */
static inline psc_instance_t* psc_create_instance(const char *instance_name) {
    psc_instance_t *instance = malloc(sizeof(psc_instance_t));
    if (!instance) return NULL;
    
    memset(instance, 0, sizeof(psc_instance_t));
    
    // Set instance name
    if (instance_name) {
        strncpy_zero(instance->state.instance_name, instance_name, sizeof(instance->state.instance_name) - 1);
    } else {
        snprintf_zero(instance->state.instance_name, sizeof(instance->state.instance_name), 
                "cache_%p", (void*)instance);
    }
    
    return instance;
}

/**
 * @brief Destroy cache instance and free all resources
 * @param instance Cache instance to destroy (can be NULL)
 * 
 * This function automatically cleans up the cache contents before
 * freeing the instance memory. Safe to call even if not initialized.
 */
static inline void psc_destroy_instance(psc_instance_t *instance) {
    if (!instance) return;
    
    if (instance->config.debug_mode) {
        post("PSC[%s]: Destroying cache instance\n", instance->state.instance_name);
    }
    
    // Automatically cleanup if initialized
    if (instance->state.initialized) {
        psc_rwlock_wrlock(&instance->state.lock);
        
        // Free all cache entries
        for (int i = 0; i < PSC_CACHE_SIZE; i++) {
            struct psc_cache_entry *current = instance->hash_table[i];
            while (current) {
                struct psc_cache_entry *next = current->next;
                psc_free_entry(current);
                current = next;
            }
            instance->hash_table[i] = NULL;
        }
        
        psc_rwlock_unlock_wr(&instance->state.lock);
        psc_rwlock_destroy(&instance->state.lock);
    }
    
    // Free the instance itself
    free(instance);
}

// ============================================================================
// CORE COMPILATION ENGINE
// ============================================================================

/**
 * Compile Python source code to function object
 */
static inline psc_result_t psc_compile_function(psc_instance_t *instance, const char *source_code, 
                                               const char *filename,
                                               PyObject **code_obj,
                                               PyObject **func_obj,
                                               PyObject **globals_dict) {
    
    clock_t compile_start = clock();
    
    // Compile the source code to bytecode
    *code_obj = Py_CompileString(source_code, filename, Py_file_input);
    if (!*code_obj) {
        return PSC_ERROR_COMPILE;
    }
    
    // Create isolated globals dictionary
    *globals_dict = PyDict_New();
    if (!*globals_dict) {
        Py_DECREF(*code_obj);
        return PSC_ERROR_MEMORY;
    }
    
    // Set up builtins and commonly needed modules in isolated namespace
    PyDict_SetItemString(*globals_dict, "__builtins__", PyEval_GetBuiltins());
    
    // Import commonly used modules into the globals to prevent NameError
    // This ensures modules like 'math', 'time', etc. are available
    PyRun_String("import builtins", Py_file_input, *globals_dict, *globals_dict);
    
    // Add commonly used modules that functions might reference
    const char *common_modules[] = {"math", "time", "random", "os", "sys", "json", "re", NULL};
    for (int i = 0; common_modules[i] != NULL; i++) {
        PyObject *module = PyImport_ImportModule(common_modules[i]);
        if (module) {
            PyDict_SetItemString(*globals_dict, common_modules[i], module);
            Py_DECREF(module);
        }
    }
    
    // Clear any import errors (non-critical - some modules may not be available)
    if (PyErr_Occurred()) {
        PyErr_Clear();
    }
    
    // Create temporary locals for function definition execution
    PyObject *locals_dict = PyDict_New();
    if (!locals_dict) {
        Py_DECREF(*code_obj);
        Py_DECREF(*globals_dict);
        return PSC_ERROR_MEMORY;
    }
    
    // Execute the module code to define the function
    PyObject *result = PyEval_EvalCode(*code_obj, *globals_dict, locals_dict);
    if (!result) {
        // Print compilation error for debugging
        if (PyErr_Occurred()) {
            PyObject *ptype, *pvalue, *ptraceback;
            PyErr_Fetch(&ptype, &pvalue, &ptraceback);
            
            if (pvalue) {
                PyObject *str_exc_value = PyObject_Str(pvalue);
                if (str_exc_value) {
                    const char *error_msg = PyUnicode_AsUTF8(str_exc_value);
                    if (error_msg && instance->config.debug_mode) {
                        error("PSC[%s]: Compilation error: %s\n", 
                               instance->state.instance_name, error_msg);
                    }
                    Py_DECREF(str_exc_value);
                }
            }
            
            Py_XDECREF(ptype);
            Py_XDECREF(pvalue); 
            Py_XDECREF(ptraceback);
        }
        
        Py_DECREF(*code_obj);
        Py_DECREF(*globals_dict);
        Py_DECREF(locals_dict);
        return PSC_ERROR_COMPILE;
    }
    Py_DECREF(result);
    
    // Extract function name from source
    if (psc_extract_function_name(instance, source_code) != 0) {
        Py_DECREF(*code_obj);
        Py_DECREF(*globals_dict);
        Py_DECREF(locals_dict);
        return PSC_ERROR_COMPILE;
    }
    
    // Get the function object from locals
    *func_obj = PyDict_GetItemString(locals_dict, instance->workspace.temp_function_name);
    if (!*func_obj || !PyFunction_Check(*func_obj)) {
        Py_DECREF(*code_obj);
        Py_DECREF(*globals_dict);
        Py_DECREF(locals_dict);
        return PSC_ERROR_COMPILE;
    }
    
    Py_INCREF(*func_obj);
    Py_DECREF(locals_dict);
    
    // Update compilation time statistics
    clock_t compile_end = clock();
    instance->stats.total_compilation_time += 
        ((double)(compile_end - compile_start)) / CLOCKS_PER_SEC;
    
    return PSC_SUCCESS;
}

// ============================================================================
// CACHE OPERATIONS
// ============================================================================

/**
 * Find function in cache (assumes lock is held)
 */
static inline struct psc_cache_entry* psc_find_function_unlocked(psc_instance_t *instance, 
                                                                const char *function_name) {
    size_t hash_idx = psc_hash_string(function_name);
    struct psc_cache_entry *current = instance->hash_table[hash_idx];
    
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

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * @brief Initialize a cache instance for first use
 * @param instance Cache instance to initialize
 * @return PSC_SUCCESS on success, error code on failure
 */
static inline psc_result_t psc_init(psc_instance_t *instance) {
    if (!instance) return PSC_ERROR_NULL_INSTANCE;
    
    if (instance->state.initialized) {
        return PSC_SUCCESS; // Already initialized
    }
    
    // Initialize threading
    if (psc_rwlock_init(&instance->state.lock) != 0) {
        return PSC_ERROR_INIT;
    }
    
    // Set default configuration
    instance->config.cache_threshold_score = PSC_CACHE_THRESHOLD_SCORE;
    instance->config.min_lines_for_cache = PSC_MIN_LINES_FOR_CACHE;
    instance->config.max_simple_length = PSC_MAX_SIMPLE_LENGTH;
    instance->config.adaptive_threshold = 0;
    instance->config.debug_mode = 0;
    
    // Initialize system state
    instance->state.initialized = 1;
    instance->state.system_start_time = time(NULL);
    
    return PSC_SUCCESS;
}

/**
 * @brief Reset cache instance to clean state for reuse
 * @param instance Cache instance to reset
 * @return PSC_SUCCESS on success, error code on failure
 * 
 * This function combines cleanup + reinit in one atomic operation.
 * After calling this, the cache is ready for immediate reuse with
 * fresh statistics and empty function cache.
 */
static inline psc_result_t psc_reset(psc_instance_t *instance) {
    if (!instance) return PSC_ERROR_NULL_INSTANCE;
    
    if (instance->config.debug_mode) {
        printf("PSC[%s]: Resetting cache instance\n", instance->state.instance_name);
    }
    
    // Step 1: Cleanup existing state if initialized
    if (instance->state.initialized) {
        psc_rwlock_wrlock(&instance->state.lock);
        
        // Free all cache entries
        for (int i = 0; i < PSC_CACHE_SIZE; i++) {
            struct psc_cache_entry *current = instance->hash_table[i];
            while (current) {
                struct psc_cache_entry *next = current->next;
                psc_free_entry(current);
                current = next;
            }
            instance->hash_table[i] = NULL;
        }
        
        // Reset statistics but preserve configuration
        int old_threshold = instance->config.cache_threshold_score;
        int old_min_lines = instance->config.min_lines_for_cache;
        int old_max_simple = instance->config.max_simple_length;
        int old_adaptive = instance->config.adaptive_threshold;
        int old_debug = instance->config.debug_mode;
        
        memset(&instance->stats, 0, sizeof(instance->stats));
        
        psc_rwlock_unlock_wr(&instance->state.lock);
        psc_rwlock_destroy(&instance->state.lock);
        
        // Step 2: Reinitialize fresh state
        if (psc_rwlock_init(&instance->state.lock) != 0) {
            return PSC_ERROR_INIT;
        }
        
        // Restore configuration
        instance->config.cache_threshold_score = old_threshold;
        instance->config.min_lines_for_cache = old_min_lines;
        instance->config.max_simple_length = old_max_simple;
        instance->config.adaptive_threshold = old_adaptive;
        instance->config.debug_mode = old_debug;
        
        // Reset system state
        instance->state.system_start_time = time(NULL);
        instance->state.last_error[0] = '\0';
        
    } else {
        // Not initialized yet, just do initial init
        return psc_init(instance);
    }
    
    return PSC_SUCCESS;
}

/**
 * @brief Add a function to cache with smart caching decision
 * @param instance Cache instance
 * @param source_code Python source code containing function definition
 * @param filename Filename for debugging purposes
 * @return PSC_SUCCESS on success, PSC_ERROR_FUNCTION_TOO_SIMPLE if skipped, other error codes on failure
 */
static inline psc_result_t psc_add_function(psc_instance_t *instance, const char *source_code, const char *filename) {
    if (!instance) return PSC_ERROR_NULL_INSTANCE;
    if (!instance->state.initialized) return PSC_ERROR_NOT_INITIALIZED;
    
    if (!source_code || strlen(source_code) == 0) {
        return PSC_ERROR_INVALID_ARGS;
    }
    
    clock_t analysis_start = clock();
    
    // Create temporary entry for analysis
    struct psc_cache_entry temp_entry = {0};
    
    // Perform smart caching decision
    instance->stats.functions_analyzed++;
    int should_cache = psc_should_cache_function(instance, source_code, &temp_entry);
    
    clock_t analysis_end = clock();
    instance->stats.total_analysis_time += 
        ((double)(analysis_end - analysis_start)) / CLOCKS_PER_SEC;
    
    if (!should_cache) {
        instance->stats.functions_skipped++;
        if (instance->config.debug_mode) {
            post("PSC[%s]: Skipping function - %s\n", 
                   instance->state.instance_name, temp_entry.analysis.decision_reason);
        }
        return PSC_ERROR_FUNCTION_TOO_SIMPLE;
    }
    
    // Extract function name
    char function_name[PSC_MAX_FUNCTION_NAME];
    if (psc_extract_function_name(instance, source_code) != 0) {
        return PSC_ERROR_COMPILE;
    }
    strncpy_zero(function_name, instance->workspace.temp_function_name, sizeof(function_name) - 1);
    
    // Compile the function
    PyObject *code_obj, *func_obj, *globals_dict;
    psc_result_t compile_result = psc_compile_function(instance, source_code, filename, 
                                                      &code_obj, &func_obj, &globals_dict);
    if (compile_result != PSC_SUCCESS) {
        return compile_result;
    }
    
    // Create cache entry
    struct psc_cache_entry *entry = psc_create_entry(function_name, source_code, 
                                                    code_obj, func_obj, globals_dict);
    if (!entry) {
        Py_DECREF(code_obj);
        Py_DECREF(func_obj);
        Py_DECREF(globals_dict);
        return PSC_ERROR_MEMORY;
    }
    
    // Copy analysis results to the entry
    entry->analysis = temp_entry.analysis;
    entry->analysis.was_cached = 1;
    
    // Insert into cache with write lock
    psc_rwlock_wrlock(&instance->state.lock);
    
    size_t hash_idx = psc_hash_string(function_name);
    
    // Check for existing entry and replace if found
    struct psc_cache_entry *current = instance->hash_table[hash_idx];
    struct psc_cache_entry *prev = NULL;
    
    while (current) {
        if (strcmp(current->function_name, function_name) == 0) {
            // Replace existing entry
            if (prev) {
                prev->next = entry;
            } else {
                instance->hash_table[hash_idx] = entry;
            }
            entry->next = current->next;
            psc_free_entry(current);
            
            psc_rwlock_unlock_wr(&instance->state.lock);
            
            // Update statistics
            instance->stats.functions_cached++;
            
            // Clean up temporary references
            Py_DECREF(code_obj);
            Py_DECREF(func_obj);
            Py_DECREF(globals_dict);
            
            return PSC_SUCCESS;
        }
        prev = current;
        current = current->next;
    }
    
    // Add new entry
    entry->next = instance->hash_table[hash_idx];
    instance->hash_table[hash_idx] = entry;
    instance->stats.total_entries++;
    instance->stats.functions_cached++;
    
    psc_rwlock_unlock_wr(&instance->state.lock);
    
    if (instance->config.debug_mode) {
        post("PSC[%s]: Cached function '%s' - %s\n", 
               instance->state.instance_name, function_name, entry->analysis.decision_reason);
    }
    
    // Clean up temporary references
    Py_DECREF(code_obj);
    Py_DECREF(func_obj);
    Py_DECREF(globals_dict);
    
    return PSC_SUCCESS;
}

/**
 * @brief Call a cached function with arguments
 * @param instance Cache instance
 * @param function_name Name of the cached function
 * @param args Tuple of positional arguments (can be NULL)
 * @param kwargs Dictionary of keyword arguments (can be NULL)
 * @return PyObject* result or NULL on error
 */
static inline PyObject* psc_call_function(psc_instance_t *instance, const char *function_name, 
                                         PyObject *args, PyObject *kwargs) {
    if (!instance) {
        PyErr_SetString(PyExc_RuntimeError, "NULL cache instance");
        return NULL;
    }
    
    if (!instance->state.initialized) {
        PyErr_SetString(PyExc_RuntimeError, "Cache instance not initialized");
        return NULL;
    }
    
    psc_rwlock_rdlock(&instance->state.lock);
    
    struct psc_cache_entry *entry = psc_find_function_unlocked(instance, function_name);
    if (!entry) {
        instance->stats.cache_misses++;
        psc_rwlock_unlock_rd(&instance->state.lock);
        PyErr_Format(PyExc_KeyError, "Function '%s' not found in cache '%s'", 
                     function_name, instance->state.instance_name);
        return NULL;
    }
    
    instance->stats.cache_hits++;
    
    // Call the cached function
    PyObject *result;
    if (kwargs && PyDict_Size(kwargs) > 0) {
        result = PyObject_Call(entry->function_object, args ? args : PyTuple_New(0), kwargs);
    } else {
        result = PyObject_CallObject(entry->function_object, args);
    }
    
    psc_rwlock_unlock_rd(&instance->state.lock);
    
    return result;
}

/**
 * @brief Check if a function exists in the cache
 * @param instance Cache instance
 * @param function_name Name of the function to check
 * @return 1 if exists, 0 otherwise
 */
static inline int psc_has_function(psc_instance_t *instance, const char *function_name) {
    if (!instance || !instance->state.initialized) return 0;
    
    psc_rwlock_rdlock(&instance->state.lock);
    struct psc_cache_entry *entry = psc_find_function_unlocked(instance, function_name);
    int exists = (entry != NULL);
    psc_rwlock_unlock_rd(&instance->state.lock);
    
    return exists;
}

/**
 * @brief Get comprehensive system statistics
 * @param instance Cache instance
 * @param stats_buffer Buffer to store formatted statistics string
 * @param buffer_size Size of the stats buffer
 */
static inline void psc_get_stats_string(psc_instance_t *instance, char *stats_buffer, size_t buffer_size) {
    if (!instance || !instance->state.initialized) {
        snprintf_zero(stats_buffer, buffer_size, "Cache instance not initialized");
        return;
    }
    
    psc_rwlock_rdlock(&instance->state.lock);
    
    // Calculate derived statistics
    size_t total_accesses = instance->stats.cache_hits + instance->stats.cache_misses;
    double hit_ratio = total_accesses > 0 ? 
                      (double)instance->stats.cache_hits / total_accesses : 0.0;
    
    double cache_efficiency = instance->stats.functions_analyzed > 0 ?
                             (double)instance->stats.functions_skipped / instance->stats.functions_analyzed : 0.0;
    
    time_t uptime = time(NULL) - instance->state.system_start_time;
    
    snprintf_zero(stats_buffer, buffer_size,
        "=== Smart Cache Statistics [%s] ===\n"
        "Cache Entries: %zu\n"
        "Cache Hits: %zu\n"
        "Cache Misses: %zu\n"
        "Hit Ratio: %.2f%%\n"
        "Functions Analyzed: %zu\n"
        "Functions Cached: %zu\n"
        "Functions Skipped: %zu\n"
        "Cache Efficiency: %.2f%% (avoided overhead)\n"
        "Total Analysis Time: %.6f seconds\n"
        "Total Compilation Time: %.6f seconds\n"
        "Average Analysis Time: %.6f seconds\n"
        "System Uptime: %ld seconds\n"
        "Configuration: threshold=%d, min_lines=%d, max_simple=%d",
        instance->state.instance_name,
        instance->stats.total_entries,
        instance->stats.cache_hits,
        instance->stats.cache_misses,
        hit_ratio * 100.0,
        instance->stats.functions_analyzed,
        instance->stats.functions_cached,
        instance->stats.functions_skipped,
        cache_efficiency * 100.0,
        instance->stats.total_analysis_time,
        instance->stats.total_compilation_time,
        instance->stats.functions_analyzed > 0 ? 
            instance->stats.total_analysis_time / instance->stats.functions_analyzed : 0.0,
        uptime,
        instance->config.cache_threshold_score,
        instance->config.min_lines_for_cache,
        instance->config.max_simple_length
    );
    
    psc_rwlock_unlock_rd(&instance->state.lock);
}

/**
 * @brief Explain why a function would or wouldn't be cached
 * @param instance Cache instance  
 * @param source_code Python source code to analyze
 * @param explanation_buffer Buffer to store explanation
 * @param buffer_size Size of explanation buffer
 */
static inline void psc_explain_decision(psc_instance_t *instance, const char *source_code, 
                                       char *explanation_buffer, size_t buffer_size) {
    if (!instance) {
        snprintf_zero(explanation_buffer, buffer_size, "NULL cache instance");
        return;
    }
    
    if (!source_code) {
        snprintf_zero(explanation_buffer, buffer_size, "No source code provided");
        return;
    }
    
    struct psc_cache_entry temp_entry = {0};
    int should_cache = psc_should_cache_function(instance, source_code, &temp_entry);
    
    snprintf_zero(explanation_buffer, buffer_size,
        "Instance: %s\n"
        "Decision: %s\n"
        "Complexity Score: %d (threshold: %d)\n"
        "Reason: %s\n"
        "Analysis Details:\n"
        "  Lines: %d, Characters: %d\n"
        "  Imports: %d, Decorators: %d\n"  
        "  Loops: %d, Comprehensions: %d\n"
        "  Lambda: %d, Complex ops: %d\n"
        "  Nested functions: %d\n"
        "  Exception handling: %d\n"
        "  String ops: %d, Math ops: %d\n"
        "  Builtin calls: %d",
        instance->state.instance_name,
        should_cache ? "✅ CACHE" : "❌ SKIP",
        temp_entry.analysis.complexity_score,
        instance->config.cache_threshold_score,
        temp_entry.analysis.decision_reason,
        temp_entry.analysis.line_count,
        temp_entry.analysis.char_count,
        temp_entry.analysis.import_statements,
        temp_entry.analysis.decorator_count,
        temp_entry.analysis.loop_constructs,
        temp_entry.analysis.comprehensions,
        temp_entry.analysis.lambda_functions,
        temp_entry.analysis.complex_operations,
        temp_entry.analysis.nested_functions,
        temp_entry.analysis.exception_handling,
        temp_entry.analysis.string_operations,
        temp_entry.analysis.mathematical_operations,
        temp_entry.analysis.builtin_function_calls
    );
}

/**
 * @brief Configure system parameters
 * @param instance Cache instance
 * @param threshold Complexity threshold for caching decisions
 * @param debug_mode Enable debug output (1) or disable (0)
 */
static inline void psc_configure(psc_instance_t *instance, int threshold, int debug_mode) {
    if (!instance || !instance->state.initialized) return;
    
    psc_rwlock_wrlock(&instance->state.lock);
    instance->config.cache_threshold_score = threshold;
    instance->config.debug_mode = debug_mode;
    psc_rwlock_unlock_wr(&instance->state.lock);
}


// ============================================================================
// CONVENIENCE MACROS
// ============================================================================

#define PSC_CALL_NOARGS(instance, func_name) \
    psc_call_function(instance, func_name, NULL, NULL)

#define PSC_CALL_1ARG(instance, func_name, arg1) \
    psc_call_function(instance, func_name, PyTuple_Pack(1, arg1), NULL)

#define PSC_CALL_2ARGS(instance, func_name, arg1, arg2) \
    psc_call_function(instance, func_name, PyTuple_Pack(2, arg1, arg2), NULL)

#define PSC_CALL_3ARGS(instance, func_name, arg1, arg2, arg3) \
    psc_call_function(instance, func_name, PyTuple_Pack(3, arg1, arg2, arg3), NULL)

#ifdef __cplusplus
}
#endif

#endif // PY_CACHE_H

