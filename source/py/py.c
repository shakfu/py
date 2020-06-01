// py.c

/* py external api */
#include "py.h"

/* max/msp api */
#include "api.h"
#include "globex.h"

/*--------------------------------------------------------------------------*/
// GLOBALS

t_class* py_class; // global pointer to object class

static int py_global_obj_count = 0; // when 0 free interpreter

static t_hashtab* py_global_registry; // global object lookups

// static wchar_t* program;

/*--------------------------------------------------------------------------*/
// main

void ext_main(void* r)
{
    t_class* c;

    c = class_new("py", (method)py_new, (method)py_free, (long)sizeof(t_py),
                  0L, A_GIMME, 0);

    // clang-format off
    //------------------------------------------------------------------------
    // methods

    // testing
    class_addmethod(c, (method)py_bang,       "bang",       0);

    // core python
    class_addmethod(c, (method)py_import,     "import",     A_SYM,    0);
    class_addmethod(c, (method)py_eval,       "eval",       A_GIMME,  0);
    class_addmethod(c, (method)py_exec,       "exec",       A_GIMME,  0);
    class_addmethod(c, (method)py_execfile,   "execfile",   A_SYM,    0);

    class_addmethod(c, (method)py_globex,     "globex",     A_LONG, 0);

    // extra python
    // class_addmethod(c, (method)py_call,       "call",       A_GIMME,  0);
    class_addmethod(c, (method)py_assign,     "assign",     A_GIMME,  0);
    class_addmethod(c, (method)py_anything,   "anything",   A_GIMME,  0);
    
    /* you CAN'T call this from the patcher */
    class_addmethod(c, (method)py_assist,     "assist",     A_CANT, 0);

    // meta
    class_addmethod(c, (method)py_count,      "count",      A_NOTHING, 0);
    class_addmethod(c, (method)py_scan,       "scan",       A_NOTHING, 0);
    class_addmethod(c, (method)py_send,       "send",       A_GIMME,   0);

    // code editor
    class_addmethod(c, (method)py_read,       "read",       A_DEFSYM, 0);
    class_addmethod(c, (method)py_dblclick,   "dblclick",   A_CANT,   0);
    class_addmethod(c, (method)py_edclose,    "edclose",    A_CANT,   0);
    class_addmethod(c, (method)py_edsave,     "edsave",     A_CANT,   0);
    class_addmethod(c, (method)py_load,       "load",       A_SYM,    0);

    // attributes
    // CLASS_ATTR_ORDER(c,     "name", 0,  "1");
    CLASS_ATTR_LABEL(c,     "name", 0,  "unique object id");
    CLASS_ATTR_SYM(c,       "name", 0,   t_py, p_name);
    CLASS_ATTR_BASIC(c,     "name", 0);
    // CLASS_ATTR_INVISIBLE(c, "name", 0);

    // CLASS_ATTR_ORDER(c,      "file", 0,  "2");
    CLASS_ATTR_LABEL(c,      "file", 0,  "default python script");
    CLASS_ATTR_SYM(c,        "file", 0,   t_py,  p_code_filepath);
    CLASS_ATTR_STYLE(c,      "file", 0,   "file");
    CLASS_ATTR_BASIC(c,      "file", 0);
    CLASS_ATTR_SAVE(c,       "file", 0);

    // CLASS_ATTR_ORDER(c,      "pythonpath", 0,  "3");
    CLASS_ATTR_LABEL(c,      "pythonpath", 0,  "per-object pythonpath");
    CLASS_ATTR_SYM(c,        "pythonpath", 0,  t_py, p_pythonpath);
    CLASS_ATTR_STYLE(c,      "pythonpath", 0,      "file");
    CLASS_ATTR_BASIC(c,      "pythonpath", 0);
    CLASS_ATTR_SAVE(c,       "pythonpath", 0);

    // CLASS_ATTR_ORDER(c,      "debug", 0,  "4");
    CLASS_ATTR_LABEL(c,      "debug", 0,  "debug log to console");
    CLASS_ATTR_CHAR(c,       "debug", 0,  t_py, p_debug);
    CLASS_ATTR_STYLE(c,      "debug", 0, "onoff");
    CLASS_ATTR_BASIC(c,      "debug", 0);
    CLASS_ATTR_SAVE(c,       "debug", 0);
    // CLASS_ATTR_DEFAULT(c, "debug", 0, "1");
    
    //------------------------------------------------------------------------
    // clang-format on

    class_register(CLASS_BOX, c);

    /* for js registration (can't be both box and nobox) */
    // c->c_flags = CLASS_FLAG_POLYGLOT;
    // class_register(CLASS_NOBOX, c);

    py_class = c;

    // post("py class loaded...", 0);
}

