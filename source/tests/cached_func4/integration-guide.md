# integration_guide.md - How to integrate into your C extension

Smart Function Caching Integration Guide

## Files Needed

1. `py_function_cache.h` - Core caching system
2. `smart_caching_heuristics.h` - Intelligent caching decisions

## Basic Integration

### Step 1: Include Headers
```c
#include "py_function_cache.h"
#include "smart_caching_heuristics.h"
```

### Step 2: Initialize in Module Init
```c
PyMODINIT_FUNC PyInit_your_module(void) {
    PyObject *module = PyModule_Create(&your_module_def);
    
    // Initialize cache system
    if (pyfc_init() != PYFC_SUCCESS) {
        Py_DECREF(module);
        return NULL;
    }
    
    return module;
}
```

### Step 3: Add Smart Caching Function
```c
static PyObject* your_add_function(PyObject *self, PyObject *args) {
    const char *source_code;
    const char *filename = "<dynamic>";
    
    if (!PyArg_ParseTuple(args, "s|s", &source_code, &filename)) {
        return NULL;
    }
    
    // Use smart caching - only cache if beneficial
    if (should_cache_function(source_code)) {
        pyfc_result_t result = pyfc_add_function(source_code, filename);
        if (result != PYFC_SUCCESS) {
            PyErr_SetString(PyExc_RuntimeError, "Failed to cache function");
            return NULL;
        }
        Py_RETURN_TRUE;  // Function was cached
    } else {
        Py_RETURN_FALSE; // Function skipped (too simple)
    }
}
```

### Step 4: Add Cleanup
```c
static void your_module_free(void *m) {
    pyfc_cleanup();
}

// In module definition
static PyModuleDef your_module_def = {
    PyModuleDef_HEAD_INIT,
    "your_module",
    "Module with smart caching",
    -1,
    your_methods,
    NULL,
    NULL,
    NULL,
    your_module_free  // Cleanup function
};
```

## Advanced Integration

### Custom Thresholds
```c
// Tune caching sensitivity
#define PYFC_CACHE_THRESHOLD_SCORE 15  // More aggressive caching
#define PYFC_MIN_LINES_FOR_CACHE 2     // Cache smaller functions
#include "smart_caching_heuristics.h"
```

### Debugging Decisions
```c
static PyObject* explain_caching(PyObject *self, PyObject *args) {
    const char *source_code;
    if (!PyArg_ParseTuple(args, "s", &source_code)) {
        return NULL;
    }
    
    char explanation[1024];
    pyfc_explain_caching_decision(source_code, explanation, sizeof(explanation));
    
    return PyUnicode_FromString(explanation);
}
```

### Conditional Caching
```c
// Cache based on runtime conditions
static PyObject* smart_add_function(PyObject *self, PyObject *args) {
    const char *source_code;
    int force_cache = 0;
    
    if (!PyArg_ParseTuple(args, "s|p", &source_code, &force_cache)) {
        return NULL;
    }
    
    if (force_cache || should_cache_function(source_code)) {
        return regular_add_function(self, args);
    } else {
        // Maybe compile and execute directly without caching
        return compile_and_execute_once(source_code, args);
    }
}
```

## Performance Tuning

### Threshold Configuration
```c
// For high-performance applications (cache less)
#define PYFC_CACHE_THRESHOLD_SCORE 50

// For compilation-heavy applications (cache more)  
#define PYFC_CACHE_THRESHOLD_SCORE 10

// For memory-constrained environments
#define PYFC_CACHE_SIZE 256
#define PYFC_MAX_SOURCE_LENGTH 4096
```

### Custom Complexity Analysis
```c
// Add domain-specific complexity indicators
static inline int my_custom_complexity_check(const char *source) {
    int score = 0;
    
    // Domain-specific expensive operations
    if (strstr(source, "tensorflow")) score += 100;
    if (strstr(source, "torch")) score += 100;
    if (strstr(source, "opencv")) score += 80;
    
    return score;
}

// Modify should_cache_function to include custom checks
static inline int my_should_cache_function(const char *source) {
    return should_cache_function(source) || 
           my_custom_complexity_check(source) > 50;
}
```

## Testing Your Integration

### Unit Test Template
```c
void test_smart_caching() {
    // Test simple function (should skip)
    assert(!should_cache_function("def add(x,y): return x+y"));
    
    // Test complex function (should cache)  
    assert(should_cache_function("import math\ndef complex(x): return math.sin(x)"));
    
    // Test your domain-specific cases
    assert(my_should_cache_function("import tensorflow as tf\ndef model(x): return tf.nn.relu(x)"));
}
```

### Performance Validation
```c
void benchmark_caching_decision() {
    const char *test_functions[] = { ... };
    
    clock_t start = clock();
    for (int i = 0; i < 10000; i++) {
        for (int j = 0; j < num_functions; j++) {
            should_cache_function(test_functions[j]);
        }
    }
    clock_t end = clock();
    
    printf("Caching decision time: %.6f seconds per function\n",
           ((double)(end - start)) / CLOCKS_PER_SEC / (10000 * num_functions));
}
```

## Real-World Examples

### Template Engine Integration
```c
// Only cache complex templates
if (should_cache_function(template_source) || 
    strlen(template_source) > 1000) {
    cache_template(template_source);
}
```

### Scientific Computing Module
```c
// Always cache functions with heavy imports
if (strstr(source, "numpy") || strstr(source, "scipy") || 
    should_cache_function(source)) {
    pyfc_add_function(source, filename);
}
```

### Web Framework Handler
```c
// Cache complex business logic, skip simple responses
if (should_cache_function(handler_source) && 
    !strstr(handler_source, "return {'status': 'ok'}")) {
    cache_handler(handler_source);
}
```

This integration approach ensures optimal performance by automatically
avoiding cache overhead for simple functions while capturing the benefit
for complex ones.


