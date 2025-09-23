#ifndef CODECACHING_H
#define CODECACHING_H

#define PY_SSIZE_T_CLEAN
#include <Python.h>

/* ===========================================================
 *                    Minimal C-level LRU
 * =========================================================== */

typedef struct LRUNode {
    PyObject *key;           /* strong ref to key */
    struct LRUNode *prev;
    struct LRUNode *next;
} LRUNode;

typedef struct {
    PyObject *map;    /* dict: key -> value */
    PyObject *nodes;  /* dict: key -> PyLong(ptr) */
    LRUNode  *head;   /* MRU */
    LRUNode  *tail;   /* LRU */
    Py_ssize_t size;
    Py_ssize_t capacity;
} LRU;

static inline LRUNode* lru_node_new(PyObject *key) {
    LRUNode *n = (LRUNode*)PyMem_Malloc(sizeof(LRUNode));
    if (!n) return NULL;
    Py_INCREF(key);
    n->key = key;
    n->prev = n->next = NULL;
    return n;
}

static inline void lru_node_free(LRUNode *n) {
    if (!n) return;
    Py_DECREF(n->key);
    PyMem_Free(n);
}

static inline int lru_init(LRU *lru, Py_ssize_t capacity) {
    lru->map   = PyDict_New();
    lru->nodes = PyDict_New();
    lru->head = lru->tail = NULL;
    lru->size = 0;
    lru->capacity = capacity;
    if (!lru->map || !lru->nodes) return -1;
    return 0;
}

static inline void lru_unlink(LRU *lru, LRUNode *n) {
    (void)lru;
    if (!n) return;
    if (n->prev) n->prev->next = n->next;
    if (n->next) n->next->prev = n->prev;
}

static inline void lru_push_front(LRU *lru, LRUNode *n) {
    n->prev = NULL;
    n->next = lru->head;
    if (lru->head) lru->head->prev = n;
    lru->head = n;
    if (!lru->tail) lru->tail = n;
}

static inline int lru_move_to_front(LRU *lru, PyObject *key) {
    PyObject *ptr_obj = PyDict_GetItemWithError(lru->nodes, key);
    if (!ptr_obj) {
        if (PyErr_Occurred()) return -1;
        return 0;
    }
    LRUNode *n = (LRUNode*)PyLong_AsVoidPtr(ptr_obj);
    if (!n) return -1;
    if (lru->head != n) {
        if (lru->tail == n) lru->tail = n->prev;
        lru_unlink(lru, n);
        lru_push_front(lru, n);
    }
    return 1;
}

static inline int lru_put(LRU *lru, PyObject *key, PyObject *value) {
    PyObject *existing = PyDict_GetItemWithError(lru->map, key);
    if (existing) {
        if (PyDict_SetItem(lru->map, key, value) < 0) return -1;
        return lru_move_to_front(lru, key) < 0 ? -1 : 0;
    }
    if (PyErr_Occurred()) return -1;

    LRUNode *n = lru_node_new(key);
    if (!n) return -1;

    if (PyDict_SetItem(lru->map, key, value) < 0) {
        lru_node_free(n);
        return -1;
    }
    PyObject *ptr_obj = PyLong_FromVoidPtr(n);
    if (!ptr_obj) {
        PyDict_DelItem(lru->map, key);
        lru_node_free(n);
        return -1;
    }
    if (PyDict_SetItem(lru->nodes, key, ptr_obj) < 0) {
        Py_DECREF(ptr_obj);
        PyDict_DelItem(lru->map, key);
        lru_node_free(n);
        return -1;
    }
    Py_DECREF(ptr_obj);

    lru_push_front(lru, n);
    lru->size++;

    if (lru->size > lru->capacity) {
        LRUNode *victim = lru->tail;
        if (victim->prev) victim->prev->next = NULL;
        lru->tail = victim->prev;
        if (lru->head == victim) lru->head = NULL;

        PyDict_DelItem(lru->map, victim->key);
        PyDict_DelItem(lru->nodes, victim->key);
        lru_node_free(victim);
        lru->size--;
    }
    return 0;
}

static inline PyObject* lru_get(LRU *lru, PyObject *key) {
    PyObject *value = PyDict_GetItemWithError(lru->map, key);
    if (!value) {
        if (PyErr_Occurred()) return NULL;
        Py_RETURN_NONE;
    }
    if (lru_move_to_front(lru, key) < 0) return NULL;
    Py_INCREF(value);
    return value;
}

