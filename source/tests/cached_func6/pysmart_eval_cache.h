/**
 * @file pysmart_eval_cache.h
 * @brief Simplified Python expression evaluation cache with smart caching decisions
 * @version 1.0
 * @author Smart Expression Cache System
 * 
 * A lightweight, single-header cache system specifically for Python expressions
 * that reuses existing globals namespace and focuses on Py_eval_input mode only.
 * 
 * Features:
 * - Expression-only evaluation (no function definitions)
 * - Reuses existing Python globals (no isolation)
 * - Smart caching decisions based on expression complexity
 * - Thread-safe with cross-platform locking
 * - Minimal memory footprint
 * - No module pre-importing needed
 * 
 * Usage:
 *   #include "pysmart_eval_cache.h"
 * 
 *   // Initialize
 *   psec_init();
 * 
 *   // Evaluate expressions (automatically decides whether to cache)
 *   PyObject* result = psec_eval("2 + 3 * 4", globals, locals);
 *   PyObject* result2 = psec_eval("math.sin(x) + math.cos(y)", globals, locals);
 * 
 *   // Cleanup
 *   psec_cleanup();
 */

#ifndef PYSMART_EVAL_CACHE_H
#define PYSMART_EVAL_CACHE_H

#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CONFIGURATION
// ============================================================================

#ifndef PSEC_CACHE_SIZE
#define PSEC_CACHE_SIZE 512
#endif

#ifndef PSEC_MAX_EXPR_LENGTH
#define PSEC_MAX_EXPR_LENGTH 1024
#endif

#ifndef PSEC_CACHE_THRESHOLD_SCORE
#define PSEC_CACHE_THRESHOLD_SCORE 15
#endif

#ifndef PSEC_MAX_SIMPLE_LENGTH
#define PSEC_MAX_SIMPLE_LENGTH 50
#endif

// Thread support detection
#ifdef _WIN32
    #include <windows.h>
    typedef SRWLOCK psec_rwlock_t;
    #define psec_rwlock_init(lock) InitializeSRWLock(lock)
    #define psec_rwlock_rdlock(lock) AcquireSRWLockShared(lock)
    #define psec_rwlock_wrlock(lock) AcquireSRWLockExclusive(lock)
    #define psec_rwlock_unlock_rd(lock) ReleaseSRWLockShared(lock)
    #define psec_rwlock_unlock_wr(lock) ReleaseSRWLockExclusive(lock)
    #define psec_rwlock_destroy(lock) (void)0
#elif defined(__unix__) || defined(__APPLE__)
    #include <pthread.h>
    typedef pthread_rwlock_t psec_rwlock_t;
    #define psec_rwlock_init(lock) pthread_rwlock_init(lock, NULL)
    #define psec_rwlock_rdlock(lock) pthread_rwlock_rdlock(lock)
    #define psec_rwlock_wrlock(lock) pthread_rwlock_wrlock(lock)
    #define psec_rwlock_unlock_rd(lock) pthread_rwlock_unlock(lock)
    #define psec_rwlock_unlock_wr(lock) pthread_rwlock_unlock(lock)
    #define psec_rwlock_destroy(lock) pthread_rwlock_destroy(lock)
#else
    typedef int psec_rwlock_t;
    #define psec_rwlock_init(lock) (void)0
    #define psec_rwlock_rdlock(lock) (void)0
    #define psec_rwlock_wrlock(lock) (void)0
    #define psec_rwlock_unlock_rd(lock) (void)0
    #define psec_rwlock_unlock_wr(lock) (void)0
    #define psec_rwlock_destroy(lock) (void)0
#endif

// ============================================================================
// UNIFIED DATA STRUCTURE
// ============================================================================

/**
 * @brief Unified expression evaluation cache system
 * 
 * Single structure containing all cache data, statistics, and configuration
 */
