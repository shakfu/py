# Test Server for zedit Web Interface

## Overview

This is a **standalone test server** that allows you to test the zedit web interface without Max/MSP. It implements the same API endpoints as zedit.c but runs independently.

**Purpose**: Isolate and debug web app <-> server communication issues.

## Features

- ✅ No Max/MSP dependencies
- ✅ Pure Python C API + Mongoose HTTP server
- ✅ Same API endpoints as zedit
- ✅ Full stdout/stderr capture
- ✅ Shared Python namespace (editor/terminal)
- ✅ Token authentication
- ✅ Rate limiting

## Building

```bash
make -f Makefile.test
```

This creates the `test-server` executable.

## Running

```bash
./test-server
```

You should see:
```
=== Standalone Test Server for zedit ===

Auth token: abc123xyz...
Use this in X-Auth-Token header for API requests

Initializing Python...
Python initialized

Starting HTTP server on http://localhost:8000
Web root: ./web/public

Server running. Press Ctrl+C to stop.
Open http://localhost:8000 in your browser
```

## Testing

1. **Open browser**: Navigate to http://localhost:8000
2. **Check auth token**: Server prints token on startup (also visible at `/api/auth`)
3. **Test editor**:
   - Type: `print('hello from editor')`
   - Click "Run"
   - Check browser console for `[DEBUG]` messages
   - Check server console for `[RUN]` messages
4. **Test terminal**:
   - Switch to Terminal tab
   - Type: `print('hello from terminal')`
   - Press Enter
   - Check server console for `[REPL]` messages

5. **Test multi-line REPL**:
   - Type: `def color(x):`
   - Press Enter → prompt changes to `... `
   - Type: `    return x+1`
   - Press Enter → prompt stays `... `
   - Press Enter again (empty line) → executes function
   - Type: `color(5)` → should output `6`

## Server Output

The test server prints detailed logs:

```
[AUTH] Token requested                    ← Client fetched auth token
[RUN] Executing code: print('hello')      ← Code received
[RUN] Output: hello                       ← Python output captured
```

```
[REPL] Executing code: 1+1                ← REPL code
[REPL] Output: 2                          ← REPL result
```

## API Endpoints

All endpoints match zedit.c exactly:

- `GET /api/auth` - Get authentication token
- `POST /api/code/save` - Save and execute code
- `POST /api/code/run` - Execute code
- `POST /api/repl/send` - Execute REPL code

## Debugging

### Check Network Tab

Open DevTools → Network tab:
- Filter: `/api/`
- Click on request
- Check "Response" tab for JSON

Expected response:
```json
{
  "result": "OK",
  "output": "hello\n"
}
```

### Check Browser Console

Should see our debug logs:
```
[DEBUG] Run response received: {result: "OK", output: "hello\n"}
[DEBUG] Displaying output: hello
```

### Check Server Console

Should see execution logs:
```
[RUN] Executing code: print('hello')
[RUN] Output: hello
```

## How Multi-Line REPL Works

The REPL terminal behaves like Python's interactive console:

**Single-line execution** (prompt: `>>>`):
```python
>>> 1 + 1
2
>>> print('hello')
hello
```

**Multi-line execution** (prompt changes to `...`):
```python
>>> def greet(name):
...     return f"Hello, {name}!"
...                                 ← Empty line executes
>>> greet("Claude")
'Hello, Claude!'
```

**Implementation** (frontend-only):
1. Frontend detects multi-line start (line ends with `:` or starts with `def`, `class`, `if`, `for`, etc.)
2. Frontend switches to `... ` prompt and accumulates lines in buffer
3. Empty line triggers: frontend sends complete code block to backend
4. Backend executes with `Py_single_input` (shows expression results)
5. Frontend resets to `>>> ` prompt

**Key behaviors**:
- Multi-line detection happens entirely in JavaScript (editor.mjs)
- Backend always receives complete code blocks
- `Py_single_input`: shows expression results (REPL mode)
- `Py_file_input`: doesn't show expression results (editor mode)
- Empty line in multi-line mode executes accumulated code

## Differences from zedit

The test server:
- ❌ No Max/MSP integration
- ❌ No Max console output
- ✅ Same API endpoints
- ✅ Same authentication
- ✅ Same output capture
- ✅ Same Python namespace handling
- ✅ Same REPL multi-line behavior

## Testing Namespace Persistence

**In Editor**:
```python
def greet(name):
    return f"Hello, {name}!"

print(greet("World"))
```

Click "Run" → Output: `Hello, World!`

**In Terminal**:
```python
>>> greet("Claude")
Hello, Claude!
```

This proves the shared namespace works correctly.

## Cleaning Up

```bash
make -f Makefile.test clean
```

## Files

- `test-server.c` - Standalone server implementation
- `Makefile.test` - Build configuration
- `mongoose.c/h` - HTTP server library (shared with zedit)

## Troubleshooting

**Problem**: `Failed to start server`
**Solution**: Port 8000 is in use. Stop other servers or change `s_listening_address` in code.

**Problem**: `Failed to import sys` or `Failed to import io`
**Solution**: Python installation issue. Check Python is properly installed.

**Problem**: Browser shows "Unauthorized"
**Solution**: Check that X-Auth-Token header matches token printed by server.

**Problem**: No output in console
**Solution**:
1. Check browser DevTools Network tab for response
2. Check server console for `[RUN]` or `[REPL]` messages
3. Check if output is empty string or contains data

## Comparison with zedit.c

| Feature | test-server | zedit.c |
|---------|-------------|---------|
| HTTP Server | Mongoose | Mongoose |
| Python Execution | Python C API | py.h wrapper |
| Output Capture | Direct StringIO | py.h StringIO |
| Max Integration | None | Full |
| Use Case | Testing | Production |

## Next Steps After Testing

Once you've verified the test-server works correctly:

1. If **test-server works** but **zedit doesn't**:
   - Issue is in Max/MSP integration
   - Check Max console for errors
   - Check py.h sandboxing

2. If **neither works**:
   - Issue is in web app
   - Check JavaScript console for errors
   - Check network requests

3. If **both work**:
   - Web app is functioning correctly
   - Continue using zedit normally
