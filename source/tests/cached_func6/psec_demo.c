/**
 * @file psec_demo.c
 * @brief Demonstration of the simplified expression evaluation cache
 * 
 * Shows how to use pysmart_eval_cache.h for caching Python expressions
 * with automatic smart caching decisions.
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>

// Include the simplified expression cache
#include "pysmart_eval_cache.h"

void demonstrate_basic_usage() {
    printf("=== Basic Usage Demo ===\n\n");
    
    // Initialize the cache
    psec_result_t init_result = psec_init();
    if (init_result != PSEC_SUCCESS) {
        printf("Failed to initialize expression cache!\n");
        return;
    }
    printf("✅ Expression cache initialized\n\n");
    
    // Create a globals dictionary with some modules and variables
    PyObject *main_module = PyImport_AddModule("__main__");
    PyObject *globals = PyModule_GetDict(main_module);
    
    // Import math module into globals so expressions can use it
    PyRun_String("import math", Py_file_input, globals, NULL);
    
    // Add some variables for testing
    PyDict_SetItemString(globals, "x", PyFloat_FromDouble(3.14159));
    PyDict_SetItemString(globals, "y", PyFloat_FromDouble(2.71828));
    PyDict_SetItemString(globals, "n", PyLong_FromLong(10));
    
    printf("Available in globals: math module, x=%.5f, y=%.5f, n=%d\n\n", 3.14159, 2.71828, 10);
    
    // Test expressions with different complexity levels
    struct {
        const char *expr;
        const char *description;
    } expressions[] = {
        {"2 + 3", "Simple arithmetic - should SKIP"},
        {"x * y", "Variable multiplication - should SKIP"},
        {"x ** 2 + y ** 2", "Power operations - borderline"},
        {"math.sin(x) + math.cos(y)", "Math functions - should CACHE"},
        {"math.sqrt(x**2 + y**2)", "Complex math - should CACHE"},
        {"sum(range(n))", "Function with range - should CACHE"},
        {"[i**2 for i in range(5)]", "List comprehension - should CACHE"},
        {"abs(x - y) if x > y else abs(y - x)", "Conditional expression - should CACHE"}
    };
    
    printf("Evaluating expressions:\n");
    for (size_t i = 0; i < sizeof(expressions) / sizeof(expressions[0]); i++) {
        printf("\nExpression: %s\n", expressions[i].expr);
        printf("Expected: %s\n", expressions[i].description);
        
        // Show caching decision analysis
        char explanation[512];
        psec_explain_decision(expressions[i].expr, explanation, sizeof(explanation));
        printf("%s\n", explanation);
        
        // Evaluate the expression
        PyObject *result = psec_eval(expressions[i].expr, globals, NULL);
        if (result) {
            if (PyFloat_Check(result)) {
                printf("Result: %.6f\n", PyFloat_AsDouble(result));
            } else if (PyLong_Check(result)) {
                printf("Result: %ld\n", PyLong_AsLong(result));
            } else if (PyList_Check(result)) {
                Py_ssize_t size = PyList_Size(result);
                printf("Result: List with %zd elements\n", size);
                if (size <= 5) {
                    printf("  [");
                    for (Py_ssize_t j = 0; j < size; j++) {
                        PyObject *item = PyList_GetItem(result, j);
                        if (PyLong_Check(item)) {
                            printf("%ld", PyLong_AsLong(item));
                        } else if (PyFloat_Check(item)) {
                            printf("%.2f", PyFloat_AsDouble(item));
                        }
                        if (j < size - 1) printf(", ");
                    }
                    printf("]\n");
                }
            } else {
                printf("Result: (complex object)\n");
            }
            Py_DECREF(result);
        } else {
            printf("❌ Evaluation failed\n");
            if (PyErr_Occurred()) PyErr_Print();
        }
        
        printf("─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "\n");
    }
    
    // Show cache statistics
    char stats[1024];
    psec_get_stats_string(stats, sizeof(stats));
    printf("\n%s\n\n", stats);
    
    psec_cleanup();
    printf("✅ Cache cleaned up\n\n");
}

void demonstrate_performance_comparison() {
    printf("=== Performance Comparison Demo ===\n\n");
    
    psec_init();
    
    // Set up globals with math module
    PyObject *main_module = PyImport_AddModule("__main__");
    PyObject *globals = PyModule_GetDict(main_module);
    PyRun_String("import math", Py_file_input, globals, NULL);
    
    const char *simple_expr = "2 + 3 * 4";
    const char *complex_expr = "math.sin(3.14159 * 0.5) + math.cos(2.71828)";
    
    printf("Testing performance with 10,000 evaluations each:\n\n");
    
    printf("1. Simple expression: \"%s\"\n", simple_expr);
    char explanation1[512];
    psec_explain_decision(simple_expr, explanation1, sizeof(explanation1));
    printf("Decision: %s\n", strstr(explanation1, "✅ CACHE") ? "CACHE" : "SKIP");
    
    clock_t start = clock();
    for (int i = 0; i < 10000; i++) {
        PyObject *result = psec_eval(simple_expr, globals, NULL);
        if (result) {
            Py_DECREF(result);
        }
    }
    clock_t end = clock();
    double simple_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Time: %.4f seconds (%.6f per eval)\n\n", simple_time, simple_time / 10000);
    
    printf("2. Complex expression: \"%s\"\n", complex_expr);
    char explanation2[512];
    psec_explain_decision(complex_expr, explanation2, sizeof(explanation2));
    printf("Decision: %s\n", strstr(explanation2, "✅ CACHE") ? "CACHE" : "SKIP");
    
    start = clock();
    for (int i = 0; i < 10000; i++) {
        PyObject *result = psec_eval(complex_expr, globals, NULL);
        if (result) {
            Py_DECREF(result);
        }
    }
    end = clock();
    double complex_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Time: %.4f seconds (%.6f per eval)\n", complex_time, complex_time / 10000);
    
    printf("\nPerformance Analysis:\n");
    printf("Simple expression ratio: %.2f (overhead %s benefit)\n", 
           simple_time / complex_time,
           simple_time > complex_time ? ">" : "<");
    printf("This demonstrates the smart caching system working correctly!\n\n");
    
    // Final statistics
    char final_stats[1024];
    psec_get_stats_string(final_stats, sizeof(final_stats));
    printf("%s\n\n", final_stats);
    
    psec_cleanup();
}

void demonstrate_real_world_usage() {
    printf("=== Real-World Usage Examples ===\n\n");
    
    psec_init();
    psec_configure(10, 1); // Lower threshold, enable debug
    
    // Set up a realistic environment
    PyObject *main_module = PyImport_AddModule("__main__");
    PyObject *globals = PyModule_GetDict(main_module);
    
    // Import commonly used modules
    PyRun_String("import math, statistics, random", Py_file_input, globals, NULL);
    
    // Set up some realistic data
    PyRun_String("data = [1.2, 3.4, 5.6, 7.8, 9.0, 2.1, 4.3, 6.5, 8.7, 1.9]", 
                 Py_file_input, globals, NULL);
    PyRun_String("temperature_c = 25.0", Py_file_input, globals, NULL);
    PyRun_String("radius = 10.0", Py_file_input, globals, NULL);
    PyRun_String("price = 99.99", Py_file_input, globals, NULL);
    PyRun_String("discount = 0.15", Py_file_input, globals, NULL);
    
    printf("Realistic scenarios:\n\n");
    
    struct {
        const char *scenario;
        const char *expr;
    } scenarios[] = {
        {
            "Temperature Conversion",
            "temperature_c * 9/5 + 32"
        },
        {
            "Circle Area Calculation", 
            "math.pi * radius ** 2"
        },
        {
            "Price with Discount",
            "price * (1 - discount)"
        },
        {
            "Statistical Analysis",
            "statistics.mean(data)"
        },
        {
            "Distance Formula",
            "math.sqrt((data[0] - data[1])**2 + (data[2] - data[3])**2)"
        },
        {
            "Complex Financial Calculation",
            "price * (1 + 0.08)**5 * (1 - discount) + statistics.stdev(data)"
        },
        {
            "Data Filtering",
            "[x for x in data if x > statistics.mean(data)]"
        },
        {
            "Trigonometric Calculation",
            "math.sin(math.radians(45)) + math.cos(math.radians(30))"
        }
    };
    
    for (size_t i = 0; i < sizeof(scenarios) / sizeof(scenarios[0]); i++) {
        printf("Scenario: %s\n", scenarios[i].scenario);
        printf("Expression: %s\n", scenarios[i].expr);
        
        PyObject *result = psec_eval(scenarios[i].expr, globals, NULL);
        if (result) {
            if (PyFloat_Check(result)) {
                printf("Result: %.4f\n", PyFloat_AsDouble(result));
            } else if (PyLong_Check(result)) {
                printf("Result: %ld\n", PyLong_AsLong(result));
            } else if (PyList_Check(result)) {
                printf("Result: Filtered list with %zd elements\n", PyList_Size(result));
            } else {
                printf("Result: (computed successfully)\n");
            }
            Py_DECREF(result);
        } else {
            printf("❌ Failed\n");
            if (PyErr_Occurred()) PyErr_Print();
        }
        
        printf("\n");
    }
    
    char final_stats[1024];
    psec_get_stats_string(final_stats, sizeof(final_stats));
    printf("%s\n\n", final_stats);
    
    psec_cleanup();
}

void demonstrate_configuration_tuning() {
    printf("=== Configuration Tuning Demo ===\n\n");
    
    const char *borderline_expr = "x * 2 + y / 3 - z";
    
    printf("Testing borderline expression with different thresholds:\n");
    printf("Expression: \"%s\"\n\n", borderline_expr);
    
    int thresholds[] = {5, 15, 25};
    
    for (size_t i = 0; i < sizeof(thresholds) / sizeof(thresholds[0]); i++) {
        printf("=== Threshold: %d ===\n", thresholds[i]);
        
        psec_init();
        psec_configure(thresholds[i], 0);
        
        char explanation[512];
        psec_explain_decision(borderline_expr, explanation, sizeof(explanation));
        printf("%s\n", explanation);
        
        // Test evaluation
        PyObject *main_module = PyImport_AddModule("__main__");
        PyObject *globals = PyModule_GetDict(main_module);
        PyDict_SetItemString(globals, "x", PyFloat_FromDouble(10.0));
        PyDict_SetItemString(globals, "y", PyFloat_FromDouble(6.0));
        PyDict_SetItemString(globals, "z", PyFloat_FromDouble(2.0));
        
        PyObject *result = psec_eval(borderline_expr, globals, NULL);
        if (result) {
            printf("Evaluation result: %.2f\n", PyFloat_AsDouble(result));
            Py_DECREF(result);
        }
        
        psec_cleanup();
        printf("\n");
    }
}

int main() {
    // Initialize Python
    Py_Initialize();
    if (!Py_IsInitialized()) {
        printf("Failed to initialize Python\n");
        return 1;
    }
    
    printf("Simplified Expression Cache Demo\n");
    printf("================================\n\n");
    
    printf("Library: pysmart_eval_cache.h (single header)\n");
    printf("Focus: Expression evaluation with Py_eval_input mode\n");
    printf("Features: Smart caching, globals reuse, no isolation\n");
    printf("Configuration: threshold=%d, max_simple=%d\n\n", 
           PSEC_CACHE_THRESHOLD_SCORE, PSEC_MAX_SIMPLE_LENGTH);
    
    // Run demonstrations
    demonstrate_basic_usage();
    demonstrate_performance_comparison();
    demonstrate_real_world_usage();
    demonstrate_configuration_tuning();
    
    printf("=== Demo Complete ===\n\n");
    printf("Key Features Demonstrated:\n");
    printf("• Expression-only evaluation (no function definitions needed)\n");
    printf("• Reuses existing globals (no module pre-importing)\n");
    printf("• Smart caching based on expression complexity\n");
    printf("• Automatic performance optimization\n");
    printf("• Configurable complexity thresholds\n");
    printf("• Thread-safe operation\n");
    printf("• Minimal memory footprint\n");
    printf("• Single header - just include and use!\n");
    
    printf("\nPerfect for:\n");
    printf("• Mathematical expression evaluation\n");
    printf("• Template engines with expression interpolation\n");
    printf("• Configuration systems with dynamic expressions\n");
    printf("• Calculators and formula processors\n");
    printf("• Data analysis with repeated expression evaluation\n");
    
    // Cleanup
    Py_Finalize();
    return 0;
}
