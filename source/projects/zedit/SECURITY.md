# zedit Security Documentation

## Overview

This document outlines the security features, configuration options, and best practices for the zedit web-based Python code editor.

## Security Features (Implemented)

### 1. Authentication & Authorization

**Token-Based Authentication:**
- 32-character random token generated on server start
- Required for all `/api/*` endpoints (except `/api/auth`)
- Token sent via `X-Auth-Token` header
- HTTP 401 Unauthorized for invalid/missing tokens

**Location:** `zedit.c:107-134, 239-246`

**Usage:**
```c
// Server generates token on start
generate_auth_token();

// Client fetches token
GET /api/auth
Response: {"token": "abc123..."}

// Client includes in requests
POST /api/code/save
X-Auth-Token: abc123...
```

### 2. Network Isolation

**Localhost-Only Binding:**
- Server binds to `localhost:8000` (127.0.0.1)
- Not accessible from external network
- Prevents remote code execution attacks

**Location:** `zedit.c:28`

**Configuration:**
```c
static const char* s_listening_address = "http://localhost:8000";
```

### 3. Rate Limiting

**Request Throttling:**
- Max 60 requests per minute per IP
- Applied to all `/api/*` endpoints
- HTTP 429 Rate Limit Exceeded response
- Sliding window algorithm
- Tracks up to 10 unique IPs

**Location:** `zedit.c:42-174, 218-228`

**Configuration:**
```c
#define MAX_REQUESTS_PER_MINUTE 60
```

### 4. Input Validation

**Code Size Limits:**
- `/api/code/save`: Max 1MB (1,048,576 bytes)
- `/api/repl/send`: Max 100KB (102,400 bytes)

**Content Validation:**
- Null byte detection
- JSON structure validation
- UTF-8 encoding validation

**Location:** `zedit.c:276-343`

**Error Responses:**
```json
{"error": "Code too large (max 1MB)"}
{"error": "Invalid input: null bytes detected"}
{"error": "Parameters missing"}
```

### 5. Security Headers

**HTTP Security Headers:**
- `X-Frame-Options: DENY` - Prevents clickjacking
- `X-Content-Type-Options: nosniff` - Prevents MIME sniffing
- `X-XSS-Protection: 1; mode=block` - XSS protection
- `Content-Security-Policy` - Restricts resource loading
- `Referrer-Policy: no-referrer` - Privacy protection

**Location:** `zedit.c:210-216`

**CSP Configuration:**
```
default-src 'self';
script-src 'self' 'unsafe-inline';
style-src 'self' 'unsafe-inline'
```

### 6. HTTPS Support (Optional)

**TLS/SSL Configuration:**
- Optional HTTPS support via Mongoose TLS
- Requires certificate and key files
- Easy configuration via static variables

**Location:** `zedit.c:32-41, 673-680`

**Setup:**
```bash
# Generate self-signed certificate
openssl req -x509 -newkey rsa:2048 \
  -keyout key.pem -out cert.pem \
  -days 365 -nodes

# Configure in zedit.c
static const char* s_tls_cert = "/path/to/cert.pem";
static const char* s_tls_key = "/path/to/key.pem";
static const char* s_listening_address = "https://localhost:8000";
```

### 7. Dependency Security

**Updated Dependencies:**
- PrismJS: 1.28.0 → 1.30.0 (CVE fixed)
- All npm packages audited (0 vulnerabilities)

**Vulnerability Scan:**
```bash
cd source/projects/zedit/web
npm audit
# found 0 vulnerabilities
```

## Security Configuration

### Development Mode (Current)

**Profile:** Localhost development
**Use case:** Single user, trusted code
**Configuration:**
- ✅ Localhost binding
- ✅ Token authentication
- ✅ Rate limiting
- ✅ Input validation
- ✅ Security headers
- ❌ HTTPS (optional)
- ❌ Sandboxing (optional)

**Risk Level:** Low

### Production Mode (Recommended)

**Profile:** Production deployment
**Use case:** Multiple users, potentially untrusted code
**Configuration:**
- ✅ All development features
- ✅ HTTPS enabled
- ✅ Code sandboxing (see SANDBOXING.md)
- ✅ Container isolation (Docker)
- ✅ Regular security updates
- ✅ Audit logging

**Risk Level:** Medium-Low

### High Security Mode

**Profile:** Untrusted users, hostile environment
**Use case:** Public-facing, untrusted code execution
**Configuration:**
- ✅ All production features
- ✅ Full OS-level sandboxing
- ✅ Network isolation
- ✅ Resource limits (CPU, memory, time)
- ✅ Read-only file system
- ✅ IDS/IPS monitoring
- ❌ Consider: Don't allow arbitrary code execution

**Risk Level:** Medium

## Threat Model

### Threats Mitigated

✅ **Remote Code Execution (RCE)**
- Mitigated by: Localhost binding, authentication

✅ **Cross-Site Request Forgery (CSRF)**
- Mitigated by: Token authentication, same-origin policy

✅ **Denial of Service (DoS)**
- Mitigated by: Rate limiting, input size limits

✅ **Code Injection**
- Mitigated by: Input validation, null byte detection

✅ **Clickjacking**
- Mitigated by: X-Frame-Options header

✅ **MIME Sniffing**
- Mitigated by: X-Content-Type-Options header

✅ **Dependency Vulnerabilities**
- Mitigated by: Updated packages, npm audit

### Threats Requiring Additional Mitigation

⚠️ **Arbitrary Code Execution**
- Current: Full Python access
- Mitigation: Enable sandboxing (see SANDBOXING.md)

⚠️ **Resource Exhaustion**
- Current: No CPU/memory limits
- Mitigation: OS-level resource limits, containerization