static inline void lru_clear(LRU *lru) {
    LRUNode *cur = lru->head;
    while (cur) {
        LRUNode *next = cur->next;
        lru_node_free(cur);
        cur = next;
    }
    lru->head = lru->tail = NULL;
    lru->size = 0;
    if (lru->map) PyDict_Clear(lru->map);
    if (lru->nodes) PyDict_Clear(lru->nodes);
}

static inline void lru_free(LRU *lru) {
    lru_clear(lru);
    Py_XDECREF(lru->map);
    Py_XDECREF(lru->nodes);
    lru->map = lru->nodes = NULL;
}

/* ===========================================================
 *                 Hybrid Codecaching State
 * =========================================================== */

typedef struct {
    int using_lru_code;
    int using_lru_memo;
    PyObject *code_dict;  /* (expr, mode) -> code object */
    PyObject *memo_dict;  /* (expr, frozenset(locals.items()), mode) -> result */
    LRU code_lru;
    LRU memo_lru;

    Py_ssize_t code_dict_threshold;
    Py_ssize_t memo_dict_threshold;
    Py_ssize_t code_lru_capacity;
    Py_ssize_t memo_lru_capacity;

    int sandbox_default;
    int lock_enabled;
    PyThread_type_lock lock;
} CodecachingState;

/* Defaults */
#define CODE_DICT_THRESHOLD_DEFAULT  64
#define MEMO_DICT_THRESHOLD_DEFAULT  1024
#define CODE_LRU_CAPACITY_DEFAULT    256
#define MEMO_LRU_CAPACITY_DEFAULT    8192

#define CC_LOCK(st)   do { if ((st)->lock_enabled && (st)->lock) PyThread_acquire_lock((st)->lock, 1); } while(0)
#define CC_UNLOCK(st) do { if ((st)->lock_enabled && (st)->lock) PyThread_release_lock((st)->lock); } while(0)

/* ===========================================================
 *                       Utilities
 * =========================================================== */

static inline PyObject *
cc_build_memo_key(PyObject *expr_str, PyObject *locals, int mode) {
    PyObject *items = PyDict_Items(locals);
    if (!items) return NULL;
    PyObject *frozen = PyFrozenSet_New(items);
    Py_DECREF(items);
    if (!frozen) return NULL;
    PyObject *mode_obj = PyLong_FromLong(mode);
    if (!mode_obj) { Py_DECREF(frozen); return NULL; }
    PyObject *tup = PyTuple_Pack(3, expr_str, frozen, mode_obj);
    Py_DECREF(frozen);
    Py_DECREF(mode_obj);
    return tup;
}

static inline int
cc_migrate_dict_to_lru(PyObject *dict, LRU *lru) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    while (PyDict_Next(dict, &pos, &key, &value)) {
        if (lru_put(lru, key, value) < 0) return -1;
    }
    return 0;
}

/* Hybrid: code cache, keyed by (expr_str, mode) */
static inline PyObject *
cc_get_or_compile_code(CodecachingState *st, PyObject *expr_str, int mode) {
    PyObject *mode_obj = PyLong_FromLong(mode);
    if (!mode_obj) return NULL;
    PyObject *key = PyTuple_Pack(2, expr_str, mode_obj);
    Py_DECREF(mode_obj);
    if (!key) return NULL;

    PyObject *code = NULL;

    if (!st->using_lru_code) {
        code = PyDict_GetItemWithError(st->code_dict, key);
        if (!code && PyErr_Occurred()) { Py_DECREF(key); return NULL; }
        if (code) { Py_INCREF(code); Py_DECREF(key); return code; }

        const char *expr = PyUnicode_AsUTF8(expr_str);
        if (!expr) { Py_DECREF(key); return NULL; }
        PyObject *compiled = Py_CompileString(expr, "<expr>",
                                mode == 0 ? Py_eval_input : Py_file_input);
        if (!compiled) { Py_DECREF(key); return NULL; }

        if (PyDict_SetItem(st->code_dict, key, compiled) < 0) {
            Py_DECREF(compiled); Py_DECREF(key); return NULL;
        }

        if (PyDict_Size(st->code_dict) > st->code_dict_threshold) {
            if (lru_init(&st->code_lru, st->code_lru_capacity) < 0) {
                Py_DECREF(key); return NULL;
            }
            if (cc_migrate_dict_to_lru(st->code_dict, &st->code_lru) < 0) {
                Py_DECREF(key); return NULL;
            }
            st->using_lru_code = 1;
            PyDict_Clear(st->code_dict);
        }

        Py_DECREF(key);
        return compiled;
    } else {
        code = lru_get(&st->code_lru, key);
        if (!code) { Py_DECREF(key); return NULL; }
        if (code != Py_None) { Py_DECREF(key); return code; }
        Py_DECREF(code);

        const char *expr = PyUnicode_AsUTF8(expr_str);
        if (!expr) { Py_DECREF(key); return NULL; }
        PyObject *compiled = Py_CompileString(expr, "<expr>",
                                mode == 0 ? Py_eval_input : Py_file_input);
        if (!compiled) { Py_DECREF(key); return NULL; }
        if (lru_put(&st->code_lru, key, compiled) < 0) {
            Py_DECREF(compiled); Py_DECREF(key); return NULL;
        }
        Py_INCREF(compiled);
        Py_DECREF(key);
        return compiled;
    }
}