typedef struct psec_system {
    // Cache entries
    struct psec_entry {
        char expression[PSEC_MAX_EXPR_LENGTH];
        PyObject *code_object;
        size_t expr_hash;
        time_t created_time;
        time_t last_access;
        int access_count;
        struct psec_entry *next;
        
        // Embedded complexity analysis
        struct {
            int char_count;
            int operators;
            int function_calls;
            int attribute_access;
            int subscripts;
            int comparisons;
            int logical_ops;
            int mathematical_ops;
            int complexity_score;
            int was_cached;
            char decision_reason[128];
        } analysis;
    } *hash_table[PSEC_CACHE_SIZE];
    
    // Statistics
    struct {
        size_t total_entries;
        size_t cache_hits;
        size_t cache_misses;
        size_t expressions_analyzed;
        size_t expressions_cached;
        size_t expressions_skipped;
        double total_analysis_time;
        double total_compilation_time;
    } stats;
    
    // Configuration
    struct {
        int cache_threshold_score;
        int max_simple_length;
        int debug_mode;
    } config;
    
    // System state
    struct {
        int initialized;
        psec_rwlock_t lock;
        time_t system_start_time;
    } state;
    
} psec_system_t;

// Error codes
typedef enum {
    PSEC_SUCCESS = 0,
    PSEC_ERROR_INIT = -1,
    PSEC_ERROR_COMPILE = -2,
    PSEC_ERROR_MEMORY = -3,
    PSEC_ERROR_NOT_FOUND = -4,
    PSEC_ERROR_INVALID_ARGS = -5,
    PSEC_ERROR_NOT_INITIALIZED = -6,
    PSEC_ERROR_EXPR_TOO_SIMPLE = -7
} psec_result_t;

// Global system instance
static psec_system_t g_psec_system = {0};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * Hash function for expressions
 */
static inline size_t psec_hash_string(const char *str) {
    size_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % PSEC_CACHE_SIZE;
}

/**
 * Count occurrences of character in expression
 */
static inline int psec_count_char(const char *expr, char ch) {
    int count = 0;
    const char *p = expr;
    while (*p) {
        if (*p == ch) count++;
        p++;
    }
    return count;
}

/**
 * Count occurrences of substring in expression
 */
static inline int psec_count_substr(const char *expr, const char *substr) {
    int count = 0;
    const char *p = expr;
    size_t substr_len = strlen(substr);
    
    while ((p = strstr(p, substr)) != NULL) {
        count++;
        p += substr_len;
    }
    
    return count;
}

/**
 * Check if expression contains any of the given patterns
 */
static inline int psec_contains_any(const char *expr, const char *patterns[], int pattern_count) {
    for (int i = 0; i < pattern_count; i++) {
        if (strstr(expr, patterns[i]) != NULL) {
            return 1;
        }
    }
    return 0;
}

// ============================================================================
// EXPRESSION COMPLEXITY ANALYSIS
// ============================================================================

/**
 * Analyze expression complexity for caching decisions
 */
static inline void psec_analyze_expression(const char *expression, struct psec_entry *entry) {
    memset(&entry->analysis, 0, sizeof(entry->analysis));
    
    entry->analysis.char_count = (int)strlen(expression);
    
    // Count arithmetic operators
    entry->analysis.operators = 
        psec_count_char(expression, '+') +
        psec_count_char(expression, '-') + 
        psec_count_char(expression, '*') +
        psec_count_char(expression, '/') +
        psec_count_char(expression, '%') +
        psec_count_substr(expression, "**") +
        psec_count_substr(expression, "//");
    
    // Count function calls (look for parentheses after identifiers)
    entry->analysis.function_calls = psec_count_char(expression, '(');
    
    // Count attribute access (dot notation)
    entry->analysis.attribute_access = psec_count_char(expression, '.');
    
    // Count subscripts/indexing
    entry->analysis.subscripts = psec_count_char(expression, '[');
    
    // Count comparison operators
    entry->analysis.comparisons = 
        psec_count_substr(expression, "==") +
        psec_count_substr(expression, "!=") +
        psec_count_substr(expression, "<=") +
        psec_count_substr(expression, ">=") +
        psec_count_char(expression, '<') +
        psec_count_char(expression, '>') - 
        (psec_count_substr(expression, "<=") + psec_count_substr(expression, ">=")) * 2;
    
    // Count logical operators
    entry->analysis.logical_ops = 
        psec_count_substr(expression, " and ") +
        psec_count_substr(expression, " or ") +
        psec_count_substr(expression, " not ");
    
    // Count mathematical functions (common ones)
    const char *math_funcs[] = {
        "sin", "cos", "tan", "exp", "log", "sqrt", "abs", "pow", "round", "ceil", "floor"
    };
    entry->analysis.mathematical_ops = 0;
    for (size_t i = 0; i < sizeof(math_funcs) / sizeof(math_funcs[0]); i++) {
        entry->analysis.mathematical_ops += psec_count_substr(expression, math_funcs[i]);
    }
    
    // Calculate complexity score
    int score = 0;
    
    // Base score for length
    if (entry->analysis.char_count > 20) {
        score += (entry->analysis.char_count - 20) / 5; // 1 point per 5 chars after 20
    }
    
    // Weight different complexity factors
    score += entry->analysis.operators * 2;              // Simple operations
    score += entry->analysis.function_calls * 8;         // Function calls expensive
    score += entry->analysis.attribute_access * 5;       // Module/attribute access
    score += entry->analysis.subscripts * 4;             // Indexing operations
    score += entry->analysis.comparisons * 3;            // Comparison operations
    score += entry->analysis.logical_ops * 6;            // Logical operations
    score += entry->analysis.mathematical_ops * 10;      // Math functions expensive
    
    entry->analysis.complexity_score = score;
}