⚠️ **Data Exfiltration**
- Current: Python can make network requests
- Mitigation: Network isolation, sandboxing

⚠️ **Man-in-the-Middle (MITM)**
- Current: HTTP only (localhost mitigates)
- Mitigation: Enable HTTPS

⚠️ **Privilege Escalation**
- Current: Runs with user privileges
- Mitigation: Run as unprivileged user, containers

## Security Best Practices

### Deployment Checklist

**Before Production:**
- [ ] Enable HTTPS with valid certificates
- [ ] Configure sandboxing (see SANDBOXING.md)
- [ ] Set up container isolation
- [ ] Configure resource limits
- [ ] Enable audit logging
- [ ] Review and restrict Python module imports
- [ ] Test all security controls
- [ ] Perform security scan
- [ ] Review and update dependencies
- [ ] Document security architecture

**Ongoing:**
- [ ] Monitor server logs
- [ ] Update dependencies regularly
- [ ] Rotate authentication tokens
- [ ] Review rate limit effectiveness
- [ ] Test backup/recovery procedures
- [ ] Conduct security audits
- [ ] Stay informed about security advisories

### Operational Security

**Logging:**
```c
// Current logging (zedit.c)
post("Auth token       : %s", s_auth_token);
post("code executed (length: %zu bytes)", code_len);
post("repl executed (length: %zu bytes)", code_len);
```

**Monitoring:**
- Watch for rate limit triggers
- Monitor authentication failures
- Track code execution patterns
- Alert on suspicious activity

**Incident Response:**
1. Stop the server immediately
2. Review execution logs
3. Check for data compromise
4. Rotate authentication tokens
5. Update security controls
6. Document incident
7. Implement preventive measures

## Security Testing

### Manual Testing

```bash
# Test authentication
curl http://localhost:8000/api/hello
# Should return 401 Unauthorized

curl -H "X-Auth-Token: invalid" http://localhost:8000/api/hello
# Should return 401 Unauthorized

curl -H "X-Auth-Token: VALID_TOKEN" http://localhost:8000/api/hello
# Should return 200 OK

# Test rate limiting
for i in {1..65}; do
  curl -H "X-Auth-Token: VALID_TOKEN" http://localhost:8000/api/hello
done
# Should return 429 after 60 requests

# Test input validation
curl -X POST -H "X-Auth-Token: VALID_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"content":"'$(python -c "print('A'*2000000)")'"}'  \
  http://localhost:8000/api/code/save
# Should return 400 Code too large

# Test security headers
curl -I http://localhost:8000/
# Should include X-Frame-Options, CSP, etc.
```

### Automated Testing

```python
# security_tests.py
import requests
import time

BASE_URL = "http://localhost:8000"

def test_auth_required():
    """Test that authentication is required"""
    resp = requests.get(f"{BASE_URL}/api/hello")
    assert resp.status_code == 401

def test_rate_limiting():
    """Test rate limiting enforcement"""
    token = get_auth_token()
    headers = {"X-Auth-Token": token}

    for i in range(65):
        resp = requests.get(f"{BASE_URL}/api/hello", headers=headers)
        if i < 60:
            assert resp.status_code == 200
        else:
            assert resp.status_code == 429

def test_input_validation():
    """Test input size limits"""
    token = get_auth_token()
    headers = {"X-Auth-Token": token, "Content-Type": "application/json"}
    data = {"file_id": 1, "content": "A" * 2000000}  # 2MB

    resp = requests.post(
        f"{BASE_URL}/api/code/save",
        json=data,
        headers=headers
    )
    assert resp.status_code == 400
    assert "too large" in resp.json()["error"]

def test_security_headers():
    """Test security headers are present"""
    resp = requests.get(f"{BASE_URL}/")
    assert "X-Frame-Options" in resp.headers
    assert "X-Content-Type-Options" in resp.headers
    assert "Content-Security-Policy" in resp.headers

# Run tests
if __name__ == "__main__":
    test_auth_required()
    test_rate_limiting()
    test_input_validation()
    test_security_headers()
    print("All security tests passed!")
```

## Reporting Security Issues

If you discover a security vulnerability:

1. **Do NOT** open a public GitHub issue
2. Email security concerns to: [security contact]
3. Include:
   - Description of vulnerability
   - Steps to reproduce
   - Potential impact
   - Suggested fix (optional)
4. Wait for response before disclosure
5. Follow coordinated disclosure timeline

## Security Roadmap

### Future Enhancements

**Short Term:**
- [ ] Audit logging to file
- [ ] Token expiration/rotation
- [ ] IP whitelist support
- [ ] Configurable rate limits

**Medium Term:**
- [ ] OAuth/OpenID Connect support
- [ ] Multi-user authentication
- [ ] Role-based access control (RBAC)
- [ ] Signed code execution

**Long Term:**
- [ ] Hardware security module (HSM) integration
- [ ] Zero-knowledge architecture
- [ ] Formal security verification
- [ ] Security certification (e.g., SOC 2)

## References

- [OWASP Top 10](https://owasp.org/www-project-top-ten/)
- [OWASP API Security Top 10](https://owasp.org/www-project-api-security/)
- [CWE Top 25](https://cwe.mitre.org/top25/)
- [NIST Cybersecurity Framework](https://www.nist.gov/cyberframework)
- [Mongoose Security](https://mongoose.ws/documentation/#security)
- [Python Security](https://python.readthedocs.io/en/stable/library/security_warnings.html)

## License

This security documentation is part of the zedit project and follows the same license.

## Version

**Document Version:** 1.0
**Last Updated:** 2025-10-08
**zedit Version:** Latest
