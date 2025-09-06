# api_jit.pxd
"""
'x' -> the header has been fully exposed to cython

'-' -> the header is not explicitly exposed to cython and presently
       not required for the external.

'p' -> partial analyzed but not yet included in api_jit.pxd

' ' -> an empty box means it is planned

- [x] max_types.h
- [x] ext_mess.h

- [ ] jit.bin.h
- [ ] jit.byteorder.h
- [x] jit.common.h
- [x] jit.cpost.h
- [x] jit.critical.h
- [ ] jit.cubicspline.h
- [x] jit.error.h
- [x] jit.file.h
- [ ] jit.fixmath.h
- [ ] jit.functor.h
- [ ] jit.gl.cache.h
- [ ] jit.gl.chunk.h
- [ ] jit.gl.common.h
- [ ] jit.gl.context.h
- [ ] jit.gl.context.view.h
- [ ] jit.gl.draw.h
- [ ] jit.gl.drawinfo.h
- [ ] jit.gl.h
- [ ] jit.gl.ob3d.h
- [ ] jit.gl.pixelformat.h
- [ ] jit.gl.procs.h
- [ ] jit.gl.support.h
- [ ] jit.glext.h
- [ ] jit.glext_nv.h
- [ ] jit.gworld.h
- [ ] jit.half.h
- [ ] jit.linklist.h
- [ ] jit.mac.h
- [x] jit.math.h
- [x] jit.matrix.util.h
- [x] jit.max.h
- [ ] jit.namespace.h
- [x] jit.op.h
- [ ] jit.parallel.utils.h
- [ ] jit.platform.h
- [x] jit.symbols.h
- [ ] jit.vecmath.h
- [ ] jit.wglext.h
- [ ] jit.window.h
- [x] max.jit.mop.h


"""

cdef extern from "max_types.h":
    ctypedef unsigned int t_uint            # an unsigned int as defined by the architecture / platform  @ingroup misc
    ctypedef char t_int8                    # a 1-byte int  @ingroup misc
    ctypedef unsigned char t_uint8          # an unsigned 1-byte int  @ingroup misc
    ctypedef short t_int16                  # a 2-byte int  @ingroup misc
    ctypedef unsigned short t_uint16        # an unsigned 2-byte int  @ingroup misc
    ctypedef int t_int32                    # a 4-byte int  @ingroup misc
    ctypedef unsigned int t_uint32          # an unsigned 4-byte int  @ingroup misc
    ctypedef long long t_int64              # an 8-byte int  @ingroup misc
    ctypedef unsigned long long t_uint64    # an unsigned 8-byte int  @ingroup misc
    ctypedef t_uint32 t_fourcc              # an integer of suitable size to hold a four char code / identifier  @ingroup misc

    ctypedef unsigned long long t_ptr_uint  # an unsigned pointer-sized int  @ingroup misc
    ctypedef long long t_ptr_int            # a pointer-sized int  @ingroup misc
    ctypedef double t_atom_float            # the type that is an A_FLOAT in a #t_atom  @ingroup misc
    ctypedef t_ptr_uint t_getbytes_size     # like size_t but for getbytes()  @ingroup misc
    ctypedef t_ptr_int t_int                # an integer  @ingroup misc
    ctypedef t_ptr_uint t_ptr_size          # unsigned pointer-sized value for counting (like size_t)  @ingroup misc
    ctypedef t_ptr_int t_atom_long          # the type that is an A_LONG in a #t_atom  @ingroup misc
    ctypedef t_atom_long t_max_err          # an integer value suitable to be returned as an error code  @ingroup misc
    ctypedef char **t_handle                # a handle (address of a pointer)  @ingroup misc
    ctypedef char *t_ptr                    # a pointer  @ingroup misc
    ctypedef t_uint8 t_bool                 # a true/false variable  @ingroup misc
    ctypedef t_int16 t_filepath             # i.e. path/vol in file APIs identifying a folder  @ingroup misc


cdef extern from "ext_mess.h":
    ctypedef void *(*method)(void *)

    ctypedef struct t_symbol:
        # The symbol.
        char *s_name            # name: a c-string
        t_object *s_thing       # possible binding to a t_object

    ctypedef struct t_messlist:
        # A list of symbols and their corresponding methods complete with typechecking information. 
        t_symbol *m_sym         # Name of the message
        method m_fun            # Method associated with the message
        char m_type[81]         # Argument type information

    ctypedef struct t_class: pass
    ctypedef struct t_outlet: pass
    ctypedef struct t_inlet: pass

    ctypedef struct t_object:
        # The structure for the head of any object which wants to have inlets or outlets or support attributes.
        t_messlist *o_messlist  # list of messages and methods. The -1 entry of the message list of an object contains a pointer to its #t_class entry.
                                # (also used as freelist link)
        t_inlet *o_inlet        # list of inlets
        t_outlet *o_outlet      # list of outlets

    ctypedef union word:
        t_atom_long w_long
        t_atom_float w_float
        t_symbol *w_sym
        #object *w_obj

    ctypedef struct t_atom:
        # An atom is a typed datum.
        short a_type
        word a_w


cdef extern from "jit.error.h":
    ctypedef t_atom_long t_jit_err

    void jit_object_post(t_object *x, char *s, ...)
    void jit_object_error(t_object *x, char *s, ...)

    ctypedef enum t_jit_error_code:
        JIT_ERR_NONE 				= 0
        JIT_ERR_GENERIC				= 1163022162 # FOUR_CHAR('EROR')
        JIT_ERR_INVALID_OBJECT		= 1229868866 # FOUR_CHAR('INOB')
        JIT_ERR_OBJECT_BUSY			= 1329746777 # FOUR_CHAR('OBSY')
        JIT_ERR_OUT_OF_MEM			= 1330464077 # FOUR_CHAR('OMEM')
        JIT_ERR_INVALID_PTR			= 1229870672 # FOUR_CHAR('INVP')
        JIT_ERR_DUPLICATE			= 1146441804 # FOUR_CHAR('DUPL')
        JIT_ERR_OUT_OF_BOUNDS		= 1329745476 # FOUR_CHAR('OBND')
        JIT_ERR_INVALID_INPUT		= 1229870665 # FOUR_CHAR('INVI')
        JIT_ERR_INVALID_OUTPUT		= 1229870671 # FOUR_CHAR('INVO')
        JIT_ERR_MISMATCH_TYPE		= 1297306704 # FOUR_CHAR('MSTP')
        JIT_ERR_MISMATCH_PLANE		= 1297305676 # FOUR_CHAR('MSPL')
        JIT_ERR_MISMATCH_DIM		= 1297302605 # FOUR_CHAR('MSDM')
        JIT_ERR_MATRIX_UNKNOWN		= 1297634638 # FOUR_CHAR('MXUN')
        JIT_ERR_SUPPRESS_OUTPUT		= 1397772883 # FOUR_CHAR('SPRS')
        JIT_ERR_DATA_UNAVAILABLE	= 1146443340 # FOUR_CHAR('DUVL')
        JIT_ERR_HW_UNAVAILABLE		= 1213552204 # FOUR_CHAR('HUVL')


cdef extern from "jit.file.h":
    ctypedef long t_jit_fileref

    t_jit_err jit_file_fsclose(t_jit_fileref refnum)
    t_jit_err jit_file_fsread(t_jit_fileref refnum, long *count, void *bufptr)
    t_jit_err jit_file_fswrite(t_jit_fileref refnum, long *count, const void *bufptr)
    t_jit_err jit_file_seteof(t_jit_fileref refnum, long logeof)
    t_jit_err jit_file_geteof(t_jit_fileref refnum, long *logeof)
    t_jit_err jit_file_setfpos(t_jit_fileref refnum, long mode, long offset)
    t_jit_err jit_file_getfpos(t_jit_fileref refnum, long *filepos)



