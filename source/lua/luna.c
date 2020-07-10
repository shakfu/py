#include "ext.h"
#include "ext_obex.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

#define LUNA_MAX_ATOMS 128
#define LUNA_MAX_LOG_CHAR 500 // high number during development
#define LUNA_MAX_ERR_CHAR LUNA_MAX_LOG_CHAR


typedef struct _luna {
    /* object header */
    t_object l_ob;

    t_symbol* l_name;           /* unique object name */
    t_bool l_debug;
} t_luna;

/* setup methods */
void* luna_new(t_symbol* s, long argc, t_atom* argv);
void luna_free(t_luna* x);
void luna_init(t_luna* x);

// helpers
void luna_log(t_luna* x, char* fmt, ...);
void luna_error(t_luna* x, char* fmt, ...);


/* message methods */
void luna_run(t_luna* x, t_symbol* s);
void luna_test(t_luna* x);





/* globals */
static t_class* luna_class;

void ext_main(void* r)
{
    t_class* c;

    c = class_new("luna", (method)luna_new, (method)luna_free,
                  (long)sizeof(t_luna), 0L, A_GIMME, 0);

    // methods
    class_addmethod(c, (method)luna_run,      "run",      A_SYM, 0);
    class_addmethod(c, (method)luna_test,     "test",     A_NOTHING, 0);

    // attributes
    CLASS_ATTR_SYM(c,  "name",  0, t_luna, l_name);
    CLASS_ATTR_CHAR(c, "debug", 0, t_luna, l_debug);

    class_register(CLASS_BOX, c);
    luna_class = c;
}

void luna_free(t_luna* x)
{
    ;
}


void* luna_new(t_symbol* s, long argc, t_atom* argv)
{
    t_luna* x = NULL;

    // object instantiation, NEW STYLE
    if ((x = (t_luna*)object_alloc(luna_class))) {
        // Initialize values

        x->l_name = symbol_unique();
        x->l_debug = 1;

        // process @arg attributes
        attr_args_process(x, argc, argv);

        // lua init
        luna_init(x);
    }
    return (x);
}


void luna_init(t_luna* x)
{
    ;
}

void luna_log(t_luna* x, char* fmt, ...)
{
    if (x->l_debug) {
        char msg[LUNA_MAX_LOG_CHAR];

        va_list va;
        va_start(va, fmt);
        vsprintf(msg, fmt, va);
        va_end(va);

        post("[luna %s]: %s", x->l_name->s_name, msg);
    }
}


void luna_error(t_luna* x, char* fmt, ...)
{
    char msg[LUNA_MAX_ERR_CHAR];

    va_list va;
    va_start(va, fmt);
    vsprintf(msg, fmt, va);
    va_end(va);

    error("[luna %s]: %s", x->l_name->s_name, msg);
}



void luna_run(t_luna* x, t_symbol* s)
{
    if (s != gensym("")) {
        luna_log(x, "run %s", s->s_name);
    }
    return;
}


// CRASH!!
void luna_test(t_luna* x)
{
     luna_log(x, "hello");
     int status, result, i;
     double sum;
     lua_State *L;

     L = luaL_newstate();

      luaL_openlibs(L);

      status = luaL_loadfile(L, "script.lua");
      if (status) {
          luna_error(x, "Couldn't load file: %s\n", lua_tostring(L, -1));
          return;
      }

      lua_newtable(L);

      for (i = 1; i <= 5; i++) {
          lua_pushnumber(L, i);
          lua_pushnumber(L, i*2);
          lua_rawset(L, -3);
      }

      lua_setglobal(L, "foo");

      result = lua_pcall(L, 0, LUA_MULTRET, 0);
      if (result) {
          luna_error(x, "Failed to run script: %s\n", lua_tostring(L, -1));
          return;
      }

      sum = lua_tonumber(L, -1);

      luna_log(x, "Script returned: %.0f\n", sum);

      lua_pop(L, 1);
     lua_close(L);

     return;
 }

