/*
    Copyright 2001-2005 - Cycling '74
    Joshua Kit Clayton jkc@cycling74.com
*/

#include "jit.common.h"

typedef struct _max_jit_fill
{
    t_object ob;
    void* obex;
    void* bangout;
    t_symbol* matrix_name;
    long plane;
    long offsetcount;
    long offset[JIT_MATRIX_MAX_DIMCOUNT];

} t_max_jit_fill;

void* max_jit_fill_new(t_symbol* s, long argc, t_atom* argv);
void max_jit_fill_free(t_max_jit_fill* x);
void max_jit_fill_assist(t_max_jit_fill* x, void* b, long m, long a, char* s);
void max_jit_fill_int(t_max_jit_fill* x, long val);
void max_jit_fill_float(t_max_jit_fill* x, double val);
void max_jit_fill_list(t_max_jit_fill* x, t_symbol* s, long argc, t_atom* argv);
void max_jit_set_data(t_max_jit_fill* x, t_symbol* s, long argc, t_atom* argv);
void max_jit_get_cell(t_max_jit_fill* x, t_symbol* s, long argc, t_atom* argv);

t_class* max_jit_fill_class = NULL;


C74_EXPORT void ext_main(void* r)
{
    long attrflags;
    void *attr;

    t_class* c = NULL;

    c = class_new("jit.fill2",
        (method)max_jit_fill_new,
        (method)max_jit_fill_free,
        (long)sizeof(t_max_jit_fill), 0L, A_GIMME, 0);

    max_jit_class_obex_setup(c, calcoffset(t_max_jit_fill, obex));

    class_addmethod(c, (method)max_jit_fill_int,    "int",   A_LONG, 0);
    class_addmethod(c, (method)max_jit_fill_float,  "float", A_FLOAT, 0);
    class_addmethod(c, (method)max_jit_set_data,    "setdata", A_GIMME, 0);
    class_addmethod(c, (method)max_jit_get_cell,     "get_cell", A_GIMME, 0);

    max_jit_class_addmethod_defer_low(c, (method)max_jit_fill_list, "list");

    attrflags = JIT_ATTR_GET_DEFER_LOW | JIT_ATTR_SET_DEFER_LOW;

    attr = jit_object_new(_jit_sym_jit_attr_offset, "matrix_name", _jit_sym_symbol, attrflags,
                          (method)0L, (method)0L, calcoffset(t_max_jit_fill, matrix_name));
    object_addattr_parse(attr, "label", _jit_sym_symbol, 0, "\"Matrix Name\"");
    max_jit_class_addattr(c, attr);
    attr = jit_object_new(_jit_sym_jit_attr_offset, "plane", _jit_sym_long, attrflags,
                          (method)0L, (method)0L, calcoffset(t_max_jit_fill, plane));
    object_addattr_parse(attr, "label", _jit_sym_symbol, 0, "Plane");
    max_jit_class_addattr(c, attr);
    attr = jit_object_new(_jit_sym_jit_attr_offset_array, "offset", _jit_sym_long, JIT_MATRIX_MAX_DIMCOUNT, attrflags,
                          (method)0L, (method)0L, calcoffset(t_max_jit_fill, offsetcount), calcoffset(t_max_jit_fill, offset));
    object_addattr_parse(attr, "label", _jit_sym_symbol, 0, "Offset");
    max_jit_class_addattr(c, attr);

    max_jit_class_wrap_standard(c, NULL, 0);
    class_addmethod(c, (method)max_jit_fill_assist, "assist", A_CANT, 0);

    class_register(CLASS_BOX, c);

    max_jit_fill_class = c;
}

void max_jit_fill_int(t_max_jit_fill* x, long val)
{
    t_atom a;

    jit_atom_setlong(&a, val);
    typedmess((t_object*)x, _jit_sym_list, 1, &a);
}

void max_jit_fill_float(t_max_jit_fill* x, double val)
{
    t_atom a;

    jit_atom_setfloat(&a, val);
    typedmess((t_object*)x, _jit_sym_list, 1, &a);
}