cdef extern from "jit.symbols.h":
    cdef t_symbol *_jit_sym_nothing
    cdef t_symbol *_jit_sym_new
    cdef t_symbol *_jit_sym_free
    cdef t_symbol *_jit_sym_classname
    cdef t_symbol *_jit_sym_getname
    cdef t_symbol *_jit_sym_getmethod
    cdef t_symbol *_jit_sym_get
    cdef t_symbol *_jit_sym_set
    cdef t_symbol *_jit_sym_register
    cdef t_symbol *_jit_sym_char
    cdef t_symbol *_jit_sym_long
    cdef t_symbol *_jit_sym_float32
    cdef t_symbol *_jit_sym_float64
    cdef t_symbol *_jit_sym_symbol
    cdef t_symbol *_jit_sym_pointer
    cdef t_symbol *_jit_sym_object
    cdef t_symbol *_jit_sym_atom
    cdef t_symbol *_jit_sym_list
    cdef t_symbol *_jit_sym_type
    cdef t_symbol *_jit_sym_dim
    cdef t_symbol *_jit_sym_planecount
    cdef t_symbol *_jit_sym_val
    cdef t_symbol *_jit_sym_plane
    cdef t_symbol *_jit_sym_cell
    cdef t_symbol *_jit_sym_jit_matrix
    cdef t_symbol *_jit_sym_class_jit_matrix
    cdef t_symbol *_jit_sym_togworld
    cdef t_symbol *_jit_sym_fromgworld
    cdef t_symbol *_jit_sym_frommatrix
    cdef t_symbol *_jit_sym_class_jit_attribute
    cdef t_symbol *_jit_sym_jit_attribute
    cdef t_symbol *_jit_sym_jit_attr_offset
    cdef t_symbol *_jit_sym_jit_attr_offset_array
    cdef t_symbol *_jit_sym_rebuilding
    cdef t_symbol *_jit_sym_modified
    cdef t_symbol *_jit_sym_lock
    cdef t_symbol *_jit_sym_setinfo
    cdef t_symbol *_jit_sym_setinfo_ex
    cdef t_symbol *_jit_sym_getinfo
    cdef t_symbol *_jit_sym_data
    cdef t_symbol *_jit_sym_getdata
    cdef t_symbol *_jit_sym_outputmatrix
    cdef t_symbol *_jit_sym_clear
    cdef t_symbol *_jit_sym_clear_custom
    cdef t_symbol *_jit_sym_err_calculate
    cdef t_symbol *_jit_sym_max_jit_classex
    cdef t_symbol *_jit_sym_setall
    cdef t_symbol *_jit_sym_chuck
    cdef t_symbol *_jit_sym_getsize
    cdef t_symbol *_jit_sym_getindex
    cdef t_symbol *_jit_sym_objptr2index
    cdef t_symbol *_jit_sym_append
    cdef t_symbol *_jit_sym_insertindex
    cdef t_symbol *_jit_sym_deleteindex
    cdef t_symbol *_jit_sym_chuckindex
    cdef t_symbol *_jit_sym_makearray
    cdef t_symbol *_jit_sym_reverse
    cdef t_symbol *_jit_sym_rotate
    cdef t_symbol *_jit_sym_shuffle
    cdef t_symbol *_jit_sym_swap
    cdef t_symbol *_jit_sym_findfirst
    cdef t_symbol *_jit_sym_findall
    cdef t_symbol *_jit_sym_methodall
    cdef t_symbol *_jit_sym_methodindex
    cdef t_symbol *_jit_sym_sort
    cdef t_symbol *_jit_sym_matrix_calc
    cdef t_symbol *_jit_sym_genframe
    cdef t_symbol *_jit_sym_filter
    cdef t_symbol *_jit_sym_jit_mop
    cdef t_symbol *_jit_sym_newcopy
    cdef t_symbol *_jit_sym_jit_linklist
    cdef t_symbol *_jit_sym_inputcount
    cdef t_symbol *_jit_sym_outputcount
    cdef t_symbol *_jit_sym_getinput
    cdef t_symbol *_jit_sym_getoutput
    cdef t_symbol *_jit_sym_getinputlist
    cdef t_symbol *_jit_sym_getoutputlist
    cdef t_symbol *_jit_sym_ioname
    cdef t_symbol *_jit_sym_matrixname
    cdef t_symbol *_jit_sym_outputmode
    cdef t_symbol *_jit_sym_matrix
    cdef t_symbol *_jit_sym_getmatrix
    cdef t_symbol *_jit_sym_typelink
    cdef t_symbol *_jit_sym_dimlink
    cdef t_symbol *_jit_sym_planelink
    cdef t_symbol *_jit_sym_restrict_type
    cdef t_symbol *_jit_sym_restrict_planecount
    cdef t_symbol *_jit_sym_restrict_dim
    cdef t_symbol *_jit_sym_special
    cdef t_symbol *_jit_sym_getspecial
    cdef t_symbol *_jit_sym_adapt
    cdef t_symbol *_jit_sym_decorator
    cdef t_symbol *_jit_sym_frommatrix_trunc
    cdef t_symbol *_jit_sym_ioproc
    cdef t_symbol *_jit_sym_getioproc
    cdef t_symbol *_jit_sym_name
    cdef t_symbol *_jit_sym_types
    cdef t_symbol *_jit_sym_minplanecount
    cdef t_symbol *_jit_sym_maxplanecount
    cdef t_symbol *_jit_sym_mindimcount
    cdef t_symbol *_jit_sym_maxdimcount
    cdef t_symbol *_jit_sym_mindim
    cdef t_symbol *_jit_sym_maxdim
    cdef t_symbol *_jit_sym_gl_points
    cdef t_symbol *_jit_sym_gl_point_sprite
    cdef t_symbol *_jit_sym_gl_lines
    cdef t_symbol *_jit_sym_gl_line_strip
    cdef t_symbol *_jit_sym_gl_line_loop
    cdef t_symbol *_jit_sym_gl_triangles
    cdef t_symbol *_jit_sym_gl_tri_strip
    cdef t_symbol *_jit_sym_gl_tri_fan
    cdef t_symbol *_jit_sym_gl_quads
    cdef t_symbol *_jit_sym_gl_quad_strip
    cdef t_symbol *_jit_sym_gl_polygon
    cdef t_symbol *_jit_sym_gl_tri_grid
    cdef t_symbol *_jit_sym_gl_quad_grid
    cdef t_symbol *_jit_sym_err_lockout_stack
    cdef t_symbol *_jit_sym_class_jit_namespace
    cdef t_symbol *_jit_sym_jit_namespace
    cdef t_symbol *_jit_sym_findsize
    cdef t_symbol *_jit_sym_attach
    cdef t_symbol *_jit_sym_detach
    cdef t_symbol *_jit_sym_add
    cdef t_symbol *_jit_sym_replace
    cdef t_symbol *_jit_sym_gettype
    cdef t_symbol *_jit_sym_ob_sym
    cdef t_symbol *_jit_sym_resolve_name
    cdef t_symbol *_jit_sym_resolve_raw
    cdef t_symbol *_jit_sym_notifyall
    cdef t_symbol *_jit_sym_block
    cdef t_symbol *_jit_sym_unblock
    cdef t_symbol *_jit_sym_position
    cdef t_symbol *_jit_sym_rotatexyz
    cdef t_symbol *_jit_sym_scale
    cdef t_symbol *_jit_sym_quat
    cdef t_symbol *_jit_sym_direction
    cdef t_symbol *_jit_sym_lookat
    cdef t_symbol *_jit_sym_anim
    cdef t_symbol *_jit_sym_bounds
    cdef t_symbol *_jit_sym_boundcalc
    cdef t_symbol *_jit_sym_calcboundscdef


