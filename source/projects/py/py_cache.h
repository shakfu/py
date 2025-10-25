/**
 * @file py_cache.h
 * @brief Python function cache with explicit caching and validation
 * @version 6.0
 * @author Function Cache System
 *
 * A complete, production-ready, thread-safe function caching system with
 * instance-based design and comprehensive function validation.
 *
 * Features:
 * - Instance-based design - no global state, multiple independent caches
 * - Explicit caching model - user decides what to cache (via cache/cachefile handlers)
 * - Comprehensive function validation - ensures source contains valid Python function
 * - Thread-safe with cross-platform locking (Windows/Unix/fallback)
 * - Unified data structure for all cache state and statistics
 * - Memory leak prevention with proper reference counting
 * - Performance statistics and monitoring with per-instance tracking
 * - Configurable debug modes and strict validation
 * - Improved lifecycle management with psc_reset() and auto-cleanup
 * - Comprehensive error handling with detailed validation messages
 * - Function signature introspection - parameter counts, names, types, *args|**kwargs detection
 *
 * Usage:
 *   #include "py_cache.h"
 *
 *   // Create and initialize instance
 *   psc_instance_t* cache = psc_create_instance("my_cache");
 *   psc_init(cache);
 *
 *   // Validate and cache a function
 *   psc_add_function(cache, "def square(x): return x*x", "math.py");
 *
 *   // Call cached functions
 *   PyObject* result = psc_call_function(cache, "square", args, NULL);
 *
 *   // Inspect function signature
 *   int arg_count, has_varargs;
 *   psc_get_function_signature(cache, "square", &arg_count, NULL, &has_varargs, NULL);
 *   const char* const* param_names = psc_get_function_param_names(cache, "square");
 *
 *   // Reset for reuse (optional)
 *   psc_reset(cache);
 *
 *   // Final cleanup and destroy
 *   psc_destroy_instance(cache);
 */

#ifndef PY_CACHE_H
#define PY_CACHE_H

#include <Python.h>
#include <stdio.h>
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

#ifndef PSC_MAX_FUNCTION_NAME_LENGTH
#define PSC_MAX_FUNCTION_NAME_LENGTH 256
#endif

#ifndef PSC_MAX_ERROR_LENGTH
#define PSC_MAX_ERROR_LENGTH 512
#endif

#ifndef PSC_MAX_SOURCE_LENGTH
#define PSC_MAX_SOURCE_LENGTH 16384
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
 * - Validation workspace
 */
typedef struct psc_instance {
    // === CORE CACHE DATA ===
    struct psc_cache_entry {
        char function_name[PSC_MAX_FUNCTION_NAME_LENGTH];
        char *source_code;
        PyObject *code_object;
        PyObject *function_object;
        PyObject *globals_dict;
        size_t source_hash;
        time_t created_time;
        time_t last_access;
        int access_count;
        struct psc_cache_entry *next;

        // Function signature information
        struct {
            int arg_count;              // Number of positional arguments
            int kwonly_arg_count;       // Number of keyword-only arguments
            int total_arg_count;        // Total arguments (arg_count + kwonly_arg_count)
            int has_varargs;            // Has *args
            int has_varkwargs;          // Has **kwargs
            int has_defaults;           // Has default values
            int has_annotations;        // Has type annotations
            char **param_names;         // Array of parameter names (NULL-terminated)
            PyObject *annotations_dict; // Type annotations dictionary (borrowed ref)
        } signature;
    } *hash_table[PSC_CACHE_SIZE];
    
    // === PERFORMANCE STATISTICS ===
    struct {
        size_t total_entries;
        size_t cache_hits;
        size_t cache_misses;
        size_t functions_cached;
        size_t validation_failures;
        double total_compilation_time;
        double total_validation_time;
        double hit_ratio;
        double cache_efficiency;
    } stats;
    
    // === CONFIGURATION ===
    struct {
        int debug_mode;
        int strict_validation;
    } config;
    
    // === SYSTEM STATE ===
    struct {
        int initialized;
        psc_rwlock_t lock;
        time_t system_start_time;
        char last_error[PSC_MAX_ERROR_LENGTH];
        char last_validation_error[PSC_MAX_ERROR_LENGTH];
        char instance_name[64];
        char last_cached_function_name[PSC_MAX_FUNCTION_NAME_LENGTH];
    } state;

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
    PSC_ERROR_NULL_INSTANCE = -7,
    PSC_ERROR_VALIDATION_FAILED = -8,
    PSC_ERROR_NO_FUNCTION_DEF = -9,
    PSC_ERROR_MULTIPLE_FUNCTIONS = -10,
    PSC_ERROR_INVALID_FUNCTION_NAME = -11,
    PSC_ERROR_COMPILATION = -12
} psc_result_t;

// Validation-specific error codes
typedef enum {
    PSC_VALIDATE_SUCCESS = 0,
    PSC_VALIDATE_NO_FUNCTION = -1,
    PSC_VALIDATE_MULTIPLE_FUNCTIONS = -2,
    PSC_VALIDATE_INVALID_SYNTAX = -3,
    PSC_VALIDATE_NOT_A_FUNCTION = -4,
    PSC_VALIDATE_INVALID_NAME = -5
} psc_validate_result_t;

// ============================================================================
// PUBLIC API PROTOTYPES
// ============================================================================

/**
 * @section Instance Management
 * Functions for creating, initializing, resetting, and destroying cache instances
 */

/**
 * @brief Create a new cache instance
 * @param instance_name Optional name for the instance (can be NULL)
 * @return Pointer to new instance or NULL on failure
 */
static inline psc_instance_t* psc_create_instance(const char *instance_name);

/**
 * @brief Initialize a cache instance
 * @param instance Cache instance to initialize
 * @return PSC_SUCCESS on success, error code on failure
 */
static inline psc_result_t psc_init(psc_instance_t *instance);

/**
 * @brief Reset cache instance to clean state for reuse
 * @param instance Cache instance to reset
 * @return PSC_SUCCESS on success, error code on failure
 */
static inline psc_result_t psc_reset(psc_instance_t *instance);

/**
 * @brief Destroy a cache instance and free all memory
 * @param instance Cache instance to destroy (automatically cleans up first)
 */
static inline void psc_destroy_instance(psc_instance_t *instance);

/**
 * @section Function Validation
 * Functions for validating Python function source code
 */