/*--------------------------------------------------------------------------*/
// general helper functions

// NOTE: try to not call it at all objects are freed!
void py_log(t_py* x, char* fmt, ...)
{
    if (x->p_debug) {
        char msg[50];

        va_list va;
        va_start(va, fmt);
        vsprintf(msg, fmt, va);
        va_end(va);

        post("[py '%s']: %s", x->p_name->s_name, msg);
    }
}

void py_error(t_py* x, char* fmt, ...)
{
    char msg[50];

    va_list va;
    va_start(va, fmt);
    vsprintf(msg, fmt, va);
    va_end(va);

    error("[py '%s']: %s", x->p_name->s_name, msg);
}

void py_locatefile(t_py* x, char* filename)
{
    // works for folders as well
    char name[MAX_FILENAME_CHARS];
    short path;
    t_fourcc type;

    char pathname[MAX_PATH_CHARS];
    short err;

    if (filename == NULL)
        return;

    strncpy_zero(name, filename, MAX_FILENAME_CHARS);

    if (locatefile_extended(name, &path, &type, NULL, 0)) {
        error("path %s not found", name);
    } else {
        post("path %s, path %d", name, path);
        err = path_topathname(path, name, pathname);
        if (err == 0) {
            post("absolute path: %s", pathname);
        }
    }
}

/*--------------------------------------------------------------------------*/
// object creation, initialization and release

void* py_new(t_symbol* s, long argc, t_atom* argv)
{
    t_py* x = NULL;

    x = (t_py*)object_alloc(py_class);

    if (x) {
        // core
        if (py_global_obj_count == 0) {
            x->p_name = gensym("__main__"); // first is __main__
        } else {
            x->p_name = symbol_unique();
        }

        // communication
        x->p_patcher = NULL;
        // x->p_box = NULL;
        // t_hashtab* registry; // experimental (perhaps better as a global)

        // python-related
        x->p_pythonpath = gensym("");
        x->p_debug = 1;

        // text editor
        x->p_code = sysmem_newhandle(0);
        x->p_code_size = 0;
        x->p_code_editor = NULL;
        x->p_code_filepath = gensym("");

        // create inlet(s)
        // create outlet(s)
        x->p_outlet_right = outlet_new(x, NULL);
        x->p_outlet_middle = outlet_new(x, NULL);
        x->p_outlet_left = outlet_new(x, NULL);

        // process @arg attributes
        attr_args_process(x, argc, argv);

        object_obex_lookup(x, gensym("#P"), (t_object**)&x->p_patcher);
        // object_obex_lookup(x, gensym("#P"), (t_patcher**)&x->p_patcher);
        // if (err != MAX_ERR_NONE)
        //     return;

        if (x->p_patcher == NULL)
            error("patcher object not created.");

        // object_obex_lookup(x, gensym("#P"), (t_patcher**)&x->p_patcher);
        // if (x->p_patcher == NULL)
        //     error("patcher object not created.");

        // object_obex_lookup(x, gensym("#B"), (t_box**)&x->p_box);
        // if (x->p_box == NULL)
        //     error("patcher object not created.");

        // python init
        py_init(x);

        py_log(x, "object created");
        for (int i = 0; i < argc; i++) {
            py_log(x, "%d: %s", i, atom_getsym(argv + i)->s_name);
            post("argc: %d  argv: %s", i, atom_getsym(argv + i)->s_name);
        }
    }

    return (x);
}

void py_init(t_py* x)
{
    // wchar_t *program;

    // program = Py_DecodeLocale(argv[0], NULL);
    // program = Py_DecodeLocale("py", NULL);
    // if (program == NULL) {
    //     exit(1);
    // }

    /* Add the cythonized 'api' built-in module, before Py_Initialize */
    if (PyImport_AppendInittab("api", PyInit_api) == -1) {
        py_error(x, "could not add 'api' to builtin modules table");
    }

    if (PyImport_AppendInittab("globex", PyInit_globex) == -1) {
        py_error(x, "could not add 'globex' to builtin modules table");
    }

    // Py_SetProgramName(program);

    Py_Initialize();

    // python init
    PyObject* main_mod = PyImport_AddModule(x->p_name->s_name); // borrowed
    x->p_globals = PyModule_GetDict(main_mod); // borrowed reference
    PyDict_SetItemString(x->p_globals, "__builtins__", PyEval_GetBuiltins());

    /* start: add additional python objects to the globals dict here */
    /* end */

    // py_log(x, "globals initialized");
    object_register(CLASS_BOX, x->p_name, x);
    // py_log(x, "object registered");

    // increment global object counter
    py_global_obj_count++;

    if (py_global_obj_count == 1) {
        // if first py object create the py_global_registry;
        py_global_registry = (t_hashtab*)hashtab_new(0);
        hashtab_flags(py_global_registry, OBJ_FLAG_REF);
    }
}

