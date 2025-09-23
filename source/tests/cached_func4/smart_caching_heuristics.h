/**
 * @file smart_caching_heuristics.h
 * @brief Smart caching decision engine for py_function_cache.h
 * 
 * This header extends the function cache with intelligent caching decisions
 * based on source code analysis heuristics. It determines whether a function
 * is worth caching based on complexity indicators without requiring compilation.
 * 
 * Usage:
 *   #include "py_function_cache.h"
 *   #include "smart_caching_heuristics.h"
 * 
 *   if (should_cache_function(source_code)) {
 *       pyfc_add_function(source_code, filename);
 *   }
 */

#ifndef SMART_CACHING_HEURISTICS_H
#define SMART_CACHING_HEURISTICS_H

#include <string.h>
#include <ctype.h>

// Configuration constants - tune these based on your use case
#ifndef PYFC_CACHE_THRESHOLD_SCORE
#define PYFC_CACHE_THRESHOLD_SCORE 25
#endif

#ifndef PYFC_MIN_LINES_FOR_CACHE
#define PYFC_MIN_LINES_FOR_CACHE 3
#endif

#ifndef PYFC_MAX_SIMPLE_LENGTH
#define PYFC_MAX_SIMPLE_LENGTH 80
#endif

// Complexity indicators and their weights
typedef struct {
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
} pyfc_complexity_analysis_t;

/**
 * Count the number of lines in source code
 */