cdef extern from "jit.max.h":
    ctypedef t_object    t_jit_object       # object header @ingroup jitter
    ctypedef t_class     t_max_class
    ctypedef t_object    t_max_object
    ctypedef t_messlist  t_max_messlist

    cdef int A_DEFER
    cdef int A_USURP
    cdef int A_DEFER_LOW
    cdef int A_USURP_LOW

    cdef enum:
        MAX_JIT_CLASS_FLAGS_GIMMEBACK_WRAP      = 0x00000001L  # uses standard dumpout A_DEFER_LOW method
        MAX_JIT_CLASS_FLAGS_OWN_INLETINFO       = 0x00000002L  # override stdinletinfo in class's main

    void *max_jit_object_alloc(t_class *mclass, t_symbol *jitter_classname)
    void max_jit_object_free(void *x)
    void max_jit_class_obex_setup(t_class *mclass, long oboffset)
    t_jit_err max_jit_class_addattr(t_class *mclass, void *attr)
    void max_jit_class_wrap_standard(t_class *mclass, t_class *jclass, long flags)
    void max_jit_class_wrap_addmethods(t_class *mclass, t_class *jclass)
    void max_jit_class_wrap_addmethods_flags(t_class *mclass, t_class *jclass, long flags)
    void max_jit_class_wrap_ob3d_inletinfo(t_class *mclass, t_class *jclass, long flags)
    void max_jit_class_wrap_attrlist2methods(t_class *mclass, t_class *jclass)
    void max_jit_class_addmethod_defer(t_class *mclass, method m, char *s)
    void max_jit_class_addmethod_defer_low(t_class *mclass, method m, char *s)
    void max_jit_class_addmethod_usurp(t_class *mclass, method m, char *s)
    void max_jit_class_addmethod_usurp_low(t_class *mclass, method m, char *s) 

    # instance specific wrapping
    t_jit_err max_jit_object_addattr(t_object *x, void *attr)
    void max_jit_object_wrap_standard(t_object *mob, t_object *job, long flags)
    void max_jit_object_wrap_complete(t_object *mob, t_object *job, long flags)
    void max_jit_object_wrap_addmethods(t_object *mob, t_object *job)
    void max_jit_object_wrap_addmethods_flags(t_object *mob, t_object *job, long flags)
    void max_jit_object_wrap_attrlist2methods(t_object *mob, t_object *job)
    void max_jit_object_addmethod_defer(t_object *x, method m, char *s)
    void max_jit_object_addmethod_defer_low(t_object *x, method m, char *s)
    void max_jit_object_addmethod_usurp(t_object *x, method m, char *s)
    void max_jit_object_addmethod_usurp_low(t_object *x, method m, char *s)
        
    void max_jit_object_attr_dump(void *x)
    long max_jit_attr_args_offset(short ac, t_atom *av)
    void max_jit_attr_args(void *x, short ac, t_atom *av)
    void max_jit_attr_set(void *x, t_symbol *s, short ac, t_atom *av)
        
    void *max_jit_obex_attrlist_get(void *x)
    t_jit_err max_jit_obex_attr_set(void *x, t_symbol *s, long ac, t_atom *av)
    t_jit_err max_jit_obex_attr_get(void *x, t_symbol *s, long *ac, t_atom **av)
    void max_jit_obex_attr_getdump(void *x, t_symbol *s, short argc, t_atom *argv)
    t_jit_err max_jit_obex_set(void *x, void *p)
    void *max_jit_obex_get(void *x)
    void *max_jit_obex_jitob_get(void *x)
    void max_jit_obex_jitob_set(void *x, void *jitob)
    void *max_jit_obex_usurplist_get(void *x)
    void max_jit_obex_usurplist_set(void *x, void *usurplist)
    void *max_jit_obex_proxylist_get(void *x)
    void max_jit_obex_proxylist_set(void *x, void *proxylist)
    long max_jit_obex_inletnumber_get(void *x)
    void max_jit_obex_inletnumber_set(void *x, long inletnumber)
    t_jit_err max_jit_obex_proxy_new(void *x, long c)
    t_jit_err max_jit_obex_proxy_resize(void *x, long count)
    t_jit_err max_jit_obex_proxy_deletetail(void *x)
    t_jit_err max_jit_obex_proxy_append(void *x, long c)
    void max_jit_obex_dumpout_set(void *x, void *outlet)
    void *max_jit_obex_dumpout_get(void *x)
    void max_jit_obex_dumpout(void *x, t_symbol *s, short argc, t_atom *argv)
    void *max_jit_obex_adornmentlist_get(void *x)
    void max_jit_obex_adornmentlist_set(void *x, void *adornmentlist)
    void *max_jit_obex_adornment_get(void *x, t_symbol *classname)
    t_jit_err max_jit_obex_addadornment(void *x,void *adornment)
    void max_jit_obex_gimmeback(void *x, t_symbol *s, long ac, t_atom *av)
    void max_jit_obex_gimmeback_dumpout(void *x, t_symbol *s, long ac, t_atom *av)

    t_atom_long max_jit_method_is_attr(void *x, t_symbol *s)
    t_atom_long max_jit_method_is_undocumented(void *x, t_symbol *s)
    t_atom_long max_jit_method_is_groupreference(void *x, t_symbol *s)

    long max_jit_getqueuestate()