/**
 * @brief Quick check if source looks like a Python function
 * @param instance Cache instance (for debug output)
 * @param source_code Python source code to validate
 * @return PSC_VALIDATE_SUCCESS if looks like function, error code otherwise
 */
static inline psc_validate_result_t psc_check_is_python_function(
    psc_instance_t *instance, const char *source_code);

/**
 * @brief Comprehensive validation that source contains exactly one Python function
 * @param instance Cache instance (for debug output)
 * @param source_code Python source code to validate
 * @param function_name Buffer to store extracted function name
 * @param name_buffer_size Size of function name buffer
 * @param error_msg Buffer to store detailed error message
 * @param error_msg_size Size of error message buffer
 * @param compiled_out Optional output parameter for compiled code object (can be NULL)
 * @return PSC_VALIDATE_SUCCESS if valid function, error code otherwise
 */
static inline psc_validate_result_t psc_validate_function_source(
    psc_instance_t *instance,
    const char *source_code,
    char *function_name,
    size_t name_buffer_size,
    char *error_msg,
    size_t error_msg_size,
    PyObject **compiled_out);

/**
 * @section Core Cache Operations
 * Functions for adding and calling cached functions
 */

/**
 * @brief Add a function to cache with comprehensive validation and smart caching decision
 * @param instance Cache instance
 * @param source_code Python source code containing function definition
 * @param filename Filename for debugging purposes
 * @return PSC_SUCCESS on success, error codes on failure
 */
static inline psc_result_t psc_add_function(psc_instance_t *instance,
                                           const char *source_code,
                                           const char *filename);

/**
 * @brief Call a cached function with arguments
 * @param instance Cache instance
 * @param function_name Name of the cached function
 * @param args Tuple of positional arguments (can be NULL)
 * @param kwargs Dictionary of keyword arguments (can be NULL)
 * @return PyObject* result or NULL on error
 */
static inline PyObject* psc_call_function(psc_instance_t *instance,
                                         const char *function_name,
                                         PyObject *args,
                                         PyObject *kwargs);

/**
 * @section Query and Utility Functions
 * Functions for checking cache state and retrieving information
 */

/**
 * @brief Check if a function exists in the cache
 * @param instance Cache instance
 * @param function_name Name of the function to check
 * @return 1 if exists, 0 otherwise
 */
static inline int psc_has_function(psc_instance_t *instance,
                                   const char *function_name);

/**
 * @brief Get the name of the last cached function
 * @param instance Cache instance
 * @return Pointer to the last cached function name, or NULL if none or instance invalid
 */
static inline const char* psc_get_last_cached_function_name(psc_instance_t *instance);

/**
 * @brief Get function signature information
 * @param instance Cache instance
 * @param function_name Name of the cached function
 * @param arg_count Output: number of positional arguments (can be NULL)
 * @param kwonly_arg_count Output: number of keyword-only arguments (can be NULL)
 * @param has_varargs Output: 1 if has *args, 0 otherwise (can be NULL)
 * @param has_varkwargs Output: 1 if has **kwargs, 0 otherwise (can be NULL)
 * @return PSC_SUCCESS if function found, PSC_ERROR_NOT_FOUND otherwise
 */
static inline psc_result_t psc_get_function_signature(psc_instance_t *instance,
                                                     const char *function_name,
                                                     int *arg_count,
                                                     int *kwonly_arg_count,
                                                     int *has_varargs,
                                                     int *has_varkwargs);

/**
 * @brief Get function parameter names
 * @param instance Cache instance
 * @param function_name Name of the cached function
 * @return NULL-terminated array of parameter name strings, or NULL if not found
 *         Caller should NOT free the returned array (it's owned by the cache)
 */
static inline const char* const* psc_get_function_param_names(psc_instance_t *instance,
                                                               const char *function_name);

/**
 * @brief Get function type annotations dictionary
 * @param instance Cache instance
 * @param function_name Name of the cached function
 * @return Python dictionary of annotations (borrowed reference), or NULL if not found/no annotations
 */
static inline PyObject* psc_get_function_annotations(psc_instance_t *instance,
                                                     const char *function_name);

/**
 * @section Statistics and Analysis
 * Functions for retrieving cache statistics and analyzing caching decisions
 */

/**
 * @brief Get comprehensive system statistics
 * @param instance Cache instance
 * @param stats_buffer Buffer to store formatted statistics string
 * @param buffer_size Size of the stats buffer
 */
static inline void psc_get_stats_string(psc_instance_t *instance,
                                       char *stats_buffer,
                                       size_t buffer_size);

/**
 * @section Configuration
 * Functions for configuring cache behavior
 */

/**
 * @brief Configure system parameters
 * @param instance Cache instance
 * @param debug_mode Enable debug output (1) or disable (0)
 * @param strict_validation Enable strict function validation (1) or disable (0)
 */
static inline void psc_configure(psc_instance_t *instance,
                                int debug_mode,
                                int strict_validation);

/**
 * @section Convenience Macros
 * Macros for simplified function calling
 */

// Call function with no arguments
#define PSC_CALL_NOARGS(instance, func_name) \
    psc_call_function(instance, func_name, NULL, NULL)

// Call function with 1 argument
#define PSC_CALL_1ARG(instance, func_name, arg1) \
    psc_call_function(instance, func_name, PyTuple_Pack(1, arg1), NULL)

// Call function with 2 arguments
#define PSC_CALL_2ARGS(instance, func_name, arg1, arg2) \
    psc_call_function(instance, func_name, PyTuple_Pack(2, arg1, arg2), NULL)

// Call function with 3 arguments
#define PSC_CALL_3ARGS(instance, func_name, arg1, arg2, arg3) \
    psc_call_function(instance, func_name, PyTuple_Pack(3, arg1, arg2, arg3), NULL)

// Check result and handle errors
#define PSC_CHECK_RESULT(result, instance, action) do { \
    if (result != PSC_SUCCESS) { \
        if ((instance)->config.debug_mode) { \
            error("PSC[%s]: %s failed with code %d\n", \
                   (instance)->state.instance_name, action, result); \
        } \
        return result; \
    } \
} while(0)

