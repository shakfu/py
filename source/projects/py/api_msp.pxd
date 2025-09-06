# api_msp.pxd
"""
'x' -> the header has been fully exposed to cython

'-' -> the header is not explicitly exposed to cython and presently
       not required for the external. It is exposed to non-cython c code
       via the primary includes in "ext.h"

'p' -> partial analyzed but not yet included in api_msp.pxd

' ' -> an empty box means it is planned

- [x] ext_buffer.h

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
    ctypedef struct t_object
    ctypedef struct t_symbol

cdef extern from "ext_buffer.h":
    ctypedef struct t_buffer_ref
    ctypedef t_object t_buffer_obj
    ctypedef struct t_buffer_info:
        t_symbol	*b_name
        float		*b_samples
        long		b_frames
        long		b_nchans
        long		b_size
        float		b_sr
        long		b_modtime
        long		b_rfu[57]

    cdef t_buffer_ref* buffer_ref_new(t_object *x, t_symbol *name)
    cdef void buffer_ref_set(t_buffer_ref *x, t_symbol *name)
    cdef t_atom_long buffer_ref_exists(t_buffer_ref *x)
    cdef t_buffer_obj *buffer_ref_getobject(t_buffer_ref *x)
    cdef t_max_err buffer_ref_notify(t_buffer_ref *x, t_symbol *s, t_symbol *msg, void *sender, void *data)
    cdef void buffer_view(t_buffer_obj *buffer_object)
    cdef float *buffer_locksamples(t_buffer_obj *buffer_object)
    cdef void buffer_unlocksamples(t_buffer_obj *buffer_object)
    cdef t_atom_long buffer_getchannelcount(t_buffer_obj *buffer_object)
    cdef t_atom_long buffer_getframecount(t_buffer_obj *buffer_object)
    cdef t_atom_float buffer_getsamplerate(t_buffer_obj *buffer_object)
    cdef t_atom_float buffer_getmillisamplerate(t_buffer_obj *buffer_object)
    cdef t_max_err buffer_setpadding(t_buffer_obj *buffer_object, t_atom_long samplecount)
    cdef t_max_err buffer_setdirty(t_buffer_obj *buffer_object)
    cdef t_symbol *buffer_getfilename(t_buffer_obj *buffer_object)
    # start internal low-level
    cdef t_max_err buffer_perform_begin(t_buffer_obj *buffer_object)
    cdef t_max_err buffer_perform_end(t_buffer_obj *buffer_object)
    cdef t_max_err buffer_getinfo(t_buffer_obj *buffer_object, t_buffer_info *info)
    cdef t_max_err buffer_edit_begin(t_buffer_obj *buffer_object)
    cdef t_max_err buffer_edit_end(t_buffer_obj *buffer_object, long valid)
    cdef t_max_err buffer_lock(t_buffer_obj *buffer_object)
    cdef t_max_err buffer_trylock(t_buffer_obj *buffer_object)
    cdef t_max_err buffer_unlock(t_buffer_obj *buffer_object)
    cdef t_buffer_obj *buffer_findowner(t_buffer_obj *buffer_object)
    cdef long buffer_spinwait(t_buffer_obj *buffer_object)
    cdef long buffer_valid(t_buffer_obj *buffer_object, long way)
    # end internal low-level