/**
 * @file smart_caching_demo.c
 * @brief Demonstration of intelligent function caching integration
 * 
 * This example shows how to integrate the smart caching heuristics
 * with the original function cache to automatically determine which
 * functions are worth caching.
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "py_function_cache.h"
#include "smart_caching_heuristics.h"

// Test functions with varying complexity levels
static const char* test_functions[] = {
    // SIMPLE - Should NOT be cached
    "def simple_add(x, y):\n    return x + y",
    
    "def square(x):\n    return x * x",
    
    "def celsius_to_fahrenheit(c):\n    return c * 9/5 + 32",
    
    // MODERATE - Borderline cases
    "def quadratic(a, b, c, x):\n    return a * x * x + b * x + c",
    
    "def fibonacci_iterative(n):\n    if n <= 1: return n\n    a, b = 0, 1\n    for i in range(2, n + 1):\n        a, b = b, a + b\n    return b",
    
    "def list_statistics(numbers):\n    total = sum(numbers)\n    count = len(numbers)\n    mean = total / count\n    return {'sum': total, 'mean': mean, 'count': count}",
    
    // COMPLEX - Should definitely be cached
    "import math\n\ndef complex_math(x, y):\n    return math.sqrt(x**2 + y**2) + math.sin(x) * math.cos(y)",
    
    "import statistics\nimport math\n\ndef advanced_stats(data):\n    mean = statistics.mean(data)\n    stdev = statistics.stdev(data)\n    return [math.log(x + 1) * math.sqrt(abs(x - mean)) for x in data]",
    
    "@property\ndef decorated_function(self):\n    return self._value * 2",
    
    "def nested_complexity():\n    def inner_function(x):\n        return x * 2\n    def another_inner(y):\n        return y + 1\n    return lambda z: inner_function(z) + another_inner(z)",
    
    "async def async_function(data):\n    result = []\n    for item in data:\n        if await some_condition(item):\n            result.append(item)\n    return result",
    
    "def exception_handler(data):\n    try:\n        result = []\n        for item in data:\n            if item > 0:\n                result.append(math.sqrt(item))\n    except ValueError as e:\n        return None\n    finally:\n        cleanup()\n    return result"
};

static const char* function_names[] = {
    "simple_add", "square", "celsius_to_fahrenheit",
    "quadratic", "fibonacci_iterative", "list_statistics", 
    "complex_math", "advanced_stats", "decorated_function",
    "nested_complexity", "async_function", "exception_handler"
};

void demonstrate_caching_decisions() {
    printf("=== Smart Caching Decision Analysis ===\n\n");
    
    size_t num_functions = sizeof(test_functions) / sizeof(test_functions[0]);
    
    for (size_t i = 0; i < num_functions; i++) {
        printf("Function: %s\n", function_names[i]);
        printf("Source:\n%s\n", test_functions[i]);
        
        int should_cache = should_cache_function(test_functions[i]);
        printf("Decision: %s\n", should_cache ? "✅ CACHE" : "❌ SKIP");
        
        char explanation[1024];
        pyfc_explain_caching_decision(test_functions[i], explanation, sizeof(explanation));
        printf("%s\n", explanation);
        printf("\n" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "─" "\n\n");
    }
}

void demonstrate_smart_integration() {
    printf("=== Smart Integration Demo ===\n\n");
    
    // Initialize cache
    pyfc_init();
    
    int cached_count = 0;
    int skipped_count = 0;
    
    size_t num_functions = sizeof(test_functions) / sizeof(test_functions[0]);
    
    for (size_t i = 0; i < num_functions; i++) {
        printf("Processing: %s... ", function_names[i]);
        
        if (should_cache_function(test_functions[i])) {
            pyfc_result_t result = pyfc_add_function(test_functions[i], function_names[i]);
            if (result == PYFC_SUCCESS) {
                printf("✅ Cached\n");
                cached_count++;
            } else {
                printf("❌ Cache failed\n");
            }
        } else {
            printf("⏭️ Skipped (too simple)\n");
            skipped_count++;
        }
    }
    
    printf("\nSummary:\n");
    printf("  Functions cached: %d\n", cached_count);
    printf("  Functions skipped: %d\n", skipped_count);
    printf("  Cache efficiency: %.1f%% (avoided caching %d simple functions)\n", 
           (double)skipped_count / num_functions * 100, skipped_count);
    
    // Show cache statistics
    pyfc_stats_t stats;
    pyfc_get_stats(&stats);
    printf("  Total cache entries: %zu\n\n", stats.total_entries);
    
    pyfc_cleanup();
}

void demonstrate_performance_impact() {
    printf("=== Performance Impact Analysis ===\n\n");
    
    pyfc_init();
    
    // Test simple function (should be skipped)
    const char *simple_func = "def test_simple(x): return x * 2";
    const char *complex_func = "import math\ndef test_complex(x): return math.sqrt(x**2) + math.sin(x)";
    
    printf("Simple function caching decision:\n");
    if (should_cache_function(simple_func)) {
        printf("  Decision: Cache (unexpected!)\n");
    } else {
        printf("  Decision: Skip caching ✅\n");
        printf("  Reason: Too simple, cache overhead > benefit\n");
    }
    
    printf("\nComplex function caching decision:\n");
    if (should_cache_function(complex_func)) {
        printf("  Decision: Cache ✅\n");
        printf("  Reason: Contains imports, compilation cost justified\n");
    } else {
        printf("  Decision: Skip (unexpected!)\n");
    }
    
    // Add only the complex function to cache
    if (should_cache_function(complex_func)) {
        pyfc_add_function(complex_func, "complex.py");
        printf("\nCached complex function for performance testing.\n");
    }
    
    pyfc_cleanup();
}

void demonstrate_custom_thresholds() {
    printf("=== Custom Threshold Demonstration ===\n\n");
    
    const char *borderline_func = 
        "def borderline_function(data):\n"
        "    result = []\n"
        "    for item in data:\n"
        "        if item > 0:\n"
        "            result.append(item * 2)\n"
        "    return sum(result)";
    
    printf("Borderline function analysis:\n");
    printf("Source:\n%s\n", borderline_func);
    
    // Test with default threshold
    printf("\nWith default threshold (%d):\n", PYFC_CACHE_THRESHOLD_SCORE);
    int default_decision = should_cache_function(borderline_func);
    printf("Decision: %s\n", default_decision ? "Cache" : "Skip");
    
    char explanation[1024];
    pyfc_explain_caching_decision(borderline_func, explanation, sizeof(explanation));
    printf("%s\n", explanation);
    
    printf("\nThis demonstrates how the threshold can be tuned based on your\n");
    printf("specific performance requirements and use case.\n\n");
}

void demonstrate_real_world_scenarios() {
    printf("=== Real-World Scenario Examples ===\n\n");
    
    struct {
        const char *scenario;
        const char *code;
        const char *expected_reason;
    } scenarios[] = {
        {
            "Data Science Function",
            "import pandas as pd\nimport numpy as np\n\ndef analyze_dataset(df):\n    return df.groupby('category').agg({'value': ['mean', 'std', 'count']})",
            "Multiple imports make compilation expensive"
        },
        {
            "Web API Endpoint",
            "def api_handler(request):\n    return {'status': 'ok', 'data': request.get('data', [])}",
            "Too simple - direct execution faster than cache overhead"
        },
        {
            "Template Engine",
            "def render_template(template, **kwargs):\n    result = template\n    for key, value in kwargs.items():\n        result = result.replace(f'{{{key}}}', str(value))\n    return result",
            "Moderate complexity with string operations"
        },
        {
            "Machine Learning Preprocessing",
            "import sklearn.preprocessing\nimport numpy as np\n\ndef preprocess_features(X):\n    scaler = sklearn.preprocessing.StandardScaler()\n    return scaler.fit_transform(X)",
            "Heavy imports justify caching cost"
        }
    };
    
    for (size_t i = 0; i < sizeof(scenarios) / sizeof(scenarios[0]); i++) {
        printf("Scenario: %s\n", scenarios[i].scenario);
        printf("Expected: %s\n", scenarios[i].expected_reason);
        
        int decision = should_cache_function(scenarios[i].code);
        printf("Decision: %s\n", decision ? "✅ CACHE" : "❌ SKIP");
        
        char explanation[512];
        pyfc_explain_caching_decision(scenarios[i].code, explanation, sizeof(explanation));
        printf("Analysis: %s\n\n", strchr(explanation, '\n') + 1); // Skip first line
    }
}

int main() {
    Py_Initialize();
    
    printf("Smart Function Caching System Demo\n");
    printf("==================================\n\n");
    
    demonstrate_caching_decisions();
    demonstrate_smart_integration();  
    demonstrate_performance_impact();
    demonstrate_custom_thresholds();
    demonstrate_real_world_scenarios();
    
    printf("=== Demo Complete ===\n");
    printf("Key Takeaways:\n");
    printf("• Simple arithmetic functions are skipped (cache overhead > benefit)\n");
    printf("• Functions with imports are always cached (compilation cost high)\n");  
    printf("• Decorators, async, and complex features trigger caching\n");
    printf("• Threshold can be tuned for specific use cases\n");
    printf("• System prevents unnecessary cache overhead automatically\n");
    
    Py_Finalize();
    return 0;
}