cdef extern from "jit.common.h":

    cdef int SIZE_INT32
    cdef int SIZE_INT64
    cdef int SIZE_FLOAT32
    cdef int SIZE_FLOAT64
    cdef int SIZE_PTR

    ctypedef enum t_jit_attr_flags:
        JIT_ATTR_GET_OPAQUE         = 0x00000001    # private getter (all)          @ingroup jitter
        JIT_ATTR_SET_OPAQUE         = 0x00000002    # private setter (all)          @ingroup jitter
        JIT_ATTR_GET_OPAQUE_USER    = 0x00000100    # private getter (user)         @ingroup jitter
        JIT_ATTR_SET_OPAQUE_USER    = 0x00000200    # private setter (user)         @ingroup jitter
        JIT_ATTR_GET_DEFER          = 0x00010000    # defer getter (deprecated)     @ingroup jitter
        JIT_ATTR_GET_USURP          = 0x00020000    # usurp getter (deprecated)     @ingroup jitter
        JIT_ATTR_GET_DEFER_LOW      = 0x00040000    # defer getter                  @ingroup jitter
        JIT_ATTR_GET_USURP_LOW      = 0x00080000    # usurp getter                  @ingroup jitter
        JIT_ATTR_SET_DEFER          = 0x01000000    # defer setter (deprecated)     @ingroup jitter
        JIT_ATTR_SET_USURP          = 0x02000000    # usurp setter (deprecated)     @ingroup jitter
        JIT_ATTR_SET_DEFER_LOW      = 0x04000000    # defer setter                  @ingroup jitter
        JIT_ATTR_SET_USURP_LOW      = 0x08000000    # usurp setter                  @ingroup jitter


    ctypedef enum t_jit_matrix_info_flags:
        JIT_MATRIX_DATA_HANDLE      = 0x00000002    # data is handle
        JIT_MATRIX_DATA_REFERENCE   = 0x00000004    # data is reference to outside memory
        JIT_MATRIX_DATA_PACK_TIGHT  = 0x00000008    # data is tightly packed (doesn't use standard 16 byte alignment)
        JIT_MATRIX_DATA_FLAGS_USE   = 0x00008000    # necessary if using handle/reference data flags when creating
                                                    # jit_matrix, however, it is never stored in matrix


    cdef enum:
        JIT_MATRIX_MAX_DIMCOUNT     = 32            # maximum dimension count
        JIT_MATRIX_MAX_PLANECOUNT   = 32            # maximum plane count


    # t_matrix_conv_info flags
    ctypedef enum t_matrix_conv_info_flags:
        JIT_MATRIX_CONVERT_CLAMP    = 0x00000001    # not currently used
        JIT_MATRIX_CONVERT_INTERP   = 0x00000002    # use interpolation
        JIT_MATRIX_CONVERT_SRCDIM   = 0x00000004    # use source dimensions
        JIT_MATRIX_CONVERT_DSTDIM   = 0x00000008    # use destination dimensions


    ctypedef unsigned long   ulong
    ctypedef unsigned int    uint
    ctypedef unsigned short  ushort
    ctypedef unsigned char   uchar


    ctypedef struct t_jit_attr:
        t_jit_object    ob             # common object header
        t_symbol        *name          # attribute name
        t_symbol        *type          # attribute type (char, long, float32, float64, symbol, atom, or obj)
        long            flags          # flags for public/private get/set methods
        method          get            # override default get method
        method          set            # override default set method
        void            *filterget     # filterobject for get method
        void            *filterset     # filterobject for set method
        void            *reserved      # for future use


    ctypedef struct t_jit_matrix_info:
        # Matrix information struct. Used to get/set multiple matrix attributes at once.
        long            size           # in bytes (0xFFFFFFFF=UNKNOWN)
        t_symbol        *type          # primitifve type (char, long, float32, or float64)
        long            flags          # flags to specify data reference, handle, or tightly packed
        long            dimcount       # number of dimensions
        long            dim[JIT_MATRIX_MAX_DIMCOUNT]       # dimension sizes
        long            dimstride[JIT_MATRIX_MAX_DIMCOUNT] # stride across dimensions in bytes
        long            planecount     # number of planes

    ctypedef struct t_matrix_conv_info:
        # Matrix conversion struct. Used to copy data from one matrix to another with special characteristics.
        long    flags                                  # flags for whether or not to use interpolation, or source/destination dimensions
        long    planemap[JIT_MATRIX_MAX_PLANECOUNT]    # plane mapping
        long    srcdimstart[JIT_MATRIX_MAX_DIMCOUNT]   # source dimension start 
        long    srcdimend[JIT_MATRIX_MAX_DIMCOUNT]     # source dimension end
        long    dstdimstart[JIT_MATRIX_MAX_DIMCOUNT]   # destination dimension start    
        long    dstdimend[JIT_MATRIX_MAX_DIMCOUNT]     # destination dimension end

    t_atom_long jit_method_true(void *x)
    t_atom_long jit_method_false(void *x)

    void *jit_class_new(const char *name, method mnew, method mfree, long size, ...)
    t_jit_err jit_class_free(void *c)
    t_jit_err jit_class_register(void *c)
    t_jit_err jit_class_addmethod(void *c, method m, const char *name, ...)
    t_jit_err jit_class_addattr(void *c, t_jit_object *attr)
    t_jit_err jit_class_addadornment(void *c, t_jit_object *o)
    t_jit_err jit_class_addinterface(void *c, void *interfaceclass, long byteoffset, long flags)
    void *jit_class_adornment_get(void *c, t_symbol *classname)
    t_symbol *jit_class_nameget(void *c)
    void *jit_class_findbyname(t_symbol *classname)
    long jit_object_classname_compare(void *x, t_symbol *name)
    method jit_class_method(void *c, t_symbol *methodname)
    void *jit_class_attr_get(void *c, t_symbol *attrname)
    t_jit_err jit_class_addtypedwrapper(void *c, method m, char *name, ...)
    t_messlist *jit_class_typedwrapper_get(void *c, t_symbol *s)
    t_jit_err jit_class_method_addargsafe(void *c, char *argname, char *methodname)
    t_symbol *jit_class_method_argsafe_get(void *c, t_symbol *s)

    void *jit_object_alloc(void *c)
    void *jit_object_new(t_symbol *classname, ...)
    #ifdef C74_X64
    #define jit_object_new(...) C74_VARFUN(jit_object_new_imp, __VA_ARGS__)
    #endif
    void *jit_object_new_imp(void *classname, void *p1, void *p2, void *p3, void *p4, void *p5, void *p6, void *p7, void *p8, void *dummy)
    t_jit_err jit_object_free(void *x)
    void *jit_object_method(void *x, t_symbol *s, ...)
    #ifdef C74_X64
    #define jit_object_method(...) C74_VARFUN(jit_object_method_imp, __VA_ARGS__)
    #endif
    void *jit_object_method_imp(void *x, void *s, void *p1, void *p2, void *p3, void *p4, void *p5, void *p6, void *p7, void *p8)
    void *jit_object_method_typed(void *x, t_symbol *s, long ac, t_atom *av, t_atom *rv)
    method jit_object_getmethod(void *x, t_symbol *s)
    t_symbol *jit_object_classname(void *x)
    void * jit_object_register(void *x, t_symbol *s)
    void *jit_object_findregistered(t_symbol *s)
    t_symbol *jit_object_findregisteredbyptr(void *x)
    t_jit_err jit_object_unregister(void *x)
    void *jit_object_attach(t_symbol *s, void *x)
    t_jit_err jit_object_detach(t_symbol *s, void *x)
    t_jit_err jit_object_notify(void *x, t_symbol *s, void *data)
    void *jit_object_class(void *x)
    long jit_object_attr_usercanget(void *x,t_symbol *s)
    long jit_object_attr_usercanset(void *x,t_symbol *s)
    void *jit_object_attr_get(void *x, t_symbol *attrname)
    t_jit_err jit_object_importattrs(void *x, t_symbol *s, long argc, t_atom *argv)
    t_jit_err jit_object_exportattrs(void *x, t_symbol *s, long argc, t_atom *argv)
    t_jit_err jit_object_exportsummary(void *x, t_symbol *s, long argc, t_atom *argv)
    t_symbol *jit_object_method_argsafe_get(void *x, t_symbol *s)

    # memory functions
    void *jit_getbytes(long size)
    void jit_freebytes(void *ptr,long size)
    void **jit_handle_new(long size)
    void jit_handle_free(void **handle)
    long jit_handle_size_get(void **handle)
    t_jit_err jit_handle_size_set(void **handle, long size)
    long jit_handle_lock(void **handle, long lock)
    void jit_copy_bytes(void *dest, const void *src, long bytes)
    long jit_freemem()
    char *jit_newptr(long size)
    void jit_disposeptr(char *ptr)

    # atom functions
    t_jit_err jit_atom_setlong(t_atom *a, t_atom_long b)
    t_jit_err jit_atom_setfloat(t_atom *a, double b)
    t_jit_err jit_atom_setsym(t_atom *a, t_symbol *b)              
    t_jit_err jit_atom_setobj(t_atom *a, void *b)
    t_atom_long jit_atom_getlong(t_atom *a)
    double jit_atom_getfloat(t_atom *a)
    t_symbol *jit_atom_getsym(t_atom *a)
    void *jit_atom_getobj(t_atom *a)
    long jit_atom_getcharfix(t_atom *a)
    # the following are useful for setting the values _only_ if there is an arg
    # rather than setting it to 0 or _jit_sym_nothing
    long jit_atom_arg_getlong(t_atom_long *c, long idx, long ac, t_atom *av)
    long jit_atom_arg_getfloat(float *c, long idx, long ac, t_atom *av)
    long jit_atom_arg_getdouble(double *c, long idx, long ac, t_atom *av)
    long jit_atom_arg_getsym(t_symbol **c, long idx, long ac, t_atom *av)

    # matrix info utils
    t_jit_err jit_matrix_info_default(t_jit_matrix_info *info)
    long jit_matrix_info_typesize(t_jit_matrix_info *minfo) 

    # mop utils
    t_jit_err jit_mop_single_type(void *x, t_symbol *s)
    t_jit_err jit_mop_single_planecount(void *x, long c)
    t_jit_err jit_mop_methodall(void *x, t_symbol *s, ...)
    t_jit_err jit_mop_input_nolink(void *mop, long c)
    t_jit_err jit_mop_output_nolink(void *mop, long c)
    t_jit_err jit_mop_ioproc_copy_adapt(void *mop, void *mop_io, void *matrix)
    t_jit_err jit_mop_ioproc_copy_trunc(void *mop, void *mop_io, void *matrix)
    t_jit_err jit_mop_ioproc_copy_trunc_zero(void *mop, void *mop_io, void *matrix)
    t_symbol *jit_mop_ioproc_tosym(void *ioproc)

    # attr functions
    long max_jit_attr_args_offset(short ac, t_atom *av)
    void max_jit_attr_args(void *x, short ac, t_atom *av)
    # for easy access of simple attributes
    t_atom_long jit_attr_getlong(void *x, t_symbol *s)
    t_jit_err jit_attr_setlong(void *x, t_symbol *s, t_atom_long c)
    t_atom_float jit_attr_getfloat(void *x, t_symbol *s)
    t_jit_err jit_attr_setfloat(void *x, t_symbol *s, t_atom_float c)
    t_symbol *jit_attr_getsym(void *x, t_symbol *s)
    t_jit_err jit_attr_setsym(void *x, t_symbol *s, t_symbol *c)
    long jit_attr_getlong_array(void *x, t_symbol *s, long max, t_atom_long *vals)
    t_jit_err jit_attr_setlong_array(void *x, t_symbol *s, long count, t_atom_long *vals)
    # long jit_attr_getchar_array(void *x, t_symbol *s, long max, uchar *vals)
    # t_jit_err jit_attr_setchar_array(void *x, t_symbol *s, long count, uchar *vals)
    long jit_attr_getfloat_array(void *x, t_symbol *s, long max, float *vals)
    t_jit_err jit_attr_setfloat_array(void *x, t_symbol *s, long count, float *vals)
    long jit_attr_getdouble_array(void *x, t_symbol *s, long max, double *vals)
    t_jit_err jit_attr_setdouble_array(void *x, t_symbol *s, long count, double *vals)
    long jit_attr_getsym_array(void *x, t_symbol *s, long max, t_symbol **vals)
    t_jit_err jit_attr_setsym_array(void *x, t_symbol *s, long count, t_symbol **vals)

    # attr filters util
    t_jit_err jit_attr_addfilterset_clip(void *x, double min, double max, long usemin, long usemax)
    t_jit_err jit_attr_addfilterset_clip_scale(void *x, double scale, double min, double max, long usemin, long usemax)
    t_jit_err jit_attr_addfilterget_clip(void *x, double min, double max, long usemin, long usemax)
    t_jit_err jit_attr_addfilterget_clip_scale(void *x, double scale, double min, double max, long usemin, long usemax)
    t_jit_err jit_attr_addfilter_clip(void *x, double min, double max, long usemin, long usemax)
    t_jit_err jit_attr_addfilter_clip_scale(void *x, double scale, double min, double max, long usemin, long usemax)
    t_jit_err jit_attr_addfilterset_proc(void *x, method proc)
    t_jit_err jit_attr_addfilterget_proc(void *x, method proc)

    # attr functions for assisting in user level notifications when setting attributes from C code
    t_jit_err jit_attr_user_setvalueof(void *x, t_symbol *s, long ac, t_atom *av)
    t_jit_err jit_attr_user_touch(void *x, t_symbol *s)
    t_jit_err jit_attr_user_touch_parse(void *x, char *attrnames)
    t_jit_err jit_object_addattr(void *x, t_object *attr)
    t_jit_err jit_object_deleteattr(void *x, t_symbol *attrname)

    void jit_attr_cleanall(t_object *x)
    void jit_attr_clean(t_object *x, t_symbol *name)
            
    # more util functions
    void jit_rand_setseed(long n)
    long jit_rand()
    t_symbol *jit_symbol_unique()
    void jit_error_code(void *x, t_jit_err v) # interrupt safe
    void jit_error_sym(void *x, t_symbol *s) # interrupt safe
    void jit_post_sym(void *x, t_symbol *s)  # interrupt safe

    t_jit_err jit_err_from_max_err(t_max_err err)

    # jit_matrix - type = 0, jit_gl_texture type = 1
    t_jit_err jit_video_recreate_outlet(t_object *maxob, long type, void **curoutlet)


