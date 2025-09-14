/* py_config.h */

#ifndef PY_CONFIG_H
#define PY_CONFIG_H

/**
 * @file py_config.h
 *
 * @brief User configuration file for the py external
 *
 * This header provides centralized configuration options for the py external,
 * allowing users to customize security settings, module restrictions, and
 * other behavioral parameters without modifying core source files.
 *
 * To customize the py external behavior:
 * 1. Modify the arrays and constants in this file
 * 2. Rebuild the external using your build system
 *
 * Security Note: Changes to these configurations can affect the security
 * posture of the py external. Review all modifications carefully.
 */

/*--------------------------------------------------------------------------*/
/* Security Configuration */

/**
 * @brief Default security mode setting
 *
 * When enabled (1), security validation is performed on Python code before execution.
 * When disabled (0), code is executed without validation checks.
 *
 * This can be overridden per-object using the @security_mode attribute.
 */
#define PY_DEFAULT_SECURITY_MODE 1

/**
 * @brief Default restricted imports setting
 *
 * When enabled (1), only modules in PY_SAFE_MODULES are allowed to be imported.
 * When disabled (0), all modules can be imported (subject to other security checks).
 *
 * This can be overridden per-object using the @restrict_imports attribute.
 */
#define PY_DEFAULT_RESTRICT_IMPORTS 0

/**
 * @brief Default file access restriction setting
 *
 * When enabled (1), file system access functions are blocked.
 * When disabled (0), file system access is allowed (subject to other security checks).
 *
 * This can be overridden per-object using the @restrict_file_access attribute.
 */
#define PY_DEFAULT_RESTRICT_FILE_ACCESS 0

/**
 * @brief Default maximum execution time (milliseconds)
 *
 * Maximum time allowed for Python code execution before timeout.
 * Set to 0 to disable timeout checking.
 *
 * This can be overridden per-object using the @max_execution_time attribute.
 */
#define PY_DEFAULT_MAX_EXECUTION_TIME 5000

/*--------------------------------------------------------------------------*/
/* Code Validation Configuration */

/**
 * @brief Dangerous code patterns detected during security validation
 *
 * These patterns are blocked when security_mode is enabled.
 * Add or remove patterns as needed for your security requirements.
 *
 * Note: Patterns are matched using simple string search (strstr).
 * Be careful with short patterns that might match legitimate code.
 */
static const char* PY_DANGEROUS_PATTERNS[] = {
    // Import and execution functions
    "__import__",
    "exec(",
    "eval(",
    "compile(",

    // File system access
    "open(",
    "file(",

    // Introspection and manipulation
    "__builtins__",
    "globals(",
    "locals(",
    "vars(",
    "dir(",
    "getattr(",
    "setattr(",
    "delattr(",

    // Add custom dangerous patterns here
    // "your_dangerous_pattern",

    NULL  // Terminator - do not remove
};

/**
 * @brief Statements only allowed in exec mode (not eval mode)
 *
 * These patterns are blocked when using the 'eval' method but allowed
 * in 'exec' method. This enforces the Python distinction between
 * expressions (eval) and statements (exec).
 */
static const char* PY_EXEC_ONLY_STATEMENTS[] = {
    "import ",
    "from ",
    "def ",
    "class ",
    "=",  // Assignment

    // Add custom exec-only patterns here
    // "your_exec_only_pattern",

    NULL  // Terminator - do not remove
};

/*--------------------------------------------------------------------------*/
/* Module Import Configuration */

/**
 * @brief Safe Python modules allowed in restricted import mode
 *
 * When restrict_imports is enabled, only modules in this list can be imported.
 * This provides a whitelist approach to module importing.
 *
 * Modules are checked by exact name match during import validation.
 */
static const char* PY_SAFE_MODULES[] = {
    // Core language modules
    "abc",
    "ast",
    "collections",
    "copy",
    "copyreg",
    "enum",
    "functools",
    "operator",
    "types",
    "typing",

    // Data processing
    "array",
    "bisect",
    "heapq",
    "itertools",
    "numbers",
    "random",
    "statistics",

    // String and text processing
    "string",
    "re",
    "difflib",
    "textwrap",
    "codecs",

    // Date and time
    "calendar",
    "datetime",

    // Mathematics
    "decimal",
    "fractions",
    "math",

    // Data formats
    "base64",
    "json",
    "csv",
    "html",
    "xml",

    // Utilities
    "keyword",
    "inspect",
    "dis",
    "traceback",
    "warnings",
    "weakref",

    // Testing (often useful for development)
    "unittest",
    "doctest",

    // Add custom safe modules here
    // "your_safe_module",

    NULL  // Terminator - do not remove
};