/* ===========================================================
 *                      Public API
 * =========================================================== */

static inline int
codecaching_init(CodecachingState *st) {
    st->using_lru_code = 0;
    st->using_lru_memo = 0;
    st->code_dict_threshold = CODE_DICT_THRESHOLD_DEFAULT;
    st->memo_dict_threshold = MEMO_DICT_THRESHOLD_DEFAULT;
    st->code_lru_capacity   = CODE_LRU_CAPACITY_DEFAULT;
    st->memo_lru_capacity   = MEMO_LRU_CAPACITY_DEFAULT;
    st->sandbox_default = 0;
    st->lock_enabled = 1;

    st->code_dict = PyDict_New();
    st->memo_dict = PyDict_New();
    if (!st->code_dict || !st->memo_dict) return -1;

    if (lru_init(&st->code_lru, st->code_lru_capacity) < 0) return -1;
    if (lru_init(&st->memo_lru, st->memo_lru_capacity) < 0) return -1;

    st->lock = PyThread_allocate_lock();
    return 0;
}

static inline void
codecaching_free(CodecachingState *st) {
    if (!st) return;
    lru_free(&st->code_lru);
    lru_free(&st->memo_lru);
    Py_XDECREF(st->code_dict);
    Py_XDECREF(st->memo_dict);
    st->code_dict = st->memo_dict = NULL;
    if (st->lock) { PyThread_free_lock(st->lock); st->lock = NULL; }
}

static inline void
codecaching_clean_cache(CodecachingState *st) {
    CC_LOCK(st);
    if (st->code_dict) PyDict_Clear(st->code_dict);
    if (st->memo_dict) PyDict_Clear(st->memo_dict);
    if (st->using_lru_code) { lru_clear(&st->code_lru); st->using_lru_code = 0; }
    if (st->using_lru_memo) { lru_clear(&st->memo_lru); st->using_lru_memo = 0; }
    CC_UNLOCK(st);
}

static inline void
codecaching_set_limits(CodecachingState *st,
                       Py_ssize_t code_dict_threshold,
                       Py_ssize_t memo_dict_threshold,
                       Py_ssize_t code_lru_capacity,
                       Py_ssize_t memo_lru_capacity) {
    CC_LOCK(st);
    if (code_dict_threshold > 0) st->code_dict_threshold = code_dict_threshold;
    if (memo_dict_threshold > 0) st->memo_dict_threshold = memo_dict_threshold;
    if (code_lru_capacity > 0)   st->code_lru_capacity   = code_lru_capacity;
    if (memo_lru_capacity > 0)   st->memo_lru_capacity   = memo_lru_capacity;
    CC_UNLOCK(st);
}

static inline void
codecaching_set_sandbox_default(CodecachingState *st, int flag) {
    CC_LOCK(st);
    st->sandbox_default = (flag != 0);
    CC_UNLOCK(st);
}

static inline void
codecaching_set_thread_safety(CodecachingState *st, int flag) {
    CC_LOCK(st);
    st->lock_enabled = (flag != 0);
    CC_UNLOCK(st);
}