void py_free(t_py* x)
{
    // code editor cleanup
    object_free(x->p_code_editor);
    if (x->p_code)
        sysmem_freehandle(x->p_code);

    // python objects cleanup
    py_log(x, "will be deleted");
    py_global_obj_count--;
    if (py_global_obj_count == 0) {
        /* WARNING: don't call x here or max will crash */
        hashtab_chuck(py_global_registry);

        post("last py obj freed -> finalizing py mem / interpreter.");
        // PyMem_RawFree(program);
        Py_FinalizeEx();
    }
}

/*--------------------------------------------------------------------------*/
// help

void py_assist(t_py* x, void* b, long m, long a, char* s)
{
    if (m == ASSIST_INLET) { // inlet
        sprintf(s, "I am inlet %ld", a);
    } else { // outlet
        sprintf(s, "I am outlet %ld", a);
    }
}

/*--------------------------------------------------------------------------*/
// object methods

void py_bang(t_py* x) { outlet_bang(x->p_outlet_left); }

void py_scan(t_py* x)
{
    long result = 0;

    hashtab_clear(py_global_registry);

    object_method(x->p_patcher, gensym("iterate"), (method)scan_callback, x,
                  PI_DEEP | PI_WANTBOX, &result);
}

long scan_callback(t_py* x, t_object* box)
{
    t_rect jr;
    t_object* p;
    t_symbol* s;
    t_symbol* varname;
    t_object* obj;
    t_symbol* obj_id;

    jbox_get_patching_rect(box, &jr);
    p = jbox_get_patcher(box);
    varname = jbox_get_varname(box);
    obj = jbox_get_object(box);

    // lifted from scheme-for-max (Thanks, Iain!)
    if (varname != gensym("")) {
        py_log(x, "storing object '%s' in the global registry",
               varname->s_name);
        hashtab_store(py_global_registry, varname, obj);
    }

    obj_id = jbox_get_id(box);
    s = jpatcher_get_name(p);
    object_post(
        (t_object*)x,
        "in patcher:%s, varname:%s id:%s box @ x %ld y %ld, w %ld, h %ld",
        s->s_name, varname->s_name, obj_id->s_name, (long)jr.x, (long)jr.y,
        (long)jr.width, (long)jr.height);
    return 0;
}

void py_send2(t_py* x, t_symbol* s, long argc, t_atom* argv)
{
    // see:
    // https://cycling74.com/forums/error-handling-with-object_method_typed
    t_object* obj = NULL;
    char* obj_name = NULL;
    t_symbol* msg_sym = NULL;
    t_max_err err = NULL;

    // argv+0 is the object name
    obj_name = atom_getsym(argv)->s_name;
    if (obj_name == NULL) {
        goto error;
    }

    // lifted from scheme-for-max (Thanks, Iain!)
    err = hashtab_lookup(py_global_registry, gensym(obj_name), &obj);
    if (err || obj == NULL) {
        py_error(x, "no object found in the registry with the name %s",
                 obj_name);
        goto error;
    }

    // atom after the name of the destination
    switch ((argv + 1)->a_type) {
    case A_SYM: {
        msg_sym = atom_getsym(argv + 1);
        if (msg_sym == NULL) { // should check type here
            goto error;
        }
        // address the minimum case: e.g a bang
        if (argc - 2 == 0) { //
            argc = 0;
            argv = NULL;
        } else {
            argc = argc - 2;
            argv = argv + 2;
        }
        break;
    }
    case A_FLOAT: {
        msg_sym = gensym("float");
        if (msg_sym == NULL) { // should check type here
            goto error;
        }

        argc = argc - 1;
        argv = argv + 1;

        break;
    }
    case A_LONG: {
        msg_sym = gensym("int");
        if (msg_sym == NULL) { // should check type here
            goto error;
        }

        argc = argc - 1;
        argv = argv + 1;

        break;
    }
    default:
        py_log(x, "cannot process unknown type");
        break;
    }

    err = object_method_typed(obj, msg_sym, argc, argv, NULL);
    if (err) {
        py_error(x, "failed to send a message to object %s", obj_name);
        goto error;
    }

    // success
    return;

error:
    return;
}

