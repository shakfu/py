#include "ext.h"
#include "ext_obex.h"

#define CASE 1
#define SYM_MAX 128

typedef struct _demo {
    t_object ob;
    t_symbol* separator;  // what separates atoms, like a " "
    void* outlet;         // outlet
} t_demo;


// method prototypes
void* demo_new(void);
void demo_free(t_demo* x);
void demo_anything(t_demo* x, t_symbol* s, long argc, t_atom* argv);

static t_class* demo_class = NULL;


void ext_main(void* r)
{
    t_class* c = class_new("demo", (method)demo_new,
                           (method)demo_free, (long)sizeof(t_demo), 0L,
                           A_GIMME, 0);

    class_addmethod(c, (method)demo_anything,   "list",      A_GIMME, 0);
    class_addmethod(c, (method)demo_anything,   "anything",  A_GIMME, 0);

    class_register(CLASS_BOX, c);
    demo_class = c;
}


void* demo_new(void)
{
    t_demo* x = (t_demo*)object_alloc(demo_class);

    if (x) {
        x->outlet = outlet_new(x, NULL); // outlet
        x->separator = gensym(" ");
    }
    return (x);
}

void demo_free(t_demo* x) {

}

#if CASE == 1

void demo_anything(t_demo* x, t_symbol* s, long argc, t_atom* argv)
{
    if (!s) {
        return;
    }

    long textsize = 0;
    char* text = NULL;
    t_max_err err;

    if (!argc) {
        t_atom atom[1];
        atom_setsym(atom, s);
        err = atom_gettext(1, atom, &textsize, &text,
                       OBEX_UTIL_ATOM_GETTEXT_SYM_NO_QUOTE | 
                       OBEX_UTIL_ATOM_GETTEXT_NOESCAPE);
        if (err == MAX_ERR_NONE && textsize && text) {
            post("s: %s", text);
        }
        if (text) {
            sysmem_freeptr(text);
        }
        return;
    }

    err = atom_gettext(argc, argv, &textsize, &text,
                       OBEX_UTIL_ATOM_GETTEXT_SYM_NO_QUOTE | 
                       OBEX_UTIL_ATOM_GETTEXT_NOESCAPE);
    if (err == MAX_ERR_NONE && textsize && text) {
        post("<s: %s> %s", s->s_name, text);
    }
    if (text) {
        sysmem_freeptr(text);
    }
}

#else

void demo_anything(t_demo* x, t_symbol* s, long argc, t_atom* argv)
{
    if (!s) {
        return;
    }

    post("s -> %s", s->s_name);

    char tosymbol_tempspace[SYM_MAX] = "";
    char tosymbol_tempspace2[ATOM_MAX_STRLEN] = "";
    long i;

    for (i = 0; i < argc; i++, argv++) {
        switch (argv->a_type) {
            case A_LONG:
                sprintf(tosymbol_tempspace, "%ld", (long)atom_getlong(argv));
                break;
            case A_FLOAT:
                sprintf(tosymbol_tempspace, "%.4f", atom_getfloat(argv));
                break;
            case A_SYM:
                strncpy_zero(tosymbol_tempspace, atom_getsym(argv)->s_name, SYM_MAX);
                break;
            default:
                continue;
        }
        if (i) {
            // add separator
            strncat_zero(tosymbol_tempspace2, x->separator->s_name, ATOM_MAX_STRLEN);
        }

        strncat_zero(tosymbol_tempspace2, tosymbol_tempspace, ATOM_MAX_STRLEN);
    }
    post("parsed: %s", tosymbol_tempspace2);
    outlet_anything(x->outlet, gensym(tosymbol_tempspace2), 0, 0);
}

#endif