/**
 * Smart caching decision for expressions
 */
static inline int psec_should_cache_expression(const char *expression, struct psec_entry *entry) {
    if (!expression || strlen(expression) == 0) {
        strcpy(entry->analysis.decision_reason, "Empty expression");
        return 0;
    }
    
    // Perform complexity analysis
    psec_analyze_expression(expression, entry);
    
    // Quick filters for obviously simple expressions
    size_t length = strlen(expression);
    
    // Very short simple expressions
    if (length < g_psec_system.config.max_simple_length && 
        entry->analysis.function_calls == 0 && 
        entry->analysis.attribute_access == 0) {
        strcpy(entry->analysis.decision_reason, "Too short and simple");
        return 0;
    }
    
    // Simple arithmetic without function calls
    if (entry->analysis.function_calls == 0 && 
        entry->analysis.attribute_access == 0 &&
        entry->analysis.mathematical_ops == 0 &&
        entry->analysis.complexity_score < 10) {
        strcpy(entry->analysis.decision_reason, "Simple arithmetic");
        return 0;
    }
    
    // Always cache expressions with function calls
    if (entry->analysis.function_calls > 0) {
        strcpy(entry->analysis.decision_reason, "Has function calls");
        return 1;
    }
    
    // Always cache expressions with attribute access (module functions)
    if (entry->analysis.attribute_access > 0) {
        strcpy(entry->analysis.decision_reason, "Has attribute access");
        return 1;
    }
    
    // Always cache mathematical functions
    if (entry->analysis.mathematical_ops > 0) {
        strcpy(entry->analysis.decision_reason, "Has math functions");
        return 1;
    }
    
    // Use threshold for borderline cases
    if (entry->analysis.complexity_score >= g_psec_system.config.cache_threshold_score) {
        snprintf(entry->analysis.decision_reason, sizeof(entry->analysis.decision_reason),
                "Score %d >= %d", entry->analysis.complexity_score, 
                g_psec_system.config.cache_threshold_score);
        return 1;
    } else {
        snprintf(entry->analysis.decision_reason, sizeof(entry->analysis.decision_reason),
                "Score %d < %d", entry->analysis.complexity_score, 
                g_psec_system.config.cache_threshold_score);
        return 0;
    }
}

// ============================================================================
// MEMORY MANAGEMENT
// ============================================================================

/**
 * Create a new cache entry
 */
static inline struct psec_entry* psec_create_entry(const char *expression, PyObject *code_object) {
    struct psec_entry *entry = malloc(sizeof(struct psec_entry));
    if (!entry) return NULL;
    
    memset(entry, 0, sizeof(struct psec_entry));
    
    strncpy(entry->expression, expression, PSEC_MAX_EXPR_LENGTH - 1);
    entry->code_object = code_object;
    entry->expr_hash = psec_hash_string(expression);
    entry->created_time = time(NULL);
    entry->last_access = entry->created_time;
    entry->access_count = 0;
    entry->next = NULL;
    
    Py_INCREF(code_object);
    
    return entry;
}

