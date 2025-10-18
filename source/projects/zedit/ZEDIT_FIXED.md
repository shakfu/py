# zedit.c - Complete Fix Summary

This document summarizes all fixes applied to the zedit Max/MSP external to resolve hanging, threading, and functionality issues.

## Overview

The zedit external embeds a Mongoose HTTP webserver in a Max/MSP external, providing a web-based Python REPL and code editor. The implementation had several critical issues that prevented proper operation.

## Issues Identified and Fixed

### 1. Verbose Event Handler Causing Performance Issues

**Problem:** The mongoose event handler logged every single event (MG_EV_POLL, MG_EV_READ, MG_EV_WRITE, etc.), occurring hundreds of times per second. This caused I/O blocking and potential performance degradation.

**Location:** `zedit.c:445-538` (original)

**Fix:** Simplified event handler to match working test-server.c pattern (zedit.c:512-527):

```c
// Before: 80+ lines with verbose switch statement logging every event
static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    switch(ev) {
        case MG_EV_ERROR: ...
        case MG_EV_OPEN: post("MG_EV_OPEN"); break;
        case MG_EV_POLL: ... break;  // Called constantly!
        case MG_EV_READ: post("MG_EV_READ"); break;
        // ... many more cases
    }
}

// After: Clean, minimal handler
static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    if (ev == MG_EV_HTTP_MSG) {
        handle_event_http_message(c, ev, ev_data, fn_data);
    }
    (void)fn_data;
}
```

**Result:** Eliminated performance overhead from excessive logging.

---

### 2. Webserver Shutdown Hanging Max/MSP

**Problem:** Sending "cancel" message to stop the webserver caused Max/MSP to hang indefinitely. The thread was not properly exiting, causing `systhread_join()` to block forever.

**Location:** `zedit.c:695-710`, `zedit.c:720-771`

**Fixes:**

#### a. Simplified Thread Loop (zedit.c:720-771)

```c
// Before: Confusing nested loops
while (1) {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    for (;;) {
        mg_mgr_poll(&mgr, 1000);
        if (x->x_systhread_cancel)
            break;
    }
    mg_mgr_free(&mgr);
    break;  // Redundant outer break
}

// After: Clean single loop
struct mg_mgr mgr;
mg_mgr_init(&mgr);

while (!x->x_systhread_cancel) {
    mg_mgr_poll(&mgr, 1000);
}

mg_mgr_free(&mgr);
```

#### b. Added Comprehensive Logging

```c
void* zedit_threadproc(t_zedit* x) {
    post("[THREAD] Webserver thread started");
    // ... initialization ...
    post("[THREAD] Entering event loop...");

    while (!x->x_systhread_cancel) {
        mg_mgr_poll(&mgr, 1000);
    }

    post("[THREAD] Cancel requested, cleaning up...");
    mg_mgr_free(&mgr);
    post("[THREAD] Mongoose manager freed");
    post("[THREAD] Webserver thread exiting normally");
    systhread_exit(0);
}
```

#### c. Improved zedit_stop() Logging

```c
void zedit_stop(t_zedit* x) {
    if (x->x_systhread) {
        post("[STOP] Requesting webserver thread to stop...");
        x->x_systhread_cancel = true;
        post("[STOP] Waiting for thread to exit (this may take up to 1 second)...");
        systhread_join(x->x_systhread, &ret);
        post("[STOP] Webserver thread stopped (exit code: %u)", ret);
    }
}
```

**Result:** Clean shutdown within 1-2 seconds with full diagnostic logging.

---

### 3. Web REPL Not Returning Results

**Problem:** When entering Python expressions like `1+1` in the web REPL, no output was returned to the web interface. The response only contained `{"result":"OK"}` without the actual result.

**Location:** `zedit.c:28`

**Fix:** Enabled output capture feature flag:

```c
// Feature flags - comment out to disable
#define ENABLE_AUTH 1              // Authentication token (REQUIRED for web frontend)
#define ENABLE_OUTPUT_CAPTURE 1    // Capture Python output (REQUIRED for web REPL)
```

**How it works:**
- Web frontend sends: `{"content":"1+1"}`
- zedit calls `py_exec_single_input_with_output()`
- Python executes in REPL mode (`Py_single_input`)
- Output is captured via StringIO redirection
- Response returned: `{"result":"OK", "output":"2\n"}`
- Web frontend displays: `2`

**Result:** Web REPL now shows Python expression results immediately.

---

### 4. Python GIL Threading Deadlock (CRITICAL)

**Problem:** Web requests would hang indefinitely until a Max message was sent to the zedit object. This was a classic Python Global Interpreter Lock (GIL) threading issue.

**Root Cause Analysis:**