void py_send(t_py* x, t_symbol* s, long argc, t_atom* argv)
{
    // see:
    // https://cycling74.com/forums/error-handling-with-object_method_typed
    t_object* obj = NULL;
    char* obj_name = NULL;
    t_symbol* msg_sym = NULL;
    t_max_err err = NULL;

    // argv+0 is the object name
    obj_name = atom_getsym(argv)->s_name;
    if (obj_name == NULL) {
        goto error;
    }

    // lifted from scheme-for-max (Thanks, Iain!)
    err = hashtab_lookup(py_global_registry, gensym(obj_name), &obj);
    if (err || obj == NULL) {
        py_error(x, "no object found in the registry with the name %s",
                 obj_name);
        goto error;
    }

    // atom after the name of the destination
    switch ((argv + 1)->a_type) {
    case A_SYM: {
        msg_sym = atom_getsym(argv + 1);
        if (msg_sym == NULL) { // should check type here
            goto error;
        }
        // address the minimum case: e.g a bang
        if (argc - 2 == 0) { //
            argc = 0;
            argv = NULL;
        } else {
            argc = argc - 2;
            argv = argv + 2;
        }
        break;
    }
    case A_FLOAT: {
        msg_sym = gensym("float");
        if (msg_sym == NULL) { // should check type here
            goto error;
        }

        argc = argc - 1;
        argv = argv + 1;

        break;
    }
    case A_LONG: {
        msg_sym = gensym("int");
        if (msg_sym == NULL) { // should check type here
            goto error;
        }

        argc = argc - 1;
        argv = argv + 1;

        break;
    }
    default:
        py_log(x, "cannot process unknown type");
        break;
    }

    /*
    clang-format off
    see: https://github.com/Cycling74/min-api/blob/55c65a02a7d4133ac261908f5d47e1be2b7ef1fb/include/c74_min_patcher.h#L101

    template<typename T1, typename T2>
            atom operator()(symbol method_name, T1 arg1, T2 arg2) {
                auto m { find_method(method_name) };

                if (m.type == max::A_GIMME) {
                    atoms   as { arg1, arg2 };
                    return max::object_method_typed(m.ob, method_name,
    as.size(), &as[0], nullptr);
                }
                else if (m.type == max::A_GIMMEBACK) {
                    atoms       as { arg1, arg2 };
                    max::t_atom rv {};

                    max::object_method_typed(m.ob, method_name, as.size(),
    &as[0], &rv); return rv;
                }
                else {
                    if (typeid(T1) != typeid(atom))
                        return m.fn(m.ob, arg1, arg2);
                    else {
                        // atoms must be converted to native types and then
    reinterpreted as void*
                        // doubles cannot be converted -- supporting those will
    need to be handled separately return m.fn(m.ob, atom_to_generic(arg1),
    atom_to_generic(arg2));
                    }
                }
            }
    clang-format on
    */
    // method m = object_getmethod(obj, msg_sym);
    // if (m.type == A_GIMME) {
    //     post("NICE");
    // } else {
    //     post("NOT NICE");
    // }

    // 1
    // t_messlist* mlist1 = object_mess(obj, msg_sym);

    // // 2
    // t_messlist* mlist2 = obj->o_messlist;

    // 3
    t_method_object* mobj = object_getmethod_object(obj, msg_sym);

    t_messlist m_entry = mobj->messlist_entry;

    post("MESSLIST_ENTRY method_name %s type %s", m_entry.m_sym->s_name,
         m_entry.m_type);

    err = object_method_typed(obj, msg_sym, argc, argv, NULL);
    if (err) {
        py_error(x, "failed to send a message to object %s", obj_name);
        goto error;
    }

    // success
    return;

error:
    return;
}

// retrieves a count of the number of 'active' py objects from a global var
void py_count(t_py* x) { outlet_int(x->p_outlet_left, py_global_obj_count); }

/*--------------------------------------------------------------------------*/
// python helper functions

/* python error handler helper function */
void handle_py_error(t_py* x, char* fmt, ...)
{
    if (PyErr_Occurred()) {

        // build custom msg
        char msg[50];

        va_list va;
        va_start(va, fmt);
        vsprintf(msg, fmt, va);
        va_end(va);

        // get error info
        PyObject *ptype, *pvalue, *ptraceback;
        PyErr_Fetch(&ptype, &pvalue, &ptraceback);
        PyErr_NormalizeException(&ptype, &pvalue, &ptraceback);

        // PyObject* ptype_pstr = PyObject_Repr(ptype);
        // const char* ptype_str = PyUnicode_AsUTF8(ptype_pstr);
        Py_XDECREF(ptype);
        // Py_XDECREF(ptype_pstr);

        PyObject* pvalue_pstr = PyObject_Repr(pvalue);
        const char* pvalue_str = PyUnicode_AsUTF8(pvalue_pstr);
        Py_XDECREF(pvalue);
        Py_XDECREF(pvalue_pstr);

        Py_XDECREF(ptraceback);

        error("[py '%s'] <- (%s): %s", x->p_name->s_name, msg, pvalue_str);
    }
}