cdef extern from "jit.cpost.h":
    void jit_cpost(const char *format, ...)


cdef extern from "jit.critical.h":
    void jit_global_critical_enter()
    void jit_global_critical_exit()


cdef extern from "jit.math.h":
    double jit_math_cos     (double x)
    double jit_math_sin     (double x)
    double jit_math_tan     (double x)
    double jit_math_acos    (double x)
    double jit_math_asin    (double x)
    double jit_math_atan    (double x)
    double jit_math_atan2   (double y, double x)
    double jit_math_cosh    (double x)
    double jit_math_sinh    (double x)
    double jit_math_tanh    (double x)
    double jit_math_acosh   (double x)
    double jit_math_asinh   (double x)
    double jit_math_atanh   (double x)
    double jit_math_exp     (double x)
    double jit_math_expm1   (double x)
    double jit_math_exp2    (double x)
    double jit_math_log     (double x)
    double jit_math_log2    (double x)
    double jit_math_log10   (double x)
    double jit_math_hypot   (double x, double y)
    double jit_math_pow     (double x, double y)
    double jit_math_sqrt    (double x)
    double jit_math_ceil    (double x)
    double jit_math_floor   (double x)
    double jit_math_round   (double x)
    double jit_math_trunc   (double x)
    double jit_math_fmod    (double x, double y)
    double jit_math_wrap    (double x, double lo, double hi)
    double jit_math_fold    (double x, double lo, double hi)
    double jit_math_j1      (double x)

    long jit_math_is_valid(float x)
    unsigned long jit_math_roundup_poweroftwo(unsigned long x)
    long jit_math_is_poweroftwo(long x)
    long jit_math_is_finite(float x)
    long jit_math_is_nan(float x)

    float jit_math_project_to_sphere(float r, float x, float y)

    float jit_math_fast_sqrt(float x)
    float jit_math_fast_invsqrt(float x)

    float jit_math_fast_sin(float x)  # absolute error of 1.7e-04 for [0, PI/2]
    float jit_math_fast_cos(float x)  # absolute error of 1.2e-03 for [0, PI/2]
    float jit_math_fast_tan(float x)  # absolute error of 1.9e-08 for [0, PI/4]
    float jit_math_fast_asin(float x) # absolute error of 6.8e-05 for [0, 1]
    float jit_math_fast_acos(float x) # absolute error of 6.8e-05 for [0, 1]
    float jit_math_fast_atan(float x) # absolute error of 1.43-08 for [-1, 1]

    # constants (compiler will collapse these)
    double JIT_MATH_F32_PI
    double JIT_MATH_F32_TWO_PI
    double JIT_MATH_F32_HALF_PI
    double JIT_MATH_F32_INV_PI
    double JIT_MATH_F32_DEGTORAD
    double JIT_MATH_F32_RADTODEG
    double JIT_MATH_F32_EPS
    double JIT_MATH_F32_MAXVAL

    double JIT_MATH_F64_PI
    double JIT_MATH_F64_TWO_PI
    double JIT_MATH_F64_HALF_PI
    double JIT_MATH_F64_INV_PI
    double JIT_MATH_F64_DEGTORAD
    double JIT_MATH_F64_RADTODEG
    double JIT_MATH_F64_EPS
    double JIT_MATH_F64_MAXVAL

    double JIT_MATH_FIXED_PI
    double JIT_MATH_FIXED_TWO_PI
    double JIT_MATH_FIXED_HALF_PI
    double JIT_MATH_FIXED_INV_PI
    double JIT_MATH_FIXED_DEGTORAD
    double JIT_MATH_FIXED_RADTODEG
    double JIT_MATH_FIXED_EPS
    double JIT_MATH_FIXED_MAXVAL


