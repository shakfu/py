/**
 * @file pysmart_cache_demo.c
 * @brief Complete demonstration of the integrated smart caching system
 * 
 * This demo shows all features of the single-header pysmart_cache.h library:
 * - Automatic smart caching decisions
 * - Performance analysis and statistics
 * - Configuration and debugging
 * - Real-world usage patterns
 */
#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <math.h>

// Configure the cache before including (optional)
#define PSC_CACHE_THRESHOLD_SCORE 20
#define PSC_MAX_SIMPLE_LENGTH 100

// Include the complete smart caching system - that's it!
#include "pysmart_cache2.h"

void demonstrate_smart_decisions() {
    printf("=== Smart Caching Decisions Demo ===\n\n");
    
    struct {
        const char *name;
        const char *source;
        const char *expected;
    } test_cases[] = {
        {
            "Simple Add",
            "def add(x, y):\n    return x + y",
            "Should SKIP - too simple"
        },
        {
            "Complex Math",
            "import math\ndef complex_calc(x, y):\n    return math.sqrt(x**2 + y**2) + math.sin(x)",
            "Should CACHE - has imports"
        },
        {
            "Data Processor", 
            "def process_data(items):\n    result = []\n    for item in items:\n        if item > 0:\n            result.append(item * 2)\n    return sum(result)",
            "Borderline - depends on threshold"
        },
        {
            "ML Function",
            "import numpy as np\nimport sklearn.preprocessing\n\ndef preprocess(X):\n    scaler = sklearn.preprocessing.StandardScaler()\n    return scaler.fit_transform(X)",
            "Should CACHE - multiple imports"
        },
        {
            "Decorated Function",
            "@property\ndef value(self):\n    return self._value * 2",
            "Should CACHE - has decorator"
        }
    };
    
    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        printf("Function: %s\n", test_cases[i].name);
        printf("Expected: %s\n", test_cases[i].expected);
        
        char explanation[1024];
        psc_explain_decision(test_cases[i].source, explanation, sizeof(explanation));
        printf("%s\n", explanation);
        
        printf("\n" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "\n\n");
    }
}

void demonstrate_caching_workflow() {
    printf("=== Smart Caching Workflow Demo ===\n\n");
    
    // Initialize the system
    psc_result_t init_result = psc_init();
    if (init_result != PSC_SUCCESS) {
        printf("Failed to initialize smart cache!\n");
        return;
    }
    printf("✅ Smart cache system initialized\n\n");
    
    // Test functions with varying complexity
    struct {
        const char *source;
        const char *name;
    } functions[] = {
        {"def simple(x): return x * 2", "simple"},
        {"def quadratic(a, b, c, x): return a*x*x + b*x + c", "quadratic"},
        {"import math\ndef trig(x): return math.sin(x) + math.cos(x)", "trig"},
        {"def factorial(n):\n    if n <= 1: return 1\n    return n * factorial(n-1)", "factorial"}
    };
    
    printf("Adding functions to cache:\n");
    for (size_t i = 0; i < sizeof(functions) / sizeof(functions[0]); i++) {
        printf("Processing '%s'... ", functions[i].name);
        
        psc_result_t result = psc_add_function(functions[i].source, "demo.py");
        
        switch (result) {
            case PSC_SUCCESS:
                printf("✅ CACHED\n");
                break;
            case PSC_ERROR_FUNCTION_TOO_SIMPLE:
                printf("⏭️ SKIPPED (too simple)\n");
                break;
            default:
                printf("❌ ERROR (%d)\n", result);
                break;
        }
    }
    
    printf("\n");
    
    // Test calling cached functions
    printf("Testing function calls:\n");
    
    // Try to call the trigonometric function (should be cached)
    if (psc_has_function("trig")) {
        PyObject *args = PyTuple_Pack(1, PyFloat_FromDouble(1.5708)); // π/2
        PyObject *result = psc_call_function("trig", args, NULL);
        if (result) {
            printf("trig(π/2) = %.6f\n", PyFloat_AsDouble(result));
            Py_DECREF(result);
        }
        Py_DECREF(args);
    } else {
        printf("trig function not found in cache\n");
    }
    
    // Try factorial if it was cached
    if (psc_has_function("factorial")) {
        PyObject *args = PyTuple_Pack(1, PyLong_FromLong(5));
        PyObject *result = psc_call_function("factorial", args, NULL);
        if (result) {
            printf("factorial(5) = %ld\n", PyLong_AsLong(result));
            Py_DECREF(result);
        }
        Py_DECREF(args);
    } else {
        printf("factorial function not in cache (recursive functions need special handling)\n");
    }
    
    // Show statistics
    char stats[2048];
    psc_get_stats_string(stats, sizeof(stats));
    printf("\n%s\n\n", stats);
    
    psc_cleanup();
    printf("✅ Cache cleaned up\n\n");
}