static inline int pyfc_count_lines(const char *source) {
    int lines = 1; // At least one line
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
 * Count occurrences of a substring in source code
 */
static inline int pyfc_count_occurrences(const char *source, const char *pattern) {
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
 * Check if string contains any of the given patterns
 */
static inline int pyfc_contains_any(const char *source, const char *patterns[], int pattern_count) {
    for (int i = 0; i < pattern_count; i++) {
        if (strstr(source, patterns[i]) != NULL) {
            return 1;
        }
    }
    return 0;
}

/**
 * Count complex mathematical operations
 */
static inline int pyfc_count_math_complexity(const char *source) {
    const char *math_patterns[] = {
        "math.", "numpy.", "scipy.", "statistics.",
        "**", "//", "sqrt", "log", "exp", "sin", "cos", "tan"
    };
    int count = 0;
    
    for (size_t i = 0; i < sizeof(math_patterns) / sizeof(math_patterns[0]); i++) {
        count += pyfc_count_occurrences(source, math_patterns[i]);
    }
    
    return count;
}

/**
 * Count builtin function calls (these are generally fast but add complexity)
 */
static inline int pyfc_count_builtin_calls(const char *source) {
    const char *builtin_patterns[] = {
        "len(", "range(", "enumerate(", "zip(", "map(", "filter(", 
        "sum(", "max(", "min(", "sorted(", "reversed(", "any(", "all("
    };
    int count = 0;
    
    for (size_t i = 0; i < sizeof(builtin_patterns) / sizeof(builtin_patterns[0]); i++) {
        count += pyfc_count_occurrences(source, builtin_patterns[i]);
    }
    
    return count;
}

/**
 * Analyze source code complexity
 */
static inline pyfc_complexity_analysis_t pyfc_analyze_complexity(const char *source) {
    pyfc_complexity_analysis_t analysis = {0};
    
    // Basic metrics
    analysis.line_count = pyfc_count_lines(source);
    analysis.char_count = (int)strlen(source);
    
    // Import statements (heavy compilation cost)
    analysis.import_statements = pyfc_count_occurrences(source, "import ") +
                                pyfc_count_occurrences(source, "from ");
    
    // Decorators (moderate compilation cost)
    analysis.decorator_count = pyfc_count_occurrences(source, "@");
    
    // Control flow constructs
    analysis.loop_constructs = pyfc_count_occurrences(source, " for ") +
                              pyfc_count_occurrences(source, " while ") +
                              pyfc_count_occurrences(source, "\nfor ") +
                              pyfc_count_occurrences(source, "\nwhile ");
    
    // List/dict/set comprehensions (moderate cost)
    analysis.comprehensions = pyfc_count_occurrences(source, " for ") +
                             pyfc_count_occurrences(source, " if ") - analysis.loop_constructs;
    if (analysis.comprehensions < 0) analysis.comprehensions = 0;
    
    // Lambda functions (compilation complexity)
    analysis.lambda_functions = pyfc_count_occurrences(source, "lambda ");
    
    // Complex operations
    const char *complex_ops[] = {
        "yield", "async", "await", "with ", "class ", "try:", "except", "finally"
    };
    analysis.complex_operations = pyfc_contains_any(source, complex_ops, 8);
    
    // Nested function definitions
    analysis.nested_functions = pyfc_count_occurrences(source, "\n    def ") +
                               pyfc_count_occurrences(source, "\n        def ");
    
    // Exception handling
    analysis.exception_handling = pyfc_count_occurrences(source, "try:") +
                                 pyfc_count_occurrences(source, "except") +
                                 pyfc_count_occurrences(source, "raise ");
    
    // String operations (can be expensive)
    analysis.string_operations = pyfc_count_occurrences(source, ".format(") +
                                pyfc_count_occurrences(source, "f\"") +
                                pyfc_count_occurrences(source, "f'") +
                                pyfc_count_occurrences(source, ".join(") +
                                pyfc_count_occurrences(source, ".split(");
    
    // Mathematical operations
    analysis.mathematical_operations = pyfc_count_math_complexity(source);
    
    // Builtin function calls
    analysis.builtin_function_calls = pyfc_count_builtin_calls(source);
    
    return analysis;
}

/**
 * Calculate complexity score based on analysis
 */
static inline int pyfc_calculate_complexity_score(const pyfc_complexity_analysis_t *analysis) {
    int score = 0;
    
    // Line count contributes to compilation time
    if (analysis->line_count > 5) {
        score += (analysis->line_count - 5) * 2; // 2 points per line after 5
    }
    
    // Import statements are expensive
    score += analysis->import_statements * 50;
    
    // Decorators add compilation complexity
    score += analysis->decorator_count * 20;
    
    // Control flow structures
    score += analysis->loop_constructs * 8;
    score += analysis->comprehensions * 6;
    
    // Advanced language features
    score += analysis->lambda_functions * 10;
    score += analysis->complex_operations * 25;
    score += analysis->nested_functions * 15;
    score += analysis->exception_handling * 12;
    
    // String and math operations (moderate cost)
    score += analysis->string_operations * 4;
    score += analysis->mathematical_operations * 3;
    
    // Builtin calls (low cost but indicate complexity)
    score += analysis->builtin_function_calls * 2;
    
    return score;
}

/**
 * Determine if a function should be cached based on heuristic analysis
 * 
 * @param source_code Python source code containing function definition
 * @return 1 if function should be cached, 0 if it's too simple to benefit
 * 
 * This function analyzes source code complexity without compilation to decide
 * whether caching would be beneficial. It considers:
 * 
 * HIGH CACHE VALUE (always cache):
 * - Import statements (expensive compilation)
 * - Decorators (compilation complexity)  
 * - Complex language features (async, yield, etc.)
 * - Many lines of code
 * - Nested functions
 * - Exception handling
 * 
 * MODERATE CACHE VALUE:
 * - Loops and comprehensions
 * - Lambda functions
 * - String formatting operations
 * - Mathematical operations
 * 
 * LOW CACHE VALUE (usually don't cache):
 * - Simple arithmetic
 * - Basic builtin calls
 * - Short, straightforward functions
 */
static inline int should_cache_function(const char *source_code) {
    if (!source_code || strlen(source_code) == 0) {
        return 0;
    }
    
    // Quick filters for obviously simple functions
    size_t length = strlen(source_code);
    
    // Very short functions are rarely worth caching
    if (length < PYFC_MAX_SIMPLE_LENGTH && 
        !strstr(source_code, "import") && 
        !strstr(source_code, "@")) {
        return 0;
    }
    
    // Single-line arithmetic functions
    int line_count = pyfc_count_lines(source_code);
    if (line_count < PYFC_MIN_LINES_FOR_CACHE && 
        !strstr(source_code, "import") &&
        !strstr(source_code, "lambda")) {
        return 0;
    }
    
    // Analyze complexity
    pyfc_complexity_analysis_t analysis = pyfc_analyze_complexity(source_code);
    int complexity_score = pyfc_calculate_complexity_score(&analysis);
    
    // Always cache functions with imports (expensive compilation)
    if (analysis.import_statements > 0) {
        return 1;
    }
    
    // Always cache functions with decorators
    if (analysis.decorator_count > 0) {
        return 1;
    }
    
    // Always cache functions with advanced features
    if (analysis.complex_operations > 0 || analysis.nested_functions > 0) {
        return 1;
    }
    
    // Use threshold for borderline cases
    return complexity_score >= PYFC_CACHE_THRESHOLD_SCORE;
}

/**
 * Enhanced add_function that includes intelligent caching decision
 * 
 * This function wraps pyfc_add_function() with smart caching logic.
 * It will only cache functions that are likely to benefit from caching.
 * 
 * @param source_code Python source code
 * @param filename Filename for debugging
 * @return PYFC_SUCCESS if function was cached or deemed unnecessary to cache,
 *         error code on actual failure
 */
static inline pyfc_result_t pyfc_add_function_smart(const char *source_code, const char *filename) {
    if (!should_cache_function(source_code)) {
        // Function is too simple to benefit from caching
        return PYFC_SUCCESS; // Not an error - just not worth caching
    }
    
    return pyfc_add_function(source_code, filename);
}

/**
 * Get detailed analysis of why a function would or wouldn't be cached
 * Useful for debugging and optimization
 */
static inline void pyfc_explain_caching_decision(const char *source_code, char *explanation, size_t explanation_size) {
    if (!source_code) {
        snprintf(explanation, explanation_size, "No source code provided");
        return;
    }
    
    pyfc_complexity_analysis_t analysis = pyfc_analyze_complexity(source_code);
    int score = pyfc_calculate_complexity_score(&analysis);
    int should_cache = should_cache_function(source_code);
    
    snprintf(explanation, explanation_size,
        "Caching Decision: %s (Score: %d, Threshold: %d)\n"
        "Analysis:\n"
        "  Lines: %d, Characters: %d\n"
        "  Imports: %d (+%d points)\n"
        "  Decorators: %d (+%d points)\n"
        "  Loops: %d (+%d points)\n"
        "  Lambdas: %d (+%d points)\n"
        "  Complex ops: %d (+%d points)\n"
        "  Math ops: %d (+%d points)",
        should_cache ? "CACHE" : "SKIP",
        score, PYFC_CACHE_THRESHOLD_SCORE,
        analysis.line_count, analysis.char_count,
        analysis.import_statements, analysis.import_statements * 50,
        analysis.decorator_count, analysis.decorator_count * 20,
        analysis.loop_constructs, analysis.loop_constructs * 8,
        analysis.lambda_functions, analysis.lambda_functions * 10,
        analysis.complex_operations, analysis.complex_operations * 25,
        analysis.mathematical_operations, analysis.mathematical_operations * 3
    );
}

// Convenience macros for tuning
#define PYFC_SET_CACHE_THRESHOLD(threshold) \
    const int pyfc_runtime_threshold = threshold;

#define PYFC_SHOULD_CACHE_VERBOSE(source, explanation_buffer) \
    do { \
        int decision = should_cache_function(source); \
        pyfc_explain_caching_decision(source, explanation_buffer, sizeof(explanation_buffer)); \
        printf("Caching decision for function: %s\n%s\n", decision ? "CACHE" : "SKIP", explanation_buffer); \
    } while(0)

#endif // SMART_CACHING_HEURISTICS_H