void max_jit_get_cell(t_max_jit_fill* x, t_symbol* s, long argc, t_atom* argv)
{
    void* matrix;
    long savelock;
    long i;
    t_atom rv;
    t_jit_matrix_info minfo;
    t_object *rvarr;

    if (argc && argv) {
        matrix = jit_object_findregistered(x->matrix_name);
        if (matrix && jit_object_method(matrix, _jit_sym_class_jit_matrix)) {
            savelock = (long)jit_object_method(matrix, _jit_sym_lock, 1);
            jit_object_method(matrix, _jit_sym_getinfo, &minfo);
            if (argc != minfo.dimcount) {
                jit_error_sym(x, _jit_sym_err_calculate);
                jit_object_method(matrix, _jit_sym_lock, savelock);
                goto out;                
            }
            // post("inside");
            jit_object_method_typed(matrix, gensym("getcell"), argc, argv, &rv);
            if (rvarr = jit_atom_getobj(&rv)) {
                long rvac = 0;
                t_atom * rvav = NULL;
                object_getvalueof(rvarr, &rvac, &rvav);
                if (rvac && rvav) {
                    post("length of response: %d", rvac);
                    post("minfo.planecount: %d", minfo.planecount);

                    if (minfo.type == _jit_sym_char) {
                        uchar char_value = 0;
                        for (i = 0; i < rvac; i++) {
                            char_value = jit_atom_getcharfix(rvav + i);
                            post("char value: %d", (long)char_value);
                        }
                    }
                    else if (minfo.type == _jit_sym_long) {
                        t_atom_long long_value = 0;
                        for (i = 0; i < rvac; i++) {
                            long_value = atom_getlong(rvav + i);
                            post("long value: %d", long_value);
                        }
                    }
                    else if (minfo.type == _jit_sym_float32) {
                        float float_value = 0.0;
                        for (i = 0; i < rvac; i++) {
                            float_value = atom_getfloat(rvav + i);
                            post("float value: %f", float_value);
                        }
                    }
                    if (minfo.type == _jit_sym_float64) {
                        double double_value = 0.0;
                        for (i = 0; i < rvac; i++) {
                            double_value = atom_getfloat(rvav + i);
                            post("double value: %f", double_value);
                        }
                    }
                }
            }
            // post("after");
        }
        jit_object_method(matrix, _jit_sym_lock, savelock);
    }
out:
    outlet_bang(x->bangout);
    return;
}

void max_jit_fill_list(t_max_jit_fill* x, t_symbol* s, long argc, t_atom* argv)
{
    
    void* matrix;
    long err, i, j;
    long savelock, offset0, offset1;
    t_jit_matrix_info minfo;
    char *bp, *p;

    if (argc && argv) {
        // find matrix
        matrix = jit_object_findregistered(x->matrix_name);
        if (matrix && jit_object_method(matrix, _jit_sym_class_jit_matrix)) {
            savelock = (long)jit_object_method(matrix, _jit_sym_lock, 1);
            jit_object_method(matrix, _jit_sym_getinfo, &minfo);
            jit_object_method(matrix, _jit_sym_getdata, &bp);

            if ((!bp) || (x->plane >= minfo.planecount) || (x->plane < 0)) {
                jit_error_sym(x, _jit_sym_err_calculate);
                jit_object_method(matrix, _jit_sym_lock, savelock);
                goto out;
            }

            // limited to filling at most into 2 dimensions per list
            offset0 = (x->offsetcount > 0) ? x->offset[0] : 0;
            offset1 = (x->offsetcount > 1) ? x->offset[1] : 0;
            CLIP_ASSIGN(offset0, 0, minfo.dim[0] - 1);
            CLIP_ASSIGN(offset1, 0, minfo.dim[1] - 1);
            CLIP_ASSIGN(argc, 0, (minfo.dim[0] * (minfo.dim[1] - offset1)) - offset0);
            j = offset0 + offset1 * minfo.dim[0];

            if (minfo.type == _jit_sym_char) {
                bp += x->plane;
                for (i = 0; i < argc; i++, j++) {
                    p = bp + (j / minfo.dim[0]) * minfo.dimstride[1] + (j % minfo.dim[0]) * minfo.dimstride[0];

                    *((uchar*)p) = jit_atom_getcharfix(argv + i);
                }
            }
            else if (minfo.type == _jit_sym_long) {
                bp += x->plane * 4;
                for (i = 0; i < argc; i++, j++) {
                    p = bp + (j / minfo.dim[0]) * minfo.dimstride[1] + (j % minfo.dim[0]) * minfo.dimstride[0];
                    *((t_int32*)p) = (t_int32)jit_atom_getlong(argv + i);
                }
            }
            else if (minfo.type == _jit_sym_float32) {
                bp += x->plane * 4;
                for (i = 0; i < argc; i++, j++) {
                    p = bp + (j / minfo.dim[0]) * minfo.dimstride[1] + (j % minfo.dim[0]) * minfo.dimstride[0];
                    *((float*)p) = jit_atom_getfloat(argv + i);
                }
            }
            if (minfo.type == _jit_sym_float64) {
                bp += x->plane * 8;
                for (i = 0; i < argc; i++, j++) {
                    p = bp + (j / minfo.dim[0]) * minfo.dimstride[1] + (j % minfo.dim[0]) * minfo.dimstride[0];
                    *((double*)p) = jit_atom_getfloat(argv + i);
                }
            }

            jit_object_method(matrix, _jit_sym_lock, savelock);
        }
        else {
            jit_error_sym(x, _jit_sym_err_calculate);
        }
    }
out:
    outlet_bang(x->bangout);
    return;
}
/* old, incorrect way
void max_jit_fill_offset_bp(t_max_jit_fill *x, char **bp, t_jit_matrix_info *minfo)
{
        long i,dimcount,offset;

        dimcount = MIN(x->offsetcount,minfo->dimcount);
        for (i=0;i<dimcount;i++)
        {
                offset = x->offset[i];
                CLIP_ASSIGN(offset,0,minfo->dim[i]-1);
                *bp += offset*minfo->dimstride[i];
        }
}
*/
void max_jit_fill_assist(t_max_jit_fill* x, void* b, long m, long a, char* s)
{
    if (m == 1) {
        sprintf(s, "list");
    }
    else {
        if (a == 1) {
            sprintf(s, "dumpout");
        }
        else {
            sprintf(s, "bang when done");
        }
    }
}