/*--------------------------------------------------------------------------*/
// code editor

void py_dblclick(t_py* x)
{
    if (x->p_code_editor)
        object_attr_setchar(x->p_code_editor, gensym("visible"), 1);
    else {
        x->p_code_editor = object_new(CLASS_NOBOX, gensym("jed"), x, 0);
        object_method(x->p_code_editor, gensym("settext"), *x->p_code,
                      gensym("utf-8"));
        object_attr_setchar(x->p_code_editor, gensym("scratch"), 1);
        object_attr_setsym(x->p_code_editor, gensym("title"),
                           gensym("py-editor"));
    }
}

void py_read(t_py* x, t_symbol* s)
{
    defer((t_object*)x, (method)py_doread, s, 0, NULL);
}

void py_doread(t_py* x, t_symbol* s, long argc, t_atom* argv)
{
    char filename[MAX_PATH_CHARS];
    short path;
    t_fourcc type = FOUR_CHAR_CODE('TEXT');
    long err;
    t_filehandle fh;

    if (s == gensym("")) {
        if (x->p_code_filepath != gensym("")) {
            strcpy(filename, x->p_code_filepath->s_name);
        } else {
            filename[0] = 0;
            if (open_dialog(filename, &path, &type, &type, 1))
                x->p_code_filepath = gensym(filename);
            return;
        }
    } else {
        strcpy(filename, s->s_name);
        x->p_code_filepath = s;
    }

    if (locatefile_extended(filename, &path, &type, &type, 1)) {
        py_error(x, "can't find file %s", filename);
        return;
    }

    // success
    err = path_opensysfile(filename, path, &fh, READ_PERM);
    if (!err) {
        sysfile_readtextfile(fh, x->p_code, 0,
                             TEXT_LB_UNIX | TEXT_NULL_TERMINATE);
        sysfile_close(fh);
        x->p_code_size = sysmem_handlesize(x->p_code);
    }
}

void py_edclose(t_py* x, char** text, long size)
{
    if (x->p_code)
        sysmem_freehandle(x->p_code);

    x->p_code = sysmem_newhandleclear(size + 1);
    sysmem_copyptr((char*)*text, *x->p_code, size);
    x->p_code_size = size + 1;
    x->p_code_editor = NULL;
}

void py_edsave(t_py* x, char** text, long size)
{
    // py_log(x, "saving editor code to %s", x->p_code_filepath->s_name);

    PyObject* pval = NULL;

    if (text == NULL) {
        goto error;
    }

    pval = PyRun_String(*text, Py_file_input, x->p_globals, x->p_globals);
    if (pval == NULL) {
        goto error;
    }

    // success cleanup
    Py_DECREF(pval);
    return;

error:
    handle_py_error(x, "edclose-exec %s", x->p_code_filepath->s_name);
    Py_XDECREF(pval);
}

void py_load(t_py* x, t_symbol* s)
{
    // py_log(x, "load: %s", s->s_name);
    py_read(x, s);
    py_execfile(x, s);
}

/*--------------------------------------------------------------------------*/
// core python methods

void py_import(t_py* x, t_symbol* s)
{
    PyObject* x_module = NULL;

    if (s != gensym("")) {
        x_module = PyImport_ImportModule(s->s_name); // x_module borrrowed ref
        if (x_module == NULL) {
            goto error;
        }
        PyDict_SetItemString(x->p_globals, s->s_name, x_module);
        outlet_bang(x->p_outlet_right);
        py_log(x, "imported: %s", s->s_name);
    }
    return;
error:
    handle_py_error(x, "import %s", s->s_name);
    outlet_bang(x->p_outlet_middle);
}

void py_globex(t_py* x, long n)
{
    PyObject* globex_mod = NULL;

    globex_mod = PyImport_ImportModule("globex"); // x_module borrrowed ref
    if (globex_mod == NULL) {
        goto error;
    }

    PyDict_SetItemString(x->p_globals, "globex", globex_mod);

    PyObject* globex_dict = PyModule_GetDict(globex_mod);

    if (PyDict_SetItemString(globex_dict, NAME_INT, PyLong_FromLong(n))
        == -1) {
        py_error(x, "cannot set long to NAME_INT");
        goto error;
    }

    outlet_bang(x->p_outlet_right);
    py_log(x, "globex import and globex.INT = %ld", n);
    return;
error:
    handle_py_error(x, "globex %ld", n);
    outlet_bang(x->p_outlet_middle);
}