void demonstrate_performance_benefits() {
    printf("=== Performance Benefits Demo ===\n\n");
    
    psc_init();
    
    // Test with a function that should be cached
    const char *complex_source = 
        "import math\n"
        "def complex_computation(x, iterations):\n"
        "    result = x\n"
        "    for i in range(iterations):\n"
        "        result = math.sin(result) + math.cos(result * 0.5)\n"
        "    return result";
    
    printf("Testing performance with complex computation function:\n");
    
    char explanation[1024];
    psc_explain_decision(complex_source, explanation, sizeof(explanation));
    printf("Decision analysis:\n%s\n\n", explanation);
    
    // Add the function (should be cached due to import and complexity)
    psc_result_t add_result = psc_add_function(complex_source, "perf_test.py");
    if (add_result == PSC_SUCCESS) {
        printf("✅ Function cached successfully\n");
        
        // Benchmark the cached function
        printf("Running performance test (1000 calls)...\n");
        
        clock_t start = clock();
        for (int i = 0; i < 1000; i++) {
            PyObject *args = PyTuple_Pack(2, 
                                        PyFloat_FromDouble(1.0 + i * 0.001), 
                                        PyLong_FromLong(10));
            PyObject *result = psc_call_function("complex_computation", args, NULL);
            if (result) {
                if (i % 200 == 0) {
                    printf("  Call %d: %.6f\n", i, PyFloat_AsDouble(result));
                }
                Py_DECREF(result);
            }
            Py_DECREF(args);
        }
        clock_t end = clock();
        
        double total_time = ((double)(end - start)) / CLOCKS_PER_SEC;
        printf("Total time for 1000 calls: %.4f seconds\n", total_time);
        printf("Average time per call: %.6f seconds\n", total_time / 1000);
        
    } else if (add_result == PSC_ERROR_FUNCTION_TOO_SIMPLE) {
        printf("⏭️ Function was deemed too simple to cache\n");
    } else {
        printf("❌ Failed to add function: %d\n", add_result);
    }
    
    char final_stats[2048];
    psc_get_stats_string(final_stats, sizeof(final_stats));
    printf("\n%s\n\n", final_stats);
    
    psc_cleanup();
}

void demonstrate_configuration() {
    printf("=== Configuration Demo ===\n\n");
    
    const char *borderline_function = 
        "def borderline(data):\n"
        "    total = 0\n"
        "    for item in data:\n"
        "        if item > 0:\n"
        "            total += item * 2\n"
        "    return total";
    
    printf("Testing borderline function with different thresholds:\n");
    printf("Function source:\n%s\n\n", borderline_function);
    
    // Test with different thresholds
    int thresholds[] = {10, 25, 50};
    for (size_t i = 0; i < sizeof(thresholds) / sizeof(thresholds[0]); i++) {
        printf("--- Threshold: %d ---\n", thresholds[i]);
        
        psc_init();
        psc_configure(thresholds[i], 1); // Enable debug mode
        
        char explanation[1024];
        psc_explain_decision(borderline_function, explanation, sizeof(explanation));
        printf("%s\n", explanation);
        
        psc_result_t result = psc_add_function(borderline_function, "test.py");
        printf("Result: %s\n\n", 
               result == PSC_SUCCESS ? "CACHED" : 
               result == PSC_ERROR_FUNCTION_TOO_SIMPLE ? "SKIPPED" : "ERROR");
        
        psc_cleanup();
    }
}