void max_jit_fill_free(t_max_jit_fill* x)
{
    // only max object, no jit object
    max_jit_object_free(x);
}

void* max_jit_fill_new(t_symbol* s, long argc, t_atom* argv)
{
    t_max_jit_fill* x;
    long attrstart;

    x = (t_max_jit_fill*)max_jit_object_alloc(max_jit_fill_class, NULL);

    if (x) { // only max object, no jit object
        max_jit_obex_dumpout_set(x, outlet_new(x, 0L)); // general purpose outlet(rightmost)
        x->bangout = outlet_new(x, 0L); // after done, outputs bang

        x->matrix_name = _jit_sym_nothing;
        x->plane = 0;
        x->offsetcount = 0;

        attrstart = max_jit_attr_args_offset(argc, argv);
        if (attrstart && argv) {
            t_atom_long al;
            jit_atom_arg_getsym(&x->matrix_name, 0, attrstart, argv);
            if (!jit_atom_arg_getlong(&al, 1, attrstart, argv)) {
                C74_ASSERT_FITS_LONG(al);
                x->plane = (long)al;
            }
        }

        max_jit_attr_args(x, argc, argv); // handle attribute args
    }
    return (x);
}


// def myfill2d(self):
//     cdef int i, j, z = 0
//     cdef char* p = NULL
//     cdef char* bp = self.data

//     xs = list(range(self.height * self.width * self.planecount))

//     for i in range(self.info.dim[0]):
//         for j in range(self.info.dim[1]):
//             p = bp + (j * self.info.dimstride[1]) + (i * self.info.dimstride[0])
//             for k in range(self.planecount):
//                 p[0] = <long>xs[z]
//                 p += 1
//                 z += 1


void max_jit_set_data(t_max_jit_fill* x, t_symbol* s, long argc, t_atom* argv)
{
    void* matrix;
    long err, i, j;
    long savelock, offset0, offset1;
    t_jit_matrix_info info;
    char *bp, *p;

    if (argc && argv) {
        // find matrix
        matrix = jit_object_findregistered(x->matrix_name);
        if (matrix && jit_object_method(matrix, _jit_sym_class_jit_matrix)) {
            savelock = (long)jit_object_method(matrix, _jit_sym_lock, 1);
            jit_object_method(matrix, _jit_sym_getinfo, &info);
            jit_object_method(matrix, _jit_sym_getdata, &bp);

            if (!bp) {
                jit_object_method(matrix, _jit_sym_lock, savelock);
                goto out;
            }

            // limited to filling at most into 2 dimensions per list
            offset0 = (x->offsetcount > 0) ? x->offset[0] : 0;
            offset1 = (x->offsetcount > 1) ? x->offset[1] : 0;
            post("argc: %d", argc);
            post("offset0: %d", offset0);
            post("offset1: %d", offset1);
            CLIP_ASSIGN(offset0, 0, info.dim[0] - 1);
            CLIP_ASSIGN(offset1, 0, info.dim[1] - 1);
            CLIP_ASSIGN(argc, 0, (info.dim[0] * (info.dim[1] - offset1)) - offset0);
            post("clipped argc: %d", argc);
            post("clipped offset0: %d", offset0);
            post("clipped offset1: %d", offset1);
            j = offset0 + offset1 * info.dim[0];

            if (info.type == _jit_sym_char) {
                bp += x->plane;
                for (i = 0; i < argc; i++, j++) {
                    p = bp + (j / info.dim[0]) * info.dimstride[1] + (j % info.dim[0]) * info.dimstride[0];
                    *((uchar*)p) = jit_atom_getcharfix(argv + i);
                }
            }
            else if (info.type == _jit_sym_long) {
                bp += x->plane * 4;
                for (i = 0; i < argc; i++, j++) {
                    p = bp + (j / info.dim[0]) * info.dimstride[1] + (j % info.dim[0]) * info.dimstride[0];
                    *((t_int32*)p) = (t_int32)jit_atom_getlong(argv + i);
                }
            }
            else if (info.type == _jit_sym_float32) {
                bp += x->plane * 4;
                for (i = 0; i < argc; i++, j++) {
                    p = bp + (j / info.dim[0]) * info.dimstride[1] + (j % info.dim[0]) * info.dimstride[0];
                    *((float*)p) = jit_atom_getfloat(argv + i);
                }
            }
            if (info.type == _jit_sym_float64) {
                bp += x->plane * 8;
                for (i = 0; i < argc; i++, j++) {
                    p = bp + (j / info.dim[0]) * info.dimstride[1] + (j % info.dim[0]) * info.dimstride[0];
                    *((double*)p) = jit_atom_getfloat(argv + i);
                }
            }

            jit_object_method(matrix, _jit_sym_lock, savelock);
        }
        else {
            jit_error_sym(x, _jit_sym_err_calculate);
        }
    }
out:
    outlet_bang(x->bangout);
    return;
}