/* Main API
   - expr_str: Unicode str with code.
   - locals:   dict of bindings (must be hashable for memo key).
   - sandbox:  -1 => use default; 0 => current globals; 1 => fresh globals with __builtins__.
   - mode:     0 => Py_eval_input (expression); 1 => Py_file_input (statements; returns locals["_result"] or None).
*/
static inline PyObject *
codecaching_run_cached(CodecachingState *st,
                       PyObject *expr_str,
                       PyObject *locals,
                       int sandbox,
                       int mode) {
    if (!PyUnicode_Check(expr_str) || !PyDict_Check(locals)) {
        PyErr_SetString(PyExc_TypeError, "codecaching_run_cached: expected (str, dict, int, int)");
        return NULL;
    }
    int use_sandbox = (sandbox < 0) ? st->sandbox_default : !!sandbox;

    PyObject *memo_key = cc_build_memo_key(expr_str, locals, mode);
    if (!memo_key) return NULL;

    PyObject *result = NULL;
    CC_LOCK(st);

    /* -------- Memo lookup -------- */
    if (!st->using_lru_memo) {
        PyObject *cached = PyDict_GetItemWithError(st->memo_dict, memo_key);
        if (!cached && PyErr_Occurred()) { CC_UNLOCK(st); Py_DECREF(memo_key); return NULL; }
        if (cached) { Py_INCREF(cached); result = cached; goto done; }
    } else {
        PyObject *cached = lru_get(&st->memo_lru, memo_key);
        if (!cached) { CC_UNLOCK(st); Py_DECREF(memo_key); return NULL; }
        if (cached != Py_None) { result = cached; goto done; }
        Py_DECREF(cached);
    }

    /* -------- Compile / get code -------- */
    PyObject *code = cc_get_or_compile_code(st, expr_str, mode);
    if (!code) { CC_UNLOCK(st); Py_DECREF(memo_key); return NULL; }

    CC_UNLOCK(st);

    /* -------- Execute -------- */
    PyObject *globals = NULL;
    if (use_sandbox) {
        globals = PyDict_New();
        if (!globals) { Py_DECREF(code); Py_DECREF(memo_key); return NULL; }
        PyObject *builtins = PyEval_GetBuiltins();
        if (builtins) PyDict_SetItemString(globals, "__builtins__", builtins);
    } else {
        globals = PyEval_GetGlobals();
        Py_XINCREF(globals);
        if (!globals) {
            globals = PyDict_New();
            if (!globals) { Py_DECREF(code); Py_DECREF(memo_key); return NULL; }
            PyObject *builtins = PyEval_GetBuiltins();
            if (builtins) PyDict_SetItemString(globals, "__builtins__", builtins);
        }
    }

    PyObject *val = PyEval_EvalCode(code, globals, locals);
    Py_DECREF(code);
    Py_DECREF(globals);

    if (val && mode == 1) {  /* Py_file_input: pull _result */
        Py_DECREF(val);
        val = PyDict_GetItemString(locals, "_result");
        if (!val) { Py_INCREF(Py_None); val = Py_None; }
        else Py_INCREF(val);
    }

    CC_LOCK(st);

    if (!val) { CC_UNLOCK(st); Py_DECREF(memo_key); return NULL; }

    if (!st->using_lru_memo) {
        if (PyDict_SetItem(st->memo_dict, memo_key, val) < 0) {
            Py_DECREF(val); CC_UNLOCK(st); Py_DECREF(memo_key); return NULL;
        }
        if (PyDict_Size(st->memo_dict) > st->memo_dict_threshold) {
            if (lru_init(&st->memo_lru, st->memo_lru_capacity) < 0) {
                Py_DECREF(val); CC_UNLOCK(st); Py_DECREF(memo_key); return NULL;
            }
            if (cc_migrate_dict_to_lru(st->memo_dict, &st->memo_lru) < 0) {
                Py_DECREF(val); CC_UNLOCK(st); Py_DECREF(memo_key); return NULL;
            }
            st->using_lru_memo = 1;
            PyDict_Clear(st->memo_dict);
        }
    } else {
        if (lru_put(&st->memo_lru, memo_key, val) < 0) {
            Py_DECREF(val); CC_UNLOCK(st); Py_DECREF(memo_key); return NULL;
        }
    }

    result = val;

done:
    CC_UNLOCK(st);
    Py_DECREF(memo_key);
    return result;
}

#endif /* CODECACHING_H */

