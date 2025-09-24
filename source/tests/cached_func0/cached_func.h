// cached_func.h

void init_cache();
unsigned int simple_hash(const char *str);
PyObject* find_cached_code(const char *source);
void cache_code_object(const char *source, PyObject *code_obj);
PyObject* compile_with_cache(const char *source, const char *filename, int mode);
PyObject* execute_code_object(PyObject *code_obj, PyObject *globals, PyObject *locals);