cdef extern from "max.jit.mop.h":

    # flags for greater control
    ctypedef enum t_max_jit_mop_flag:
        MAX_JIT_MOP_FLAGS_NONE                  = 0x00000000    # mop flag @ingroup jitter
        MAX_JIT_MOP_FLAGS_OWN_ALL               = 0x0FFFFFFF    # mop flag @ingroup jitter

        MAX_JIT_MOP_FLAGS_OWN_JIT_MATRIX        = 0x00000001    # mop flag @ingroup jitter
        MAX_JIT_MOP_FLAGS_OWN_BANG              = 0x00000002    # mop flag @ingroup jitter
        MAX_JIT_MOP_FLAGS_OWN_OUTPUTMATRIX      = 0x00000004    # mop flag @ingroup jitter
        MAX_JIT_MOP_FLAGS_OWN_NAME              = 0x00000008    # mop flag @ingroup jitter
        MAX_JIT_MOP_FLAGS_OWN_TYPE              = 0x00000010    # mop flag @ingroup jitter
        MAX_JIT_MOP_FLAGS_OWN_DIM               = 0x00000020    # mop flag @ingroup jitter
        MAX_JIT_MOP_FLAGS_OWN_PLANECOUNT        = 0x00000040    # mop flag @ingroup jitter
        MAX_JIT_MOP_FLAGS_OWN_CLEAR             = 0x00000080    # mop flag @ingroup jitter
        MAX_JIT_MOP_FLAGS_OWN_NOTIFY            = 0x00000100    # mop flag @ingroup jitter
        MAX_JIT_MOP_FLAGS_OWN_ADAPT             = 0x00000200    # mop flag @ingroup jitter
        MAX_JIT_MOP_FLAGS_OWN_OUTPUTMODE        = 0x00000400    # mop flag @ingroup jitter

        MAX_JIT_MOP_FLAGS_ONLY_MATRIX_PROBE     = 0x10000000    # mop flag @ingroup jitter


    cdef enum:
        JIT_MOP_INPUT       = 1                                 # mop flag @ingroup jitter
        JIT_MOP_OUTPUT      = 2                                 # mop flag @ingroup jitter

    t_jit_err max_jit_classex_mop_wrap(void *mclass, void *jclass, long flags)     # legacy api
    t_jit_err max_jit_class_mop_wrap(t_class *mclass, t_class *jclass, long flags) # new api
    t_jit_err max_jit_classex_mop_mproc(void *mclass, void *jclass, void *mproc)   # mproc should be method(void *x, void *mop)
    t_jit_err max_jit_object_mop_wrap(t_object *mob, t_object *job, long flags)
    t_jit_err max_jit_object_mop_mproc(void *mob, void *job, void *mproc)

    t_jit_err max_jit_mop_setup(void *x)
    t_jit_err max_jit_mop_inputs(void *x)
    t_jit_err max_jit_mop_inputs_resize(void *x, long count)
    t_jit_err max_jit_mop_outputs(void *x)
    t_jit_err max_jit_mop_outputs_resize(void *x, long count)
    t_jit_err max_jit_mop_matrix_args(void *x, long argc, t_atom *argv)
    t_jit_err max_jit_mop_jit_matrix(void *x, t_symbol *s, long argc, t_atom *argv)
    t_jit_err max_jit_mop_assist(void *x, void *b, long m, long a, char *s)
    t_jit_err max_jit_mop_bang(void *x)
    t_jit_err max_jit_mop_outputmatrix(void *x)
    t_jit_err max_jit_mop_matrixout_new(void *x, long c)
    void max_jit_mop_clear(void *x)
    t_jit_err max_jit_mop_notify(void *x, t_symbol *s, t_symbol *msg)
    void max_jit_mop_free(void *x)
    t_jit_err max_jit_mop_name(void *x, void *attr, long argc, t_atom *argv)
    t_jit_err max_jit_mop_getname(void *x, void *attr, long *argc, t_atom **argv)
    t_jit_err max_jit_mop_type(void *x, void *attr, long argc, t_atom *argv)
    t_jit_err max_jit_mop_gettype(void *x, void *attr, long *argc, t_atom **argv)
    t_jit_err max_jit_mop_dim(void *x, void *attr, long argc, t_atom *argv)
    t_jit_err max_jit_mop_getdim(void *x, void *attr, long *argc, t_atom **argv)
    t_jit_err max_jit_mop_planecount(void *x, void *attr, long argc, t_atom *argv)
    t_jit_err max_jit_mop_getplanecount(void *x, void *attr, long *argc, t_atom **argv)
    t_jit_err max_jit_mop_parse_name(t_symbol *name, long *type, long *idx)
    t_jit_err max_jit_mop_restrict_info(void *x, void *p, t_jit_matrix_info *info)
    void *max_jit_mop_get_io_by_name(void *x, t_symbol *s)
    t_jit_err max_jit_mop_outputmode(void *x, void *attr, long argc, t_atom *argv)
    t_jit_err max_jit_mop_getoutputmode_attr(void *x, void *attr, long *argc, t_atom **argv)
    t_jit_err max_jit_mop_adapt(void *x, void *attr, long argc, t_atom *argv)
    t_jit_err max_jit_mop_getadapt(void *x, void *attr, long *argc, t_atom **argv)
    void *max_jit_mop_getinput(void *x, long c)
    void *max_jit_mop_getoutput(void *x, long c)
    long max_jit_mop_getoutputmode(void *x)
    void *max_jit_mop_io_getoutlet(void *mop_io)
    t_jit_err max_jit_mop_io_setoutlet(void *mop_io, void *o)
    void *max_jit_mop_getmproc(void *mop)
    t_jit_err max_jit_mop_adapt_matrix_all(void *x, void *y)
    t_jit_err max_jit_mop_variable_parse_sym(t_symbol *name, t_symbol **msg, long *set)
    t_jit_err max_jit_mop_variable_anything(void *x, t_symbol *s, long argc, t_atom *argv)
    t_jit_err max_jit_mop_variable_addinputs(void *x, long c)
    t_jit_err max_jit_mop_variable_addoutputs(void *x, long c)

    t_jit_err max_jit_mop_setup_simple(void *x, void *o, long argc, t_atom *argv)
        # max_jit_mop_setup_simple is equivalent to :

        # max_jit_obex_jitob_set(x,o)
        # max_jit_obex_dumpout_set(x,outlet_new(x,NULL))
        # max_jit_mop_setup(x)
        # max_jit_mop_inputs(x)
        # max_jit_mop_outputs(x)
        # max_jit_mop_matrix_args(x,argc,argv)

        # NOTICE: REMOVED max_jit_attr_args from max_jit_mop_setup_simple

        # max_jit_mop_classex_mproc 

    t_jit_err max_jit_mop_setup_probing(t_class *mclass)


cdef extern from "jit.matrix.util.h":

    t_jit_err jit_matrix_list_get_matrices(void *list, long n, void **matrices)
    t_jit_err jit_matrix_array_lock(void **matrices, long n, long *savelock)
    t_jit_err jit_matrix_array_unlock(void **matrices, long n, long *savelock)
    t_jit_err jit_matrix_array_get_matrix_info(void **matrices, long n, t_jit_matrix_info *matrix_info)
    t_jit_err jit_matrix_array_get_data(void **matrices, long n, char **data)
    t_jit_err jit_matrix_info_equal_matrix_structure(t_jit_matrix_info *minfo1, t_jit_matrix_info *minfo2)
    t_jit_err jit_matrix_info_uniform_planecount(t_jit_matrix_info *matrix_info, long n, t_jit_matrix_info *info_list)
    t_jit_err jit_matrix_info_uniform_type(t_jit_matrix_info *matrix_info, long n, t_jit_matrix_info *info_list)
    t_jit_err jit_matrix_info_uniform_dim(t_jit_matrix_info *matrix_info, long n, t_jit_matrix_info *info_list)