void demonstrate_real_world_scenarios() {
    printf("=== Real-World Scenarios Demo ===\n\n");
    
    psc_init();
    psc_configure(PSC_CACHE_THRESHOLD_SCORE, 1); // Enable debug mode
    
    struct {
        const char *scenario;
        const char *source;
    } scenarios[] = {
        {
            "Web API Handler (Simple)",
            "def api_response(status, message):\n"
            "    return {'status': status, 'message': message, 'timestamp': time.time()}"
        },
        {
            "Data Science Pipeline",
            "import pandas as pd\n"
            "import numpy as np\n"
            "def preprocess_dataset(df, target_col):\n"
            "    # Remove nulls and normalize\n"
            "    df_clean = df.dropna()\n"
            "    numeric_cols = df_clean.select_dtypes(include=[np.number]).columns\n"
            "    df_clean[numeric_cols] = (df_clean[numeric_cols] - df_clean[numeric_cols].mean()) / df_clean[numeric_cols].std()\n"
            "    return df_clean"
        },
        {
            "Template Renderer",
            "def render_email_template(template, user_name, items):\n"
            "    content = template.replace('{{USER}}', user_name)\n"
            "    item_list = '\\n'.join([f'- {item}' for item in items])\n"
            "    return content.replace('{{ITEMS}}', item_list)"
        },
        {
            "ML Model Inference",
            "import tensorflow as tf\n"
            "import numpy as np\n"
            "@tf.function\n"
            "def predict_batch(model, input_data):\n"
            "    preprocessed = tf.cast(input_data, tf.float32) / 255.0\n"
            "    predictions = model(preprocessed)\n"
            "    return tf.argmax(predictions, axis=1)"
        }
    };
    
    for (size_t i = 0; i < sizeof(scenarios) / sizeof(scenarios[0]); i++) {
        printf("Scenario: %s\n", scenarios[i].scenario);
        
        char explanation[1024];
        psc_explain_decision(scenarios[i].source, explanation, sizeof(explanation));
        printf("%s\n", explanation);
        
        psc_result_t result = psc_add_function(scenarios[i].source, "scenario.py");
        printf("Cache Decision: %s\n", 
               result == PSC_SUCCESS ? "✅ CACHED" : 
               result == PSC_ERROR_FUNCTION_TOO_SIMPLE ? "❌ SKIPPED" : "❌ ERROR");
        
        printf("\n" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "\n\n");
    }
    
    char final_stats[2048];
    psc_get_stats_string(final_stats, sizeof(final_stats));
    printf("Final System Statistics:\n%s\n", final_stats);
    
    psc_cleanup();
}

int main() {
    // Initialize Python interpreter
    Py_Initialize();
    if (!Py_IsInitialized()) {
        printf("Failed to initialize Python\n");
        return 1;
    }
    
    printf("Integrated Smart Function Cache Demo\n");
    printf("===================================\n\n");
    
    printf("Library: pysmart_cache.h (single header)\n");
    printf("Features: Smart caching, complexity analysis, performance stats\n");
    printf("Configuration: threshold=%d, max_simple=%d\n\n", 
           PSC_CACHE_THRESHOLD_SCORE, PSC_MAX_SIMPLE_LENGTH);
    
    // Run all demonstrations
    demonstrate_smart_decisions();
    demonstrate_caching_workflow();
    demonstrate_performance_benefits(); 
    demonstrate_configuration();
    demonstrate_real_world_scenarios();
    
    printf("=== Demo Complete ===\n");
    printf("\nKey Takeaways:\n");
    printf("• Single header: #include \"pysmart_cache.h\" and you're ready!\n");
    printf("• Automatic decisions: System decides what to cache based on complexity\n");
    printf("• Unified structure: All data in one cohesive system structure\n");
    printf("• Performance optimized: Avoids cache overhead for simple functions\n");
    printf("• Production ready: Thread-safe, comprehensive error handling\n");
    printf("• Highly configurable: Tune thresholds for your use case\n");
    printf("• Zero dependencies: Just Python C API, no external libraries\n");
    
    // Cleanup
    Py_Finalize();
    return 0;
}