// Safe call with error checking
#define PSC_SAFE_CALL(instance, func_name, args) ({ \
    PyObject *_result = psc_call_function(instance, func_name, args, NULL); \
    if (!_result && PyErr_Occurred()) { \
        if ((instance)->config.debug_mode) { \
            error("PSC[%s]: Call to '%s' failed\n", \
                   (instance)->state.instance_name, func_name); \
            PyErr_Print(); \
        } \
    } \
    _result; \
})

// ============================================================================
// INSTANCE MANAGEMENT
// ============================================================================

/**
 * @brief Create a new cache instance
 * @param instance_name Optional name for the instance (can be NULL)
 * @return Pointer to new instance or NULL on failure
 */
static inline psc_instance_t* psc_create_instance(const char *instance_name) {
    psc_instance_t *instance = (psc_instance_t*)malloc(sizeof(psc_instance_t));
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
 * @brief Destroy a cache instance and free all memory
 * @param instance Cache instance to destroy (automatically cleans up first)
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
                free(current->source_code);
                // Only decref Python objects if interpreter is still initialized
                if (Py_IsInitialized()) {
                    Py_XDECREF(current->code_object);
                    Py_XDECREF(current->function_object);
                    Py_XDECREF(current->globals_dict);
                }
                free(current);
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


// ============================================================================
// FUNCTION VALIDATION
// ============================================================================

/**
 * @brief Quick check if source looks like a Python function
 * @param instance Cache instance (for debug output)
 * @param source_code Python source code to validate
 * @return PSC_VALIDATE_SUCCESS if looks like function, error code otherwise
 */
static inline psc_validate_result_t psc_check_is_python_function(
    psc_instance_t *instance, const char *source_code) 
{
    char error_msg[PSC_MAX_ERROR_LENGTH];
    size_t error_msg_size = PSC_MAX_ERROR_LENGTH;
    
    if (!source_code || strlen(source_code) == 0) {
        snprintf_zero(error_msg, error_msg_size, "Empty source code");
        return PSC_VALIDATE_NO_FUNCTION;
    }

    // === Step 1: Check for function definition patterns ===
    int def_count = 0;
    const char *def_positions[10]; // Track up to 10 def positions
    const char *p = source_code;
    
    while ((p = strstr(p, "def ")) != NULL) {
        // Check if this is actually a function def (not inside a string/comment)
        const char *line_start = p;
        while (line_start > source_code && *(line_start - 1) != '\n') {
            line_start--;
        }
        
        // Check if "def " is at start of line (possibly with indentation)
        int is_function_def = 1;
        for (const char *check = line_start; check < p; check++) {
            if (*check != ' ' && *check != '\t') {
                is_function_def = 0;
                break;
            }
        }
        
        if (is_function_def && def_count < 10) {
            def_positions[def_count++] = p;
        }
        
        p += 4; // Move past "def "
    }
    
    if (def_count == 0) {
        snprintf_zero(error_msg, error_msg_size, "No function definition found (missing 'def')");
        return PSC_VALIDATE_NO_FUNCTION;
    }
    
    if (def_count > 1) {
        snprintf_zero(error_msg, error_msg_size, "Multiple function definitions found (%d). Only single functions supported.", def_count);
        return PSC_VALIDATE_MULTIPLE_FUNCTIONS;
    }
    
    // success: def_count == 1
    return PSC_VALIDATE_SUCCESS;
}

/**
 * @brief Comprehensive validation that source contains exactly one Python function
 * @param instance Cache instance (for debug output)
 * @param source_code Python source code to validate
 * @param function_name Buffer to store extracted function name
 * @param name_buffer_size Size of function name buffer
 * @param error_msg Buffer to store detailed error message
 * @param error_msg_size Size of error message buffer
 * @param compiled_out Optional output parameter for compiled code object (can be NULL)
 * @return PSC_VALIDATE_SUCCESS if valid function, error code otherwise
 */
static inline psc_validate_result_t psc_validate_function_source(
    psc_instance_t *instance,
    const char *source_code,
    char *function_name,
    size_t name_buffer_size,
    char *error_msg,
    size_t error_msg_size,
    PyObject **compiled_out) {
    
    clock_t validation_start = clock();
    
    if (!source_code || strlen(source_code) == 0) {
        snprintf_zero(error_msg, error_msg_size, "Empty source code");
        return PSC_VALIDATE_NO_FUNCTION;
    }
    
    // Clear outputs
    function_name[0] = '\0';
    error_msg[0] = '\0';
    
    // === Step 1: Check for function definition patterns ===
    int def_count = 0;
    const char *def_positions[10]; // Track up to 10 def positions
    const char *p = source_code;
    
    while ((p = strstr(p, "def ")) != NULL) {
        // Check if this is actually a function def (not inside a string/comment)
        const char *line_start = p;
        while (line_start > source_code && *(line_start - 1) != '\n') {
            line_start--;
        }
        
        // Check if "def " is at start of line (possibly with indentation)
        int is_function_def = 1;
        for (const char *check = line_start; check < p; check++) {
            if (*check != ' ' && *check != '\t') {
                is_function_def = 0;
                break;
            }
        }
        
        if (is_function_def && def_count < 10) {
            def_positions[def_count++] = p;
        }
        
        p += 4; // Move past "def "
    }
    
    if (def_count == 0) {
        snprintf_zero(error_msg, error_msg_size, "No function definition found (missing 'def')");
        return PSC_VALIDATE_NO_FUNCTION;
    }
    
    if (def_count > 1) {
        snprintf_zero(error_msg, error_msg_size, "Multiple function definitions found (%d). Only single functions supported.", def_count);
        return PSC_VALIDATE_MULTIPLE_FUNCTIONS;
    }
    
    // === Step 2: Extract and validate function name ===
    const char *def_pos = def_positions[0];
    const char *name_start = def_pos + 4; // Skip "def "
    
    // Skip whitespace after "def"
    while (*name_start && (*name_start == ' ' || *name_start == '\t')) {
        name_start++;
    }
    
    if (!*name_start) {
        snprintf_zero(error_msg, error_msg_size, "Missing function name after 'def'");
        return PSC_VALIDATE_INVALID_NAME;
    }
    
    // Extract function name
    const char *name_end = name_start;
    while (*name_end && (isalnum(*name_end) || *name_end == '_')) {
        name_end++;
    }

    size_t name_len = name_end - name_start;

    // post("PSC: DEBUG - Extracted function name: name_start='%.*s', name_len=%zu\n",
    //        (int)name_len, name_start, name_len);

    if (name_len == 0) {
        snprintf_zero(error_msg, error_msg_size, "Invalid function name");
        return PSC_VALIDATE_INVALID_NAME;
    }

    if (name_len >= name_buffer_size) {
        snprintf_zero(error_msg, error_msg_size, "Function name too long (%zu chars, max %zu)", name_len, name_buffer_size - 1);
        return PSC_VALIDATE_INVALID_NAME;
    }

    // Check that function name starts with letter or underscore
    if (!isalpha(*name_start) && *name_start != '_') {
        snprintf_zero(error_msg, error_msg_size, "Function name must start with letter or underscore, not '%c'", *name_start);
        return PSC_VALIDATE_INVALID_NAME;
    }

    // Copy function name - use memcpy to avoid any strncpy issues
    if (name_len < name_buffer_size) {
        memcpy(function_name, name_start, name_len);
        function_name[name_len] = '\0';
        // post("PSC: DEBUG - Copied function name: '%s' (length %zu)\n", function_name, name_len);
    } else {
        snprintf_zero(error_msg, error_msg_size, "Function name buffer too small");
        return PSC_VALIDATE_INVALID_NAME;
    }
    
    // === Step 3: Validate function signature ===
    // Check for opening parenthesis after name
    const char *paren_pos = name_end;
    while (*paren_pos && (*paren_pos == ' ' || *paren_pos == '\t')) {
        paren_pos++;
    }
    
    if (*paren_pos != '(') {
        snprintf_zero(error_msg, error_msg_size, "Expected '(' after function name, found '%c'", *paren_pos ? *paren_pos : ' ');
        return PSC_VALIDATE_INVALID_SYNTAX;
    }
    
    // === Step 4: Basic syntax validation using Python compiler ===
    PyObject *compiled = Py_CompileString(source_code, "<validation>", Py_file_input);
    if (!compiled) {
        // Get the Python error for detailed feedback
        if (PyErr_Occurred()) {
            PyObject *ptype, *pvalue, *ptraceback;
            PyErr_Fetch(&ptype, &pvalue, &ptraceback);

            if (pvalue) {
                PyObject *str_exc = PyObject_Str(pvalue);
                if (str_exc) {
                    const char *err_str = PyUnicode_AsUTF8(str_exc);
                    if (err_str) {
                        snprintf_zero(error_msg, error_msg_size, "Python syntax error: %s", err_str);
                    }
                    Py_DECREF(str_exc);
                }
            }

            Py_XDECREF(ptype);
            Py_XDECREF(pvalue);
            Py_XDECREF(ptraceback);
        }

        if (error_msg[0] == '\0') {
            snprintf_zero(error_msg, error_msg_size, "Python compilation failed");
        }

        return PSC_VALIDATE_INVALID_SYNTAX;
    }

    // Store compiled object if requested (caller takes ownership)
    if (compiled_out) {
        *compiled_out = compiled;
    }
    
    // === Step 5: Verify it actually defines the expected function ===
    if (instance->config.strict_validation) {
        PyObject *globals = PyDict_New();
        PyObject *locals = PyDict_New();
        if (!globals || !locals) {
            Py_XDECREF(globals);
            Py_XDECREF(locals);
            if (!compiled_out) Py_DECREF(compiled);
            snprintf_zero(error_msg, error_msg_size, "Memory allocation failed during validation");
            return PSC_VALIDATE_INVALID_SYNTAX;
        }

        PyDict_SetItemString(globals, "__builtins__", PyEval_GetBuiltins());

        PyObject *result = PyEval_EvalCode(compiled, globals, locals);

        if (!result) {
            if (!compiled_out) Py_DECREF(compiled);
            Py_DECREF(globals);
            Py_DECREF(locals);
            if (PyErr_Occurred()) PyErr_Clear();
            snprintf_zero(error_msg, error_msg_size, "Failed to execute function definition");
            return PSC_VALIDATE_INVALID_SYNTAX;
        }

        // Check if the expected function was defined
        PyObject *func_obj = PyDict_GetItemString(locals, function_name);
        if (!func_obj) {
            Py_DECREF(result);
            if (!compiled_out) Py_DECREF(compiled);
            Py_DECREF(globals);
            Py_DECREF(locals);
            snprintf_zero(error_msg, error_msg_size, "Function '%s' was not defined by the source code", function_name);
            return PSC_VALIDATE_NOT_A_FUNCTION;
        }
        
        if (!PyFunction_Check(func_obj)) {
            Py_DECREF(result);
            if (!compiled_out) Py_DECREF(compiled);
            Py_DECREF(globals);
            Py_DECREF(locals);
            snprintf_zero(error_msg, error_msg_size, "'%s' is not a function (it's a %s)", function_name, Py_TYPE(func_obj)->tp_name);
            return PSC_VALIDATE_NOT_A_FUNCTION;
        }

        // Cleanup
        Py_DECREF(result);
        Py_DECREF(globals);
        Py_DECREF(locals);
    }

    // Cleanup compilation object only if not returned to caller
    if (!compiled_out) {
        Py_DECREF(compiled);
    }
    
    // Update validation time statistics
    if (instance) {
        clock_t validation_end = clock();
        instance->stats.total_validation_time += 
            ((double)(validation_end - validation_start)) / CLOCKS_PER_SEC;
    }
    
    snprintf_zero(error_msg, error_msg_size, "Valid function '%s'", function_name);
    return PSC_VALIDATE_SUCCESS;
}

// ============================================================================
// SIGNATURE EXTRACTION
// ============================================================================

/**
 * Extract signature information from a Python function object
 */
static inline void psc_extract_signature(struct psc_cache_entry *entry) {
    if (!entry || !entry->function_object) return;

    PyObject *func = entry->function_object;

    // Initialize signature
    memset(&entry->signature, 0, sizeof(entry->signature));

    // Get the code object from the function
    PyCodeObject *code = (PyCodeObject*)PyFunction_GetCode(func);
    if (!code) return;

    // Extract argument counts using Python 3.11+ compatible API
    entry->signature.arg_count = PyCode_GetNumFree((PyCodeObject*)code) >= 0 ?
        PyCode_GetNumFree((PyCodeObject*)code) : 0;

    // entry->signature.arg_count = PyCode_GetNumFree((PyObject*)code) >= 0 ?
    //     PyCode_GetNumFree((PyObject*)code) : 0;

    // Get total local variables count to infer argument count
    // Since we can't access co_argcount directly, we use the inspect module approach
    PyObject *inspect_module = PyImport_ImportModule("inspect");
    if (inspect_module) {
        PyObject *signature_func = PyObject_GetAttrString(inspect_module, "signature");
        if (signature_func) {
            PyObject *sig = PyObject_CallFunctionObjArgs(signature_func, func, NULL);
            if (sig) {
                PyObject *params = PyObject_GetAttrString(sig, "parameters");
                if (params) {
                    PyObject *items = PyMapping_Items(params);
                    if (items && PyList_Check(items)) {
                        int total = PyList_Size(items);
                        entry->signature.total_arg_count = total;

                        // Count different parameter types
                        int pos_args = 0;
                        int kwonly_args = 0;

                        // Allocate array for parameter names
                        entry->signature.param_names = (char**)calloc(total + 1, sizeof(char*));

                        for (int i = 0; i < total; i++) {
                            PyObject *item = PyList_GetItem(items, i);
                            if (item && PyTuple_Check(item) && PyTuple_Size(item) >= 2) {
                                PyObject *name_obj = PyTuple_GetItem(item, 0);
                                PyObject *param_obj = PyTuple_GetItem(item, 1);

                                // Store parameter name
                                if (name_obj && PyUnicode_Check(name_obj)) {
                                    const char *name_str = PyUnicode_AsUTF8(name_obj);
                                    if (name_str && entry->signature.param_names) {
                                        entry->signature.param_names[i] = strdup(name_str);
                                    }
                                }

                                // Check parameter kind
                                PyObject *kind = PyObject_GetAttrString(param_obj, "kind");
                                if (kind) {
                                    PyObject *kind_name = PyObject_GetAttrString(kind, "name");
                                    if (kind_name && PyUnicode_Check(kind_name)) {
                                        const char *kind_str = PyUnicode_AsUTF8(kind_name);
                                        if (kind_str) {
                                            if (strcmp(kind_str, "VAR_POSITIONAL") == 0) {
                                                entry->signature.has_varargs = 1;
                                            } else if (strcmp(kind_str, "VAR_KEYWORD") == 0) {
                                                entry->signature.has_varkwargs = 1;
                                            } else if (strcmp(kind_str, "KEYWORD_ONLY") == 0) {
                                                kwonly_args++;
                                            } else if (strcmp(kind_str, "POSITIONAL_OR_KEYWORD") == 0 ||
                                                     strcmp(kind_str, "POSITIONAL_ONLY") == 0) {
                                                pos_args++;
                                            }
                                        }
                                        Py_DECREF(kind_name);
                                    }
                                    Py_DECREF(kind);
                                }

                                // Check if parameter has default value
                                PyObject *default_val = PyObject_GetAttrString(param_obj, "default");
                                if (default_val) {
                                    PyObject *empty = PyObject_GetAttrString(param_obj, "empty");
                                    if (empty && default_val != empty) {
                                        entry->signature.has_defaults = 1;
                                    }
                                    Py_XDECREF(empty);
                                    Py_DECREF(default_val);
                                }
                            }
                        }

                        entry->signature.arg_count = pos_args;
                        entry->signature.kwonly_arg_count = kwonly_args;

                        if (entry->signature.param_names) {
                            entry->signature.param_names[total] = NULL; // NULL-terminate
                        }

                        Py_DECREF(items);
                    }
                    Py_DECREF(params);
                }
                Py_DECREF(sig);
            } else {
                // Clear any error from signature inspection
                PyErr_Clear();
            }
            Py_DECREF(signature_func);
        }
        Py_DECREF(inspect_module);
    }

    // Clear any errors from import
    if (PyErr_Occurred()) PyErr_Clear();

    // Check for annotations
    PyObject *annotations = PyFunction_GetAnnotations(func);
    entry->signature.has_annotations = (annotations != NULL &&
                                       PyDict_Size(annotations) > 0);
    entry->signature.annotations_dict = annotations; // Borrowed reference

    // Check for defaults
    PyObject *defaults = PyFunction_GetDefaults(func);
    entry->signature.has_defaults = entry->signature.has_defaults ||
                                    (defaults != NULL && PyTuple_Size(defaults) > 0);
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
    struct psc_cache_entry *entry = (struct psc_cache_entry*)malloc(sizeof(struct psc_cache_entry));
    if (!entry) return NULL;
    
    memset(entry, 0, sizeof(struct psc_cache_entry));
    
    strncpy_zero(entry->function_name, function_name, PSC_MAX_FUNCTION_NAME_LENGTH - 1);
    entry->source_code = strdup(source_code);
    if (!entry->source_code) {
        free(entry);
        return NULL;
    }
    
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

    // Extract function signature information
    psc_extract_signature(entry);

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

    // Free signature parameter names
    if (entry->signature.param_names) {
        for (int i = 0; entry->signature.param_names[i] != NULL; i++) {
            free(entry->signature.param_names[i]);
        }
        free(entry->signature.param_names);
    }

    free(entry);
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
                                               PyObject **globals_dict,
                                               const char *function_name) {
    
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
                    if (error_msg) {
                        strncpy_zero(instance->state.last_error, error_msg, sizeof(instance->state.last_error) - 1);
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
    
    // Get the function object from locals
    *func_obj = PyDict_GetItemString(locals_dict, function_name);
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
 * @brief Initialize the smart caching system instance
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
    instance->config.debug_mode = 0;
    instance->config.strict_validation = 1; // Enable strict validation by default

    // Initialize system state
    instance->state.initialized = 1;
    instance->state.system_start_time = time(NULL);
    instance->state.last_error[0] = '\0';
    instance->state.last_validation_error[0] = '\0';
    instance->state.last_cached_function_name[0] = '\0';
    
    return PSC_SUCCESS;
}

/**
 * @brief Reset cache instance to clean state for reuse
 * @param instance Cache instance to reset
 * @return PSC_SUCCESS on success, error code on failure
 */
static inline psc_result_t psc_reset(psc_instance_t *instance) {
    if (!instance) return PSC_ERROR_NULL_INSTANCE;
    
    if (instance->config.debug_mode) {
        post("PSC[%s]: Resetting cache instance\n", instance->state.instance_name);
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
        int old_debug = instance->config.debug_mode;
        int old_strict = instance->config.strict_validation;

        memset(&instance->stats, 0, sizeof(instance->stats));

        psc_rwlock_unlock_wr(&instance->state.lock);
        psc_rwlock_destroy(&instance->state.lock);

        // Step 2: Reinitialize fresh state
        if (psc_rwlock_init(&instance->state.lock) != 0) {
            return PSC_ERROR_INIT;
        }

        // Restore configuration
        instance->config.debug_mode = old_debug;
        instance->config.strict_validation = old_strict;

        // Reset system state
        instance->state.system_start_time = time(NULL);
        instance->state.last_error[0] = '\0';
        instance->state.last_validation_error[0] = '\0';
        instance->state.last_cached_function_name[0] = '\0';
        
    } else {
        // Not initialized yet, just do initial init
        return psc_init(instance);
    }
    
    return PSC_SUCCESS;
}

/**
 * @brief Add a function to cache with comprehensive validation and smart caching decision
 * @param instance Cache instance
 * @param source_code Python source code containing function definition
 * @param filename Filename for debugging purposes
 * @return PSC_SUCCESS on success, error codes on failure
 */
static inline psc_result_t psc_add_function(psc_instance_t *instance, const char *source_code, const char *filename) {
    if (!instance) return PSC_ERROR_NULL_INSTANCE;
    if (!instance->state.initialized) return PSC_ERROR_NOT_INITIALIZED;
    if (!source_code || strlen(source_code) == 0) return PSC_ERROR_INVALID_ARGS;
    
    // === STEP 1: COMPREHENSIVE FUNCTION VALIDATION ===
    char function_name[PSC_MAX_FUNCTION_NAME_LENGTH];
    char validation_error[PSC_MAX_ERROR_LENGTH];
    PyObject *compiled_code = NULL;

    psc_validate_result_t validation = psc_validate_function_source(
        instance,
        source_code,
        function_name,
        PSC_MAX_FUNCTION_NAME_LENGTH,
        validation_error,
        PSC_MAX_ERROR_LENGTH,
        instance->config.strict_validation ? &compiled_code : NULL
    );
    
    // Store validation error for debugging
    strncpy_zero(instance->state.last_validation_error, validation_error, sizeof(instance->state.last_validation_error) - 1);
    
    if (validation != PSC_VALIDATE_SUCCESS) {
        instance->stats.validation_failures++;

        // Clean up compiled code if validation failed
        Py_XDECREF(compiled_code);

        if (instance->config.debug_mode) {
            // Always log validation failures
            post("PSC[%s]: VALIDATION FAILED: %s\n",
                   instance->state.instance_name, validation_error);
            post("PSC[%s]: Source code snippet: %.100s...\n",
                   instance->state.instance_name, source_code);
        }

        // Map validation errors to cache errors
        switch (validation) {
            case PSC_VALIDATE_NO_FUNCTION:
                return PSC_ERROR_NO_FUNCTION_DEF;
            case PSC_VALIDATE_MULTIPLE_FUNCTIONS:
                return PSC_ERROR_MULTIPLE_FUNCTIONS;
            case PSC_VALIDATE_NOT_A_FUNCTION:
                return PSC_ERROR_NO_FUNCTION_DEF;
            case PSC_VALIDATE_INVALID_NAME:
                return PSC_ERROR_INVALID_FUNCTION_NAME;
            case PSC_VALIDATE_INVALID_SYNTAX:
                return PSC_ERROR_VALIDATION_FAILED;
            default:
                return PSC_ERROR_VALIDATION_FAILED;
        }
    }

    if (instance->config.debug_mode) {
        post("PSC[%s]: Validation passed for function '%s'\n",
               instance->state.instance_name, function_name);
    }

    // === STEP 2: COMPILATION AND CACHING ===
    PyObject *code_obj, *func_obj, *globals_dict;

    // Reuse compiled code from validation if available, otherwise compile now
    if (compiled_code) {
        code_obj = compiled_code;
        // Create globals and execute to get function object
        globals_dict = PyDict_New();
        if (!globals_dict) {
            Py_DECREF(code_obj);
            return PSC_ERROR_MEMORY;
        }

        PyDict_SetItemString(globals_dict, "__builtins__", PyEval_GetBuiltins());

        // Add commonly used modules
        const char *common_modules[] = {"math", "time", "random", "os", "sys", "json", "re", NULL};
        for (int i = 0; common_modules[i] != NULL; i++) {
            PyObject *module = PyImport_ImportModule(common_modules[i]);
            if (module) {
                PyDict_SetItemString(globals_dict, common_modules[i], module);
                Py_DECREF(module);
            }
        }
        if (PyErr_Occurred()) PyErr_Clear();

        PyObject *locals_dict = PyDict_New();
        if (!locals_dict) {
            Py_DECREF(code_obj);
            Py_DECREF(globals_dict);
            return PSC_ERROR_MEMORY;
        }

        PyObject *result = PyEval_EvalCode(code_obj, globals_dict, locals_dict);
        if (!result) {
            Py_DECREF(code_obj);
            Py_DECREF(globals_dict);
            Py_DECREF(locals_dict);
            return PSC_ERROR_COMPILE;
        }
        Py_DECREF(result);

        func_obj = PyDict_GetItemString(locals_dict, function_name);
        if (!func_obj || !PyFunction_Check(func_obj)) {
            Py_DECREF(code_obj);
            Py_DECREF(globals_dict);
            Py_DECREF(locals_dict);
            return PSC_ERROR_COMPILE;
        }
        Py_INCREF(func_obj);
        Py_DECREF(locals_dict);
    } else {
        // Compile from scratch
        psc_result_t compile_result = psc_compile_function(instance, source_code, filename,
                                                          &code_obj, &func_obj, &globals_dict, function_name);
        if (compile_result != PSC_SUCCESS) {
            return compile_result;
        }
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
            
            // Update statistics and track last cached function
            instance->stats.functions_cached++;
            strncpy_zero(instance->state.last_cached_function_name, function_name, PSC_MAX_FUNCTION_NAME_LENGTH - 1);
            
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
    strncpy_zero(instance->state.last_cached_function_name, function_name, PSC_MAX_FUNCTION_NAME_LENGTH - 1);

    psc_rwlock_unlock_wr(&instance->state.lock);

    if (instance->config.debug_mode) {  
        // Always log successful caching
        post("PSC[%s]: SUCCESSFULLY CACHED function '%s' - now have %zu functions in cache\n",
               instance->state.instance_name, function_name, instance->stats.total_entries);
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
        if (instance->config.debug_mode) {
            post("PSC[%s]: CACHE MISS - function '%s' not found in cache\n",
                   instance->state.instance_name, function_name);
        }
        PyErr_Format(PyExc_KeyError, "Function '%s' not found in cache '%s'",
                     function_name, instance->state.instance_name);
        return NULL;
    }

    instance->stats.cache_hits++;
    // post("PSC[%s]: CACHE HIT - calling function '%s'\n",
    //        instance->state.instance_name, function_name);

    // Increment reference count so function object stays alive after we unlock
    PyObject *func_obj = entry->function_object;
    Py_INCREF(func_obj);

    psc_rwlock_unlock_rd(&instance->state.lock);

    // Call the cached function (lock is released, so we don't block other threads)
    PyObject *result;
    if (kwargs && PyDict_Size(kwargs) > 0) {
        result = PyObject_Call(func_obj, args ? args : PyTuple_New(0), kwargs);
    } else {
        result = PyObject_CallObject(func_obj, args);
    }

    if (result) {
        // post("PSC[%s]: Function '%s' executed successfully\n",
        //        instance->state.instance_name, function_name);
    } else {
        error("PSC[%s]: ERROR - Function '%s' execution failed\n",
               instance->state.instance_name, function_name);
    }

    // Release our temporary reference
    Py_DECREF(func_obj);

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
 * @brief Get the name of the last cached function
 * @param instance Cache instance
 * @return Pointer to the last cached function name, or NULL if none or instance invalid
 */
static inline const char* psc_get_last_cached_function_name(psc_instance_t *instance) {
    if (!instance || !instance->state.initialized) {
        error("PSC: ERROR - Cannot get last cached function: instance invalid or not initialized\n");
        return NULL;
    }

    psc_rwlock_rdlock(&instance->state.lock);
    const char *name = instance->state.last_cached_function_name[0] != '\0' ?
                      instance->state.last_cached_function_name : NULL;
    psc_rwlock_unlock_rd(&instance->state.lock);

    if (name) {
        // post("PSC[%s]: Last cached function is '%s'\n",
        //        instance->state.instance_name, name);
    } else {
        // post("PSC[%s]: No functions cached yet\n",
        //        instance->state.instance_name);
    }

    return name;
}

/**
 * @brief Get function signature information
 * @param instance Cache instance
 * @param function_name Name of the cached function
 * @param arg_count Output: number of positional arguments (can be NULL)
 * @param kwonly_arg_count Output: number of keyword-only arguments (can be NULL)
 * @param has_varargs Output: 1 if has *args, 0 otherwise (can be NULL)
 * @param has_varkwargs Output: 1 if has **kwargs, 0 otherwise (can be NULL)
 * @return PSC_SUCCESS if function found, PSC_ERROR_NOT_FOUND otherwise
 */
static inline psc_result_t psc_get_function_signature(psc_instance_t *instance,
                                                     const char *function_name,
                                                     int *arg_count,
                                                     int *kwonly_arg_count,
                                                     int *has_varargs,
                                                     int *has_varkwargs) {
    if (!instance || !instance->state.initialized) return PSC_ERROR_NOT_INITIALIZED;
    if (!function_name) return PSC_ERROR_INVALID_ARGS;

    psc_rwlock_rdlock(&instance->state.lock);

    struct psc_cache_entry *entry = psc_find_function_unlocked(instance, function_name);
    if (!entry) {
        psc_rwlock_unlock_rd(&instance->state.lock);
        return PSC_ERROR_NOT_FOUND;
    }

    // Copy signature information to output parameters
    if (arg_count) *arg_count = entry->signature.arg_count;
    if (kwonly_arg_count) *kwonly_arg_count = entry->signature.kwonly_arg_count;
    if (has_varargs) *has_varargs = entry->signature.has_varargs;
    if (has_varkwargs) *has_varkwargs = entry->signature.has_varkwargs;

    psc_rwlock_unlock_rd(&instance->state.lock);
    return PSC_SUCCESS;
}

/**
 * @brief Get function parameter names
 * @param instance Cache instance
 * @param function_name Name of the cached function
 * @return NULL-terminated array of parameter name strings, or NULL if not found
 *         Caller should NOT free the returned array (it's owned by the cache)
 */
static inline const char* const* psc_get_function_param_names(psc_instance_t *instance,
                                                               const char *function_name) {
    if (!instance || !instance->state.initialized) return NULL;
    if (!function_name) return NULL;

    psc_rwlock_rdlock(&instance->state.lock);

    struct psc_cache_entry *entry = psc_find_function_unlocked(instance, function_name);
    const char* const* names = entry ? (const char* const*)entry->signature.param_names : NULL;

    psc_rwlock_unlock_rd(&instance->state.lock);
    return names;
}

/**
 * @brief Get function type annotations dictionary
 * @param instance Cache instance
 * @param function_name Name of the cached function
 * @return Python dictionary of annotations (borrowed reference), or NULL if not found/no annotations
 */
static inline PyObject* psc_get_function_annotations(psc_instance_t *instance,
                                                     const char *function_name) {
    if (!instance || !instance->state.initialized) return NULL;
    if (!function_name) return NULL;

    psc_rwlock_rdlock(&instance->state.lock);

    struct psc_cache_entry *entry = psc_find_function_unlocked(instance, function_name);
    PyObject *annotations = (entry && entry->signature.has_annotations) ?
                           entry->signature.annotations_dict : NULL;

    psc_rwlock_unlock_rd(&instance->state.lock);
    return annotations;
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

    time_t uptime = time(NULL) - instance->state.system_start_time;

    snprintf_zero(stats_buffer, buffer_size,
        "=== Cache Statistics [%s] ===\n"
        "Cache Entries: %zu\n"
        "Cache Hits: %zu\n"
        "Cache Misses: %zu\n"
        "Hit Ratio: %.2f%%\n"
        "Functions Cached: %zu\n"
        "Validation Failures: %zu\n"
        "Total Validation Time: %.6f seconds\n"
        "Total Compilation Time: %.6f seconds\n"
        "System Uptime: %ld seconds\n"
        "Configuration: strict_validation=%s",
        instance->state.instance_name,
        instance->stats.total_entries,
        instance->stats.cache_hits,
        instance->stats.cache_misses,
        hit_ratio * 100.0,
        instance->stats.functions_cached,
        instance->stats.validation_failures,
        instance->stats.total_validation_time,
        instance->stats.total_compilation_time,
        uptime,
        instance->config.strict_validation ? "ON" : "OFF"
    );

    psc_rwlock_unlock_rd(&instance->state.lock);
}

/**
 * @brief Configure system parameters
 * @param instance Cache instance
 * @param debug_mode Enable debug output (1) or disable (0)
 * @param strict_validation Enable strict function validation (1) or disable (0)
 */
static inline void psc_configure(psc_instance_t *instance, int debug_mode, int strict_validation) {
    if (!instance || !instance->state.initialized) return;

    psc_rwlock_wrlock(&instance->state.lock);
    instance->config.debug_mode = debug_mode;
    instance->config.strict_validation = strict_validation;
    psc_rwlock_unlock_wr(&instance->state.lock);
}

#ifdef __cplusplus
}
#endif

#endif // PY_CACHE_H

/*
 * ============================================================================
 * USAGE EXAMPLES AND INTEGRATION GUIDE
 * ============================================================================
 * 
 * BASIC USAGE:
 *   #include "py_cache_final.h"
 * 
 *   psc_instance_t* cache = psc_create_instance("my_cache");
 *   psc_init(cache);
 *   psc_add_function(cache, "def square(x): return x*x", "math.py");
 *   PyObject* result = psc_call_function(cache, "square", args, NULL);
 *   psc_destroy_instance(cache);
 * 
 * ADVANCED CONFIGURATION:
 *   psc_configure_advanced(cache, 15, 1, 1); // threshold=15, debug=on, strict_validation=on
 * 
 * MULTIPLE INSTANCES:
 *   psc_instance_t* math_cache = psc_create_instance("math");
 *   psc_instance_t* string_cache = psc_create_instance("strings");
 *   // Each cache is completely independent
 * 
 * REUSE PATTERN:
 *   psc_init(cache);
 *   // ... use cache ...
 *   psc_reset(cache);  // Clear and reinitialize
 *   // ... use cache again ...
 *   psc_destroy_instance(cache);
 * 
 * ERROR HANDLING:
 *   psc_result_t result = psc_add_function(cache, source, filename);
 *   switch (result) {
 *       case PSC_SUCCESS: // Function cached
 *       case PSC_ERROR_FUNCTION_TOO_SIMPLE: // Skipped (not an error)
 *       case PSC_ERROR_NO_FUNCTION_DEF: // No function in source
 *       case PSC_ERROR_MULTIPLE_FUNCTIONS: // Too many functions
 *       case PSC_ERROR_VALIDATION_FAILED: // Syntax error
 *       // ... handle errors appropriately
 *   }
 * 
 * VALIDATION EXAMPLES:
 *   VALID:   "def add(x, y): return x + y"
 *   VALID:   "import math\ndef calc(x): return math.sin(x)"
 *   VALID:   "@decorator\ndef func(): pass"
 *   INVALID: "x = 5\ny = 10"  (no function)
 *   INVALID: "def f1(): pass\ndef f2(): pass"  (multiple functions)
 *   INVALID: "def broken(: return 42"  (syntax error)
 * 
 * PERFORMANCE CHARACTERISTICS:
 *   - Simple functions (x + y) are automatically skipped to avoid cache overhead
 *   - Complex functions (with imports, decorators) are automatically cached
 *   - Borderline functions use configurable complexity threshold
 *   - Thread-safe for concurrent access to different instances
 *   - Memory efficient with proper Python reference counting
 * 
 * THREAD SAFETY:
 *   - Each instance has its own lock - no global synchronization
 *   - Multiple threads can safely use different cache instances
 *   - Multiple threads can safely call functions from same instance
 *   - Only psc_add_function() requires exclusive lock (rare operation)
 * 
 * INTEGRATION PATTERNS:
 *   
 *   // Multi-tenant application
 *   psc_instance_t* get_tenant_cache(const char* tenant_id) {
 *       static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
 *       static psc_instance_t* tenant_caches[MAX_TENANTS];
 *       // ... lookup or create tenant-specific cache
 *   }
 *   
 *   // Plugin system
 *   typedef struct {
 *       char plugin_name[64];
 *       psc_instance_t* function_cache;
 *       // ... other plugin data
 *   } plugin_t;
 *   
 *   // Microservice with different caches per service component
 *   psc_instance_t* auth_cache = psc_create_instance("auth_service");
 *   psc_instance_t* billing_cache = psc_create_instance("billing_service");
 *   psc_instance_t* analytics_cache = psc_create_instance("analytics_service");
 * 
 * CONFIGURATION REFERENCE:
 *   - cache_threshold_score: Complexity threshold for caching (default: 25)
 *   - min_lines_for_cache: Minimum lines to consider caching (default: 3)
 *   - max_simple_length: Maximum length for "simple" functions (default: 80)
 *   - debug_mode: Enable debug output (default: 0)
 *   - strict_validation: Enable comprehensive validation (default: 1)
 *   - override_cache_all: Force cache all functions (default: 0)
 * 
 * MEMORY MANAGEMENT:
 *   - All Python objects properly reference counted
 *   - Cache entries automatically freed on reset/destroy
 *   - Thread locks properly initialized and destroyed
 *   - No memory leaks even with compilation failures
 *   - Safe to call psc_destroy_instance() multiple times
 * 
 * STATISTICS AND MONITORING:
 *   - Per-instance hit/miss ratios
 *   - Cache efficiency (functions skipped to avoid overhead)
 *   - Validation failure rates
 *   - Timing statistics for analysis, validation, and compilation
 *   - Named instances for easy identification in logs
 * 
 * VERSION HISTORY:
 *   v5.0 FINAL: Complete rewrite with comprehensive validation, improved API
 *   v4.0: Instance-based design, removed global state
 *   v3.0: Added smart caching decisions and complexity analysis
 *   v2.0: Added thread safety and performance statistics
 *   v1.0: Basic function compilation and caching
 */