void py_eval(t_py* x, t_symbol* s, long argc, t_atom* argv)
{
    char* py_argv = atom_getsym(argv)->s_name;
    py_log(x, "%s %s", s->s_name, py_argv);

    PyObject* locals = PyDict_New();
    PyObject* pval = PyRun_String(py_argv, Py_eval_input, x->p_globals,
                                  locals);

    if (pval != NULL) {

        // handle ints and longs
        if (PyLong_Check(pval)) {
            long int_result = PyLong_AsLong(pval);
            outlet_int(x->p_outlet_left, int_result);
            outlet_bang(x->p_outlet_right);
        }

        // handle floats and doubles
        if (PyFloat_Check(pval)) {
            float float_result = (float)PyFloat_AsDouble(pval);
            outlet_float(x->p_outlet_left, float_result);
            outlet_bang(x->p_outlet_right);
        }

        // handle strings
        if (PyUnicode_Check(pval)) {
            const char* unicode_result = PyUnicode_AsUTF8(pval);
            outlet_anything(x->p_outlet_left, gensym(unicode_result), 0, NIL);
            outlet_bang(x->p_outlet_right);
        }

        // handle any sequence except strings, and presently
        // bytes and byte arrays (until there is a reason to)
        if (PySequence_Check(pval) && !PyUnicode_Check(pval)
            && !PyBytes_Check(pval) && !PyByteArray_Check(pval)) {
            PyObject* iter;
            PyObject* item;
            int i = 0;

            t_atom atoms_static[PY_MAX_ATOMS];
            t_atom* atoms;
            int is_dynamic = 0;

            Py_ssize_t seq_size = PySequence_Length(pval);

            if (seq_size > PY_MAX_ATOMS) {
                py_log(x, "dynamically increasing size of atom array");
                atoms = atom_dynamic_start(atoms_static, PY_MAX_ATOMS,
                                           seq_size + 1);
                is_dynamic = 1;

            } else {
                atoms = atoms_static;
            }

            if ((iter = PyObject_GetIter(pval)) != NULL) {
                while ((item = PyIter_Next(iter)) != NULL) {
                    if (PyLong_Check(item)) {
                        long long_item = PyLong_AsLong(item);
                        atom_setlong(atoms + i, long_item);
                        py_log(x, "%d long: %ld\n", i, long_item);
                        i++;
                    }

                    if PyFloat_Check (item) {
                        float float_item = PyFloat_AsDouble(item);
                        atom_setfloat(atoms + i, float_item);
                        py_log(x, "%d float: %f\n", i, float_item);
                        i++;
                    }

                    if PyUnicode_Check (item) {
                        const char* unicode_item = PyUnicode_AsUTF8(item);
                        py_log(x, "%d unicode: %s\n", i, unicode_item);
                        atom_setsym(atoms + i, gensym(unicode_item));
                        i++;
                    }
                    Py_DECREF(item);
                }
                outlet_anything(x->p_outlet_left, gensym("list"), i, atoms);
                outlet_bang(x->p_outlet_right);
                py_log(x, "end iter op: %d", i);
            }

            if (is_dynamic) {
                py_log(x, "restoring to static atom array");
                atom_dynamic_end(atoms_static, atoms);
            }
        }

        // cleanup
        Py_XDECREF(pval);
    }

    else {
        handle_py_error(x, "eval %s", py_argv);
        // cleanup
        Py_XDECREF(pval);
    }
}

void py_exec(t_py* x, t_symbol* s, long argc, t_atom* argv)
{
    char* py_argv = NULL;
    PyObject* pval = NULL;

    py_argv = atom_getsym(argv)->s_name;
    if (py_argv == NULL) {
        goto error;
    }

    pval = PyRun_String(py_argv, Py_single_input, x->p_globals, x->p_globals);
    if (pval == NULL) {
        goto error;
    }
    outlet_bang(x->p_outlet_right);

    // success cleanup
    Py_DECREF(pval);
    py_log(x, "exec %s", py_argv);
    return;

error:
    handle_py_error(x, "exec %s", py_argv);
    Py_XDECREF(pval);
    outlet_bang(x->p_outlet_middle);
}

void py_execfile(t_py* x, t_symbol* s)
{
    PyObject* pval = NULL;
    FILE* fhandle = NULL;

    if (s == gensym("")) {
        py_error(x, "py execfile: missing filepath");
        goto error;
    }

    fhandle = fopen(s->s_name, "r");
    if (fhandle == NULL) {
        py_error(x, "could not open file '%s'", s->s_name);
        goto error;
    }

    pval = PyRun_File(fhandle, s->s_name, Py_file_input, x->p_globals,
                      x->p_globals);
    if (pval == NULL) {
        goto error;
    }
    outlet_bang(x->p_outlet_right);

    // success cleanup
    fclose(fhandle);
    Py_DECREF(pval);
    py_log(x, "execfile %s", s->s_name);
    return;

error:
    handle_py_error(x, "execfile %s", s->s_name);
    Py_XDECREF(pval);
    outlet_bang(x->p_outlet_middle);
}

