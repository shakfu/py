#include "ext.h"
#include "ext_obex.h"

#include "luajit.h"
#include "lualib.h"
#include "lauxlib.h"

#define LUNAR_MAX_ATOMS 128
#define LUNAR_MAX_LOG_CHAR 500 // high number during development
#define LUNAR_MAX_ERR_CHAR LUNAR_MAX_LOG_CHAR


typedef struct _lunar {
    /* object header */
    t_object l_ob;

    t_symbol* l_name;           /* unique object name */
    t_bool l_debug;
} t_lunar;

/* setup methods */
void* lunar_new(t_symbol* s, long argc, t_atom* argv);
void lunar_free(t_lunar* x);
void lunar_init(t_lunar* x);

// helpers
void lunar_log(t_lunar* x, char* fmt, ...);
void lunar_error(t_lunar* x, char* fmt, ...);


/* message methods */
void lunar_run(t_lunar* x, t_symbol* s);
void lunar_test(t_lunar* x);





/* globals */
static t_class* lunar_class;

void ext_main(void* r)
{
    t_class* c;

    c = class_new("lunar", (method)lunar_new, (method)lunar_free,
                  (long)sizeof(t_lunar), 0L, A_GIMME, 0);

    // methods
    class_addmethod(c, (method)lunar_run,      "run",      A_SYM, 0);
    class_addmethod(c, (method)lunar_test,     "test",     A_NOTHING, 0);

    // attributes
    CLASS_ATTR_SYM(c,  "name",  0, t_lunar, l_name);
    CLASS_ATTR_CHAR(c, "debug", 0, t_lunar, l_debug);

    class_register(CLASS_BOX, c);
    lunar_class = c;
}

void lunar_free(t_lunar* x)
{
    ;
}


void* lunar_new(t_symbol* s, long argc, t_atom* argv)
{
    t_lunar* x = NULL;

    // object instantiation, NEW STYLE
    if ((x = (t_lunar*)object_alloc(lunar_class))) {
        // Initialize values

        x->l_name = symbol_unique();
        x->l_debug = 1;

        // process @arg attributes
        attr_args_process(x, argc, argv);

        // lua init
        lunar_init(x);
    }
    return (x);
}


void lunar_init(t_lunar* x)
{
    ;
}

void lunar_log(t_lunar* x, char* fmt, ...)
{
    if (x->l_debug) {
        char msg[LUNAR_MAX_LOG_CHAR];

        va_list va;
        va_start(va, fmt);
        vsprintf(msg, fmt, va);
        va_end(va);

        post("[lunar %s]: %s", x->l_name->s_name, msg);
    }
}


void lunar_error(t_lunar* x, char* fmt, ...)
{
    char msg[LUNAR_MAX_ERR_CHAR];

    va_list va;
    va_start(va, fmt);
    vsprintf(msg, fmt, va);
    va_end(va);

    error("[lunar %s]: %s", x->l_name->s_name, msg);
}



void lunar_run(t_lunar* x, t_symbol* s)
{
    if (s != gensym("")) {
        lunar_log(x, "run %s", s->s_name);
    }
    return;
}


// CRASH!!
void lunar_test(t_lunar* x)
{
    lunar_log(x, "hello");
    int status, result, i;
    double sum;
    lua_State *L;

    L = luaL_newstate();

    // luaL_openlibs(L);

    // status = luaL_loadfile(L, "script.lua");
    // if (status) {
    //   lunar_error(x, "Couldn't load file: %s\n", lua_tostring(L, -1));
    //   return;
    // }

    // lua_newtable(L);

    // for (i = 1; i <= 5; i++) {
    //   lua_pushnumber(L, i);
    //   lua_pushnumber(L, i*2);
    //   lua_rawset(L, -3);
    // }

    // lua_setglobal(L, "foo");

    // result = lua_pcall(L, 0, LUA_MULTRET, 0);
    // if (result) {
    //   lunar_error(x, "Failed to run script: %s\n", lua_tostring(L, -1));
    //   return;
    // }

    // sum = lua_tonumber(L, -1);

    // lunar_log(x, "Script returned: %.0f\n", sum);

    // lua_pop(L, 1);
    lua_close(L);

    return;
 }

