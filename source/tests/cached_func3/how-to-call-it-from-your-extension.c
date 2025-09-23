#include "codecaching.h"

static CodecachingState g_cc;

static int myexec(PyObject *m) {
    if (codecaching_init(&g_cc) < 0) return -1;
    codecaching_set_sandbox_default(&g_cc, 1);
    return 0;
}

static void myfree(void *m) {
    (void)m;
    codecaching_free(&g_cc);
}

/* ... inside a method ... */
PyObject *res = codecaching_run_cached(&g_cc, expr_unicode, locals_dict, -1, /*mode=*/0); /* eval */
PyObject *res2 = codecaching_run_cached(&g_cc, code_unicode, locals_dict, -1, /*mode=*/1); /* file */