/*--------------------------------------------------------------------------*/
// extra python methods

void py_assign(t_py* x, t_symbol* s, long argc, t_atom* argv)
{
    char* varname = NULL;
    PyObject* list = NULL;

    if (s != gensym(""))
        py_log(x, "s: %s", s->s_name);

    // first atom in argv must be a symbol
    if (argv->a_type != A_SYM) {
        py_error(x, "first atom must be a symbol!");
        goto error;

    } else {
        // strncpy_zero(varname, atom_getsym(argv)->s_name, 50);
        varname = atom_getsym(argv)->s_name;
        py_log(x, "varname: %s", varname);
    }

    if ((list = PyList_New(0)) == NULL) {
        py_error(x, "list == NULL");
        goto error;
    }

    // NOTE: n C it’s illegal to have a declaration as the first statement
    // after a label enclosing the whole subblock in a {} seems to work
    for (int i = 1; i < argc; i++) {
        switch ((argv + i)->a_type) {
        case A_FLOAT: {
            double c_float = atom_getfloat(argv + i);
            PyObject* p_float = PyFloat_FromDouble(c_float);
            if (p_float == NULL) {
                error("p_float == NULL");
                goto error;
            }
            PyList_Append(list, p_float);
            Py_DECREF(p_float);
            py_log(x, "%d: %f", i, atom_getfloat(argv + i));
            break;
        }
        case A_LONG: {
            PyObject* p_long = PyLong_FromLong(atom_getlong(argv + i));
            if (p_long == NULL) {
                py_error(x, "p_long == NULL");
                goto error;
            }
            PyList_Append(list, p_long);
            Py_DECREF(p_long);
            py_log(x, "%d: %ld", i, atom_getlong(argv + i));
            break;
        }
        case A_SYM: {
            PyObject* p_str = PyUnicode_FromString(
                atom_getsym(argv + i)->s_name);
            if (p_str == NULL) {
                py_error(x, "p_str == NULL");
                goto error;
            }
            PyList_Append(list, p_str);
            Py_DECREF(p_str);
            py_log(x, "%d: %s", i, atom_getsym(argv + i)->s_name);
            break;
        }
        default:
            py_log(x, "cannot process unknown type");
            break;
        }
    }

    if (PyList_Size(list) != argc - 1) {
        py_error(x, "PyList_Size(list) != argc - 1");
        goto error;
    } else {
        py_log(x, "length of list: %d", PyList_Size(list));
    }

    // finally, assign list to varname in object namespace
    py_log(x, "setting %s to list in namespace", varname);
    int res = PyDict_SetItemString(x->p_globals, varname, list);
    if (res != 0) {
        py_error(x, "assign varname to list failed");
        goto error;
    }
    // Py_XDECREF(list); // causes a crash
    outlet_bang(x->p_outlet_right);
    return;

error:
    handle_py_error(x, "assign %s", s->s_name);
    Py_XDECREF(list);
    outlet_bang(x->p_outlet_middle);
}