cdef extern from "jit.op.h":

    ctypedef struct t_jit_op_info:
        # Provides base pointer and stride for vector operator functions
        void    *p         # base pointer (coerced to appropriate type)
        long    stride     # stride between elements (in type, not bytes)

    ctypedef void (*t_jit_op_fn)(long) # NO LONGER VAR ARG TO PREVENT ISSUES UNDER APPLE SILICON. MUST TYPES BELOW TO CALL
    ctypedef void (*t_jit_op_fn_unary)(long, void *vecdata, t_jit_op_info *in0, t_jit_op_info *out)
    ctypedef void (*t_jit_op_fn_binary)(long, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out)
    ctypedef void (*t_jit_op_fn_ternary)(long, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *in2, t_jit_op_info *out)
    ctypedef void *(*t_jit_op_fn_rv)(long) # NO LONGER VAR ARG TO PREVENT ISSUES UNDER APPLE SILICON. MUST TYPES BELOW TO CALL
    ctypedef void *(*t_jit_op_fn_unary_rv)(long, void *vecdata, t_jit_op_info *in0, t_jit_op_info *out)
    ctypedef void *(*t_jit_op_fn_binary_rv)(long, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out)
    ctypedef void *(*t_jit_op_fn_ternary_rv)(long, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *in2, t_jit_op_info *out)


    ctypedef struct t_jit_op_fn_object:
        t_object        ob
        t_symbol        *name
        long            argcount
        t_jit_op_fn     charfn
        t_jit_op_fn     longfn
        t_jit_op_fn     float32fn
        t_jit_op_fn     float64fn  


    t_jit_err jit_op_init()
    t_jit_op_fn jit_op_sym2fn(t_symbol *opsym, t_symbol *type)
    t_jit_op_fn_object *jit_op_fn_lookup(t_symbol *opsym)
    t_jit_err jit_op_fn_store(t_symbol *opsym, t_jit_op_fn_object *x)

    # note vecdata is unused by the following functions.

    # arith
    void jit_op_vector_pass_char    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_mult_char    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_div_char     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_mod_char     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_add_char     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_adds_char    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_sub_char     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_subs_char    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_min_char     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_max_char     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_avg_char     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_absdiff_char (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    void jit_op_vector_pass_char_altivec    (long n, void *vecdata, uchar *ip1, uchar *op) 
    void jit_op_vector_mult_char_altivec    (long n, void *vecdata, uchar *ip1, uchar *ip2, uchar *op) 
    void jit_op_vector_div_char_altivec     (long n, void *vecdata, uchar *ip1, uchar *ip2, uchar *op) # unimplemented
    void jit_op_vector_mod_char_altivec     (long n, void *vecdata, uchar *ip1, uchar *ip2, uchar *op) # unimplemented
    void jit_op_vector_add_char_altivec     (long n, void *vecdata, uchar *ip1, uchar *ip2, uchar *op) 
    void jit_op_vector_adds_char_altivec    (long n, void *vecdata, uchar *ip1, uchar *ip2, uchar *op) 
    void jit_op_vector_sub_char_altivec     (long n, void *vecdata, uchar *ip1, uchar *ip2, uchar *op) 
    void jit_op_vector_subs_char_altivec    (long n, void *vecdata, uchar *ip1, uchar *ip2, uchar *op) 
    void jit_op_vector_min_char_altivec     (long n, void *vecdata, uchar *ip1, uchar *ip2, uchar *op) 
    void jit_op_vector_max_char_altivec     (long n, void *vecdata, uchar *ip1, uchar *ip2, uchar *op) 
    void jit_op_vector_avg_char_altivec     (long n, void *vecdata, uchar *ip1, uchar *ip2, uchar *op) 

    void jit_op_vector_pass_long    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_mult_long    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_div_long     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_mod_long     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_add_long     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_sub_long     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_min_long     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_max_long     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_abs_long     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_avg_long     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_absdiff_long (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    void jit_op_vector_pass_long_altivec    (long n, void *vecdata, t_int32 *ip1, t_int32 *op) 
    void jit_op_vector_mult_long_altivec    (long n, void *vecdata, t_int32 *ip1, t_int32 *ip2, t_int32 *op) 
    void jit_op_vector_div_long_altivec     (long n, void *vecdata, t_int32 *ip1, t_int32 *ip2, t_int32 *op) # unimplemented
    void jit_op_vector_mod_long_altivec     (long n, void *vecdata, t_int32 *ip1, t_int32 *ip2, t_int32 *op) # unimplemented
    void jit_op_vector_add_long_altivec     (long n, void *vecdata, t_int32 *ip1, t_int32 *ip2, t_int32 *op) 
    void jit_op_vector_sub_long_altivec     (long n, void *vecdata, t_int32 *ip1, t_int32 *ip2, t_int32 *op) 
    void jit_op_vector_min_long_altivec     (long n, void *vecdata, t_int32 *ip1, t_int32 *ip2, t_int32 *op) 
    void jit_op_vector_max_long_altivec     (long n, void *vecdata, t_int32 *ip1, t_int32 *ip2, t_int32 *op) 
    void jit_op_vector_abs_long_altivec     (long n, void *vecdata, t_int32 *ip1, t_int32 *op) 
    void jit_op_vector_avg_long_altivec     (long n, void *vecdata, t_int32 *ip1, t_int32 *ip2, t_int32 *op) 

    void jit_op_vector_pass_float32     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_mult_float32     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_div_float32      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_add_float32      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_sub_float32      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_min_float32      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_max_float32      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_abs_float32      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_avg_float32      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_absdiff_float32  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_mod_float32      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_fold_float32     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_wrap_float32     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    void jit_op_vector_pass_float32_altivec     (long n, void *vecdata, float *ip1, float *op) 
    void jit_op_vector_mult_float32_altivec     (long n, void *vecdata, float *ip1, float *ip2, float *op) 
    void jit_op_vector_div_float32_altivec      (long n, void *vecdata, float *ip1, float *ip2, float *op) # unimplemented
    void jit_op_vector_add_float32_altivec      (long n, void *vecdata, float *ip1, float *ip2, float *op) 
    void jit_op_vector_sub_float32_altivec      (long n, void *vecdata, float *ip1, float *ip2, float *op) 
    void jit_op_vector_min_float32_altivec      (long n, void *vecdata, float *ip1, float *ip2, float *op) 
    void jit_op_vector_max_float32_altivec      (long n, void *vecdata, float *ip1, float *ip2, float *op) 
    void jit_op_vector_abs_float32_altivec      (long n, void *vecdata, float *ip1, float *op) 
    void jit_op_vector_avg_float32_altivec      (long n, void *vecdata, float *ip1, float *ip2, float *op) 
    void jit_op_vector_absdiff_float32_altivec(long n, void *vecdata, float *ip1, float *ip2, float *op)

    void jit_op_vector_pass_float64     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_mult_float64     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_div_float64      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_add_float64      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_sub_float64      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_min_float64      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_max_float64      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_abs_float64      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_avg_float64      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_absdiff_float64  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_mod_float64      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_fold_float64     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_wrap_float64     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    # fliparith(calls corresponding arith function)
    void jit_op_vector_flippass_char    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_flipdiv_char     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_flipmod_char     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_flipsub_char     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    void jit_op_vector_flippass_long    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_flipdiv_long     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_flipmod_long     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_flipsub_long     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    void jit_op_vector_flippass_float32 (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_flipdiv_float32  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_flipmod_float32  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_flipsub_float32  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    void jit_op_vector_flippass_float64 (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_flipdiv_float64  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_flipmod_float64  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_flipsub_float64  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    # bitwise
    void jit_op_vector_bitand_char  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_bitor_char   (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_bitxor_char  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_bitnot_char  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_lshift_char  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_rshift_char  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    void jit_op_vector_bitand_char_altivec  (long n, void *vecdata, uchar *ip1, uchar *ip2, uchar *op) 
    void jit_op_vector_bitor_char_altivec   (long n, void *vecdata, uchar *ip1, uchar *ip2, uchar *op) 
    void jit_op_vector_bitxor_char_altivec  (long n, void *vecdata, uchar *ip1, uchar *ip2, uchar *op) 
    void jit_op_vector_bitnot_char_altivec  (long n, void *vecdata, uchar *ip1, uchar *op) 
    void jit_op_vector_lshift_char_altivec  (long n, void *vecdata, uchar *ip1, uchar *ip2, uchar *op) 
    void jit_op_vector_rshift_char_altivec  (long n, void *vecdata, uchar *ip1, uchar *ip2, uchar *op) 

    void jit_op_vector_bitand_long  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_bitor_long   (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_bitxor_long  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_bitnot_long  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_lshift_long  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_rshift_long  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    void jit_op_vector_bitand_long_altivec  (long n, void *vecdata, t_int32 *ip1, t_int32 *ip2, t_int32 *op) 
    void jit_op_vector_bitor_long_altivec   (long n, void *vecdata, t_int32 *ip1, t_int32 *ip2, t_int32 *op) 
    void jit_op_vector_bitxor_long_altivec  (long n, void *vecdata, t_int32 *ip1, t_int32 *ip2, t_int32 *op) 
    void jit_op_vector_bitnot_long_altivec  (long n, void *vecdata, t_int32 *ip1, t_int32 *op) 
    void jit_op_vector_lshift_long_altivec  (long n, void *vecdata, t_int32 *ip1, t_int32 *ip2, t_int32 *op) 
    void jit_op_vector_rshift_long_altivec  (long n, void *vecdata, t_int32 *ip1, t_int32 *ip2, t_int32 *op) 

    # logical
    void jit_op_vector_and_char (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_or_char  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_not_char (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_gt_char  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_gte_char (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_lt_char  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_lte_char (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_eq_char  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_neq_char (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    void jit_op_vector_and_char_altivec (long n, void *vecdata, uchar *ip1, uchar *ip2, uchar *op) 
    void jit_op_vector_or_char_altivec  (long n, void *vecdata, uchar *ip1, uchar *ip2, uchar *op) 
    void jit_op_vector_not_char_altivec (long n, void *vecdata, uchar *ip1, uchar *op) 
    void jit_op_vector_gt_char_altivec  (long n, void *vecdata, uchar *ip1, uchar *ip2, uchar *op) 
    void jit_op_vector_gte_char_altivec (long n, void *vecdata, uchar *ip1, uchar *ip2, uchar *op) 
    void jit_op_vector_lt_char_altivec  (long n, void *vecdata, uchar *ip1, uchar *ip2, uchar *op)  
    void jit_op_vector_lte_char_altivec (long n, void *vecdata, uchar *ip1, uchar *ip2, uchar *op)  
    void jit_op_vector_eq_char_altivec  (long n, void *vecdata, uchar *ip1, uchar *ip2, uchar *op)  
    void jit_op_vector_neq_char_altivec (long n, void *vecdata, uchar *ip1, uchar *ip2, uchar *op)  

    void jit_op_vector_and_long (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_or_long  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_not_long (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_gt_long  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_gte_long (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_lt_long  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_lte_long (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_eq_long  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_neq_long (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    void jit_op_vector_and_float32  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_or_float32   (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_not_float32  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_gt_float32   (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_gte_float32  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_lt_float32   (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_lte_float32  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_eq_float32   (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_neq_float32  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    void jit_op_vector_and_float64  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_or_float64   (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_not_float64  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_gt_float64   (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_gte_float64  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_lt_float64   (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_lte_float64  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_eq_float64   (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_neq_float64  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    # logical pass
    void jit_op_vector_gtp_char     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_gtep_char    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_ltp_char     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_ltep_char    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_eqp_char     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_neqp_char    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    void jit_op_vector_gtp_long     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_gtep_long    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_ltp_long     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_ltep_long    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_eqp_long     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_neqp_long    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    void jit_op_vector_gtp_float32  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_gtep_float32 (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_ltp_float32  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_ltep_float32 (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_eqp_float32  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_neqp_float32 (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    void jit_op_vector_gtp_float64  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_gtep_float64 (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_ltp_float64  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_ltep_float64 (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_eqp_float64  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_neqp_float64 (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    # transcendental(float32/float64 only)
    void jit_op_vector_sin_float32      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_cos_float32      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_tan_float32      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_asin_float32     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_acos_float32     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_atan_float32     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_atan2_float32    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_sinh_float32     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_cosh_float32     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_tanh_float32     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_asinh_float32    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_acosh_float32    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_atanh_float32    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    void jit_op_vector_exp_float32      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_exp2_float32     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_log_float32      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_log2_float32     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_log10_float32    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_hypot_float32    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_pow_float32      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_sqrt_float32     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    void jit_op_vector_ceil_float32     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_floor_float32    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_round_float32    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_trunc_float32    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    void jit_op_vector_sin_float64      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_cos_float64      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_tan_float64      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_asin_float64     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_acos_float64     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_atan_float64     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_atan2_float64    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_sinh_float64     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_cosh_float64     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_tanh_float64     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_asinh_float64    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_acosh_float64    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_atanh_float64    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    void jit_op_vector_exp_float64      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_exp2_float64     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_log_float64      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_log2_float64     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_log10_float64    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_hypot_float64    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_pow_float64      (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_sqrt_float64     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    void jit_op_vector_ceil_float64     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_floor_float64    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_round_float64    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_trunc_float64    (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    # linear algebra
    void jit_op_vector_ax_float32               (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_ax_float32_complex       (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_ax_float64               (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_ax_float64_complex       (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    void jit_op_vector_axpy_float32             (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_axpy_float32_complex     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_axpy_float64             (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_axpy_float64_complex     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    void jit_op_vector_dotprod_float32          (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_dotprod_float32_complex  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_dotprod_float64          (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_dotprod_float64_complex  (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

    void jit_op_vector_swap_float32             (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_swap_float32_complex     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_swap_float64             (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 
    void jit_op_vector_swap_float64_complex     (long n, void *vecdata, t_jit_op_info *in0, t_jit_op_info *in1, t_jit_op_info *out) 

cdef extern from "jit.parallel.utils.h":

    cdef enum:
        JIT_PARALLEL_NDIM_MAX_IO = 32

    cdef enum:
        JIT_PARALLEL_NDIM_FLAGS_FULL_MATRIX = 0x00000001

    ctypedef struct t_jit_parallel_ndim_io:
        long                flags
        t_jit_matrix_info   *minfo
        char                *bp

    ctypedef struct t_jit_parallel_ndim:
        long flags
        void *data
        long dimcount
        long *dim
        long planecount
        long iocount
        t_jit_parallel_ndim_io  io[JIT_PARALLEL_NDIM_MAX_IO]
        method fn

    ctypedef struct t_jit_parallel_ndim_worker:
        t_jit_parallel_ndim     *paralleldata
        long                    workercount
        long                    workerid
        long                    offset[2]  
        long                    extent[2]  

    void jit_parallel_utils_init()
    void jit_parallel_ndim_calc(t_jit_parallel_ndim *p)
    void jit_parallel_ndim_simplecalc1(method fn, void *data, long dimcount, long *dim, long planecount, t_jit_matrix_info *minfo1, char *bp1, long flags1)
    void jit_parallel_ndim_simplecalc2(method fn, void *data, long dimcount, long *dim, long planecount, t_jit_matrix_info *minfo1, char *bp1, t_jit_matrix_info *minfo2, char *bp2, long flags1, long flags2)
    void jit_parallel_ndim_simplecalc3(method fn, void *data, long dimcount, long *dim, long planecount, t_jit_matrix_info *minfo1, char *bp1, t_jit_matrix_info *minfo2, char *bp2, t_jit_matrix_info *minfo3, char *bp3, long flags1, long flags2, long flags3)
    void jit_parallel_ndim_simplecalc4(method fn, void *data, long dimcount, long *dim, long planecount, t_jit_matrix_info *minfo1, char *bp1, t_jit_matrix_info *minfo2, char *bp2, t_jit_matrix_info *minfo3, char *bp3, t_jit_matrix_info *minfo4, char *bp4, long flags1, long flags2, long flags3, long flags4)


