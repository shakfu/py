# zedit: a web-based code-editor embedded in an max external

This subproject provides an example of a python3 external whith the following features:

- Embeds a python interpreter via the mamba single header python3 c library
- Embeds the c-based mongoose webserver
- Provides a web-based code-editor
- Provides a web-based interactive terminal

This ui in this project is very powerfull using typescript/javascript-based web technologies. This is probably overkill for a python3 external, but illustratrive nonetheless of the potential achieved by embedding an small webserver in an external.

For simpler examples of user interfaces which interact with python3 externals see the `patchers/bpatchers_ui` and the `patchers/bpatcher_py` folders.

## Usage

You have to setup the project before being able to use it:

```bash
cd py-js/source/projects/zedit/web
make
```

then return to the root of the project

```bash
cd py-js
make dev
```

It is possible to build the external and embed everything into a bundle by building the project in Release mode.

This will build and deploy the javascript / typescript code to the `zedit/web/public` folder. There is folder called `zedit/webroot` which is a symlink to the above public folder.

Note that the `n4m` folder contains an alternative impplementation which can be ignored for the time being as it is a work-in-pgress

## Current Status

The current implementation uses [codemirror](https://codemirror.net), [xtermjs](https://github.com/xtermjs/xterm.js) and the [mongoose](https://github.com/cesanta/mongoose) embedded webserver library to interact with the user and the max external.

Current features are:

- Embedded webserver running in a separate thread
- Python3 web-editor with dark theme, syntax highlighting, auto-complete, and search, ..
- Basic web terminal
- Commands
  - Help: open a list of keboard commands
  - Open: open a file from the client side
  - Save: dump contents to external
  - Run: dump and run contents

See `zedit.maxhelp` for a demo of the external launching the embedded webserver and running the code-mirror web-editor.

There is also a node-for-max variation on (1) using expressjs as the webserver.

## Security

zedit implements comprehensive security controls for safe code execution:

**Core Security Features:**
- 🔒 **Token Authentication** - Required for all API endpoints
- 🏠 **Localhost-Only** - Binds to 127.0.0.1 (no external access)
- ⏱️ **Rate Limiting** - Max 60 requests/minute per IP
- ✅ **Input Validation** - Size limits and content validation
- 🛡️ **Security Headers** - CSP, X-Frame-Options, XSS protection
- 🔐 **HTTPS Support** - Optional TLS/SSL (production ready)
- 📦 **Zero Vulnerabilities** - All dependencies audited and updated

### Token-Based Authentication

**How It Works:**

1. **Token Generation** - Server generates 32-character random token on startup
2. **Token Distribution** - Available via unauthenticated endpoint: `GET /api/auth`
3. **Token Validation** - Required in `X-Auth-Token` header for all `/api/*` endpoints
4. **Token Lifecycle** - Valid for entire server session (regenerated on restart)

**Token Location** (`zedit.c:142-149, 253-268`):
```c
// Generated once on server start
void generate_auth_token(void) {
    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    for (int i = 0; i < 32; i++) {
        s_auth_token[i] = charset[rand() % (sizeof(charset) - 1)];
    }
}
```

**Authentication Flow:**
```
Client                          Server
  |                               |
  |----GET /api/auth------------->|
  |<---{"token": "abc123..."}-----|
  |                               |
  |----POST /api/code/run-------->|
  | Header: X-Auth-Token: abc123  |
  |<---200 OK (authenticated)-----|
  |                               |
  |----POST /api/code/run-------->|
  | (no token or invalid)         |
  |<---401 Unauthorized-----------|
```

### Token Handling: Automatic vs Manual

**Web Interface (Browser): ✅ AUTOMATIC**

The web editor automatically handles all token operations:

```javascript
// Auto-fetch on page load (editor.mjs:9-17)
fetch("/api/auth")
    .then((response) => response.json())
    .then((data) => {
        authToken = data.token;  // Stored globally
    })

// Auto-included in all API requests (editor.mjs:80-83)
headers: {
    "Content-type": "application/json",
    "X-Auth-Token": authToken,  // ← Automatically added
}
```

**You don't need to do anything** - just open the web interface and it works.

**Other Clients (curl, Postman, Python): ⚠️ MANUAL**

For API calls outside the web interface, you must manually include the token:

**Step 1**: Get token from Max console when server starts:
```
Listening on     : http://localhost:8000
Auth token       : abc123xyz456...
IMPORTANT: Use this token in X-Auth-Token header for API requests
```

**Step 2**: Include in every API request:

**curl example**:
```bash
# Get token
TOKEN=$(curl -s http://localhost:8000/api/auth | jq -r '.token')

# Use in requests
curl -H "X-Auth-Token: $TOKEN" \
     -H "Content-Type: application/json" \
     -d '{"content":"print(123)"}' \
     http://localhost:8000/api/code/run
```

**Python example**:
```python
import requests

# Get token
resp = requests.get("http://localhost:8000/api/auth")
token = resp.json()["token"]

# Use in requests
headers = {"X-Auth-Token": token}
requests.post("http://localhost:8000/api/code/run",
              headers=headers,
              json={"content": "print(123)"})
```

**Summary Table:**

| **Context** | **Token Handling** | **Action Required** |
|-------------|-------------------|---------------------|
| Web editor (browser) | Automatic | None - just open the page |
| curl / API clients | Manual | Fetch from `/api/auth` and add `X-Auth-Token` header |
| Max console | Manual | Copy token from startup logs |

### HTTPS Configuration

**Current Status**: ❌ **HTTP only** (not HTTPS)

```c
// zedit.c:25-41
static const char* s_listening_address = "http://localhost:8000";  // HTTP
static const char* s_tls_cert = NULL;  // No certificate
static const char* s_tls_key = NULL;   // No key
```

**Why HTTP is Acceptable** (currently):
- Server binds to `localhost` only - no external network exposure
- MITM attacks require local access (already compromised)
- Suitable for development/testing

**HTTPS Support**: ✅ **Available but disabled by default**

**To Enable HTTPS**:

**Step 1**: Generate self-signed certificate:
```bash
openssl req -x509 -newkey rsa:2048 \
  -keyout key.pem -out cert.pem \
  -days 365 -nodes
```

**Step 2**: Edit `zedit.c` (lines 38-41):
```c
static const char* s_listening_address = "https://localhost:8000";  // Change to https
static const char* s_tls_cert = "/path/to/cert.pem";  // Set path
static const char* s_tls_key = "/path/to/key.pem";    // Set path
```

**Step 3**: Rebuild:
```bash
make clean
make build
```

**For Production**: HTTPS strongly recommended for untrusted environments.

### Security Concerns & Limitations

**🔴 Critical Issues**:

1. **Weak Token Generation** - Uses `rand()` instead of cryptographically secure RNG
   - **Risk**: Predictable if attacker knows seed
   - **Fix**: Use `getrandom()` or `/dev/urandom`

2. **No Token Expiration** - Token valid indefinitely until restart
   - **Risk**: Stolen token never expires
   - **Fix**: Implement token rotation (see SECURITY.md future work)

3. **Unauthenticated Token Endpoint** - `/api/auth` publicly exposes token
   - **Risk**: Anyone on localhost can fetch token
   - **Rationale**: Designed for single-user local development

4. **Timing Attack Vulnerability** - Token comparison not constant-time
   ```c
   return (strcmp(token, s_auth_token) == 0);  // ⚠️ Not constant-time
   ```
   - **Risk**: Allows timing-based token guessing
   - **Fix**: Use constant-time comparison function

5. **HTTP Transmits Token in Cleartext**
   - **Risk**: Vulnerable to local packet sniffing
   - **Mitigation**: Localhost-only binding reduces risk
   - **Production**: Requires HTTPS

**Recommendation for Production**:
- Enable HTTPS
- Use cryptographically secure token generation
- Add token expiration/rotation
- Implement constant-time token comparison
- See `SECURITY.md` for complete hardening guide

### Additional Security Documentation

**For Production Deployment:**
- `SECURITY.md` - Complete security guide with threat model and best practices
- `SANDBOXING_IMPLEMENTATION.md` - Python code sandboxing (enabled by default)
- `HARDENED.md` - Production deployment checklist
- `PY_H_SECURITY_ANALYSIS.md` - Detailed vulnerability analysis and fixes

**Python Sandboxing** (Enabled by Default):
- Removes dangerous builtins: `eval`, `exec`, `open`, `compile`, etc.
- Restricts imports to safe modules: `math`, `random`, `json`, etc.
- Blocks system access: `os`, `sys`, `subprocess`, `socket`, etc.
- See `SANDBOXING_IMPLEMENTATION.md` for details

## Future Direction

- use [jquery.terminal](https://github.com/jcubic/jquery.terminal)