void py_anything(t_py* x, t_symbol* s, long argc, t_atom* argv)
{
    char* py_argv = NULL;
    PyObject* pval = NULL;
    // PyObject* py_callable_str = NULL;
    PyObject* py_callable = NULL;
    PyObject* py_argslist = NULL; // python list
    PyObject* py_args = NULL;     // python tuple

    if (s == gensym("")) {
        py_error(x, "could not retrieve callable name", s->s_name);
        goto error;
    }

    py_callable = PyRun_String(s->s_name, Py_eval_input, x->p_globals,
                               x->p_globals);
    if (py_callable == NULL) {
        py_error(x, "could not evaluate '%s' as a python callable", s->s_name);
        goto error;
    }

    if ((py_argslist = PyList_New(0)) == NULL) {
        py_error(x, "could not create an empyt python list");
        goto error;
    }

    for (int i = 0; i < argc; i++) {
        switch ((argv + i)->a_type) {
        case A_FLOAT: {
            double c_float = atom_getfloat(argv + i);
            PyObject* p_float = PyFloat_FromDouble(c_float);
            if (p_float == NULL) {
                py_error(x, "p_float == NULL");
                goto error;
            }
            PyList_Append(py_argslist, p_float);
            Py_DECREF(p_float);
            py_log(x, "%d: %f", i, atom_getfloat(argv + i));
            break;
        }
        case A_LONG: {
            PyObject* p_long = PyLong_FromLong(atom_getlong(argv + i));
            if (p_long == NULL) {
                py_error(x, "p_long == NULL");
                goto error;
            }
            PyList_Append(py_argslist, p_long);
            Py_DECREF(p_long);
            py_log(x, "%d: %ld", i, atom_getlong(argv + i));
            break;
        }
        case A_SYM: {
            PyObject* p_str = PyUnicode_FromString(
                atom_getsym(argv + i)->s_name);
            if (p_str == NULL) {
                py_error(x, "p_str == NULL");
                goto error;
            }
            PyList_Append(py_argslist, p_str);
            Py_DECREF(p_str);
            py_log(x, "%d: %s", i, atom_getsym(argv + i)->s_name);
            break;
        }
        default:
            py_log(x, "cannot process unknown type");
            break;
        }
    }

    if (PyList_Size(py_argslist) != argc) {
        py_error(x, "PyList_Size(list) != argc");
        goto error;
    } else {
        py_log(x, "length of list: %d", PyList_Size(py_argslist));
    }

    // convert py_args to tuple
    py_args = PyList_AsTuple(py_argslist);
    if (py_args == NULL) {
        py_error(x, "unable to convert args list to tuple");
        goto error;
    }

    pval = PyObject_Call(py_callable, py_args, NULL);
    if (pval == NULL) {
        py_error(x, "could not retrieve result of call");
        goto error;
    }

    // handle ints and longs
    if (PyLong_Check(pval)) {
        long int_result = PyLong_AsLong(pval);
        outlet_int(x->p_outlet_left, int_result);
    }

    // handle floats and doubles
    if (PyFloat_Check(pval)) {
        float float_result = (float)PyFloat_AsDouble(pval);
        outlet_float(x->p_outlet_left, float_result);
    }

    // handle strings
    if (PyUnicode_Check(pval)) {
        const char* unicode_result = PyUnicode_AsUTF8(pval);
        outlet_anything(x->p_outlet_left, gensym(unicode_result), 0, NIL);
    }

    // handle lists, tuples and sets
    if (PyList_Check(pval) || PyTuple_Check(pval) || PyAnySet_Check(pval)) {
        PyObject* iter = NULL;
        PyObject* item = NULL;
        int i = 0;

        t_atom atoms_static[PY_MAX_ATOMS];
        t_atom* atoms = NULL;
        int is_dynamic = 0;

        Py_ssize_t seq_size = PySequence_Length(pval);
        if (seq_size <= 0) {
            py_error(x,
                     "cannot convert python sequence with zero or less length "
                     "to atoms");
            goto error;
        }

        if ((iter = PyObject_GetIter(pval)) == NULL) {
            goto error;
        }

        if (seq_size > PY_MAX_ATOMS) {
            py_log(x, "dynamically increasing size of atom array");
            atoms = atom_dynamic_start(atoms_static, PY_MAX_ATOMS,
                                       seq_size + 1);
            is_dynamic = 1;

        } else {
            atoms = atoms_static;
        }

        while ((item = PyIter_Next(iter)) != NULL) {
            if (PyLong_Check(item)) {
                long long_item = PyLong_AsLong(item);
                atom_setlong(atoms + i, long_item);
                py_log(x, "%d long: %ld\n", i, long_item);
                i++;
            }

            if PyFloat_Check (item) {
                float float_item = PyFloat_AsDouble(item);
                atom_setfloat(atoms + i, float_item);
                py_log(x, "%d float: %f\n", i, float_item);
                i++;
            }

            if PyUnicode_Check (item) {
                const char* unicode_item = PyUnicode_AsUTF8(item);
                py_log(x, "%d unicode: %s\n", i, unicode_item);
                atom_setsym(atoms + i, gensym(unicode_item));
                i++;
            }
            Py_DECREF(item);
        }
        outlet_list(x->p_outlet_left, NULL, i, atoms);
        py_log(x, "end iter op: %d", i);

        if (is_dynamic) {
            py_log(x, "restoring to static atom array");
            atom_dynamic_end(atoms_static, atoms);
        }
    }

    // success cleanup
    Py_XDECREF(py_callable);
    Py_XDECREF(py_argslist);
    Py_XDECREF(pval);
    py_log(x, "END %s: %s", s->s_name, py_argv);
    outlet_bang(x->p_outlet_right);
    return;

error:
    handle_py_error(x, "anything %s", s->s_name);
    // cleanup
    Py_XDECREF(py_callable);
    Py_XDECREF(py_argslist);
    Py_XDECREF(pval);
    outlet_bang(x->p_outlet_middle);
}
