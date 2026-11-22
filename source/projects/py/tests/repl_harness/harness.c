// SPDX-License-Identifier: MIT
// py_repl harness: py.c's repl section (between its markers) against embedded
// CPython, with the Max functions it uses stubbed. Built and run by run.sh.
// The A_CANT methods cannot be called from a patch; this checks them headless.
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct { struct { PyObject* globals; atomic_int repl_running; } python; } t_py;
#define MAX_ERR_NONE 0
#define PY_MAX_CODE_LENGTH 65536
static void* sysmem_newptrclear(size_t n) { return calloc(1, n); }
static void sysmem_freeptr(void* p) { free(p); }
static long py_validate_code(t_py* x, const char* c) { return strstr(c, "FORBIDDEN") ? 1 : 0; }
static void py_handle_error(t_py* x, const char* msg) { fprintf(stderr, "[handle_error] %s\n", msg); PyErr_Clear(); }
static atomic_int py_repl_depth = 0;

#include "py_repl_abi.h"
#include "py_repl_support.h"
#include "repl_section.c"

static char buf[65536];
static long kinds[64]; static int nk;
static void emit(void* ctx, long kind, const char* t, long n) {
    strncat(buf, t, n); if (nk < 64) kinds[nk++] = kind;
}
static py_repl_sink sink = {sizeof(py_repl_sink), NULL, emit};
static t_py X;
static int fails;
#define CHECK(c, msg) do { if (!(c)) { fails++; printf("FAIL %s\n  buf=%s\n", msg, buf); } else printf("ok   %s\n", msg); } while (0)
static long run(const char* code) { buf[0] = 0; nk = 0; return (long)(intptr_t)py_repl_run(&X, code, &sink); }

static void* interrupter(void* arg) { usleep(200000); long r = (long)(intptr_t)py_repl_interrupt(&X); (void)r; return NULL; }

int main(void) {
    Py_Initialize();
    X.python.globals = PyDict_New();
    PyDict_SetItemString(X.python.globals, "__builtins__", PyEval_GetBuiltins());
    PyDict_SetItemString(X.python.globals, "__name__", PyUnicode_FromString("foo"));
    PyObject* out0 = PySys_GetObject("stdout");
    PyThreadState* main_ts = PyEval_SaveThread(); // as py: GIL released between calls

    CHECK(run("1 + 2") == PY_REPL_OK && !strcmp(buf, "3\n") && kinds[0] == PY_REPL_RESULT, "expression displayed");
    CHECK(run("print('a')") == PY_REPL_OK && !strcmp(buf, "a\n") && kinds[0] == PY_REPL_OUT, "print to OUT");
    CHECK(run("1/0") == PY_REPL_EXCEPTION && strstr(buf, "ZeroDivisionError") && kinds[nk-1] == PY_REPL_TRACEBACK, "traceback");
    CHECK(run("raise SystemExit(2)") == PY_REPL_EXCEPTION && strstr(buf, "SystemExit: 2"), "SystemExit reported");
    CHECK(run("FORBIDDEN") == PY_REPL_REFUSED && strstr(buf, "security_mode"), "validation refuses");

    run("import sys\nkeep = sys.stdout");
    CHECK(run("n = keep.write('stale')") == PY_REPL_OK && buf[0] == 0, "kept stdout writes nowhere after its run");

    pthread_t th; pthread_create(&th, NULL, interrupter, NULL);
    long st = run("while True: pass");
    pthread_join(th, NULL);
    CHECK(st == PY_REPL_INTERRUPTED && strstr(buf, "KeyboardInterrupt"), "interrupt from another thread");

    CHECK((long)(intptr_t)py_repl_interrupt(&X) == -1, "interrupt with no run is refused");
    CHECK(run("1") == PY_REPL_OK && !strcmp(buf, "1\n"), "next run unaffected");

    atomic_store(&X.python.repl_running, 1);
    CHECK(run("1") == PY_REPL_BUSY, "re-entry is busy");
    atomic_store(&X.python.repl_running, 0);

    CHECK((long)(intptr_t)py_repl_complete(&X, "def f():") == PY_REPL_INCOMPLETE, "complete: incomplete");
    CHECK((long)(intptr_t)py_repl_complete(&X, "x = 1") == PY_REPL_COMPLETE, "complete: complete");
    buf[0] = 0; nk = 0;
    long n = (long)(intptr_t)py_repl_names(&X, "pri", &sink);
    CHECK(n >= 1 && strstr(buf, "print") && kinds[0] == PY_REPL_NAME, "names");
    CHECK((long)(intptr_t)py_repl_abi(&X) == PY_REPL_ABI, "abi");
    py_repl_sink small = {4, NULL, emit};
    CHECK((long)(intptr_t)py_repl_run(&X, "1", &small) == PY_REPL_REFUSED, "small sink refused");

    PyEval_RestoreThread(main_ts);
    CHECK(PySys_GetObject("stdout") == out0, "stdout restored");
    printf("%s\n", fails ? "FAILED" : "all passed");
    return fails != 0;
}
