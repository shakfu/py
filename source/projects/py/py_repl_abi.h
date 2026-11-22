// SPDX-License-Identifier: MIT
// py_repl_abi.h: the interface py exposes to py_repl through A_CANT methods.
// Spec: docs/dev/py_repl-spec.md, section 1. Bump PY_REPL_ABI on any change.

#ifndef PY_REPL_ABI_H
#define PY_REPL_ABI_H

#define PY_REPL_ABI 1

/* emit kinds */
enum { PY_REPL_OUT, PY_REPL_ERR, PY_REPL_RESULT, PY_REPL_TRACEBACK, PY_REPL_NAME };

/* repl_run results */
enum { PY_REPL_OK, PY_REPL_EXCEPTION, PY_REPL_INTERRUPTED, PY_REPL_BUSY, PY_REPL_REFUSED };

/* repl_complete results */
enum { PY_REPL_COMPLETE, PY_REPL_INCOMPLETE, PY_REPL_INVALID };

/* Receives UTF-8 text, valid only during the call. */
typedef struct py_repl_sink {
    long size; /* sizeof(py_repl_sink): py rejects a smaller struct */
    void* ctx;
    void (*emit)(void* ctx, long kind, const char* text, long len);
} py_repl_sink;

#endif