/**
 * @brief Unsafe Python modules blocked in restricted import mode
 *
 * These modules are explicitly blocked when restrict_imports is enabled.
 * This provides additional protection by explicitly listing dangerous modules.
 *
 * Note: This list is used in addition to the whitelist approach.
 * Modules not in PY_SAFE_MODULES are blocked regardless of this list.
 */
static const char* PY_UNSAFE_MODULES[] = {
    // System access
    "os",
    "subprocess",
    "signal",
    "pty",
    "tty",

    // File system
    "pathlib",
    "glob",
    "shutil",
    "tempfile",
    "filecmp",
    "fileinput",

    // Network access
    "socket",
    "socketserver",
    "http",
    "urllib",
    "ftplib",
    "poplib",
    "imaplib",
    "smtplib",
    "webbrowser",

    // System introspection and modification
    "ctypes",
    "platform",
    "sysconfig",
    "site",

    // Process and threading
    "multiprocessing",
    "threading",
    "concurrent",
    "queue",

    // Development and debugging
    "pdb",
    "profile",
    "cProfile",
    "pstats",
    "trace",
    "tracemalloc",

    // Package management
    "ensurepip",
    "venv",
    "pip",

    // Add custom unsafe modules here
    // "your_unsafe_module",

    NULL  // Terminator - do not remove
};

/*--------------------------------------------------------------------------*/
/* Performance and Limits Configuration */

/**
 * @brief Maximum length of Python code string (characters)
 *
 * Prevents excessive memory usage and potential DoS attacks.
 * Code longer than this limit will be rejected.
 */
#ifndef PY_MAX_CODE_LENGTH
#define PY_MAX_CODE_LENGTH 65536
#endif

/**
 * @brief Maximum length for eval expressions (characters)
 *
 * Eval expressions are typically shorter than full code blocks.
 * This provides additional protection for eval operations.
 */
#ifndef PY_MAX_EVAL_LENGTH
#define PY_MAX_EVAL_LENGTH 1024
#endif

/**
 * @brief Maximum number of elements in Max atom arrays
 *
 * Limits the size of data structures passed between Max and Python.
 * Prevents excessive memory usage.
 */
#ifndef PY_MAX_ELEMS
#define PY_MAX_ELEMS 1024
#endif

/**
 * @brief Maximum length of error messages (characters)
 *
 * Prevents buffer overflows in error message formatting.
 */
#ifndef PY_MAX_ERROR
#define PY_MAX_ERROR 4096
#endif

/*--------------------------------------------------------------------------*/
/* Feature Configuration */

/**
 * @brief Enable API module integration
 *
 * When enabled (1), the Cython-based API module is included as a builtin module.
 * When disabled (0), the API module is not available to Python code.
 */
#ifndef PY_WITH_API
#define PY_WITH_API 1
#endif

/**
 * @brief Enable Python isolated mode
 *
 * When enabled (1), Python runs in isolated mode with restricted access.
 * When disabled (0), Python runs with normal access to system resources.
 *
 * Note: This affects the Python interpreter configuration globally.
 */
#ifndef PY_CFG_ISOLATED
#define PY_CFG_ISOLATED 1
#endif

/**
 * @brief Enable reference counting debugging
 *
 * When enabled (1), Python object reference counts are logged for debugging.
 * When disabled (0), reference counting is silent.
 *
 * Note: Enabling this significantly increases log output.
 */
#ifndef PY_CHECK_REFS
#define PY_CHECK_REFS 0
#endif

/**
 * @brief Enable attributes with default values
 *
 * When enabled (1), object attributes have default values.
 * When disabled (0), attributes must be explicitly set.
 */
#ifndef PY_ATTRS_WITH_DEFAULTS
#define PY_ATTRS_WITH_DEFAULTS 0
#endif

/*--------------------------------------------------------------------------*/
/* User Customization Section */

/**
 * @brief Custom configuration macros
 *
 * Add your custom configuration options below this line.
 * This section is reserved for user-specific modifications.
 */

// Example custom configuration:
// #define PY_CUSTOM_FEATURE 1
// static const char* PY_CUSTOM_PATTERNS[] = {
//     "custom_pattern",
//     NULL
// };

/*--------------------------------------------------------------------------*/
/* Configuration Validation */

// Compile-time validation of configuration
#if PY_MAX_CODE_LENGTH < 1024
#warning "PY_MAX_CODE_LENGTH is very small and may cause issues"
#endif

#if PY_MAX_EVAL_LENGTH > PY_MAX_CODE_LENGTH
#error "PY_MAX_EVAL_LENGTH cannot be larger than PY_MAX_CODE_LENGTH"
#endif

#if PY_DEFAULT_MAX_EXECUTION_TIME < 0
#error "PY_DEFAULT_MAX_EXECUTION_TIME cannot be negative"
#endif

#endif // PY_CONFIG_H