1. Python was initialized in the **Max main thread** (via `py_init()` in `zedit_new()`)
2. After `Py_Initialize()`, the main thread **held the GIL** and never released it
3. The **webserver thread** tried to execute Python code via `PyRun_String()`
4. `PyGILState_Ensure()` in the webserver thread **blocked waiting for the GIL**
5. The main thread only released the GIL when executing Python code from Max messages
6. This created a queue: web requests waited until Max messages triggered GIL release

**Evidence:** When entering `500 * 2` from web, then `1+1` from Max, both would execute at once:
```
zedit: /api/repl/send
[DEBUG] Extracted code: '500 * 2'
[DEBUG] Executing with output capture
[EXEC] Calling py_exec_single_input_with_output...
[hangs here until Max message sent]

[py u392000517]: >>> 1+1                    <- Max message triggers GIL release
[EXEC] returned: 1000                        <- Web request executes
output: 2                                     <- Max message output
```

**Location:** `py.h:461-467`

**Fix:** Release GIL after Python initialization:

```c
t_py* py_init(t_class* c) {
    // ... Python initialization code ...

    Py_XDECREF(p_name);

    // CRITICAL: Release GIL so other threads (like webserver) can execute Python code
    // After Py_Initialize(), the main thread holds the GIL. We must release it
    // so that other threads can call PyGILState_Ensure() successfully.
    // Note: In Python 3.7+, PyEval_InitThreads() is deprecated and called automatically
    PyEval_SaveThread();  // Releases GIL and saves thread state

    py_log(x, (char*)"Python initialized and GIL released for multi-threading");

    return x;
}
```

**What `PyEval_SaveThread()` does:**
- Saves the current thread state
- **Releases the Global Interpreter Lock (GIL)**
- Allows other threads to acquire the GIL via `PyGILState_Ensure()`
- The main thread can re-acquire the GIL later when needed (happens automatically in py functions)

**Result:** Web requests execute immediately without waiting for Max messages. Both threads can execute Python code concurrently.

---

### 5. Added Comprehensive Debug Logging

To aid in diagnosing issues, comprehensive logging was added throughout:

#### Thread Lifecycle Logging
```c
[THREAD] Webserver thread started
[THREAD] Mongoose manager initialized
[THREAD] HTTP listener created on http://0.0.0.0:8000
[THREAD] Entering event loop...
[THREAD] Cancel requested, cleaning up...
[THREAD] Mongoose manager freed
[THREAD] Webserver thread exiting normally
```

#### Shutdown Logging
```c
[STOP] Requesting webserver thread to stop...
[STOP] Waiting for thread to exit (this may take up to 1 second)...
[STOP] Webserver thread stopped (exit code: 0)
```

#### REPL Request Logging
```c
[DEBUG] Received /api/repl/send request
[DEBUG] Request body length: 21
[DEBUG] Request body: {"content":"100 + 200"}
[DEBUG] Extracted code: '100 + 200'
[DEBUG] Code length: 9 bytes
[DEBUG] Executing with output capture
```

#### Python Execution Logging
```c
[EXEC] Calling py_exec_single_input_with_output...
[EXEC] py_exec_single_input_with_output returned: 300
[DEBUG] Execution complete, output: '300\n'
[DEBUG] Response sent to client
```

---

## Feature Flags Configuration

The zedit external now uses feature flags to enable/disable optional functionality:

**Location:** `zedit.c:23-28`

```c
// Feature flags - comment out to disable
#define ENABLE_AUTH 1              // Authentication token (REQUIRED for web frontend)
// #define ENABLE_RATE_LIMITING 1     // Rate limiting per IP (optional)
// #define ENABLE_SECURITY_HEADERS 1  // Security headers in HTTP responses (optional)
// #define ENABLE_INPUT_VALIDATION 1  // Input validation (null bytes, size limits) (optional)
#define ENABLE_OUTPUT_CAPTURE 1    // Capture Python output in API responses (REQUIRED for web REPL)
```

### Current Configuration

**Enabled (Required):**
- `ENABLE_AUTH` - Token-based authentication for API endpoints
- `ENABLE_OUTPUT_CAPTURE` - Captures Python stdout/stderr for web display

**Disabled (Optional):**
- `ENABLE_RATE_LIMITING` - Limits to 60 requests per minute per IP
- `ENABLE_SECURITY_HEADERS` - Adds CSP, X-Frame-Options, XSS-Protection headers
- `ENABLE_INPUT_VALIDATION` - Validates code size and checks for null bytes

Optional features can be enabled by uncommenting the corresponding `#define` directives.

---

## Working Features

### Core Functionality
- ✅ **HTTP Webserver** - Runs in separate thread without blocking Max
- ✅ **Web REPL** - Interactive Python prompt with immediate expression results
- ✅ **Code Editor** - Execute Python scripts from web interface
- ✅ **Authentication** - Secure token-based API access
- ✅ **Clean Shutdown** - Cancel/stop works without hanging (1-2 second graceful exit)
- ✅ **Multi-threaded Python Execution** - Web requests and Max messages work simultaneously
- ✅ **Output Capture** - Python stdout/stderr returned to web frontend