/**
 * Free a cache entry
 */
static inline void psec_free_entry(struct psec_entry *entry) {
    if (!entry) return;
    
    Py_XDECREF(entry->code_object);
    free(entry);
}

// ============================================================================
// CORE OPERATIONS
// ============================================================================

/**
 * Find expression in cache (assumes lock is held)
 */
static inline struct psec_entry* psec_find_expression_unlocked(const char *expression) {
    size_t hash_idx = psec_hash_string(expression);
    struct psec_entry *current = g_psec_system.hash_table[hash_idx];
    
    while (current) {
        if (strcmp(current->expression, expression) == 0) {
            current->last_access = time(NULL);
            current->access_count++;
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

/**
 * Compile expression to code object
 */
static inline psec_result_t psec_compile_expression(const char *expression, PyObject **code_obj) {
    clock_t compile_start = clock();
    
    *code_obj = Py_CompileString(expression, "<cached_expr>", Py_eval_input);
    if (!*code_obj) {
        if (g_psec_system.config.debug_mode && PyErr_Occurred()) {
            printf("PSEC: Compilation error for expression: %s\n", expression);
            PyErr_Print();
        }
        return PSEC_ERROR_COMPILE;
    }
    
    clock_t compile_end = clock();
    g_psec_system.stats.total_compilation_time += 
        ((double)(compile_end - compile_start)) / CLOCKS_PER_SEC;
    
    return PSEC_SUCCESS;
}

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * @brief Initialize the expression evaluation cache
 * @return PSEC_SUCCESS on success, error code on failure
 */
static inline psec_result_t psec_init(void) {
    if (g_psec_system.state.initialized) {
        return PSEC_SUCCESS;
    }
    
    memset(&g_psec_system, 0, sizeof(psec_system_t));
    
    if (psec_rwlock_init(&g_psec_system.state.lock) != 0) {
        return PSEC_ERROR_INIT;
    }
    
    // Set default configuration
    g_psec_system.config.cache_threshold_score = PSEC_CACHE_THRESHOLD_SCORE;
    g_psec_system.config.max_simple_length = PSEC_MAX_SIMPLE_LENGTH;
    g_psec_system.config.debug_mode = 0;
    
    g_psec_system.state.initialized = 1;
    g_psec_system.state.system_start_time = time(NULL);
    
    return PSEC_SUCCESS;
}

/**
 * @brief Evaluate a Python expression with smart caching
 * @param expression Python expression string
 * @param globals Global namespace (reused, not isolated)
 * @param locals Local namespace (can be NULL)
 * @return PyObject* result or NULL on error
 * 
 * This is the main API function. It automatically decides whether to cache
 * the expression based on complexity analysis and reuses the provided globals.
 */
static inline PyObject* psec_eval(const char *expression, PyObject *globals, PyObject *locals) {
    if (!g_psec_system.state.initialized) {
        PyErr_SetString(PyExc_RuntimeError, "Expression cache not initialized");
        return NULL;
    }
    
    if (!expression || strlen(expression) == 0) {
        PyErr_SetString(PyExc_ValueError, "Empty expression");
        return NULL;
    }
    
    if (!globals) {
        PyErr_SetString(PyExc_ValueError, "Globals dictionary required");
        return NULL;
    }
    
    clock_t analysis_start = clock();
    
    // Check cache first
    psec_rwlock_rdlock(&g_psec_system.state.lock);
    
    struct psec_entry *entry = psec_find_expression_unlocked(expression);
    if (entry) {
        g_psec_system.stats.cache_hits++;
        PyObject *code_obj = entry->code_object;
        Py_INCREF(code_obj); // Ensure we own a reference
        
        psec_rwlock_unlock_rd(&g_psec_system.state.lock);
        
        // Evaluate using cached code object
        PyObject *result = PyEval_EvalCode(code_obj, globals, locals);
        Py_DECREF(code_obj);
        return result;
    }
    
    psec_rwlock_unlock_rd(&g_psec_system.state.lock);
    g_psec_system.stats.cache_misses++;
    
    // Not in cache - analyze for caching decision
    struct psec_entry temp_entry = {0};
    strncpy(temp_entry.expression, expression, sizeof(temp_entry.expression) - 1);
    
    g_psec_system.stats.expressions_analyzed++;
    int should_cache = psec_should_cache_expression(expression, &temp_entry);
    
    clock_t analysis_end = clock();
    g_psec_system.stats.total_analysis_time += 
        ((double)(analysis_end - analysis_start)) / CLOCKS_PER_SEC;
    
    // Compile the expression
    PyObject *code_obj;
    psec_result_t compile_result = psec_compile_expression(expression, &code_obj);
    if (compile_result != PSEC_SUCCESS) {
        return NULL; // Error already set by compilation
    }
    
    // If worth caching, add to cache
    if (should_cache) {
        struct psec_entry *new_entry = psec_create_entry(expression, code_obj);
        if (new_entry) {
            new_entry->analysis = temp_entry.analysis;
            new_entry->analysis.was_cached = 1;
            
            psec_rwlock_wrlock(&g_psec_system.state.lock);
            
            size_t hash_idx = psec_hash_string(expression);
            new_entry->next = g_psec_system.hash_table[hash_idx];
            g_psec_system.hash_table[hash_idx] = new_entry;
            g_psec_system.stats.total_entries++;
            g_psec_system.stats.expressions_cached++;
            
            psec_rwlock_unlock_wr(&g_psec_system.state.lock);
            
            if (g_psec_system.config.debug_mode) {
                printf("PSEC: Cached expression - %s\n", new_entry->analysis.decision_reason);
            }
        }
    } else {
        g_psec_system.stats.expressions_skipped++;
        if (g_psec_system.config.debug_mode) {
            printf("PSEC: Skipped expression - %s\n", temp_entry.analysis.decision_reason);
        }
    }
    
    // Evaluate and return result
    PyObject *result = PyEval_EvalCode(code_obj, globals, locals);
    Py_DECREF(code_obj);
    return result;
}

/**
 * @brief Get cache statistics as formatted string
 * @param stats_buffer Buffer to store statistics
 * @param buffer_size Size of the buffer
 */
static inline void psec_get_stats_string(char *stats_buffer, size_t buffer_size) {
    if (!g_psec_system.state.initialized) {
        snprintf(stats_buffer, buffer_size, "Expression cache not initialized");
        return;
    }
    
    psec_rwlock_rdlock(&g_psec_system.state.lock);
    
    size_t total_evals = g_psec_system.stats.cache_hits + g_psec_system.stats.cache_misses;
    double hit_ratio = total_evals > 0 ? (double)g_psec_system.stats.cache_hits / total_evals : 0.0;
    double cache_efficiency = g_psec_system.stats.expressions_analyzed > 0 ?
                             (double)g_psec_system.stats.expressions_skipped / g_psec_system.stats.expressions_analyzed : 0.0;
    
    time_t uptime = time(NULL) - g_psec_system.state.system_start_time;
    
    snprintf(stats_buffer, buffer_size,
        "=== Expression Cache Statistics ===\n"
        "Cached Expressions: %zu\n"
        "Cache Hits: %zu\n"
        "Cache Misses: %zu\n"
        "Hit Ratio: %.2f%%\n"
        "Expressions Analyzed: %zu\n"
        "Expressions Cached: %zu\n"
        "Expressions Skipped: %zu\n"
        "Cache Efficiency: %.2f%% (avoided overhead)\n"
        "Total Analysis Time: %.6f seconds\n"
        "Total Compilation Time: %.6f seconds\n"
        "Average Analysis Time: %.6f seconds\n"
        "System Uptime: %ld seconds\n"
        "Configuration: threshold=%d, max_simple=%d",
        g_psec_system.stats.total_entries,
        g_psec_system.stats.cache_hits,
        g_psec_system.stats.cache_misses,
        hit_ratio * 100.0,
        g_psec_system.stats.expressions_analyzed,
        g_psec_system.stats.expressions_cached,
        g_psec_system.stats.expressions_skipped,
        cache_efficiency * 100.0,
        g_psec_system.stats.total_analysis_time,
        g_psec_system.stats.total_compilation_time,
        g_psec_system.stats.expressions_analyzed > 0 ? 
            g_psec_system.stats.total_analysis_time / g_psec_system.stats.expressions_analyzed : 0.0,
        uptime,
        g_psec_system.config.cache_threshold_score,
        g_psec_system.config.max_simple_length
    );
    
    psec_rwlock_unlock_rd(&g_psec_system.state.lock);
}

/**
 * @brief Explain why an expression would or wouldn't be cached
 * @param expression Python expression to analyze
 * @param explanation_buffer Buffer to store explanation
 * @param buffer_size Size of explanation buffer
 */
static inline void psec_explain_decision(const char *expression, char *explanation_buffer, size_t buffer_size) {
    if (!expression) {
        snprintf(explanation_buffer, buffer_size, "No expression provided");
        return;
    }
    
    struct psec_entry temp_entry = {0};
    int should_cache = psec_should_cache_expression(expression, &temp_entry);
    
    snprintf(explanation_buffer, buffer_size,
        "Expression: \"%s\"\n"
        "Decision: %s\n"
        "Complexity Score: %d (threshold: %d)\n"
        "Reason: %s\n"
        "Analysis Details:\n"
        "  Length: %d characters\n"
        "  Operators: %d\n"
        "  Function calls: %d\n"
        "  Attribute access: %d\n"
        "  Subscripts: %d\n"
        "  Comparisons: %d\n"
        "  Logical ops: %d\n"
        "  Math functions: %d",
        expression,
        should_cache ? "✅ CACHE" : "❌ SKIP",
        temp_entry.analysis.complexity_score,
        g_psec_system.config.cache_threshold_score,
        temp_entry.analysis.decision_reason,
        temp_entry.analysis.char_count,
        temp_entry.analysis.operators,
        temp_entry.analysis.function_calls,
        temp_entry.analysis.attribute_access,
        temp_entry.analysis.subscripts,
        temp_entry.analysis.comparisons,
        temp_entry.analysis.logical_ops,
        temp_entry.analysis.mathematical_ops
    );
}

/**
 * @brief Configure the expression cache
 * @param threshold Complexity threshold for caching decisions
 * @param debug_mode Enable debug output (1) or disable (0)
 */
static inline void psec_configure(int threshold, int debug_mode) {
    if (!g_psec_system.state.initialized) return;
    
    psec_rwlock_wrlock(&g_psec_system.state.lock);
    g_psec_system.config.cache_threshold_score = threshold;
    g_psec_system.config.debug_mode = debug_mode;
    psec_rwlock_unlock_wr(&g_psec_system.state.lock);
}

/**
 * @brief Clean up the expression cache system
 */
static inline void psec_cleanup(void) {
    if (!g_psec_system.state.initialized) return;
    
    psec_rwlock_wrlock(&g_psec_system.state.lock);
    
    // Free all cache entries
    for (int i = 0; i < PSEC_CACHE_SIZE; i++) {
        struct psec_entry *current = g_psec_system.hash_table[i];
        while (current) {
            struct psec_entry *next = current->next;
            psec_free_entry(current);
            current = next;
        }
        g_psec_system.hash_table[i] = NULL;
    }
    
    memset(&g_psec_system.stats, 0, sizeof(g_psec_system.stats));
    
    psec_rwlock_unlock_wr(&g_psec_system.state.lock);
    psec_rwlock_destroy(&g_psec_system.state.lock);
    
    g_psec_system.state.initialized = 0;
}

// ============================================================================
// CONVENIENCE MACROS
// ============================================================================

#define PSEC_EVAL_SIMPLE(expr, globals) \
    psec_eval(expr, globals, NULL)

#define PSEC_EVAL_WITH_LOCALS(expr, globals, locals) \
    psec_eval(expr, globals, locals)

#ifdef __cplusplus
}
#endif

#endif // PYSMART_EVAL_CACHE_H