### API Endpoints

All endpoints require `X-Auth-Token` header (except `/api/auth`):

- `GET /api/auth` - Retrieve authentication token
- `POST /api/hello` - Test endpoint
- `POST /api/code/save` - Execute Python code (file mode)
- `POST /api/code/run` - Execute Python code (file mode)
- `POST /api/repl/send` - Execute Python code (REPL mode, shows expression results)
- Static file serving for web interface

---

## Testing Instructions

1. Load zedit external in Max/MSP
2. Send `start` message to zedit object
3. Check Max console for startup messages:
   ```
   Mongoose version : v7.x
   Listening on     : http://0.0.0.0:8000
   Web root         : [/path/to/webroot]
   Auth token       : [32-char token]
   IMPORTANT: Use this token in X-Auth-Token header for API requests
   Python initialized and GIL released for multi-threading
   [THREAD] Webserver thread started
   [THREAD] Entering event loop...
   ```
4. Open web browser to `http://localhost:8000`
5. Enter auth token when prompted
6. Test REPL: Type `1+1` → should immediately return `2`
7. Test shutdown: Send `cancel` → should stop within 1-2 seconds

---

## Technical Details

### Threading Architecture

```
┌─────────────────────────────────────────────────────────┐
│ Max Main Thread (Thread B)                              │
│  - Runs Max/MSP event loop                              │
│  - Initializes Python (py_init)                         │
│  - Releases GIL after init (PyEval_SaveThread)          │
│  - Handles Max messages to zedit object                 │
│  - Re-acquires GIL when needed (PyGILState_Ensure)      │
└─────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────┐
│ Webserver Thread (Thread A)                             │
│  - Runs mongoose HTTP server (zedit_threadproc)         │
│  - Handles HTTP requests from web frontend              │
│  - Acquires GIL for Python execution (PyGILState_Ensure)│
│  - Releases GIL after execution (PyGILState_Release)    │
│  - Checks cancel flag every 1 second (mg_mgr_poll)      │
└─────────────────────────────────────────────────────────┘

                    ┌──────────────────┐
                    │ Python GIL       │
                    │ (Global Lock)    │
                    │  - Shared        │
                    │  - Thread-safe   │
                    └──────────────────┘
```

### Python GIL Management Flow

```
1. Max starts, creates zedit object
   └─> zedit_new() called

2. Python initialization
   └─> py_init() called
       ├─> Py_Initialize()           [GIL acquired by main thread]
       └─> PyEval_SaveThread()       [GIL released, available for all threads]

3. Webserver thread starts
   └─> zedit_threadproc() running

4. Web request arrives
   └─> handle_http_message()
       └─> zedit_exec_single_input_with_output()
           └─> py_exec_single_input_with_output()
               ├─> PyGILState_Ensure()      [Acquires GIL]
               ├─> PyRun_String()           [Execute Python code]
               └─> PyGILState_Release()     [Releases GIL]

5. Max message arrives
   └─> zedit_anything()
       └─> py_anything()
           ├─> PyGILState_Ensure()          [Acquires GIL]
           ├─> PyRun_String()               [Execute Python code]
           └─> PyGILState_Release()         [Releases GIL]
```

Both threads can execute Python code concurrently because:
- The GIL is released after initialization
- Each thread uses `PyGILState_Ensure/Release` to safely acquire/release the GIL
- Python's GIL mechanism ensures thread-safe execution

---

## Files Modified

1. **zedit.c** - Main zedit external implementation
   - Simplified event handler
   - Fixed thread loop
   - Added comprehensive logging
   - Enabled output capture flag

2. **py.h** - Python integration library
   - Added `PyEval_SaveThread()` after initialization to release GIL

---

## Lessons Learned

1. **Event handlers should be minimal** - Logging every event in high-frequency callbacks causes performance issues

2. **GIL must be released after Py_Initialize()** - When using Python in multi-threaded applications, the main thread must release the GIL after initialization so other threads can acquire it

3. **Thread synchronization requires careful design** - Proper use of `PyGILState_Ensure/Release` in all Python-calling functions is essential

4. **Logging is essential for debugging** - Comprehensive, tagged logging (`[THREAD]`, `[STOP]`, `[DEBUG]`, `[EXEC]`) made it possible to diagnose the threading issues

5. **Match working implementations** - When debugging, comparing with a known-working standalone version (test-server.c) was invaluable

---

## References

- Mongoose HTTP Server: https://github.com/cesanta/mongoose
- Python C API Threading: https://docs.python.org/3/c-api/init.html#thread-state-and-the-global-interpreter-lock
- Max SDK Threading: Max SDK documentation on systhread

---

**Author:** Claude (Anthropic)
**Date:** 2025-10-18
**Status:** All issues resolved, zedit fully functional
