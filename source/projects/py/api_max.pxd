# api_max.pyx
"""
'x' -> the header has been fully exposed to cython

'-' -> the header is not explicitly exposed to cython and presently
       not required for the external. It is exposed to non-cython c code
       via the primary includes in "ext.h"

'p' -> partial analyzed but not yet included in api.pxd

' ' -> an empty box means it is planned

- [x] commonsyms.h
- [ ] ext.h
- [x] ext_atomarray.h
- [x] ext_atombuf.h
- [-] ext_atomic.h
- [x] ext_backgroundtask.h
- [p] ext_boxstyle.h
- [-] ext_byteorder.h
- [-] ext_charset.h
- [-] ext_common.h
- [-] ext_critical.h
- [x] ext_database.h
- [x] ext_default.h
- [x] ext_dictionary.h
- [x] ext_dictobj.h
- [-] ext_drag.h
- [x] ext_expr.h
- [x] ext_globalsymbol.h
- [x] ext_hashtab.h
- [x] ext_itm.h
- [x] ext_linklist.h
- [x] ext_maxtypes.h
- [x] ext_mess.h
- [x] ext_obex.h
- [x] ext_obex_util.h
- [x] ext_obstring.h
- [x] ext_packages.h
- [x] ext_parameter.h
- [x] ext_sysfile.h
- [x] ext_path.h
- [x] ext_preferences.h
- [-] ext_prefix.h
- [-] ext_preprocessor.h
- [x] ext_proto.h
- [-] ext_proto_win.h
- [ ] ext_qtimage.h
- [ ] ext_qtstubs.h
- [-] ext_quickmap.h
- [ ] ext_sndfile.h
- [x] ext_strings.h
- [x] ext_symobject.h
- [x] ext_sysmem.h
- [x] ext_sysmidi.h
- [x] ext_sysparallel.h
- [x] ext_sysprocess.h
- [x] ext_syssem.h
- [x] ext_sysshmem.h
- [x] ext_systhread.h
- [x] ext_systime.h
- [x] ext_time.h
- [x] ext_wind.h
- [-] ext_xmltree.h
- [-] indexmap.h
- [x] jdataview.h
- [p] jgraphics.h
- [x] jpatcher_api.h
- [-] jpatcher_syms.h
- [x] jpatcher_utils.h
- [x] max_types.h
"""

cdef extern from "ext.h":
    pass

cdef extern from "ext_atombuf.h":
    ctypedef struct t_atombuf:
        long a_argc
        t_atom *a_argv

    void *atombuf_new(long argc, t_atom *argv)
    void atombuf_free(t_atombuf *x)
    void atombuf_text(t_atombuf **x, char **text, long size)
    short atombuf_totext(t_atombuf *x, char **text, long *size)
    short atombuf_count(t_atombuf *x)
    void atombuf_set(t_atombuf *x, long start, long num)
    long atombuf_replacepoundargs(t_atombuf *x, long argc, t_atom *argv)

## if requireed
# cdef extern from "ext_critical.h":
#     ctypedef struct pthread_mutex_t:
#        pass
#     ctypedef pthread_mutex_t* t_critical
#     void critical_new(t_critical *x)
#     void critical_enter(t_critical x)
#     void critical_exit(t_critical x)
#     void critical_free(t_critical x)
#     short critical_tryenter(t_critical x)

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
    ctypedef long (*t_intmethod)(void *)
    ctypedef void t_binbuf

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
    
    ctypedef struct t_symbol:
        # The symbol.
        char *s_name            # name: a c-string
        t_object *s_thing       # possible binding to a t_object

    cdef long MAGIC
    cdef long OB_MAGIC
    cdef int NOGOOD(void *x)
    cdef int OB_INVALID(void *x)
    cdef int MSG_MAXARG

    ctypedef union word:
        t_atom_long w_long
        t_atom_float w_float
        t_symbol *w_sym
        #object *w_obj

    ctypedef struct t_atom:
        short a_type
        word a_w

    ctypedef enum e_max_atomtypes:
        A_NOTHING = 0  # no type, thus no atom
        A_LONG         # long integer
        A_FLOAT        # 32-bit float
        A_SYM          # t_symbol pointer
        A_OBJ          # t_object pointer (for argtype lists; passes the value of sym)
        A_DEFLONG      # long but defaults to zero
        A_DEFFLOAT     # float, but defaults to zero
        A_DEFSYM       # symbol, defaults to ""
        A_GIMME        # request that args be passed as an array, the routine will check the types itself.
        A_CANT         # cannot typecheck args
        A_SEMI         # semicolon
        A_COMMA        # comma
        A_DOLLAR       # dollar
        A_DOLLSYM      # dollar
        A_GIMMEBACK    # request that args be passed as an array, the routine will check the types itself. can return atom value in final atom ptr arg. function returns long error code 0 = no err. see gimmeback_meth typedef
        A_DEFER =       0x41   # A special signature for declaring methods. This is like A_GIMME, but the call is deferred.
        A_USURP =       0x42   # A special signature for declaring methods. This is like A_GIMME, but the call is deferred and multiple calls within one servicing of the queue are filtered down to one call.
        A_DEFER_LOW =   0x43   # A special signature for declaring methods. This is like A_GIMME, but the call is deferref to the back of the queue.
        A_USURP_LOW =   0x44   # A special signature for declaring methods. This is like A_GIMME, but the call is deferred to the back of the queue and multiple calls within one servicing of the queue are filtered down to one call.
    cdef int ATOM_MAX_STRLEN
    ctypedef void *(*zero_meth)(void *x)
    ctypedef void *(*one_meth)(void *x, void *z)
    ctypedef void *(*two_meth)(void *x, void *z, void *a)
    ctypedef long *(*gimmeback_meth)(void *x, t_symbol *s, long ac, t_atom *av, t_atom *rv)
    # cdef <void*>int NIL


cdef extern from "ext_backgroundtask.h":
    ctypedef struct t_backgroundtask: pass

    cdef long backgroundtask_execute(t_object *owner, void *args, method cbtask, method cbcomplete, t_backgroundtask **task, long flags)
    cdef long backgroundtask_execute_method(
        t_object *obtask, t_symbol *mtask, long actask, t_atom *avtask,
        t_object *obcomp, t_symbol *mcomp, long accomp, t_atom *avcomp,
        t_backgroundtask **task, long flags)
    cdef void backgroundtask_purge_object(t_object *owner)
    cdef void backgroundtask_join_object(t_object *owner)
    cdef long backgroundtask_cancel(t_backgroundtask *task)
    cdef long backgroundtask_join(t_backgroundtask *task)




cdef extern from "ext_hashtab.h":
    cdef int HASH_DEFSLOTS
    ctypedef struct t_hashtab_entry: pass
    ctypedef struct t_hashtab: pass
    cdef long cmpfn(void *x, void *y)

    cdef t_hashtab *hashtab_new(long slotcount)
    cdef t_max_err hashtab_store(t_hashtab *x, t_symbol *key, t_object *val)
    cdef t_max_err hashtab_storelong(t_hashtab *x, t_symbol *key, t_atom_long val)
    cdef t_max_err hashtab_storesym(t_hashtab *x, t_symbol *key, t_symbol *val)
    cdef t_max_err hashtab_store_safe(t_hashtab *x, t_symbol *key, t_object *val)
    cdef t_max_err hashtab_storeflags(t_hashtab *x, t_symbol *key, t_object *val, long flags)
    cdef t_max_err hashtab_lookup(t_hashtab *x, t_symbol *key, t_object **val)
    cdef t_max_err hashtab_lookuplong(t_hashtab *x, t_symbol *key, t_atom_long *val)
    cdef t_max_err hashtab_lookupsym(t_hashtab *x, t_symbol *key, t_symbol **val)
    cdef t_max_err hashtab_lookupentry(t_hashtab *x, t_symbol *key, t_hashtab_entry **entry)
    cdef t_max_err hashtab_lookupflags(t_hashtab *x, t_symbol *key, t_object **val, long *flags)
    cdef t_max_err hashtab_delete(t_hashtab *x, t_symbol *key)
    cdef t_max_err hashtab_clear(t_hashtab *x)
    cdef t_max_err hashtab_chuckkey(t_hashtab *x, t_symbol *key)
    cdef t_max_err hashtab_chuck(t_hashtab *x)
    cdef t_max_err hashtab_findfirst(t_hashtab *x, void **o, cmpfn, void *cmpdata)
    cdef t_max_err hashtab_methodall(t_hashtab *x, t_symbol *s, ...)
    #cdef t_max_err hashtab_methodall(...)
    cdef t_max_err hashtab_methodall_imp(void *x, void *sym, void *p1, void *p2, void *p3, void *p4, void *p5, void *p6, void *p7, void *p8)
    cdef t_max_err hashtab_funall(t_hashtab *x, method fun, void *arg)
    cdef t_max_err hashtab_objfunall(t_hashtab *x, method fun, void *arg)
    cdef t_atom_long hashtab_getsize(t_hashtab *x)
    cdef void hashtab_print(t_hashtab *x)
    cdef void hashtab_readonly(t_hashtab *x, long _readonly)
    cdef void hashtab_flags(t_hashtab *x, long flags)
    cdef t_atom_long hashtab_getflags(t_hashtab *x)
    cdef t_max_err hashtab_keyflags(t_hashtab *x, t_symbol *key, long flags)
    cdef t_atom_long hashtab_getkeyflags(t_hashtab *x, t_symbol *key)
    cdef t_max_err hashtab_getkeys(t_hashtab *x, long *kc, t_symbol ***kv)
    cdef t_hashtab_entry *hashtab_entry_new(t_symbol *key, t_object *val)
    cdef void hashtab_entry_free(t_hashtab_entry *x)


cdef extern from "ext_linklist.h":

    ctypedef struct t_llelem: pass
    ctypedef struct t_linklist: pass
    #cdef long LINKLIST_PRUNE_CHUCK 0x00000001L
    ctypedef long (*t_cmpfn)(void *, void *)

    cdef t_linklist *linklist_new()
    cdef void linklist_chuck(t_linklist *x)
    cdef t_atom_long linklist_getsize(t_linklist *x)
    cdef void *linklist_getindex(t_linklist *x, long index)
    cdef t_llelem *linklist_index2ptr(t_linklist *x, long index)
    cdef long linklist_ptr2index(t_linklist *x, t_llelem *p)
    cdef t_atom_long linklist_objptr2index(t_linklist *x, void *p)
    cdef t_atom_long linklist_append(t_linklist *x, void *o)
    cdef t_atom_long linklist_insertindex(t_linklist *x,  void *o, long index)
    cdef long linklist_insert_sorted(t_linklist *x, void *o, cmpfn)
    cdef t_llelem *linklist_insertafterobjptr(t_linklist *x, void *o, void *objptr)
    cdef t_llelem *linklist_insertbeforeobjptr(t_linklist *x, void *o, void *objptr)
    cdef t_llelem *linklist_moveafterobjptr(t_linklist *x, void *o, void *objptr)
    cdef t_llelem *linklist_movebeforeobjptr(t_linklist *x, void *o, void *objptr)
    cdef t_llelem *linklist_insertptr(t_linklist *x,  void *o, t_llelem *p)
    cdef t_atom_long linklist_deleteindex(t_linklist *x, long index)
    cdef long linklist_chuckindex(t_linklist *x, long index)
    cdef long linklist_chuckobject(t_linklist *x, void *o)
    cdef long linklist_deleteobject(t_linklist *x, void *o)
    cdef long linklist_deleteptr(t_linklist *x, t_llelem *p)
    cdef long linklist_chuckptr(t_linklist *x, t_llelem *p)
    cdef void linklist_clear(t_linklist *x)
    cdef long linklist_insertnodeindex(t_linklist *x, t_llelem *p, long index)
    cdef t_llelem *linklist_insertnodeptr(t_linklist *x, t_llelem *p1, t_llelem *p2)
    cdef long linklist_appendnode(t_linklist *x, t_llelem *p)
    cdef t_llelem *linklistelem_new()
    cdef void linklistelem_free(t_linklist *x, t_llelem *elem)
    cdef t_atom_long linklist_makearray(t_linklist *x, void **a, long max)
    cdef void linklist_reverse(t_linklist *x)
    cdef void linklist_rotate(t_linklist *x, long i)
    cdef void linklist_shuffle(t_linklist *x)
    cdef void linklist_swap(t_linklist *x, long a, long b)
    cdef t_atom_long linklist_findfirst(t_linklist *x, void **o, cmpfn, void *cmpdata)
    cdef void linklist_findall(t_linklist *x, t_linklist **out, cmpfn, void *cmpdata)
    cdef void linklist_methodall(t_linklist *x, t_symbol *s, ...)
    cdef void linklist_methodall_imp(void *x, void *sym, void *p1, void *p2, void *p3, void *p4, void *p5, void *p6, void *p7, void *p8)
    cdef void *linklist_methodindex(t_linklist *x, t_atom_long i, t_symbol *s, ...)
    cdef void *linklist_methodindex_imp(void *x, void *i, void *s, void *p1, void *p2, void *p3, void *p4, void *p5, void *p6, void *p7)
    cdef void linklist_sort(t_linklist *x, cmpfn)
    cdef void linklist_funall(t_linklist *x, method fun, void *arg)
    cdef t_atom_long linklist_funall_break(t_linklist *x, method fun, void *arg)
    cdef void *linklist_funindex(t_linklist *x, long i, method fun, void *arg)
    cdef void *linklist_substitute(t_linklist *x, void *p, void *newp)
    cdef void *linklist_next(t_linklist *x, void *p, void **next)
    cdef void *linklist_prev(t_linklist *x, void *p, void **prev)
    cdef void *linklist_last(t_linklist *x, void **item)
    cdef void linklist_readonly(t_linklist *x, long _readonly)
    cdef void linklist_flags(t_linklist *x, long flags)
    cdef t_atom_long linklist_getflags(t_linklist *x)
    cdef long linklist_match(void *a, void *b)


cdef extern from "ext_maxtypes.h":
    ctypedef t_object t_patcher
    ctypedef t_object t_box
    ctypedef t_object t_clock
    ctypedef void* t_qelem

    ctypedef enum:
        PI_DEEP = 1
        PI_REQUIREFIRSTIN = 2
        PI_WANTBOX = 4
        PI_SKIPGEN = 8
        PI_WANTPATCHER = 16

    ctypedef struct Zll: pass
    ctypedef struct t_zll: pass

    ctypedef struct Funbuff: pass
    ctypedef struct t_funbuff: pass


cdef extern from "ext_proto.h":
    cdef void freeobject(t_object *op)
    cdef void *newinstance(const t_symbol *s, short argc, const t_atom *argv)
    cdef void alias(char *name)
    cdef void class_setname(char *obname, char *filename)
    cdef t_symbol *gensym(const char *s)
    cdef void post(const char *fmt, ...)
    cdef void cpost(const char *fmt, ...)
    cdef void error(const char *fmt, ...)
    cdef void postatom(t_atom *ap)
    cdef void error_subscribe(t_object *x)
    cdef void error_unsubscribe(t_object *x)
    cdef void object_post(t_object *x, const char *s, ...)
    cdef void object_error(t_object *x, const char *s, ...)
    cdef void object_error_obtrusive(t_object *x, const char *s, ...)
    cdef void *inlet_new(void *x, const char *s)
    cdef void *intin(void *x, short n)
    cdef void *floatin(void *x, short n)
    cdef void *outlet_new(void *x, const char *s)
    cdef void *bangout(void *x)
    cdef void *intout(void *x)
    cdef void *floatout(void *x)
    cdef void *outlet_bang(void *o)
    cdef void *outlet_int(void *o, t_atom_long n)
    cdef void *outlet_float(void *o, double f)
    cdef void *outlet_list(void *o, t_symbol *s, short ac, t_atom *av)
    cdef void *outlet_anything(void *o, const t_symbol *s, short ac, const t_atom *av)
    cdef void *clock_new(void *obj, method fn)
    cdef void clock_delay(void *x, long n)
    cdef void clock_unset(void *x)
    cdef void clock_fdelay(void *c, double time)
    cdef void clock_getftime(double *time)
    cdef void setclock_delay(t_object *x, void *c, long time)
    cdef void setclock_unset(t_object *x, void *c)
    cdef long setclock_gettime(t_object *x)
    cdef void setclock_fdelay(t_object *s, void *c, double time)
    cdef void setclock_getftime(t_object *s, double *time)
    cdef double systimer_gettime()
    cdef long gettime()
    cdef long getschedtime()
    cdef long getexttime()
    cdef void sched_suspend()
    cdef void sched_resume()
    cdef short sched_isinpoll()
    cdef short sched_isinqueue()
    cdef double gettime_forobject(t_object *x)
    cdef void schedule(void *ob, method fun, long when, t_symbol *sym, short argc, t_atom *argv)
    cdef void schedulef(void *ob, method fun, double when, t_symbol *sym, short argc, t_atom *argv)
    cdef void *scheduler_new()
    cdef void *scheduler_set(void *x)
    cdef void *scheduler_get()
    cdef void *scheduler_fromobject(t_object *o)
    cdef void scheduler_run(void *x, double until)
    cdef void scheduler_settime(void *x, double time)
    cdef void scheduler_gettime(void *x, double *time)
    cdef void scheduler_shift(void *x, double amount)
    cdef void schedule_delay(void *ob, method fun, long delay, t_symbol *sym, short argc, t_atom *argv)
    cdef void schedule_fdelay(void *ob, method fun, double delay, t_symbol *sym, short argc, t_atom *argv)
    cdef void schedule_defer(void *ob, method fun, long delay, t_symbol *sym, short argc, t_atom *arv)
    cdef void schedule_fdefer(void *ob, method fun, double delay, t_symbol *sym, short argc, t_atom *arv)
    cdef short lockout_set(short)
    cdef long isr()
    cdef short isr_set(short way)
    cdef void *qelem_new(void *obj, method fn)
    cdef void qelem_set(void *q)
    cdef void qelem_unset(void *q)
    cdef void qelem_free(void *x)
    cdef void qelem_front(void *x)
    cdef void *defer(void *ob,method fn,t_symbol *sym,short argc,t_atom *argv)
    cdef void *defer_low(void *ob,method fn,t_symbol *sym,short argc,t_atom *argv)
    cdef void *defer_medium(void *ob, method fn, t_symbol *sym, short argc, t_atom *argv)
    cdef void *defer_front(void *ob, method fn, t_symbol *sym, short argc, t_atom *argv)

    cdef void *binbuf_new()
    cdef void binbuf_vinsert(void *x, char *fmt, ...)
    cdef void binbuf_insert(void *x, t_symbol *s, short argc, t_atom *argv)
    cdef void *binbuf_eval(void *x, short ac, t_atom *av, void *to)
    cdef short binbuf_getatom(void *x, long *p1, long *p2, t_atom *ap)
    cdef short binbuf_text(void *x, char **srcText, long n)
    cdef short binbuf_totext(void *x, char **dstText, t_ptr_size *sizep)
    cdef void binbuf_set(void *x, t_symbol *s, short argc, t_atom *argv)
    cdef void binbuf_append(void *x, t_symbol *s, short argc, t_atom *argv)
    cdef void binbuf_delete(void *x, long fromType, long toType, long fromData, long toData)
    cdef void binbuf_addtext(void *x, char **text, long size)
    cdef short readatom(char *outstr, char **text, long *n, long e, t_atom *ap)
    cdef char *atom_string(t_atom *a)
    cdef void *typedmess(t_object *op, t_symbol *msg, short argc, t_atom *argp)
    cdef method getfn(t_object *op, t_symbol *msg)
    cdef method egetfn(t_object *op, t_symbol *msg)
    cdef method zgetfn(t_object *op, t_symbol *msg)
    cdef short table_get(t_symbol *s, long ***hp, long *sp)
    cdef short table_dirty(t_symbol *s)
    cdef short readtohandle(const char *name, short volume, char ***h, long *sizep)
    cdef void *fileload(const char *name, short vol)
    cdef void *intload(const char *name, short volume, t_symbol *s, short ac, t_atom *av, short couldedit)
    cdef void preset_store(char *fmt, ...)
    cdef void preset_int(t_object *x, t_atom_long n)
    cdef void *proxy_new(void *x,long id,long *stuffloc)
    cdef long proxy_getinlet(t_object *master)
    cdef void *connection_client(t_object *cli, t_symbol *name, t_symbol *classname, method traverse)
    cdef void connection_server(t_object *obj, t_symbol *name)
    cdef void connection_send(t_object *server, t_symbol *name, t_symbol *mess, void *arg)
    cdef void connection_delete(t_object *ob, t_symbol *name)
    cdef void quittask_install(method m, void *a)
    cdef void quittask_remove(method m)
    cdef void quittask_remove2(method m, void *a)
    cdef short maxversion()
    cdef short ispatcher(t_object *x)
    cdef void open_promptset(const char *s)
    cdef void saveas_promptset(const char *s)
    cdef void *filewatcher_new(t_object *owner, const short path, const char *filename)
    cdef void filewatcher_start(void *x)
    cdef void filewatcher_stop(void *x)
    cdef void fileusage_addfile(void *w, long flags, const char *name, const short path)
    cdef void fileusage_addfilename(void *w, long flags, const char *name)
    cdef void fileusage_addpackage(void *w, const char *name, t_object *subfoldernames)
    cdef void fileusage_addpathname(void *w, long flags, const char *name)
    cdef void fileusage_copyfolder(void *w, const char *name, long recursive)
    cdef void fileusage_makefolder(void *w, const char *name)
    # short open_dialog(char *name, short *volptr, t_fourcc *typeptr, t_fourcc *types, short ntypes)
    # short saveas_dialog(char *filename, short *path, short *binptr)


cdef extern from "ext_dictionary.h":
    ctypedef struct t_dictionary_entry: pass
    ctypedef struct t_dictionary: pass

    cdef t_dictionary* dictionary_new()
    # [private] cdef t_dictionary* dictionary_prototypefromclass(t_class *c)
    cdef t_max_err dictionary_appendlong(t_dictionary *d, t_symbol *key, t_atom_long value)
    cdef t_max_err dictionary_appendfloat(t_dictionary *d, t_symbol *key, double value)
    cdef t_max_err dictionary_appendsym(t_dictionary *d, t_symbol *key, t_symbol *value)
    cdef t_max_err dictionary_appendatom(t_dictionary *d, t_symbol *key, t_atom *value)
    cdef t_max_err dictionary_appendattribute(t_dictionary *d, t_symbol *key, t_symbol *attrname, t_object *obj)
    cdef t_max_err dictionary_appendstring(t_dictionary *d, t_symbol *key, const char *value)
    cdef t_max_err dictionary_appendatoms(t_dictionary *d, t_symbol *key, long argc, t_atom *argv)
    cdef t_max_err dictionary_appendatoms_flags(t_dictionary *d, t_symbol *key, long argc, t_atom *argv, long flags)
    cdef t_max_err dictionary_appendatomarray(t_dictionary *d, t_symbol *key, t_object *value)
    cdef t_max_err dictionary_appenddictionary(t_dictionary *d, t_symbol *key, t_object *value)
    cdef t_max_err dictionary_appendobject(t_dictionary *d, t_symbol *key, t_object *value)
    cdef t_max_err dictionary_appendobject_flags(t_dictionary *d, t_symbol *key, t_object *value, long flags)
    cdef t_max_err dictionary_appendbinbuf(t_dictionary *d, t_symbol *key, void *value)
    cdef t_max_err dictionary_getlong(const t_dictionary *d, t_symbol *key, t_atom_long *value)
    cdef t_max_err dictionary_getfloat(const t_dictionary *d, t_symbol *key, double *value)
    cdef t_max_err dictionary_getsym(const t_dictionary *d, t_symbol *key, t_symbol **value)
    cdef t_max_err dictionary_getatom(const t_dictionary *d, t_symbol *key, t_atom *value)
    cdef t_max_err dictionary_getattribute(const t_dictionary *d, t_symbol *key, t_symbol *attrname, t_object *obj)
    cdef t_max_err dictionary_getstring(const t_dictionary *d, t_symbol *key, const char **value)
    cdef t_max_err dictionary_getatoms(const t_dictionary *d, t_symbol *key, long *argc, t_atom **argv)
    cdef t_max_err dictionary_getatoms_ext(const t_dictionary *d, t_symbol *key, long stringstosymbols, long *argc, t_atom **argv)
    cdef t_max_err dictionary_copyatoms(const t_dictionary *d, t_symbol *key, long *argc, t_atom **argv)
    cdef t_max_err dictionary_getatomarray(const t_dictionary *d, t_symbol *key, t_object **value)
    cdef t_max_err dictionary_getdictionary(const t_dictionary *d, t_symbol *key, t_object **value)
    cdef t_max_err dictionary_get_ex(t_dictionary *d, t_symbol *key, long *ac, t_atom **av, char *errstr)
    cdef t_max_err dictionary_getobject(const t_dictionary *d, t_symbol *key, t_object **value)
    cdef long dictionary_entryisstring(const t_dictionary *d, t_symbol *key)
    cdef long dictionary_entryisatomarray(const t_dictionary *d, t_symbol *key)
    cdef long dictionary_entryisdictionary(const t_dictionary *d, t_symbol *key)
    cdef long dictionary_hasentry(const t_dictionary *d, t_symbol *key)
    cdef t_atom_long dictionary_getentrycount(const t_dictionary *d)
    cdef t_max_err dictionary_getkeys(const t_dictionary *d, long *numkeys, t_symbol ***keys)
    cdef t_max_err dictionary_getkeys_ordered(const t_dictionary *d, long *numkeys, t_symbol ***keys)
    cdef void dictionary_freekeys(t_dictionary *d, long numkeys, t_symbol **keys)
    cdef t_max_err dictionary_deleteentry(t_dictionary *d, t_symbol *key)
    cdef t_max_err dictionary_chuckentry(t_dictionary *d, t_symbol *key)
    cdef t_max_err dictionary_clear(t_dictionary *d)
    cdef t_dictionary *dictionary_clone(t_dictionary *d)
    cdef t_max_err dictionary_clone_to_existing(const t_dictionary *d, t_dictionary *dc)
    cdef t_max_err dictionary_copy_to_existing(const t_dictionary *d, t_dictionary *dc)
    cdef t_max_err dictionary_merge_to_existing(const t_dictionary *d, t_dictionary *dc)
    # cdef t_max_err dictionary_copy_nonunique_to_existing(const t_dictionary *d, t_dictionary *dc)
    cdef void dictionary_funall(t_dictionary *d, method fun, void *arg)
    cdef t_symbol* dictionary_entry_getkey(t_dictionary_entry *x)
    cdef void dictionary_entry_getvalue(t_dictionary_entry *x, t_atom *value)
    cdef void dictionary_entry_getvalues(t_dictionary_entry *x, long *argc, t_atom **argv)

    cdef t_max_err dictionary_copyunique(t_dictionary *d, t_dictionary *copyfrom)
    cdef t_max_err dictionary_getdeflong(const t_dictionary *d, t_symbol *key, t_atom_long *value, t_atom_long xdef)
    cdef t_max_err dictionary_getdeffloat(const t_dictionary *d, t_symbol *key, double *value, double xdef)
    cdef t_max_err dictionary_getdefsym(const t_dictionary *d, t_symbol *key, t_symbol **value, t_symbol *xdef)
    cdef t_max_err dictionary_getdefatom(const t_dictionary *d, t_symbol *key, t_atom *value, t_atom *xdef)
    cdef t_max_err dictionary_getdefstring(const t_dictionary *d, t_symbol *key, const char **value, char *xdef)
    cdef t_max_err dictionary_getdefatoms(t_dictionary *d, t_symbol *key, long *argc, t_atom **argv, t_atom *xdef)
    cdef t_max_err dictionary_copydefatoms(t_dictionary *d, t_symbol *key, long *argc, t_atom **argv, t_atom *xdef)
    cdef t_max_err dictionary_dump(t_dictionary *d, long recurse, long console)

    cdef t_max_err dictionary_copyentries(t_dictionary *src, t_dictionary *dst, t_symbol **keys)
    cdef t_dictionary *dictionary_sprintf(char *fmt, ...)
    cdef t_object *newobject_sprintf(t_object *patcher, const char *fmt, ...)
    cdef t_object *newobject_fromboxtext(t_object *patcher, const char *text)
    cdef t_object *newobject_fromdictionary(t_object *patcher, t_dictionary *d)
    cdef long dictionary_transaction_lock(t_dictionary *d)
    cdef long dictionary_transaction_unlock(t_dictionary *d)


cdef extern from "ext_dictobj.h":

    cdef t_dictionary *dictobj_register(t_dictionary *d, t_symbol **name)
    cdef t_max_err dictobj_unregister(t_dictionary *d)
    cdef t_dictionary *dictobj_findregistered_clone(t_symbol *name)
    cdef t_dictionary *dictobj_findregistered_retain(t_symbol *name)
    cdef t_max_err dictobj_release(t_dictionary *d)
    cdef t_symbol *dictobj_namefromptr(t_dictionary *d)
    cdef void dictobj_outlet_atoms(void *out, long argc, t_atom *argv)
    cdef long dictobj_atom_safety(t_atom *a)
    ctypedef enum:
        DICTOBJ_ATOM_FLAGS_DEFAULT = 0
        DICTOBJ_ATOM_FLAGS_REGISTER
    cdef long dictobj_atom_safety_flags(t_atom *a, long flags)
    cdef void dictobj_atom_release(t_atom *a)
    cdef long dictobj_validate(const t_dictionary *schema, const t_dictionary *candidate)
    cdef t_max_err dictobj_jsonfromstring(long *jsonsize, char **json, const char *str)
    cdef t_max_err dictobj_dictionaryfromstring(t_dictionary **d, const char *str, int str_is_already_json, char *errorstring)
    cdef t_max_err dictobj_dictionaryfromatoms(t_dictionary **d, const long argc, const t_atom *argv)
    cdef t_max_err dictobj_dictionaryfromatoms_extended(t_dictionary **d, const t_symbol *msg, long argc, const t_atom *argv)
    cdef t_max_err dictobj_dictionarytoatoms(const t_dictionary *d, long *argc, t_atom **argv)
    cdef t_max_err dictobj_key_parse(t_object *x, t_dictionary *d, t_atom *akey, t_bool create, t_dictionary **targetdict, t_symbol **targetkey, t_int32 *index)


cdef extern from "ext_expr.h":

    cdef int EXPR_MAX_VARS

    ctypedef enum e_max_expr_types:
        ET_INT =    0x1
        ET_FLT =    0x2
        ET_OP  =    0x3
        ET_STR =    0x4
        ET_TBL =    0x5
        ET_FUNC =   0x6
        ET_SYM =    0x7
        ET_VSYM =   0x8
        ET_LP =     0x9
        ET_LB =     0x10
        ET_II =     0x11
        ET_FI =     0x12
        ET_SI =     0x13

    ctypedef struct t_ex_ex: pass
    ctypedef struct t_expr: pass

    cdef void *expr_new(long argc, t_atom *argv, t_atom *types)
    cdef short expr_eval(t_expr *x, long argc, t_atom *argv, t_atom *result)
    cdef void expr_install(method fun, const char *buf, long argc)


cdef extern from "ext_globalsymbol.h":
    cdef void *globalsymbol_reference(t_object *x, const char *name, const char *classname)
    cdef void globalsymbol_dereference(t_object *x, const char *name, const char *classname)
    cdef t_max_err globalsymbol_bind(t_object *x, const char *name, long flags)
    cdef void globalsymbol_unbind(t_object *x, const char *name, long flags)
    cdef void globalsymbol_notify(t_object *x, const char *name, t_symbol *msg, void *data)


cdef extern from "ext_obex.h":

    cdef int TRUE
    cdef int FALSE
    cdef t_symbol *CLASS_BOX
    cdef t_symbol *CLASS_NOBOX

    ctypedef enum e_max_attrflags:
        ATTR_FLAGS_NONE =       0x0000000  # No flags
        ATTR_GET_OPAQUE =       0x00000001 # The attribute cannot be queried by either max message when used inside of a CLASS_BOX object, nor from C code.
        ATTR_SET_OPAQUE =       0x00000002 # The attribute cannot be set by either max message when used inside of a CLASS_BOX object, nor from C code.
        ATTR_GET_OPAQUE_USER =  0x00000100 # The attribute cannot be queried by max message when used inside of a CLASS_BOX object, but <em>can</em> be queried from C code.
        ATTR_SET_OPAQUE_USER =  0x00000200 # The attribute cannot be set by max message when used inside of a CLASS_BOX object, but <em>can</em> be set from C code.
        ATTR_GET_DEFER =        0x00010000 # Placeholder for potential future functionality: Any attribute queries will be called through a defer().
        ATTR_GET_USURP =        0x00020000 # Placeholder for potential future functionality: Any calls to query the attribute will be called through the equivalent of a defer(), repeated calls will be ignored until the getter is actually run.
        ATTR_GET_DEFER_LOW =    0x00040000 # Placeholder for potential future functionality: Any attribute queries will be called through a defer_low().
        ATTR_GET_USURP_LOW =    0x00080000 # Placeholder for potential future functionality: Any calls to query the attribute will be called through the equivalent of a defer_low(), repeated calls will be ignored until the getter is actually run.
        ATTR_SET_DEFER =        0x01000000 # Placeholder for potential future functionality: The attribute setter will be called through a defer().
        ATTR_SET_USURP =        0x02000000 # Placeholder for potential future functionality: Any calls to set the attribute will be called through the equivalent of a defer_low(), repeated calls will be ignored until the setter is actually run.
        ATTR_SET_DEFER_LOW =    0x04000000 # Placeholder for potential future functionality: The attribute setter will be called through a defer_low()
        ATTR_SET_USURP_LOW =    0x08000000 # Placeholder for potential future functionality: Any calls to set the attribute will be called through the equivalent of a defer_low(), repeated calls will be ignored until the setter is actually run.
        ATTR_IS_JBOXATTR =      0x10000000 # a common jbox attr
        ATTR_DIRTY =            0x20000000 # attr has been changed from its default value

    # Standard values returned by function calls with a return type of #t_max_err
    ctypedef enum e_max_errorcodes:
        MAX_ERR_NONE =           0 # No error
        MAX_ERR_GENERIC =       -1 # Generic error
        MAX_ERR_INVALID_PTR =   -2 # Invalid Pointer
        MAX_ERR_DUPLICATE =     -3 # Duplicate
        MAX_ERR_OUT_OF_MEM =    -4 # Out of memory

    # Flags used in linklist and hashtab objects
    ctypedef enum e_max_datastore_flags:
        OBJ_FLAG_OBJ =          0x00000000 # free using object_free()
        OBJ_FLAG_REF =          0x00000001 # don't free
        OBJ_FLAG_DATA =         0x00000002 # don't free data or call method
        OBJ_FLAG_MEMORY =       0x00000004 # don't call method, and when freeing use sysmem_freeptr() instead of freeobject
        OBJ_FLAG_SILENT =       0x00000100 # don't notify when modified
        OBJ_FLAG_INHERITABLE =  0x00000200 # obexprototype entry will be inherited by subpatchers and abstractions
        OBJ_FLAG_ITERATING =    0x00001000 # used by linklist to signal when is inside iteration
        OBJ_FLAG_DEBUG =        0x40000000 # context-dependent flag, used internally for linklist debug code


    cdef t_class *class_new(const char *name, const method mnew, const method mfree, long size, const method mmenu, short type, ...)
    cdef t_max_err class_free(t_class *c)
    cdef t_max_err class_register(t_symbol *name_space, t_class *c)
    cdef t_max_err class_alias(t_class *c, t_symbol *aliasname)

    cdef t_max_err class_copy(t_symbol *src_name_space, t_symbol *src_classname, t_symbol *dst_name_space, t_symbol *dst_classname)
    cdef t_max_err class_addmethod(t_class *c, const method m, const char *name, ...)
    cdef t_max_err class_addattr(t_class *c,t_object *attr)
    cdef t_max_err class_addadornment(t_class *c,t_object *o)
    cdef void *class_adornment_get(t_class *c,t_symbol *classname)
    cdef t_symbol *class_nameget(t_class *c)
    cdef t_class *class_findbyname(t_symbol *name_space, t_symbol *classname)
    cdef t_class *class_findbyname_casefree(t_symbol *name_space, t_symbol *classname)
    cdef t_max_err class_dumpout_wrap(t_class *c)
    cdef t_class *class_getifloaded(t_symbol *name_space, t_symbol *classname)
    cdef t_class *class_getifloaded_casefree(t_symbol *name_space, t_symbol *classname)
    cdef long object_classname_compare(void *x, t_symbol *name)
    # cdef t_hashtab *reg_object_namespace_lookup(t_symbol *name_space)
    cdef method class_method(t_class *x, t_symbol *methodname)
    cdef t_messlist *class_mess(t_class *x, t_symbol *methodname)
    cdef t_messlist *object_mess(t_object *x, t_symbol *methodname)
    cdef method class_attr_method(t_class *x, t_symbol *methodname, void **attr, long *get)
    cdef void *class_attr_get(t_class *x, t_symbol *attrname)
    cdef t_max_err class_extra_store(t_class *x,t_symbol *s,t_object *o)
    cdef t_max_err class_extra_storeflags(t_class *x,t_symbol *s,t_object *o,long flags)
    cdef void *class_extra_lookup(t_class *x,t_symbol *s)
    cdef t_max_err class_addtypedwrapper(t_class *x, method m, char *name, ...)
    cdef t_messlist *class_typedwrapper_get(t_class *x, t_symbol *s)
    cdef t_max_err object_addtypedwrapper(t_object *x, method m, char *name, ...)
    cdef t_messlist *object_typedwrapper_get(t_object *x, t_symbol *s)
    # cdef t_hashtab *class_namespace_fromsym(t_symbol *name_space)
    cdef t_max_err class_namespace_getclassnames(t_symbol *name_space, long *kc, t_symbol ***kv)
    cdef t_max_err class_setpath(t_class *x, short vol)
    cdef short class_getpath(t_class *x)
    cdef void *object_alloc(t_class *c)
    cdef void *object_new(t_symbol *name_space, t_symbol *classname, ...)
    cdef void *object_new_imp(void *p1, void *p2, void *p3, void *p4, void *p5, void *p6, void *p7, void *p8, void *p9, void *p10)
    cdef void *object_new_typed(t_symbol *name_space, t_symbol *classname, long ac, t_atom *av)
    cdef void *object_new_menufun(t_symbol *name_space, t_symbol *classname, void *p, long h, long v, long f)
    cdef t_max_err object_free(void *x)
    cdef void *object_method(void *x, t_symbol *s, ...)
    cdef void *object_method_imp(void *x, void *sym, void *p1, void *p2, void *p3, void *p4, void *p5, void *p6, void *p7, void *p8)
    cdef method object_method_direct_getmethod(t_object *x, t_symbol *sym)
    cdef void *object_method_direct_getobject(t_object *x, t_symbol *sym)
    cdef t_max_err object_method_typed(void *x, t_symbol *s, long ac, t_atom *av, t_atom *rv)
    cdef t_max_err object_method_typedfun(void *x, t_messlist *mp, t_symbol *s, long ac, t_atom *av, t_atom *rv)
    cdef method object_getmethod(void *x, t_symbol *s)
    cdef t_symbol *object_classname(void *x)
    cdef t_symbol *object_namespace(t_object *x)
    cdef t_symbol *class_namespace(t_class *c)
    cdef void *object_register(t_symbol *name_space, t_symbol *s, void *x)
    cdef t_symbol *object_register_unique(t_symbol *name_space, t_symbol *s, void *x)
    cdef void *object_findregistered(t_symbol *name_space, t_symbol *s)
    cdef t_max_err object_findregisteredbyptr(t_symbol **name_space, t_symbol **s, void *x)
    cdef t_max_err object_register_getnames(t_symbol *name_space, long *namecount, t_symbol ***names)
    cdef void *object_attach(t_symbol *name_space, t_symbol *s, void *x)
    cdef t_max_err object_detach(t_symbol *name_space, t_symbol *s, void *x)
    cdef t_max_err object_attach_byptr(void *x, void *registeredobject)
    cdef t_max_err object_attach_byptr_register(void *x, void *object_to_attach, t_symbol *reg_name_space)
    cdef t_max_err object_detach_byptr(void *x, void *registeredobject)
    cdef void *object_subscribe(t_symbol *name_space, t_symbol *s, t_symbol *classname, void *x)
    cdef t_max_err object_unsubscribe(t_symbol *name_space, t_symbol *s, t_symbol *classname, void *x)
    cdef t_max_err object_unregister(void *x)
    cdef t_max_err object_register_getnames(t_symbol *name_space, long *namecount, t_symbol ***names)
    cdef t_max_err object_notify(void *x, t_symbol *s, void *data)
    cdef t_symbol *reg_object_singlesym(t_symbol *name_space, t_symbol *s)
    cdef t_symbol *reg_object_singlesymbyptr(t_object *x)
    cdef t_max_err reg_object_singlesym_split(t_symbol *singlesym, t_symbol **name_space, t_symbol **objname)
    cdef t_class *object_class(void *x)
    cdef t_max_err object_getvalueof(void *x, long *ac, t_atom **av)
    cdef t_max_err object_setvalueof(void *x, long ac, t_atom *av)
    cdef void *object_attr_get(void *x, t_symbol *attrname)
    cdef method object_attr_method(void *x, t_symbol *methodname, void **attr, long *get)
    cdef long object_attr_usercanset(void *x,t_symbol *s)
    cdef long object_attr_usercanget(void *x,t_symbol *s)
    cdef void object_attr_getdump(void *x, t_symbol *s, long argc, t_atom *argv)
    cdef t_max_err object_attr_getvalueof(void *x, t_symbol *s, long *argc, t_atom **argv)
    cdef t_max_err object_attr_setvalueof(void *x, t_symbol *s, long argc, t_atom *argv)
    cdef t_max_err object_addattr(void *x, t_object *attr)
    cdef t_max_err object_deleteattr(void *x, t_symbol *attrsym)
    cdef t_max_err object_chuckattr(void *x, t_symbol *attrsym)
    cdef void class_obexoffset_set(t_class *c, long offset)
    cdef long class_obexoffset_get(t_class *c)
    cdef t_max_err object_obex_lookup(void *x, t_symbol *key, t_object **val)
    cdef t_max_err object_obex_lookuplong(void *x, t_symbol *key, t_atom_long *val)
    cdef t_max_err object_obex_lookupsym(void *x, t_symbol *key, t_symbol **val)
    cdef t_max_err object_obex_store(void *x,t_symbol *key, t_object *val)
    cdef t_max_err object_obex_storeflags(void *x,t_symbol *key, t_object *val, long flags)
    cdef t_max_err object_obex_storelong(void *x, t_symbol *key, t_atom_long val)
    cdef t_max_err object_obex_storesym(void *x, t_symbol *key, t_symbol *val)
    # cdef t_max_err object_obex_set(void *x, t_hashtab *obex)
    # cdef t_hashtab *object_obex_get(void *x)
    # cdef t_hashtab *object_obex_enforce(void *x)
    cdef void object_obex_dumpout(void *x, const t_symbol *s, long argc, const t_atom *argv)
    cdef t_max_err atom_setlong(t_atom *a, t_atom_long b)
    cdef t_max_err atom_setfloat(t_atom *a, double b)
    cdef t_max_err atom_setsym(t_atom *a, const t_symbol *b)
    cdef t_max_err atom_setobj(t_atom *a, void *b)
    cdef t_atom_long atom_getlong(const t_atom *a)
    cdef t_atom_float atom_getfloat(const t_atom *a)
    cdef t_symbol *atom_getsym(const t_atom *a)
    cdef void *atom_getobj(const t_atom *a)
    cdef long atom_getcharfix(const t_atom *a)
    cdef long atom_gettype(const t_atom *a)
    cdef t_max_err atom_arg_getlong(t_atom_long *c, long idx, long ac, const t_atom *av)
    cdef long atom_arg_getfloat(float *c, long idx, long ac, const t_atom *av)
    cdef long atom_arg_getdouble(double *c, long idx, long ac, const t_atom *av)
    cdef long atom_arg_getsym(t_symbol **c, long idx, long ac, const t_atom *av)
    cdef long attr_args_offset(short ac, t_atom *av)
    cdef void attr_args_process(void *x, short ac, t_atom *av)
    cdef t_object *attribute_new(const char *name, t_symbol *type, long flags, method mget, method mset)
    cdef t_object *attr_offset_new(const char *name, const t_symbol *type, long flags, const method mget, const method mset, long offset)
    cdef t_object *attr_offset_array_new(const char *name, t_symbol *type, long size, long flags, method mget, method mset, long offsetcount, long offset)
    cdef t_object *attr_filter_clip_new()
    cdef t_object *attr_filter_proc_new(method proc)
    cdef t_atom_long object_attr_getlong(void *x, t_symbol *s)
    cdef t_max_err object_attr_setlong(void *x, t_symbol *s, t_atom_long c)
    cdef t_atom_float object_attr_getfloat(void *x, t_symbol *s)
    cdef t_max_err object_attr_setfloat(void *x, t_symbol *s, t_atom_float c)
    cdef t_symbol *object_attr_getsym(void *x, t_symbol *s)
    cdef t_max_err object_attr_setsym(void *x, t_symbol *s, t_symbol *c)
    cdef char object_attr_getchar(void *x, t_symbol *s)
    cdef t_max_err object_attr_setchar(void *x, t_symbol *s, char c)
    cdef t_object* object_attr_getobj(void *x, t_symbol *s)
    cdef t_max_err object_attr_setobj(void *x, t_symbol *s, t_object *o)
    cdef long object_attr_getlong_array(void *x, t_symbol *s, long max, t_atom_long *vals)
    cdef t_max_err object_attr_setlong_array(void *x, t_symbol *s, long count, t_atom_long *vals)
    cdef long object_attr_getchar_array(void *x, t_symbol *s, long max, t_uint8 *vals)
    cdef t_max_err object_attr_setchar_array(void *x, t_symbol *s, long count, const t_uint8 *vals)
    cdef long object_attr_getfloat_array(void *x, t_symbol *s, long max, float *vals)
    cdef t_max_err object_attr_setfloat_array(void *x, t_symbol *s, long count, float *vals)
    cdef long object_attr_getdouble_array(void *x, t_symbol *s, long max, double *vals)
    cdef t_max_err object_attr_setdouble_array(void *x, t_symbol *s, long count, double *vals)
    cdef long object_attr_getsym_array(void *x, t_symbol *s, long max, t_symbol **vals)
    cdef t_max_err object_attr_setsym_array(void *x, t_symbol *s, long count, t_symbol **vals)
    cdef t_max_err attr_addfilterset_clip(void *x, double min, double max, long usemin, long usemax)
    cdef t_max_err attr_addfilterset_clip_scale(void *x, double scale, double min, double max, long usemin, long usemax)
    cdef t_max_err attr_addfilterget_clip(void *x, double min, double max, long usemin, long usemax)
    cdef t_max_err attr_addfilterget_clip_scale(void *x, double scale, double min, double max, long usemin, long usemax)
    cdef t_max_err attr_addfilter_clip(void *x, double min, double max, long usemin, long usemax)
    cdef t_max_err attr_addfilter_clip_scale(void *x, double scale, double min, double max, long usemin, long usemax)
    cdef t_max_err attr_addfilterset_proc(void *x, method proc)
    cdef t_max_err attr_addfilterget_proc(void *x, method proc)
    cdef t_symbol *symbol_unique()
    cdef t_symbol *symbol_stripquotes(t_symbol *s)
    cdef void error_code(void *x,t_max_err v)
    cdef void error_sym(void *x,t_symbol *s)
    cdef void post_sym(void *x,t_symbol *s)
    cdef t_max_err symbolarray_sort(long ac, t_symbol **av)
    cdef void object_obex_quickref(void *x, long *numitems, t_symbol **items)
    cdef method class_menufun_get(t_class *c)
    cdef long class_clonable(t_class *x)
    cdef long object_clonable(t_object *x)
    cdef t_max_err class_buildprototype(t_class *x)
    cdef t_dictionary *class_cloneprototype(t_class *x)
    cdef void attr_args_dictionary(t_dictionary *x, short ac, t_atom *av)
    cdef void attr_dictionary_process(void *x, t_dictionary *d)
    cdef void attr_dictionary_check(void *x, t_dictionary *d)
    cdef t_dictionary *object_dictionaryarg(long ac, t_atom *av)
    cdef t_max_err class_sticky(t_class *x, t_symbol *stickyname, t_symbol *s, t_object *o)
    cdef t_max_err class_sticky_clear(t_class *x, t_symbol *stickyname, t_symbol *s)
    cdef t_max_err object_retain(t_object *x)
    cdef t_max_err object_release(t_object *x)
    ctypedef struct t_method_object: pass
    cdef t_method_object *method_object_new(method m, const char *name, ...)
    cdef t_method_object *method_object_new_messlist(t_messlist *m)
    cdef void method_object_free(t_method_object *x)
    cdef t_symbol *method_object_getname(t_method_object *x)
    cdef void method_object_setname(t_method_object *x, t_symbol *s)
    cdef method method_object_getmethod(t_method_object *x)
    cdef void method_object_setmethod(t_method_object *x, method m)
    cdef t_messlist *method_object_getmesslist(t_method_object *x)
    cdef void method_object_setmesslist(t_method_object *x, t_messlist *m)
    cdef t_method_object *class_getmethod_object(t_class *x, t_symbol *methodname)
    cdef t_method_object *object_getmethod_object(t_object *x, t_symbol *methodname)
    # cdef t_max_err object_attrhash_apply(t_object *x, t_hashtab *attrhash)
    cdef t_max_err object_sticky(t_object *x, t_symbol *stickyname, t_symbol *s, t_object *o)
    cdef t_max_err object_sticky_clear(t_object *x, t_symbol *stickyname, t_symbol *s)
    cdef t_max_err object_addmethod(t_object *x, method m, const char *name, ...)
    cdef t_max_err object_addmethod_object(t_object *x, t_object *mo)
    cdef t_max_err object_deletemethod(t_object *x, t_symbol *methodsym)
    cdef t_max_err object_chuckmethod(t_object *x, t_symbol *methodsym)
    cdef t_max_err attr_typedfun_set(void *parent, t_object *x, long ac, t_atom *av)
    cdef t_max_err object_attr_getnames(void *x, long *argc, t_symbol ***argv)
    cdef t_max_err atom_alloc(long *ac, t_atom **av, char *alloc)
    cdef t_max_err atom_alloc_array(long minsize, long *ac, t_atom **av, char *alloc)
    cdef long class_is_box(t_class *c)
    cdef t_dictionary *object_dictionary_fromnewargs(t_object *patcher, t_class *c, long argc, t_atom *argv, long flags, char *freedict)
    cdef long class_is_ui(t_class *c)
    cdef t_max_err class_subclass(t_class *superclass, t_class *subclass)
    cdef t_object *class_super_construct(t_class *c, ...)
    cdef t_object *class_super_construct_imp(void *c, void *p1, void *p2, void *p3, void *p4, void *p5, void *p6, void *p7, void *p8, void *p9)
    cdef void *object_super_method(t_object *x, t_symbol *s, ...)
    cdef void *object_super_method_imp(void *x, void *s, void *p1, void *p2, void *p3, void *p4, void *p5, void *p6, void *p7, void *p8)
    cdef void *object_this_method(t_object *x, t_symbol *s, ...)
    cdef void *object_this_method_imp(void *x, void *s, void *p1, void *p2, void *p3, void *p4, void *p5, void *p6, void *p7, void *p8)
    cdef t_max_err object_attr_touch(t_object *x, t_symbol *attrname)
    cdef t_max_err object_attr_touch_parse(t_object *x, char *attrnames)


cdef extern from "ext_obex_util.h":
    cdef USESYM(x)
    cdef CLASS_ATTR_CHAR(c,attrname,flags,structname,structmember)
    cdef CLASS_ATTR_LONG(c,attrname,flags,structname,structmember)
    cdef CLASS_ATTR_ATOM_LONG(c,attrname,flags,structname,structmember)
    cdef CLASS_ATTR_INT32(c,attrname,flags,structname,structmember)
    cdef CLASS_ATTR_FILEPATH(c,attrname,flags,structname,structmember)
    cdef CLASS_ATTR_FLOAT(c,attrname,flags,structname,structmember)
    cdef CLASS_ATTR_DOUBLE(c,attrname,flags,structname,structmember)
    cdef CLASS_ATTR_SYM(c,attrname,flags,structname,structmember)

    cdef CLASS_ATTR_ATOM(c,attrname,flags,structname,structmember)
    cdef CLASS_ATTR_OBJ(c,attrname,flags,structname,structmember)
    cdef CLASS_ATTR_CHAR_ARRAY(c,attrname,flags,structname,structmember,size)
    cdef CLASS_ATTR_LONG_ARRAY(c,attrname,flags,structname,structmember,size)
    cdef CLASS_ATTR_ATOM_LONG_ARRAY(c,attrname,flags,structname,structmember,size)
    cdef CLASS_ATTR_FLOAT_ARRAY(c,attrname,flags,structname,structmember,size)
    cdef CLASS_ATTR_DOUBLE_ARRAY(c,attrname,flags,structname,structmember,size)
    cdef CLASS_ATTR_SYM_ARRAY(c,attrname,flags,structname,structmember,size)
    cdef CLASS_ATTR_ATOM_ARRAY(c,attrname,flags,structname,structmember,size)
    cdef CLASS_ATTR_OBJ_ARRAY(c,attrname,flags,structname,structmember,size)
    cdef CLASS_ATTR_CHAR_VARSIZE(c,attrname,flags,structname,structmember,sizemember,maxsize)
    cdef CLASS_ATTR_LONG_VARSIZE(c,attrname,flags,structname,structmember,sizemember,maxsize)
    cdef CLASS_ATTR_FLOAT_VARSIZE(c,attrname,flags,structname,structmember,sizemember,maxsize)
    cdef CLASS_ATTR_DOUBLE_VARSIZE(c,attrname,flags,structname,structmember,sizemember,maxsize)
    cdef CLASS_ATTR_SYM_VARSIZE(c,attrname,flags,structname,structmember,sizemember,maxsize)
    cdef CLASS_ATTR_ATOM_VARSIZE(c,attrname,flags,structname,structmember,sizemember,maxsize)
    cdef CLASS_ATTR_OBJ_VARSIZE(c,attrname,flags,structname,structmember,sizemember,maxsize)
    cdef STRUCT_ATTR_CHAR(c,flags,structname,structmember)
    cdef STRUCT_ATTR_LONG(c,flags,structname,structmember)
    cdef STRUCT_ATTR_ATOM_LONG(c,flags,structname,structmember)
    cdef STRUCT_ATTR_FLOAT(c,flags,structname,structmember)
    cdef STRUCT_ATTR_DOUBLE(c,flags,structname,structmember)
    cdef STRUCT_ATTR_SYM(c,flags,structname,structmember)
    cdef STRUCT_ATTR_ATOM(c,flags,structname,structmember)
    cdef STRUCT_ATTR_OBJ(c,flags,structname,structmember)
    cdef STRUCT_ATTR_CHAR_ARRAY(c,flags,structname,structmember,size)
    cdef STRUCT_ATTR_LONG_ARRAY(c,flags,structname,structmember,size)
    cdef STRUCT_ATTR_FLOAT_ARRAY(c,flags,structname,structmember,size)
    cdef STRUCT_ATTR_DOUBLE_ARRAY(c,flags,structname,structmember,size)
    cdef STRUCT_ATTR_SYM_ARRAY(c,flags,structname,structmember,size)
    cdef STRUCT_ATTR_ATOM_ARRAY(c,flags,structname,structmember,size)
    cdef STRUCT_ATTR_OBJ_ARRAY(c,flags,structname,structmember,size)
    cdef STRUCT_ATTR_CHAR_VARSIZE(c,flags,structname,structmember,sizemember,maxsize)
    cdef STRUCT_ATTR_LONG_VARSIZE(c,flags,structname,structmember,sizemember,maxsize)
    cdef STRUCT_ATTR_FLOAT_VARSIZE(c,flags,structname,structmember,sizemember,maxsize)
    cdef STRUCT_ATTR_DOUBLE_VARSIZE(c,flags,structname,structmember,sizemember,maxsize)
    cdef STRUCT_ATTR_SYM_VARSIZE(c,flags,structname,structmember,sizemember,maxsize)
    cdef STRUCT_ATTR_ATOM_VARSIZE(c,flags,structname,structmember,sizemember,maxsize)
    cdef STRUCT_ATTR_OBJ_VARSIZE(c,flags,structname,structmember,sizemember,maxsize)
    # cdef STATIC_ATTR_ATOMS
    # cdef STATIC_ATTR_PARSE
    # cdef STATIC_ATTR_FORMAT
    cdef STATIC_ATTR_CHAR(c,attrname,flags,val)
    cdef STATIC_ATTR_LONG(c,attrname,flags,val)
    cdef STATIC_ATTR_FLOAT(c,attrname,flags,val)
    cdef STATIC_ATTR_DOUBLE(c,attrname,flags,val)
    cdef STATIC_ATTR_SYM(c,attrname,flags,val)
    cdef STATIC_ATTR_ATOM(c,attrname,flags,val)
    cdef STATIC_ATTR_OBJ(c,attrname,flags,val)
    cdef STATIC_ATTR_CHAR_ARRAY(c,attrname,flags,count,vals)
    cdef STATIC_ATTR_LONG_ARRAY(c,attrname,flags,count,vals)
    cdef STATIC_ATTR_FLOAT_ARRAY(c,attrname,flags,count,vals)
    cdef STATIC_ATTR_DOUBLE_ARRAY(c,attrname,flags,count,vals)
    cdef STATIC_ATTR_SYM_ARRAY(c,attrname,flags,count,vals)
    # cdef STATIC_ATTR_ATOM_ARRAY
    cdef STATIC_ATTR_OBJ_ARRAY(c,attrname,flags,count,vals)
    # cdef OBJ_ATTR_ATOMS
    # cdef OBJ_ATTR_PARSE
    # cdef OBJ_ATTR_FORMAT
    cdef OBJ_ATTR_CHAR(x,attrname,flags,val)
    cdef OBJ_ATTR_LONG(x,attrname,flags,val)
    cdef OBJ_ATTR_FLOAT(x,attrname,flags,val)
    cdef OBJ_ATTR_DOUBLE(x,attrname,flags,val)
    cdef OBJ_ATTR_SYM(x,attrname,flags,val)
    cdef OBJ_ATTR_ATOM(x,attrname,flags,val)
    cdef OBJ_ATTR_OBJ(x,attrname,flags,val)
    cdef OBJ_ATTR_CHAR_ARRAY(x,attrname,flags,count,vals)
    cdef OBJ_ATTR_LONG_ARRAY(x,attrname,flags,count,vals)
    cdef OBJ_ATTR_FLOAT_ARRAY(x,attrname,flags,count,vals)
    cdef OBJ_ATTR_DOUBLE_ARRAY(x,attrname,flags,count,vals)
    cdef OBJ_ATTR_SYM_ARRAY(x,attrname,flags,count,vals)
    # cdef OBJ_ATTR_ATOM_ARRAY
    cdef OBJ_ATTR_OBJ_ARRAY(x,attrname,flags,count,vals)
    cdef CLASS_ATTR_ACCESSORS(c,attrname,getter,setter)
    cdef CLASS_ATTR_ADD_FLAGS(c,attrname,flags)
    cdef CLASS_ATTR_REMOVE_FLAGS(c,attrname,flags)
    cdef CLASS_ATTR_FILTER_MIN(c,attrname,minval)
    cdef CLASS_ATTR_FILTER_MAX(c,attrname,maxval)
    cdef CLASS_ATTR_FILTER_CLIP(c,attrname,minval,maxval)
    cdef CLASS_ATTR_ALIAS(c,attrname,aliasname)
    # cdef CLASS_ATTR_ATTR_ATOMS
    # cdef CLASS_ATTR_ATTR_PARSE
    # cdef CLASS_ATTR_ATTR_FORMAT
    cdef CLASS_ATTR_DEFAULT(c,attrname,flags,parsestr)
    cdef CLASS_ATTR_SAVE(c,attrname,flags)
    cdef CLASS_ATTR_SELFSAVE(c,attrname,flags)
    cdef CLASS_ATTR_DEFAULT_SAVE(c,attrname,flags,parsestr)
    cdef CLASS_ATTR_DEFAULTNAME(c,attrname,flags,parsestr)

    cdef CLASS_ATTR_DEFAULTNAME_SAVE(c,attrname,flags,parsestr)
    cdef CLASS_ATTR_MIN(c,attrname,flags,parsestr)
    cdef CLASS_ATTR_MAX(c,attrname,flags,parsestr)
    cdef CLASS_ATTR_PAINT(c,attrname,flags)
    cdef CLASS_ATTR_DEFAULT_PAINT(c,attrname,flags,parsestr)
    cdef CLASS_ATTR_DEFAULT_SAVE_PAINT(c,attrname,flags,parsestr)
    cdef CLASS_ATTR_DEFAULTNAME_PAINT(c,attrname,flags,parsestr)
    cdef CLASS_ATTR_DEFAULTNAME_SAVE_PAINT(c,attrname,flags,parsestr)
    cdef CLASS_ATTR_STYLE(c,attrname,flags,parsestr)
    cdef CLASS_ATTR_LABEL(c,attrname,flags,labelstr)
    cdef CLASS_ATTR_ENUM(c,attrname,flags,parsestr)
    cdef CLASS_ATTR_ENUMINDEX(c,attrname,flags,parsestr)
    cdef CLASS_ATTR_ENUMINDEX2(c,attrname,flags,enum1,enum2)
    cdef CLASS_ATTR_ENUMINDEX3(c,attrname,flags,enum1,enum2,enum3)
    cdef CLASS_ATTR_ENUMINDEX4(c,attrname,flags,enum1,enum2,enum3,enum4)
    cdef CLASS_ATTR_ENUMINDEX5(c,attrname,flags,enum1,enum2,enum3,enum4,enum5)
    cdef CLASS_ATTR_ENUMINDEX6(c,attrname,flags,enum1,enum2,enum3,enum4,enum5,enum6)
    cdef CLASS_ATTR_ENUMINDEX7(c,attrname,flags,enum1,enum2,enum3,enum4,enum5,enum6,enum7)
    cdef CLASS_ATTR_CATEGORY(c,attrname,flags,parsestr)
    cdef CLASS_ATTR_STYLE_LABEL(c,attrname,flags,stylestr,labelstr)
    cdef CLASS_ATTR_INVISIBLE(c,attrname,flags)
    cdef CLASS_ATTR_ORDER(c,attrname,flags,parsestr)
    cdef CLASS_ATTR_BASIC(c,attrname,flags)
    cdef CLASS_ATTR_ATOMARRAY(c,attrname,flags)
    cdef CLASS_METHOD_ATTR_PARSE(c,methodname,attrname,type,flags,parsestring)
    cdef CLASS_ATTR_LEGACYDEFAULT(c,legacyattrname,newattrname,flags,parsestr)
    cdef CLASS_ATTR_OBSOLETE(c,attrname,flags)
    cdef CLASS_ATTR_RENAMED(c,oldname,newname,flags)
    cdef CLASS_ATTR_INTRODUCED(c,attrname,flags,versionstr)
    cdef CLASS_METHOD_OBSOLETE(c,methodname,flags)
    cdef CLASS_METHOD_RENAMED(c,oldname,newname,flags)
    cdef CLASS_METHOD_INTRODUCED(c,methodname,flags,versionstr)
    # cdef OBJ_ATTR_ATTR_ATOMS
    # cdef OBJ_ATTR_ATTR_PARSE
    # cdef OBJ_ATTR_ATTR_FORMAT
    cdef OBJ_ATTR_DEFAULT(x,attrname,flags,parsestr)
    cdef OBJ_ATTR_SAVE(x,attrname,flags)
    cdef OBJ_ATTR_DEFAULT_SAVE(x,attrname,flags,parsestr)
    cdef CLASS_STICKY_ATTR(c,name,flags,parsestr)
    cdef CLASS_STICKY_ATTR_CLEAR(c,name)
    cdef CLASS_STICKY_CATEGORY(c,flags,name)
    cdef CLASS_STICKY_CATEGORY_CLEAR(c)
    cdef CLASS_STICKY_METHOD(c,name,flags,parsestr)
    cdef CLASS_STICKY_METHOD_CLEAR(c,name)
    cdef int OBEX_UTIL_MAX_ATOM_GETBYTES
    cdef int OBEX_UTIL_MAX_ATOM_STATIC
    # cdef OBEX_UTIL_ATOM_SETUP_VAR_STATIC
    # cdef OBEX_UTIL_ATOM_CLEANUP_VAR_STATIC
    cdef OBEX_UTIL_ATOM_SETUP_ARRAY_STATIC(ac)
    cdef OBEX_UTIL_ATOM_CLEANUP_ARRAY_STATIC(ac)
    # cdef OBEX_UTIL_ATOM_SETUP_VAR_DYN
    # cdef OBEX_UTIL_ATOM_CLEANUP_VAR_DYN
    cdef OBEX_UTIL_ATOM_SETUP_ARRAY_DYN(ac)
    cdef OBEX_UTIL_ATOM_CLEANUP_ARRAY_DYN(ac)
    # cdef OBEX_UTIL_ATOM_SETUP_VAR_COMBO
    # cdef OBEX_UTIL_ATOM_CLEANUP_VAR_COMBO
    cdef OBEX_UTIL_ATOM_SETUP_ARRAY_COMBO(ac)
    cdef OBEX_UTIL_ATOM_CLEANUP_ARRAY_COMBO(ac)
    # cdef OBEX_UTIL_ATOM_SETUP_VAR
    # cdef OBEX_UTIL_ATOM_CLEANUP_VAR
    # cdef OBEX_UTIL_ATOM_SETUP_ARRAY
    # cdef OBEX_UTIL_ATOM_CLEANUP_ARRAY
    ctypedef enum e_max_atom_gettext_flags:
        OBEX_UTIL_ATOM_GETTEXT_DEFAULT =            0x00000000
        OBEX_UTIL_ATOM_GETTEXT_TRUNCATE_ZEROS =     0x00000001
        OBEX_UTIL_ATOM_GETTEXT_SYM_NO_QUOTE =       0x00000002
        OBEX_UTIL_ATOM_GETTEXT_SYM_FORCE_QUOTE =    0x00000004
        OBEX_UTIL_ATOM_GETTEXT_COMMA_DELIM =        0x00000008
        OBEX_UTIL_ATOM_GETTEXT_FORCE_ZEROS =        0x00000010
        OBEX_UTIL_ATOM_GETTEXT_NUM_HI_RES =         0x00000020
        OBEX_UTIL_ATOM_GETTEXT_NUM_LO_RES =         0x00000040

    cdef t_max_err atom_setchar_array(long ac, t_atom *av, long count, unsigned char *vals)
    cdef t_max_err atom_setlong_array(long ac, t_atom *av, long count, t_atom_long *vals)
    cdef t_max_err atom_setfloat_array(long ac, t_atom *av, long count, float *vals)
    cdef t_max_err atom_setdouble_array(long ac, t_atom *av, long count, double *vals)
    cdef t_max_err atom_setsym_array(long ac, t_atom *av, long count, t_symbol **vals)
    cdef t_max_err atom_setatom_array(long ac, t_atom *av, long count, t_atom *vals)
    cdef t_max_err atom_setobj_array(long ac, t_atom *av, long count, t_object **vals)
    cdef t_max_err atom_setparse(long *ac, t_atom **av, const char *parsestr)
    cdef t_max_err atom_setbinbuf(long *ac, t_atom **av, void *buf)
    cdef t_max_err atom_setattrval(long *ac, t_atom **av, t_symbol *attrname, t_object *obj)
    cdef t_max_err atom_setobjval(long *ac, t_atom **av, t_object *obj)
    cdef t_max_err atom_setformat(long *ac, t_atom **av, const char *fmt, ...)
    #cdef t_max_err atom_setformat_va(long *ac, t_atom **av, const char *fmt, va_list args)
    cdef t_max_err atom_getformat(long ac, t_atom *av, const char *fmt, ...)
    #cdef t_max_err atom_getformat_va(long ac, t_atom *av, const char *fmt, va_list args)
    cdef t_max_err atom_gettext(long ac, t_atom *av, long *textsize, char **text, long flags)
    cdef t_max_err atom_getchar_array(long ac, t_atom *av, long count, unsigned char *vals)
    cdef t_max_err atom_getlong_array(long ac, t_atom *av, long count, t_atom_long *vals)
    cdef t_max_err atom_getfloat_array(long ac, t_atom *av, long count, float *vals)
    cdef t_max_err atom_getdouble_array(long ac, t_atom *av, long count, double *vals)
    cdef t_max_err atom_getsym_array(long ac, t_atom *av, long count, t_symbol **vals)
    cdef t_max_err atom_getatom_array(long ac, t_atom *av, long count, t_atom *vals)
    cdef t_max_err atom_getobj_array(long ac, t_atom *av, long count, t_object **vals)
    cdef long atomisstring(const t_atom *a)
    cdef long atomisatomarray(t_atom *a)
    cdef long atomisdictionary(t_atom *a)
    cdef OB_MSG(t_object *x, const char p)
    cdef t_max_err object_method_parse(t_object *x, t_symbol *s, const char *parsestr, t_atom *rv)
    cdef t_max_err object_method_binbuf(t_object *x, t_symbol *s, void *buf, t_atom *rv)
    cdef t_max_err object_method_attrval(t_object *x, t_symbol *s, t_symbol *attrname, t_object *obj, t_atom *rv)
    cdef t_max_err object_method_objval(t_object *x, t_symbol *s, t_object *obj, t_atom *rv)
    cdef t_max_err object_method_format(t_object *x, t_symbol *s, t_atom *rv, const char *fmt, ...)
    cdef t_max_err object_method_char(t_object *x, t_symbol *s, unsigned char v, t_atom *rv)
    cdef t_max_err object_method_long(t_object *x, t_symbol *s, long v, t_atom *rv)
    cdef t_max_err object_method_float(t_object *x, t_symbol *s, float v, t_atom *rv)
    cdef t_max_err object_method_double(t_object *x, t_symbol *s, double v, t_atom *rv)
    cdef t_max_err object_method_sym(t_object *x, t_symbol *s, t_symbol *v, t_atom *rv)
    cdef t_max_err object_method_obj(t_object *x, t_symbol *s, t_object *v, t_atom *rv)
    cdef t_max_err object_method_char_array(t_object *x, t_symbol *s, long ac, unsigned char *av, t_atom *rv)
    cdef t_max_err object_method_long_array(t_object *x, t_symbol *s, long ac, t_atom_long *av, t_atom *rv)
    cdef t_max_err object_method_float_array(t_object *x, t_symbol *s, long ac, float *av, t_atom *rv)
    cdef t_max_err object_method_double_array(t_object *x, t_symbol *s, long ac, double *av, t_atom *rv)
    cdef t_max_err object_method_sym_array(t_object *x, t_symbol *s, long ac, t_symbol **av, t_atom *rv)
    cdef t_max_err object_method_obj_array(t_object *x, t_symbol *s, long ac, t_object **av, t_atom *rv)
    cdef t_max_err call_method_typed(method m, t_object *x, t_symbol *s, long ac, t_atom *av, t_atom *rv)
    cdef t_max_err call_method_parse(method m, t_object *x, t_symbol *s, char *parsestr, t_atom *rv)
    cdef t_max_err call_method_binbuf(method m, t_object *x, t_symbol *s, void *buf, t_atom *rv)
    cdef t_max_err call_method_attrval(method m, t_object *x, t_symbol *s, t_symbol *attrname, t_object *obj, t_atom *rv)
    cdef t_max_err call_method_objval(method m, t_object *x, t_symbol *s, t_object *obj, t_atom *rv)
    cdef t_max_err call_method_format(method m, t_object *x, t_symbol *s, t_atom *rv, char *fmt, ...)
    cdef t_max_err call_method_char(method m, t_object *x, t_symbol *s, unsigned char v, t_atom *rv)
    cdef t_max_err call_method_long(method m, t_object *x, t_symbol *s, long v, t_atom *rv)
    cdef t_max_err call_method_float(method m, t_object *x, t_symbol *s,float v, t_atom *rv)
    cdef t_max_err call_method_double(method m, t_object *x, t_symbol *s, double v, t_atom *rv)
    cdef t_max_err call_method_sym(method m, t_object *x, t_symbol *s, t_symbol *v, t_atom *rv)
    cdef t_max_err call_method_obj(method m, t_object *x, t_symbol *s, t_object *v, t_atom *rv)
    cdef t_max_err call_method_char_array(method m, t_object *x, t_symbol *s, long ac, unsigned char *av, t_atom *rv)
    cdef t_max_err call_method_long_array(method m, t_object *x, t_symbol *s, long ac, t_atom_long *av, t_atom *rv)
    cdef t_max_err call_method_float_array(method m, t_object *x, t_symbol *s, long ac, float *av, t_atom *rv)
    cdef t_max_err call_method_double_array(method m, t_object *x, t_symbol *s, long ac, double *av, t_atom *rv)
    cdef t_max_err call_method_sym_array(method m, t_object *x, t_symbol *s, long ac, t_symbol **av, t_atom *rv)
    cdef t_max_err call_method_obj_array(method m, t_object *x, t_symbol *s, long ac, t_object **av, t_atom *rv)
    cdef t_max_err object_attr_setparse(t_object *x, t_symbol *s, const char *parsestr)
    cdef t_max_err object_attr_setbinbuf(t_object *x, t_symbol *s, void *buf)
    cdef t_max_err object_attr_setattrval(t_object *x, t_symbol *s, t_symbol *attrname, t_object *obj)
    cdef t_max_err object_attr_setobjval(t_object *x, t_symbol *s, t_object *obj)
    cdef t_max_err object_attr_setformat(t_object *x, t_symbol *s, const char *fmt, ...)
    cdef t_object *attribute_new_atoms(const char *attrname, t_symbol *type, long flags, long ac, t_atom *av)
    cdef t_object *attribute_new_parse(const char *attrname, t_symbol *type, long flags, const char *parsestr)
    cdef t_object *attribute_new_binbuf(const char *attrname, t_symbol *type, long flags, void *buf)
    cdef t_object *attribute_new_attrval(const char *attrname, t_symbol *type, long flags, t_symbol *objattrname, t_object *obj)
    cdef t_object *attribute_new_objval(const char *attrname, t_symbol *type, long flags, t_object *obj)
    cdef t_object *attribute_new_format(const char *attrname, t_symbol *type, long flags, const char *fmt, ...)
    cdef void *object_new_parse(t_symbol *name_space, t_symbol *classname, const char *parsestr)
    cdef void *object_new_binbuf(t_symbol *name_space, t_symbol *classname, void *buf)
    cdef void *object_new_attrval(t_symbol *name_space, t_symbol *classname, t_symbol *objattrname, t_object *obj)
    cdef void *object_new_objval(t_symbol *name_space, t_symbol *classname, t_object *obj)
    cdef void *object_new_format(t_symbol *name_space, t_symbol *classname, const char *fmt, ...)
    cdef t_max_err object_attr_addattr(t_object *x, t_symbol *attrname, t_object *attr)
    cdef t_object *object_attr_attr_get(t_object *x, t_symbol *attrname, t_symbol *attrname2)
    cdef t_max_err object_attr_attr_setvalueof(t_object *x, t_symbol *attrname, t_symbol *attrname2, long argc, t_atom *argv)
    cdef t_max_err object_attr_attr_getvalueof(t_object *x, t_symbol *attrname, t_symbol *attrname2, long *argc, t_atom **argv)
    cdef t_max_err class_attr_addattr(t_class *c, t_symbol *attrname, t_object *attr)
    cdef t_object *class_attr_attr_get(t_class *c, t_symbol *attrname, t_symbol *attrname2)
    cdef t_max_err class_attr_attr_setvalueof(t_class *c, t_symbol *attrname, t_symbol *attrname2, long argc, t_atom *argv)
    cdef t_max_err class_attr_attr_getvalueof(t_class *c, t_symbol *attrname, t_symbol *attrname2, long *argc, t_atom **argv)
    cdef t_max_err object_attr_enforcelocal(t_object *x, t_symbol *attrname)
    cdef t_max_err class_addattr_atoms(t_class *c, const char *attrname, t_symbol *type, long flags, long ac, t_atom *av)
    cdef t_max_err class_addattr_parse(t_class *c, const char *attrname, t_symbol *type, long flags, const char *parsestr)
    cdef t_max_err class_addattr_format(t_class *c, const char *attrname, t_symbol *type, long flags, const char *fmt, ...)
    cdef t_max_err class_attr_addattr_atoms(t_class *c, const char *attrname, const char *attrname2, t_symbol *type, long flags, long ac, t_atom *av)
    cdef t_max_err class_attr_addattr_parse(t_class *c, const char *attrname, const char *attrname2, t_symbol *type, long flags, const char *parsestr)
    cdef t_max_err class_attr_addattr_format(t_class *c, const char *attrname, const char *attrname2, const t_symbol *type, long flags, const char *fmt, ...)
    cdef t_max_err object_addattr_atoms(t_object *x, const char *attrname, t_symbol *type, long flags, long ac, t_atom *av)
    cdef t_max_err object_addattr_parse(t_object *x, const char *attrname, t_symbol *type, long flags, const char *parsestr)
    cdef t_max_err object_addattr_format(t_object *x, const char *attrname, t_symbol *type, long flags, const char *fmt, ...)
    cdef t_max_err object_attr_addattr_atoms(t_object *x, const char *attrname, const char *attrname2, t_symbol *type, long flags, long ac, t_atom *av)
    cdef t_max_err object_attr_addattr_parse(t_object *x, const char *attrname, const char *attrname2, t_symbol *type, long flags, const char *parsestr)
    cdef t_max_err object_attr_addattr_format(t_object *x, const char *attrname, const char *attrname2, t_symbol *type, long flags, const char *fmt, ...)
    cdef t_object *object_clone(t_object *x)
    cdef t_object *object_clone_generic(t_object *x)
    cdef void object_zero(t_object *x)
    cdef t_max_err class_addcommand(t_class *c, method cmd, method enabler, method handler, const char *message)
    cdef void *object_commandenabled(t_object *o, t_symbol *cmd)
    cdef t_max_err object_getenabler(t_object *c, t_symbol *cmd, method *m)
    cdef t_max_err object_getcommand(t_object *o, t_symbol *cmd, method *m)
    cdef void *object_handlecommand(t_object *o, t_symbol *s, long argc, t_atom *argv, t_atom *rv)
    cdef t_ptr_int object_attr_getdisabled(t_object *o, t_symbol *attrname)
    cdef t_max_err object_attr_setdisabled(t_object *o, t_symbol *attrname, long way)
    cdef t_max_err object_replaceargs(t_object *x, long argc, t_atom *argv, char match, char poundfill)
    cdef t_max_err object_attr_obsolete_getter(t_object *x, t_object *attr, long *ac, t_atom **av)
    cdef t_max_err object_attr_obsolete_setter(t_object *x, t_object *attr, long ac, t_atom *av)
    cdef void object_method_obsolete(t_object *x, t_symbol *s, long ac, t_atom *av)

cdef extern from "ext_obstring.h":

    ctypedef struct t_string:
        t_object s_obj
        char     *s_text
        long     s_size

    cdef t_string* string_new(const char *psz)
    cdef const char* string_getptr(t_string *x)
    cdef void string_reserve(t_string *x, long numbytes)
    cdef void string_append(t_string *x, const char *s)
    cdef void string_chop(t_string *x, long numchars)


cdef extern from "ext_database.h":
    ctypedef t_object t_database
    ctypedef t_object t_db_result
    ctypedef t_object t_db_view

    cdef t_max_err db_open(t_symbol *dbname, const char *fullpath, t_database **db)
    ctypedef enum t_db_open_flags:
        DB_OPEN_FLAGS_NONE = 0
        DB_OPEN_FLAGS_READONLY = 0x01

    cdef t_max_err db_open_ext(t_symbol *dbname, const char *fullpath, t_database **db, long flags)
    cdef t_max_err db_close(t_database **db)
    cdef t_max_err db_query(t_database *db, t_db_result **dbresult, const char *sql, ...)
    cdef t_max_err db_query_direct(t_database *db, t_db_result **dbresult, const char *sql)
    cdef t_max_err db_query_silent(t_database *db, t_db_result **dbresult, const char *sql, ...)
    cdef t_max_err db_query_getlastinsertid(t_database *db, long *id)
    cdef t_max_err db_query_table_new(t_database *db, const char *tablename)
    cdef t_max_err db_query_table_addcolumn(t_database *db, const char *tablename, const char *columnname, const char *columntype, const char *flags)
    cdef t_max_err db_transaction_start(t_database *db)
    cdef t_max_err db_transaction_end(t_database *db)
    cdef t_max_err db_transaction_flush(t_database *db)
    cdef t_max_err db_view_create(t_database *db, const char *sql, t_db_view **dbview)
    cdef t_max_err db_view_remove(t_database *db, t_db_view **dbview)
    cdef t_max_err db_view_getresult(t_db_view *dbview, t_db_result **result)
    cdef t_max_err db_view_setquery(t_db_view *dbview, char *newquery)
    cdef char** db_result_nextrecord(t_db_result *result)
    cdef void db_result_reset(t_db_result *result)
    cdef void db_result_clear(t_db_result *result)
    cdef long db_result_numrecords(t_db_result *result)
    cdef long db_result_numfields(t_db_result *result)
    cdef char* db_result_fieldname(t_db_result *result, long fieldindex)
    cdef char* db_result_string(t_db_result *result, long recordindex, long fieldindex)
    cdef long db_result_long(t_db_result *result, long recordindex, long fieldindex)
    cdef float db_result_float(t_db_result *result, long recordindex, long fieldindex)
    cdef t_ptr_uint db_result_datetimeinseconds(t_db_result *result, long recordindex, long fieldindex)
    cdef void db_util_stringtodate(const char *string, t_ptr_uint *date)
    cdef void db_util_datetostring(const t_ptr_uint date, char *string)


cdef extern from "ext_default.h":
    cdef t_max_err patcher_setdefault(t_object *patcher, t_symbol *key, long argc, t_atom *argv)
    cdef t_max_err patcher_getdefault(t_object *patcher, t_symbol *key, long *argc, t_atom *argv)
    cdef t_max_err patcher_removedefault(t_object *patcher, t_symbol *key)


cdef extern from "ext_parameter.h":

    cdef long PARAMETER_METHOD_FLAG_OVERRIDE
    cdef long PARAMETER_METHOD_FLAG_PRE
    cdef long PARAMETER_METHOD_FLAG_POST
    cdef long PARAMETER_METHOD_FLAG_FULL
    cdef long PARAMETER_METHOD_FLAG_DONOTHING
    cdef long PARAMETER_GESTURE_INDEX

    ctypedef enum PARAM_TYPE:
        PARAM_TYPE_INVALID = -1
        PARAM_TYPE_FLOAT = 0
        PARAM_TYPE_INT
        PARAM_TYPE_ENUM
        PARAM_TYPE_BLOB
        PARAM_TYPE_FILE

    ctypedef enum PARAM_TYPE_ENABLE:
        PARAM_TYPE_ENABLE_OFF
        PARAM_TYPE_ENABLE_ON
        PARAM_TYPE_ENABLE_BLOB

    ctypedef enum PARAM_UNITSTYLE:
        PARAM_UNITSTYLE_INVALID = -1
        PARAM_UNITSTYLE_INT
        PARAM_UNITSTYLE_FLOAT
        PARAM_UNITSTYLE_MS
        PARAM_UNITSTYLE_HZ
        PARAM_UNITSTYLE_DB
        PARAM_UNITSTYLE_PERCENT
        PARAM_UNITSTYLE_PAN
        PARAM_UNITSTYLE_SEMITONE
        PARAM_UNITSTYLE_MIDINOTE
        PARAM_UNITSTYLE_CUSTOM
        PARAM_UNITSTYLE_NATIVE

    ctypedef enum PARAM_MOD:
        PARAM_MOD_INVALID = -1
        PARAM_MOD_NONE
        PARAM_MOD_UNI
        PARAM_MOD_BI
        PARAM_MOD_ADD
        PARAM_MOD_ABS

    ctypedef enum PARAM_DATA_TYPE:
        PARAM_DATA_TYPE_INVALID = -1
        PARAM_DATA_TYPE_VALUE
        PARAM_DATA_TYPE_TYPE
        PARAM_DATA_TYPE_TYPE_ENABLE
        PARAM_DATA_TYPE_ORDER
        PARAM_DATA_TYPE_LONGNAME
        PARAM_DATA_TYPE_SHORTNAME
        PARAM_DATA_TYPE_MIN
        PARAM_DATA_TYPE_MAX
        PARAM_DATA_TYPE_ENUM
        PARAM_DATA_TYPE_MODMODE
        PARAM_DATA_TYPE_MODMIN
        PARAM_DATA_TYPE_MODMAX
        PARAM_DATA_TYPE_INITIAL_ENABLE
        PARAM_DATA_TYPE_INITIAL
        PARAM_DATA_TYPE_INITIAL_EDITABLE
        PARAM_DATA_TYPE_UNITSTYLE
        PARAM_DATA_TYPE_EXPONENT
        PARAM_DATA_TYPE_STEPS
        PARAM_DATA_TYPE_SPEEDLIM
        PARAM_DATA_TYPE_SMOOTHING
        PARAM_DATA_TYPE_UNITS
        PARAM_DATA_TYPE_INFO
        PARAM_DATA_TYPE_FOCUS
        PARAM_DATA_TYPE_INVISIBLE
        PARAM_DATA_TYPE_AUTOMATION_VALUE
        PARAM_DATA_TYPE_AUTOMATION_STATE
        PARAM_DATA_TYPE_MODULATION_VALUE
        PARAM_DATA_TYPE_DIRTY
        PARAM_DATA_TYPE_ASSIGNMENT_TEXT_MIDI
        PARAM_DATA_TYPE_ASSIGNMENT_TEXT_KEY
        PARAM_DATA_TYPE_ASSIGNMENT_TEXT_MACRO
        PARAM_DATA_TYPE_LEARNING_MODE
        PARAM_DATA_TYPE_FILEREF_PATH
        PARAM_DATA_TYPE_FILEREF_DISPLAYPATH
        PARAM_DATA_TYPE_FILEREF_DISPLAYNAME
        PARAM_DATA_TYPE_DEVICESTATE
        PARAM_DATA_TYPE_DEFER
        PARAM_DATA_TYPE_MAPPING_INDEX
        PARAM_DATA_TYPE_NOBLOBCACHE

    ctypedef enum PARAM_VALUE_SET_TYPE:
        PARAM_VALUE_SET_DISTANCE = 0
        PARAM_VALUE_SET_REAL
        PARAM_VALUE_SET_GETONLY
        PARAM_VALUE_SET_DISTANCE_NONOTIFY
        PARAM_VALUE_SET_REAL_NONOTIFY

    ctypedef enum PARAM_VALUE_GET_TYPE:
        PARAM_VALUE_GET_OUTPUT = 0
        PARAM_VALUE_GET_AUTOMATION
        PARAM_VALUE_GET_MODULATION

    ctypedef enum PARAM_VALUE_FORMAT:
        PARAM_VALUE_DISTANCE = 0
        PARAM_VALUE_LINEAR
        PARAM_VALUE_REAL

    ctypedef enum PARAM_AUTOMATION_STATE:
        PARAM_AUTOMATION_STATE_NONE         = 0x00
        PARAM_AUTOMATION_STATE_AUTOMATED    = 0x01
        PARAM_AUTOMATION_STATE_AUTOENABLED  = 0x02
        PARAM_AUTOMATION_STATE_IRRELEVANT   = 0x04
        PARAM_AUTOMATION_STATE_DISABLED     = 0x08
        PARAM_AUTOMATION_STATE_MACRO        = 0x10

    ctypedef enum PARAM_DEVICESTATE:
        PARAM_DEVICESTATE_ACTIVE            = 0
        PARAM_DEVICESTATE_INACTIVE          = 1

    ctypedef enum PARAM_LEARNING_TYPE:
        PARAM_LEARNING_TYPE_NONE = 0
        PARAM_LEARNING_TYPE_MIDI
        PARAM_LEARNING_TYPE_KEY
        PARAM_LEARNING_TYPE_MACRO

    ctypedef struct t_parameter_notify_data: pass

    ctypedef struct t_param_class_defcolor_data: pass

    cdef t_max_err class_parameter_init(t_class *c)
    cdef t_max_err object_parameter_init(t_object *x, PARAM_TYPE type)
    cdef t_max_err object_parameter_dictionary_process(t_object *x, t_dictionary *d)
    cdef t_max_err object_parameter_free(t_object *x)
    cdef t_bool object_parameter_notify(t_object *x, t_symbol *s, t_symbol *msg, void *sender, void *data, t_parameter_notify_data *pnd)
    cdef t_max_err object_parameter_getinfo(t_object *x, PARAM_DATA_TYPE type, long *ac, t_atom **av)
    cdef t_max_err object_parameter_setinfo(t_object *x, PARAM_DATA_TYPE type, long ac, t_atom *av)
    cdef t_max_err object_parameter_string_get(t_object *x, double val, char **outstr)
    cdef t_max_err object_parameter_stringtovalue(t_object *x, double *value, char *str)
    cdef t_max_err object_parameter_value_set(t_object *x, PARAM_VALUE_SET_TYPE how, double *linear, double *real, char blobnotify)
    cdef t_max_err object_parameter_value_get(t_object *x, PARAM_VALUE_GET_TYPE how, PARAM_VALUE_FORMAT what, double *value)
    cdef t_max_err object_parameter_current_to_initial(t_object *x)
    # cdef t_max_err object_parameter_color_get(t_object *x, t_symbol *s, t_jrgba *jrgba)
    cdef t_max_err object_parameter_value_setvalueof(t_object *x, long ac, t_atom *av, char blobnotify)
    cdef t_max_err object_parameter_value_setvalueof_nonotify(t_object *x, long ac, t_atom *av, char blobnotify)
    cdef t_max_err object_parameter_value_getvalueof(t_object *x, long *ac, t_atom **av)
    cdef t_max_err object_parameter_value_changed(t_object *x, char blobnotify)
    cdef t_max_err object_parameter_value_changed_nonotify(t_object *x, char blobnotify)
    cdef t_max_err class_parameter_addmethod(t_class *c, method m, char *name, long flags, ...)
    cdef t_max_err parameter_default_int(t_object *x, long n)
    cdef t_max_err parameter_default_float(t_object *x, double d)
    cdef t_max_err parameter_default_anything(t_object *x, t_symbol *s, long ac, t_atom *av)
    cdef t_max_err class_parameter_register_default_color(t_class *c, t_symbol *attrname, t_symbol *colorname)
    cdef t_bool object_parameter_is_initialized(t_object *x)
    cdef t_bool object_parameter_is_in_Live(t_object *x)
    cdef t_bool object_parameter_is_in_maxtilde(t_object *x)
    cdef t_bool object_parameter_is_automated(t_object *x)
    cdef t_max_err object_parameter_wants_focus(t_object *x)
    cdef t_bool object_parameter_is_parameter(t_object *x)
    cdef t_atom_long object_parameter_get_order(t_object *x)

    cdef t_symbol *ps_surface_bg
    cdef t_symbol *ps_control_bg
    cdef t_symbol *ps_control_text_bg
    cdef t_symbol *ps_control_fg
    cdef t_symbol *ps_control_fg_on
    cdef t_symbol *ps_control_fg_off
    cdef t_symbol *ps_control_selection
    cdef t_symbol *ps_control_zombie
    cdef t_symbol *ps_value_arc
    cdef t_symbol *ps_value_bar
    cdef t_symbol *ps_active_automation
    cdef t_symbol *ps_inactive_automation
    cdef t_symbol *ps_macro_assigned
    cdef t_symbol *ps_contrast_frame
    cdef t_symbol *ps_key_assignment
    cdef t_symbol *ps_midi_assignment
    cdef t_symbol *ps_macro_assignment
    cdef t_symbol *ps_assignment_text_bg
    cdef t_symbol *ps_control_fg_zombie
    cdef t_symbol *ps_value_arc_zombie
    cdef t_symbol *ps_numbox_triangle
    cdef t_symbol *ps_macro_title
    cdef t_symbol *ps_selection
    cdef t_symbol *ps_led_bg
    cdef char *PARAM_COLOR_SURFACE_BG
    cdef char *PARAM_COLOR_CONTROL_BG
    cdef char *PARAM_COLOR_CONTROL_TEXT_BG
    cdef char *PARAM_COLOR_CONTROL_FG
    cdef char *PARAM_COLOR_CONTROL_FG_ON
    cdef char *PARAM_COLOR_CONTROL_FG_OFF
    cdef char *PARAM_COLOR_CONTROL_SELECTION
    cdef char *PARAM_COLOR_CONTROL_ZOMBIE
    cdef char *PARAM_COLOR_VALUE_ARC
    cdef char *PARAM_COLOR_VALUE_BAR
    cdef char *PARAM_COLOR_ACTIVE_AUTOMATION
    cdef char *PARAM_COLOR_INACTIVE_AUTOMATION
    cdef char *PARAM_COLOR_MACRO_ASSIGNED
    cdef char *PARAM_COLOR_CONTRAST_FRAME
    cdef char *PARAM_COLOR_KEY_ASSIGNMENT
    cdef char *PARAM_COLOR_MIDI_ASSIGNMENT
    cdef char *PARAM_COLOR_MACRO_ASSIGNMENT
    cdef char *PARAM_COLOR_ASSIGNMENT_TEXT_BG
    cdef char *PARAM_COLOR_CONTROL_FG_ZOMBIE
    cdef char *PARAM_COLOR_VALUE_ARC_ZOMBIE
    cdef char *PARAM_COLOR_NUMBOX_TRIANGLE
    cdef char *PARAM_COLOR_MACRO_TITLE
    cdef char *PARAM_COLOR_SELECTION
    cdef char *PARAM_COLOR_LED_BG
    cdef int PARAM_COLOR_COUNT


cdef extern from "ext_sysmidi.h":

    ctypedef enum:
        SYSMIDI_ENABLED = 1
        SYSMIDI_DYNAMIC = 2
        SYSMIDI_PERMANENT = 4

    ctypedef struct t_midistate: pass
    ctypedef struct t_midiportinfo: pass
    ctypedef struct t_sysmididriver: pass

    cdef void sysmidi_enqbigpacket(t_midiportinfo *port, t_uint8 *data, double ts, long len, long contFlags)
    cdef long sysmidi_numinports()
    cdef long sysmidi_numoutports()
    cdef t_symbol *sysmidi_indextoname(long index, long io)
    cdef void sysmidi_iterate(method meth, void *arg, long io)
    cdef t_midiportinfo *sysmidi_createport(long id, long refnum, t_symbol *name, t_sysmididriver *dx, long io, long flags)
    cdef void sysmidi_deletemarked(long io)
    cdef t_midiportinfo *sysmidi_idtoport(long id, long io)
    cdef long sysmidi_uniqueid()
    cdef t_midiportinfo *sysmidi_data1toport(void *data, long io)
    cdef t_midiportinfo *sysmidi_nametoport(t_symbol *name, long io)


cdef extern from "ext_itm.h":


    ctypedef t_object t_itm
    ctypedef struct t_clocksource: pass
    ctypedef struct t_tschange: pass

    ctypedef enum:
        TIME_FLAGS_LOCATION = 1
        TIME_FLAGS_TICKSONLY = 2
        TIME_FLAGS_FIXEDONLY = 4
        TIME_FLAGS_LOOKAHEAD = 8
        TIME_FLAGS_USECLOCK = 16
        TIME_FLAGS_USEQELEM = 32
        TIME_FLAGS_FIXED = 64
        TIME_FLAGS_PERMANENT = 128
        TIME_FLAGS_TRANSPORT = 256
        TIME_FLAGS_EVENTLIST = 512
        TIME_FLAGS_CHECKSCHEDULE = 1024
        TIME_FLAGS_LISTENTICKS = 2048
        TIME_FLAGS_NOUNITS = 4096
        TIME_FLAGS_BBUSOURCE = 8192
        TIME_FLAGS_POSITIVE = 16384

    cdef void *itm_getglobal()
    cdef void *itm_getnamed(t_symbol *s, void *scheduler, t_symbol *defaultclocksourcename, long create)
    cdef void *itm_getfromarg(t_object *o, t_symbol *s)
    cdef void itm_reference(t_itm *x)
    cdef void itm_dereference(t_itm *x)
    cdef void itm_deleteeventlist(t_itm *x, t_symbol *eventlist)
    cdef void itm_eventlistseek(t_itm *x)
    cdef void itm_geteventlistnames(t_itm *x, long *count, t_symbol ***names)
    cdef void itm_switcheventlist(t_itm *x, t_symbol *eventlist, double offset)
    cdef double itm_gettime(t_itm *x)
    cdef double itm_getticks(t_itm *x)
    cdef void itm_dump(t_itm *x)
    cdef void itm_sync(t_itm *x)
    cdef void itm_settimesignature(t_itm *x, long num, long denom, long flags)
    cdef void itm_gettimesignature(t_itm *x, long *num, long *denom)
    cdef void itm_seek(t_itm *x, double oldticks, double newticks, long chase)
    cdef void itm_pause(t_itm *x)
    cdef void itm_resume(t_itm *x)
    cdef long itm_getstate(t_itm *x)
    cdef void itm_setresolution(double res)
    cdef double itm_getresolution()
    cdef t_symbol *itm_getname(t_itm *x)
    cdef t_max_err itm_parse(t_itm *x, long argc, t_atom *argv, long flags, double *ticks, double *fixed, t_symbol **unit, long *bbu, char *bbusource)
    cdef double itm_tickstoms(t_itm *x, double ticks)
    cdef double itm_mstoticks(t_itm *x, double ms)
    cdef double itm_mstosamps(t_itm *x, double ms)
    cdef double itm_sampstoms(t_itm *x, double samps)
    cdef void itm_barbeatunitstoticks(t_itm *x, long bars, long beats, double units, double *ticks, char position)
    cdef void itm_tickstobarbeatunits(t_itm *x, double ticks, long *bars, long *beats, double *units, char position)
    cdef void itm_format(t_itm *x, double ms, double ticks, long flags, t_symbol *unit, long *argc, t_atom **argv)
    cdef long itm_isunitfixed(t_symbol *u)
    cdef void itmclock_delay(t_object *x, t_itm *m, t_symbol *eventlist, double delay, long quantization)
    cdef void *itmclock_new(t_object *owner, t_object *timeobj, method task, method killer, long permanent)
    cdef void itmclock_set(t_object *x, t_itm *m, t_symbol *eventlist, double time)
    cdef void itmclock_unset(t_object *x)
    cdef void *itm_clocksource_getnamed(t_symbol *name, long create)
    cdef void itm_getclocksources(long *count, t_symbol ***sources)
    cdef double itm_getsr(t_itm *x)
    cdef double itm_gettempo(t_itm *x)


cdef extern from "ext_time.h":

    ctypedef t_object t_timeobject
    cdef void time_stop(t_timeobject *x)
    cdef void time_tick(t_timeobject *x)
    cdef double time_getms(t_timeobject *x)
    cdef double time_getticks(t_timeobject *x)
    cdef void time_getphase(t_timeobject *tx, double *phase, double *slope, double *ticks)
    cdef void time_listen(t_timeobject *x, t_symbol *attr, long flags)
    cdef void time_setvalue(t_timeobject *tx, t_symbol *s, long argc, t_atom *argv)
    cdef void class_time_addattr(t_class *c, const char *attrname, const char *attrlabel, long flags)
    cdef void *time_new(t_object *owner, t_symbol *attrname, method tick, long flags)
    cdef t_object *time_getnamed(t_object *owner, t_symbol *attrname)
    cdef void time_enable_attributes(t_object *x)
    cdef long time_isfixedunit(t_timeobject *x)
    cdef void time_schedule(t_timeobject *x, t_timeobject *quantize)
    cdef void time_schedule_limit(t_timeobject *x, t_timeobject *quantize)
    cdef void time_now(t_timeobject *x, t_timeobject *quantize)
    cdef void *time_getitm(t_timeobject *ox)
    cdef double time_calcquantize(t_timeobject *ox, t_itm *vitm, t_timeobject *oq)
    cdef void time_setclock(t_timeobject *tx, t_symbol *sc)


cdef extern from "ext_packages.h":

    ctypedef struct t_package_file: pass
    cdef short packages_getpackagepath(const char *packagename)
    cdef t_linklist *packages_createsubpathlist(const char *subfoldername, short includesysfolder)
    cdef t_max_err packages_getsubpathcontents(const char *subfoldername, const char *suffix_selector, short includesysfolder, t_linklist **subpathlist, t_dictionary **names_to_packagefiles)


cdef extern from "ext_sysfile.h":

    ctypedef struct t_filestruct: pass
    ctypedef t_filestruct *t_filehandle

    ctypedef enum t_sysfile_pos_mode:
        SYSFILE_ATMARK = 0
        SYSFILE_FROMSTART = 1
        SYSFILE_FROMLEOF = 2
        SYSFILE_FROMMARK = 3


    ctypedef enum t_sysfile_flags:
        SYSFILE_SUBFILE = 1
        SYSFILE_HANDLE = 2
        SYSFILE_RESOURCE = 4
        SYSFILE_MEMORY = 6
        SYSFILE_RESFILE = 8
        SYSFILE_OPENRESFILE = 16
        SYSFILE_EXTERNALDATA = 32
        SYSFILE_JUSTAPOINTER = 64
        SYSFILE_EXTERNALDATA_CANWRITE = 128
        SYSFILE_EXTERNALDATA_CANGROW = 256
        SYSFILE_EXTERNALDATA_FREE = 512
        SYSFILE_EXTERNALDATA_LATEFREE = 1024


    ctypedef enum t_sysfile_text_flags:
        TEXT_LB_NATIVE =            0x00000001L
        TEXT_LB_MAC =               0x00000002L
        TEXT_LB_PC =                0x00000004L
        TEXT_LB_UNIX =              0x00000008L
        TEXT_LB_MASK = 0x0000000FL
        TEXT_ENCODING_USE_FILE =    0x00000100L
        TEXT_NULL_TERMINATE =       0x00000200L

    cdef t_max_err sysfile_close(t_filehandle f)
    cdef t_max_err sysfile_read( t_filehandle f, t_ptr_size *count, void *bufptr)
    cdef t_max_err sysfile_readtohandle(t_filehandle f, char ***h)
    cdef t_max_err sysfile_readtoptr(t_filehandle f, char **p)
    cdef t_max_err sysfile_write(t_filehandle f, t_ptr_size *count, const void *bufptr)
    cdef t_max_err sysfile_seteof(t_filehandle f, t_ptr_size logeof)
    cdef t_max_err sysfile_geteof(t_filehandle f, t_ptr_size *logeof)
    cdef t_max_err sysfile_setpos(t_filehandle f, t_sysfile_pos_mode mode, t_ptr_int offset)
    cdef t_max_err sysfile_getpos(t_filehandle f, t_ptr_size *filepos)
    cdef t_max_err sysfile_spoolcopy(t_filehandle src, t_filehandle dst, t_ptr_size size)
    cdef void sysfile_setobject(t_filehandle f, t_object *o)
    cdef t_max_err sysfile_readtextfile(t_filehandle f, t_handle htext, t_ptr_size maxlen, t_sysfile_text_flags flags)
    cdef t_max_err sysfile_writetextfile(t_filehandle f, t_handle htext, t_sysfile_text_flags flags)
    cdef t_max_err sysfile_openhandle(char **h, t_sysfile_flags flags, t_filehandle *fh)
    cdef t_max_err sysfile_openptrsize(char *p, t_ptr_size length, t_sysfile_flags flags, t_filehandle *fh)


cdef extern from "ext_path.h":

    cdef int MAX_PATH_CHARS
    cdef int MAX_FILENAME_CHARS
    ctypedef short FILE_REF
    cdef char PATH_SEPARATOR_CHAR
    cdef char PATH_SEPARATOR_STRING
    cdef char SEPARATOR_CHAR

    ctypedef enum e_max_path_styles:
        PATH_STYLE_MAX = 0
        PATH_STYLE_NATIVE
        PATH_STYLE_COLON
        PATH_STYLE_SLASH
        PATH_STYLE_NATIVE_WIN


    cdef int PATH_STYLE_MAX_PLAT
    cdef int PATH_STYLE_NATIVE_PLAT

    ctypedef enum e_max_path_types:
        PATH_TYPE_IGNORE = 0
        PATH_TYPE_ABSOLUTE
        PATH_TYPE_RELATIVE
        PATH_TYPE_BOOT
        PATH_TYPE_C74
        PATH_TYPE_PATH
        PATH_TYPE_DESKTOP
        PATH_TYPE_TILDE
        PATH_TYPE_TEMPFOLDER
        PATH_TYPE_MAXDB

    cdef int PATH_CHAR_IS_SEPARATOR(char c)

    ctypedef enum e_max_fileinfo_flags:
        PATH_FILEINFO_ALIAS = 1
        PATH_FILEINFO_FOLDER = 2
        PATH_FILEINFO_PACKAGE = 4

    cdef int FILEINFO_ALIAS
    cdef int FILEINFO_FOLDER

    ctypedef enum e_max_path_folder_flags:
        PATH_REPORTPACKAGEASFOLDER = 1
        PATH_FOLDER_SNIFF = 2
        PATH_NOALIASRESOLUTION = 4


    ctypedef enum e_max_openfile_permissions:
        PATH_READ_PERM = 1
        PATH_WRITE_PERM = 2
        PATH_RW_PERM = 3


    cdef int READ_PERM
    cdef int WRITE_PERM
    cdef int RW_PERM
    cdef int PATH_DEFAULT_PATHNAME_COUNT

    ctypedef enum e_max_path_indices:
        PATH_STARTUP_PATH = 0
        PATH_SEARCH_PATH
        PATH_ACTION_PATH
        PATH_HELP_PATH


    cdef int STARTUP_PATH
    cdef int SEARCH_PATH
    cdef int ACTION_PATH
    cdef int HELP_PATH
    cdef int COLLECTIVE_FILECOPY
    cdef int COLLECTIVE_COPYTOMADEFOLDER
    cdef int TYPELIST_SIZE

    ctypedef enum e_max_typelists:
        TYPELIST_MAXFILES       = 1
        TYPELIST_EXTERNS        = 2
        TYPELIST_COLLECTIVES    = 4
        TYPELIST_MAXFORLIVE     = 8
        TYPELIST_SNAPSHOTS      = 16
        TYPELIST_GENPATCHERS    = 32
        TYPELIST_SNIPPETS       = 64


    ctypedef struct t_fileinfo:
        t_fourcc type          # type (four-char-code)
        t_fourcc creator       # Mac-only creator (four-char-code)
        t_uint32 unused        # this was date but it wasn't populated and it wasn't used
        t_int32 flags          # One of the values defined in #e_max_fileinfo_flags

    ctypedef struct t_path: pass
    ctypedef struct t_pathlink: pass

    ctypedef enum e_max_searchpath_flags:
        PATH_FLAGS_RECURSIVE    = 0x001
        PATH_FLAGS_READONLY     = 0x010

    short path_getapppath()
    short path_getsupportpath()


    # cdef short path_tofsref(const short path, const char *filename, FSRef *ref)
    # cdef short path_fromfsref(FSRef *ref)
    cdef void path_namefrompathname(char *pathname, char *name)
    cdef short locatefile(const char *name, short *outvol, short *binflag)
    cdef short locatefiletype(const char *name, short *outvol, t_fourcc filetype, t_fourcc creator)
    cdef short locatefilelist(char *name, short *outvol, t_fourcc *outtype, t_fourcc *filetypelist, short numtypes)
    cdef short locatefile_extended(char *name, short *outvol, t_fourcc *outtype, const t_fourcc *filetypelist, short numtypes)
    cdef short locatefile_pathlist(t_pathlink *list, char *name, short *outvol, t_fourcc *outtype, t_fourcc *filetypelist, short numtypes)
    cdef short path_resolvefile(char *name, const short path, short *outpath)
    cdef short path_fileinfo(const char *name, const short path, t_fileinfo *info)
    cdef short path_tempfolder()
    cdef short path_desktopfolder()
    cdef short path_userdocfolder()
    cdef short path_usermaxfolder()
    cdef short path_createfolder(const short path, const char *name, short *newpath)
    cdef short path_createnewfolder(short path, char *name, short *newpath)
    cdef short path_copyfile(short srcpath, char *srcname, short dstpath, char *dstname)
    cdef short path_copytotempfile(short srcpath, char *srcname, short *outpath, char *outname)
    cdef short path_copyfolder(short srcpath, short dstpath, char *dstname, long recurse, short *newpath)
    cdef short path_getpath(short path, const char *name, short *outpath)
    #cdef short C74_MUST_CHECK path_getpath(short path, const char *name, short *outpath)
    cdef short path_getname(short path, char *name, short *outpath)
    cdef short path_topathname(const short path, const char *file, char *name)
    cdef short path_frompathname(const char *name, short *path, char *filename)
    cdef short path_frompotentialpathname(const char *name, short *path, char *filename)
    cdef void path_splitnames(const char *pathname, char *foldername, char *filename)
    cdef short path_getnext(t_pathlink *list, short *val)
    cdef void path_setdefault(short path, short recursive)
    cdef short path_getdefault()
    # cdef void path_setdefaultlist(struct _pathlink *list)
    cdef short path_getmoddate(short path, t_ptr_uint *date)
    cdef short path_getfilemoddate(const char *filename, const short path, t_ptr_uint *date)
    # cdef short path_getfiledatesandsize(const char *filename, short path, t_uint64 *create, t_uint64 *mod, t_uint64 *access, t_uint64 *size)
    # cdef short path_getfilecreationdate(const char *filename, const short path, t_ptr_uint *date)
    # cdef short path_getfilesize(char *filename, short path, t_ptr_size *size)
    cdef long path_listcount(t_pathlink *list)
    cdef short nameinpath(char *name, short *ref)
    cdef short path_nameinpath(const char *name, const short path, short *ref)
    cdef short path_sysnameinpath(char *name, short *ref)
    cdef void *path_openfolder(short path)
    cdef short path_foldernextfile(void *xx, t_fourcc *filetype, char *name, short descend)
    cdef void path_closefolder(void *x)
    cdef short path_renamefile(const char *name, const short path, const char *newname)
    cdef long path_getprefstring(long preftype, long index, t_symbol **s)
    cdef void path_setprefstring(long preftype, long index, t_symbol *s, long flags, t_symbol *label, short update)
    cdef void path_makefromsymbol(long pathtype, t_symbol *sp, short recursive)
    cdef short path_opensysfile(const char *name, const short path, t_filehandle *ref, short perm)
    cdef short path_createsysfile(const char *name, short path, t_fourcc type, t_filehandle *ref)
    cdef short path_createressysfile(const char *name, const short path, t_fourcc type, t_filehandle *ref)
    cdef short path_nameconform(const char *src, char *dst, long style, long type)
    cdef short path_deletefile(const char *name, const short path)
    cdef short path_extendedfileinfo(const char *name, short path, t_fileinfo *info, const t_fourcc *typelist, short numtypes, short sniff)
    cdef short path_getstyle(char *name)
    cdef char path_getseparator(char *name)
    cdef short path_fileisresource(char *name, short path)
    cdef short path_topotentialname(const short path, const char *file, char *name, short check)
    cdef short path_topotentialunicodename(short path, char *file, unsigned short **name, long *outlen, short check)
    cdef short path_fromunicodepathname(unsigned short *name, short *path, char *filename, short check)
    cdef t_max_err path_toabsolutesystempath(const short in_path, const char *in_filename, char *out_filepath)
    cdef t_max_err path_absolutepath(t_symbol **returned_path, const t_symbol *s, const t_fourcc *filetypelist, short numtypes)
    cdef void path_addsearchpath(short path, short parent)
    cdef void path_addnamed(long pathtype, char *name, short recursive, short permanent)
    # cdef void path_removefromlist(t_pathlink **list, short parent)
    cdef short path_collpathnamefrompath(short vol, short *collvol, char *filename)
    cdef short defvolume()


cdef extern from "ext_preferences.h":

    t_max_err preferences_getatomforkey(t_symbol *key, t_atom *value)
    t_symbol *preferences_getsym(const char *name)
    void preferences_setsym(const char *name, t_symbol *value)
    long preferences_getlong(const char *name)
    void preferences_setlong(const char *name, long value)
    long preferences_getchar(const char *name)
    void preferences_setchar(const char *name, long value)
    t_max_err preferences_getatoms(const char *name, long *argc, t_atom **argv)
    t_max_err preferences_setatoms(const char *name, long argc, t_atom *argv)
    void *preferences_define(const char *name, const char *type, const char *label, const char *style, const char *category, long attrflags, method get, method set, long flags)
    void *preferences_class_define(t_class *c, const char *name, const char *type, const char *label, const char *style, const char *category, long attrflags, method get, method set, long flags)
    void *preferences_defineoption(const char *name, const char *label, const char *category, char *ptr, method notify, long flags)
    void *preferences_class_defineoption(t_class *c, const char *name, const char *label, const char *category, char *ptr, method notify, long flags)
    t_max_err preferences_writedictionary(const t_dictionary *d, const char *filename)
    t_max_err preferences_readdictionary(const char *filename, t_dictionary **d)
    t_dictionary *simpleprefs_dictionary()

    ctypedef enum:
        PREFERENCES_FLAGS_INVISIBLE = 1
        PREFERENCES_FLAGS_DONTSAVE = 2


cdef extern from "ext_strings.h":

    cdef char *strncpy_zero(char *dst, const char* src, long size)
    cdef char *strncat_zero(char *dst, const char* src, long size)
    cdef int snprintf_zero(char *buffer, size_t count, const char *format, ...)

    cdef int SPRINTF_MAXLEN
    cdef void ctopcpy(unsigned char *p1, char *p2)
    cdef void ptoccpy(char *p1, unsigned char *p2)
    cdef void pstrcpy(unsigned char *p2, unsigned char *p1)


cdef extern from "ext_symobject.h":

    ctypedef struct t_symobject: pass

    cdef void symobject_initclass()
    cdef void *symobject_new(t_symbol *sym)
    cdef long symobject_linklist_match(void *a, void *b)

cdef extern from "ext_sysmem.h":

    cdef t_ptr sysmem_newptr(long size)
    cdef t_ptr sysmem_newptrclear(long size)
    cdef t_ptr sysmem_resizeptr(void *ptr, long newsize)
    cdef t_ptr sysmem_resizeptrclear(void *ptr, long newsize)
    cdef long sysmem_ptrsize(void *ptr)
    cdef void sysmem_freeptr(void *ptr)
    cdef void sysmem_copyptr(const void *src, void *dst, long bytes)
    cdef t_handle sysmem_newhandle(long size)
    cdef t_handle sysmem_newhandleclear(unsigned long size)
    cdef long sysmem_resizehandle(t_handle handle, long newsize)
    cdef long sysmem_handlesize(t_handle handle)
    cdef void sysmem_freehandle(t_handle handle)
    cdef long sysmem_lockhandle(t_handle handle, long lock)
    cdef long sysmem_ptrandhand(void *p, t_handle h, long size)
    cdef long sysmem_ptrbeforehand(void *p, t_handle h, unsigned long size)
    cdef long sysmem_nullterminatehandle(t_handle h)


cdef extern from "ext_atomarray.h":

    cdef int ATOMARRAY_FLAG_FREECHILDREN

    ctypedef struct t_atomarray: pass

    cdef t_atomarray *atomarray_new(long ac, t_atom *av)
    cdef void atomarray_flags(t_atomarray *x, long flags)
    cdef long atomarray_getflags(t_atomarray *x)
    cdef t_max_err atomarray_setatoms(t_atomarray *x, long ac, t_atom *av)
    cdef t_max_err atomarray_getatoms(t_atomarray *x, long *ac, t_atom **av)
    cdef t_max_err atomarray_copyatoms(t_atomarray *x, long *ac, t_atom **av)
    cdef t_atom_long atomarray_getsize(t_atomarray *x)
    cdef t_max_err atomarray_getindex(t_atomarray *x, long index, t_atom *av)
    cdef t_max_err atomarray_setindex(t_atomarray *x, long index, t_atom *av)
    cdef void *atomarray_duplicate(t_atomarray *x)
    cdef void *atomarray_clone(t_atomarray *x)
    cdef void atomarray_appendatom(t_atomarray *x, t_atom *a)
    cdef void atomarray_appendatoms(t_atomarray *x, long ac, t_atom *av)
    cdef void atomarray_chuckindex(t_atomarray *x, long index)
    cdef void atomarray_clear(t_atomarray *x)
    cdef void atomarray_funall(t_atomarray *x, method fun, void *arg)


cdef extern from "ext_systhread.h":

    ctypedef void *t_systhread
    ctypedef void *t_systhread_mutex
    ctypedef void *t_systhread_cond
    ctypedef void *t_systhread_rwlock
    ctypedef void *t_systhread_key

    ctypedef enum e_max_systhread_mutex_flags:
        SYSTHREAD_MUTEX_NORMAL =        0x00000000
        SYSTHREAD_MUTEX_ERRORCHECK =    0x00000001
        SYSTHREAD_MUTEX_RECURSIVE =     0x00000002


    ctypedef enum e_max_systhread_priority:
        SYSTHREAD_PRIORITY_MIN = -30
        SYSTHREAD_PRIORITY_DEFAULT = 0
        SYSTHREAD_PRIORITY_MAX = 30

    ctypedef enum e_max_systhread_rwlock_flags:
        SYSTHREAD_RWLOCK_NORMAL =       0x00000000
        SYSTHREAD_RWLOCK_LITE =         0x00000001


    cdef long systhread_create(method entryproc, void *arg, long stacksize, long priority, long flags, t_systhread *thread)
    cdef long systhread_terminate(t_systhread thread)
    cdef void systhread_sleep(long milliseconds)
    cdef void systhread_exit(long status)
    cdef long systhread_join(t_systhread thread, unsigned int* retval)
    cdef long systhread_detach(t_systhread thread)
    cdef t_systhread systhread_self()
    cdef void systhread_setpriority(t_systhread thread, int priority)
    cdef int systhread_getpriority(t_systhread thread)
    cdef char *systhread_getstackbase()
    cdef void systhread_init()
    cdef void systhread_mainstacksetup()
    cdef void systhread_timerstacksetup()
    cdef short systhread_stackcheck()
    cdef short systhread_ismainthread()
    cdef short systhread_istimerthread()
    cdef short systhread_isaudiothread()
    cdef long systhread_mutex_new(t_systhread_mutex *pmutex,long flags)
    cdef long systhread_mutex_free(t_systhread_mutex pmutex)
    cdef long systhread_mutex_lock(t_systhread_mutex pmutex)
    cdef long systhread_mutex_unlock(t_systhread_mutex pmutex)
    cdef long systhread_mutex_trylock(t_systhread_mutex pmutex)
    cdef long systhread_mutex_newlock(t_systhread_mutex *pmutex,long flags)
    cdef t_max_err systhread_rwlock_new(t_systhread_rwlock *rwlock, long flags)
    cdef t_max_err systhread_rwlock_free(t_systhread_rwlock rwlock)
    cdef t_max_err systhread_rwlock_rdlock(t_systhread_rwlock rwlock)
    cdef t_max_err systhread_rwlock_tryrdlock(t_systhread_rwlock rwlock)
    cdef t_max_err systhread_rwlock_rdunlock(t_systhread_rwlock rwlock)
    cdef t_max_err systhread_rwlock_wrlock(t_systhread_rwlock rwlock)
    cdef t_max_err systhread_rwlock_trywrlock(t_systhread_rwlock rwlock)
    cdef t_max_err systhread_rwlock_wrunlock(t_systhread_rwlock rwlock)
    cdef t_max_err systhread_rwlock_setspintime(t_systhread_rwlock rwlock, double spintime_ms)
    cdef t_max_err systhread_rwlock_getspintime(t_systhread_rwlock rwlock, double *spintime_ms)
    cdef long systhread_cond_new(t_systhread_cond *pcond, long flags)
    cdef long systhread_cond_free(t_systhread_cond pcond)
    cdef long systhread_cond_wait(t_systhread_cond pcond, t_systhread_mutex pmutex)
    cdef long systhread_cond_signal(t_systhread_cond pcond)
    cdef long systhread_cond_broadcast(t_systhread_cond pcond)
    cdef long systhread_key_create(t_systhread_key *key, void (*destructor)(void*))
    cdef long systhread_key_delete(t_systhread_key key)
    cdef void* systhread_getspecific(t_systhread_key key)
    cdef long systhread_setspecific(t_systhread_key key, const void *value)


cdef extern from "ext_sysparallel.h":

    cdef int SYSPARALLEL_PRIORITY_DEFAULT
    cdef int SYSPARALLEL_PRIORITY_LOW
    cdef int SYSPARALLEL_PRIORITY_MEDIUM
    cdef int SYSPARALLEL_PRIORITY_HIGH
    cdef int SYSPARALLEL_PRIORITY_TASK_LOCAL
    cdef int SYSPARALLEL_MAX_WORKERS
    cdef int SYSPARALLEL_STATE_IDLE
    cdef int SYSPARALLEL_STATE_RUN
    cdef int SYSPARALLEL_STATE_DONE
    cdef int SYSPARALLEL_STATE_QUIT
    cdef int SYSPARALLEL_TASK_FLAG_WORKERTRIGGERS

    ctypedef struct t_sysparallel_task: pass
    ctypedef struct t_sysparallel_worker: pass
    cdef void sysparallel_init()
    cdef long sysparallel_processorcount()
    cdef t_sysparallel_task *sysparallel_task_new(void *data, method workerproc, long maxworkercount)
    cdef t_max_err sysparallel_task_workercount(t_sysparallel_task *x, long workercount)
    cdef t_max_err sysparallel_task_execute(t_sysparallel_task *x)
    cdef void sysparallel_task_signalworkers(t_sysparallel_task *x, long count)
    cdef t_max_err sysparallel_task_cancel(t_sysparallel_task *x)
    cdef void sysparallel_task_free(t_sysparallel_task *x)
    cdef void sysparallel_task_benchprint(t_sysparallel_task *x)
    cdef void sysparallel_task_data(t_sysparallel_task *x, void * data)
    cdef void sysparallel_task_workerproc(t_sysparallel_task *x, method workerproc)
    cdef t_sysparallel_worker *sysparallel_worker_new(void *data, method workerproc, t_sysparallel_task *task)
    cdef t_max_err sysparallel_worker_execute(t_sysparallel_worker *x)
    cdef void sysparallel_worker_free(t_sysparallel_worker *x)


cdef extern from "ext_sysprocess.h":

    cdef long sysprocess_isrunning(long id)
    cdef long sysprocess_launch(const char *utf8path, const char *utf8commandline)
    cdef long sysprocess_activate(long id)
    cdef long sysprocess_getid(const char *utf8path)
    cdef long sysprocess_getcurrentid()
    cdef long sysprocess_getpath(long id, char **utf8path)
    cdef t_object* sysprocesswatcher_new(long id, method m, void *arg)
    cdef long sysprocess_fitsarch(long id)


cdef extern from "ext_sysmem.h":

    ctypedef void * t_syssem
    cdef t_max_err syssem_create(t_syssem *x, const char *name, long flags, unsigned int value)
    cdef t_max_err syssem_open(t_syssem *x, const char *name, long flags)
    cdef t_max_err syssem_close(t_syssem x)
    cdef t_max_err syssem_wait(t_syssem x)
    cdef t_max_err syssem_trywait(t_syssem x)
    cdef t_max_err syssem_post(t_syssem x)


cdef extern from "ext_sysshmem.h":

    ctypedef void *t_sysshmem
    cdef int SYSSHMEM_FLAGS_READONLY
    cdef t_max_err sysshmem_alloc(t_sysshmem *x, const char *name, long size, long flags)
    cdef t_max_err sysshmem_open(t_sysshmem *x, const char *name, long flags)
    cdef t_max_err sysshmem_close(t_sysshmem x)
    cdef unsigned long sysshmem_getsize(t_sysshmem x)
    cdef void* sysshmem_getptr(t_sysshmem x)


cdef extern from "ext_systime.h":

    ctypedef struct t_datetime: pass

    ctypedef enum e_max_dateflags:
        SYSDATEFORMAT_FLAGS_SHORT = 1
        SYSDATEFORMAT_FLAGS_MEDIUM = 2
        SYSDATEFORMAT_FLAGS_LONG = 3

    cdef t_uint32 systime_ticks()
    cdef t_uint32 systime_ms()
    cdef t_int64 systime_datetime_milliseconds()
    cdef void systime_datetime(t_datetime *d)
    cdef t_ptr_uint systime_seconds()
    cdef void systime_secondstodate(t_ptr_uint secs, t_datetime *d)
    cdef t_ptr_uint systime_datetoseconds(t_datetime *d)
    cdef void sysdateformat_strftimetodatetime(char *strf, t_datetime *d)
    cdef void sysdateformat_formatdatetime(t_datetime *d, long dateflags, long timeflags, char *s, long buflen)
    cdef int SYSDATEFORMAT_RELATIVE


cdef extern from "commonsyms.h":
    cdef int COMMON_SYMBOLS_VERSION_5_0_0
    cdef int COMMON_SYMBOLS_VERSION
    ctypedef struct t_common_symbols_table:
        long    version
        t_symbol *s__preset
        t_symbol *s_abbrev
        t_symbol *s_abbrev_rowcomponent
        t_symbol *s_abbrev_setvalue
        t_symbol *s_acceptsdrag
        t_symbol *s_acceptsdrag_locked
        t_symbol *s_acceptsdrag_unlocked
        t_symbol *s_action
        t_symbol *s_action_rowcomponent
        t_symbol *s_action_setvalue
        t_symbol *s_activate
        t_symbol *s_active
        t_symbol *s_activetab
        t_symbol *s_activetabname
        t_symbol *s_activewindow
        t_symbol *s_adapt
        t_symbol *s_add
        t_symbol *s_addattr
        t_symbol *s_addattr_enable
        t_symbol *s_addclient
        t_symbol *s_addfolder
        t_symbol *s_addfolderandsave
        t_symbol *s_addquerydict
        t_symbol *s_addquerydictfromfile
        t_symbol *s_addslot
        t_symbol *s_addtopresentation
        t_symbol *s_addwiretap
        t_symbol *s_adornments
        t_symbol *s_alias
        t_symbol *s_alignboxes
        t_symbol *s_alignconnections
        t_symbol *s_alignlines
        t_symbol *s_all
        t_symbol *s_allkinds
        t_symbol *s_allowmod
        t_symbol *s_alpha
        t_symbol *s_annotation
        t_symbol *s_annotation_name
        t_symbol *s_anydate
        t_symbol *s_anykind
        t_symbol *s_anything
        t_symbol *s_append
        t_symbol *s_append_sql
        t_symbol *s_appendatoms
        t_symbol *s_appendtodictionary
        t_symbol *s_apply
        t_symbol *s_applyboxprototype
        t_symbol *s_applydeep
        t_symbol *s_applydeepif
        t_symbol *s_applyif
        t_symbol *s_args
        t_symbol *s_argument
        t_symbol *s_arguments
        t_symbol *s_argv
        t_symbol *s_ascending
        t_symbol *s_aspect
        t_symbol *s_assist
        t_symbol *s_assoc
        t_symbol *s_atbclick
        t_symbol *s_atom
        t_symbol *s_atomarray
        t_symbol *s_attach
        t_symbol *s_attr_filter_clip
        t_symbol *s_attr_get
        t_symbol *s_attr_getnames
        t_symbol *s_attr_gettarget
        t_symbol *s_attr_modified
        t_symbol *s_attr_offset_array
        t_symbol *s_attr_renamed
        t_symbol *s_attr_setdisabled
        t_symbol *s_attr_setinvisible
        t_symbol *s_attribute
        t_symbol *s_attributes
        t_symbol *s_attrname
        t_symbol *s_audiofile
        t_symbol *s_audioplugin
        t_symbol *s_author
        t_symbol *s_autocompletion
        t_symbol *s_autocompletion_query
        t_symbol *s_autofixwidth
        t_symbol *s_autoheightchanged
        t_symbol *s_autoscroll
        t_symbol *s_back
        t_symbol *s_background
        t_symbol *s_bang
        t_symbol *s_bbu
        t_symbol *s_bclear
        t_symbol *s_bcopy
        t_symbol *s_bcut
        t_symbol *s_begineditbox
        t_symbol *s_beginswith
        t_symbol *s_beginswithorcontains
        t_symbol *s_bfixwidth
        t_symbol *s_bfont
        t_symbol *s_bgcolor
        t_symbol *s_bgcount
        t_symbol *s_bghidden
        t_symbol *s_bglocked
        t_symbol *s_bgmode
        t_symbol *s_blue
        t_symbol *s_bogus
        t_symbol *s_bold
        t_symbol *s_border
        t_symbol *s_borderchanged
        t_symbol *s_bottom_inset
        t_symbol *s_boundingbox
        t_symbol *s_bounds
        t_symbol *s_box
        t_symbol *s_box1
        t_symbol *s_box2
        t_symbol *s_boxalpha
        t_symbol *s_boxanimatetime
        t_symbol *s_boxcomponent
        t_symbol *s_boxcontextitems
        t_symbol *s_boxcontextmenu
        t_symbol *s_boxes
        t_symbol *s_boxlayer
        t_symbol *s_boxnotify
        t_symbol *s_boxscreenrectchanged
        t_symbol *s_bpaste
        t_symbol *s_bpastepic
        t_symbol *s_bpatcher
        t_symbol *s_bpatcher_holder
        t_symbol *s_bpm
        t_symbol *s_bracket_default
        t_symbol *s_bracket_none
        t_symbol *s_break
        t_symbol *s_bredo
        t_symbol *s_brgba
        t_symbol *s_bringforward
        t_symbol *s_bringtofront
        t_symbol *s_bubblesize
        t_symbol *s_build
        t_symbol *s_buildcolumns
        t_symbol *s_bundo
        t_symbol *s_button
        t_symbol *s_c74object
        t_symbol *s_canback
        t_symbol *s_cancopychanged
        t_symbol *s_candropfiles
        t_symbol *s_canforward
        t_symbol *s_canhilite
        t_symbol *s_canmovebackward
        t_symbol *s_canmoveforward
        t_symbol *s_canpastechanged
        t_symbol *s_canremove
        t_symbol *s_cansave
        t_symbol *s_canschedule
        t_symbol *s_canselectchanged
        t_symbol *s_canvastoscreen
        t_symbol *s_caption
        t_symbol *s_catcolors
        t_symbol *s_category
        t_symbol *s_category_first
        t_symbol *s_cell
        t_symbol *s_cell_clue
        t_symbol *s_cellclue
        t_symbol *s_cellenabled
        t_symbol *s_cellschanged
        t_symbol *s_char
        t_symbol *s_char_comma
        t_symbol *s_char_minus
        t_symbol *s_char_semi
        t_symbol *s_char_space
        t_symbol *s_charset_converter
        t_symbol *s_checkbox
        t_symbol *s_choose
        t_symbol *s_chord
        t_symbol *s_chuck
        t_symbol *s_chuckindex
        t_symbol *s_class
        t_symbol *s_class_jit_attribute
        t_symbol *s_class_jit_matrix
        t_symbol *s_class_jit_namespace
        t_symbol *s_classname
        t_symbol *s_classsym
        t_symbol *s_clear
        t_symbol *s_clearactions
        t_symbol *s_clearcolumns
        t_symbol *s_clearitem
        t_symbol *s_clearslots
        t_symbol *s_click
        t_symbol *s_clickaction
        t_symbol *s_clientcontext
        t_symbol *s_clipboard
        t_symbol *s_clipping
        t_symbol *s_clock
        t_symbol *s_close
        t_symbol *s_closebang
        t_symbol *s_clue_cell
        t_symbol *s_clue_header
        t_symbol *s_clueclass
        t_symbol *s_cluelookupattr
        t_symbol *s_cluename
        t_symbol *s_clues
        t_symbol *s_colhead
        t_symbol *s_coll
        t_symbol *s_collectfiles
        t_symbol *s_collective
        t_symbol *s_color
        t_symbol *s_colorvalue
        t_symbol *s_columnadded
        t_symbol *s_columnclue
        t_symbol *s_columndeleted
        t_symbol *s_columnheaderclue
        t_symbol *s_columnminmaxchanged
        t_symbol *s_columnnamechanged
        t_symbol *s_columns
        t_symbol *s_columnwidthchanged
        t_symbol *s_com
        t_symbol *s_command
        t_symbol *s_command_enable
        t_symbol *s_commandgroup
        t_symbol *s_commands
        t_symbol *s_comment
        t_symbol *s_comparison
        t_symbol *s_comparisons
        t_symbol *s_completeconnection
        t_symbol *s_connect
        t_symbol *s_connectcolor
        t_symbol *s_containersizechange
        t_symbol *s_contains
        t_symbol *s_contextmenu
        t_symbol *s_contextualpaste
        t_symbol *s_controller
        t_symbol *s_convert
        t_symbol *s_copy
        t_symbol *s_copyatoms
        t_symbol *s_copyjundo
        t_symbol *s_coremidi
        t_symbol *s_count
        t_symbol *s_create_backup
        t_symbol *s_createbpatcher
        t_symbol *s_createsubpatcher
        t_symbol *s_current
        t_symbol *s_cut
        t_symbol *s_data
        t_symbol *s_date_created
        t_symbol *s_date_lastaccessed
        t_symbol *s_date_modified
        t_symbol *s_days
        t_symbol *s_dblclick
        t_symbol *s_debugwindow_show
        t_symbol *s_decorator
        t_symbol *s_default
        t_symbol *s_default_fontface
        t_symbol *s_default_fontname
        t_symbol *s_default_fontsize
        t_symbol *s_default_matrixplcolor
        t_symbol *s_default_plcolor
        t_symbol *s_default_query
        t_symbol *s_default_sigplcolor
        t_symbol *s_defaultcommand
        t_symbol *s_defaultfocusbox
        t_symbol *s_defaultname
        t_symbol *s_defaultquery
        t_symbol *s_defaults
        t_symbol *s_defaultvaluechanged
        t_symbol *s_definearg
        t_symbol *s_defineargument
        t_symbol *s_definecomparison
        t_symbol *s_definefield
        t_symbol *s_definefolder
        t_symbol *s_definequantifier
        t_symbol *s_definequery
        t_symbol *s_defineslot
        t_symbol *s_definesort
        t_symbol *s_defrect
        t_symbol *s_delete
        t_symbol *s_deleteindex
        t_symbol *s_deletemetadata
        t_symbol *s_deletequery
        t_symbol *s_depthbuffer
        t_symbol *s_descending
        t_symbol *s_description
        t_symbol *s_dest_closing
        t_symbol *s_destination
        t_symbol *s_destrect
        t_symbol *s_destroy
        t_symbol *s_detach
        t_symbol *s_devicerects
        t_symbol *s_dictionary
        t_symbol *s_digest
        t_symbol *s_dim
        t_symbol *s_dimlink
        t_symbol *s_dirty
        t_symbol *s_disabled
        t_symbol *s_disablewiretap
        t_symbol *s_disconnect
        t_symbol *s_do_get_shared_context
        t_symbol *s_document
        t_symbol *s_docwindow_docrect
        t_symbol *s_docwindow_refrect
        t_symbol *s_docwindow_searchrect
        t_symbol *s_docwindow_tocrect
        t_symbol *s_docwindow_tutrect
        t_symbol *s_domain
        t_symbol *s_done
        t_symbol *s_donewobj
        t_symbol *s_dontsave
        t_symbol *s_doublebuffer
        t_symbol *s_doubleclick
        t_symbol *s_doubleclickaction
        t_symbol *s_doubleclicked
        t_symbol *s_down
        t_symbol *s_downcaption
        t_symbol *s_downicon
        t_symbol *s_drag
        t_symbol *s_dragactive
        t_symbol *s_dragdrop
        t_symbol *s_dragenter
        t_symbol *s_dragexit
        t_symbol *s_draggablechanged
        t_symbol *s_dragmove
        t_symbol *s_dragrole
        t_symbol *s_dragtarget
        t_symbol *s_drawfirstin
        t_symbol *s_drawinlast
        t_symbol *s_drawsresizer
        t_symbol *s_dropaction
        t_symbol *s_dropaction_addapplyprototype
        t_symbol *s_dropaction_addconnectedmessage
        t_symbol *s_dropaction_addcustom
        t_symbol *s_dropaction_addcustom_extended
        t_symbol *s_dropaction_addobjectcreation
        t_symbol *s_dropaction_addopeninnewwindow
        t_symbol *s_dropaction_addprototypeobjectcreation
        t_symbol *s_dropfiles
        t_symbol *s_droprole
        t_symbol *s_droprole_locked
        t_symbol *s_droprole_unlocked
        t_symbol *s_dsp
        t_symbol *s_dump
        t_symbol *s_dumpboxes
        t_symbol *s_dumpout
        t_symbol *s_duplicate
        t_symbol *s_edit
        t_symbol *s_edit_framecolor
        t_symbol *s_editactive
        t_symbol *s_editbox
        t_symbol *s_editcell
        t_symbol *s_editing_bgcolor
        t_symbol *s_editmetadata
        t_symbol *s_editonclick
        t_symbol *s_editor
        t_symbol *s_elements
        t_symbol *s_embed
        t_symbol *s_emptytext
        t_symbol *s_enable
        t_symbol *s_enable_rowcomponent
        t_symbol *s_enable_setvalue
        t_symbol *s_enablehscroll
        t_symbol *s_enabler
        t_symbol *s_enablevscroll
        t_symbol *s_enddrag
        t_symbol *s_endmoveboxes
        t_symbol *s_endpoint
        t_symbol *s_endprobe
        t_symbol *s_endswith
        t_symbol *s_endtransaction
        t_symbol *s_enter
        t_symbol *s_entertext
        t_symbol *s_enum
        t_symbol *s_enumindex
        t_symbol *s_enummsg
        t_symbol *s_enumtarget
        t_symbol *s_enumvals
        t_symbol *s_equalto
        t_symbol *s_error
        t_symbol *s_eventinterval
        t_symbol *s_everything
        t_symbol *s_excludebg
        t_symbol *s_exec
        t_symbol *s_execstring
        t_symbol *s_execstring_safe
        t_symbol *s_execute
        t_symbol *s_executefind
        t_symbol *s_extension
        t_symbol *s_extra
        t_symbol *s_ezquery
        t_symbol *s_fadetinge
        t_symbol *s_fgcolor
        t_symbol *s_fghidden
        t_symbol *s_field
        t_symbol *s_fieldnamebyindex
        t_symbol *s_fields
        t_symbol *s_file
        t_symbol *s_filefolder
        t_symbol *s_fileformat
        t_symbol *s_filename
        t_symbol *s_filepath
        t_symbol *s_filetypemessage
        t_symbol *s_fileusage
        t_symbol *s_filter
        t_symbol *s_filterget
        t_symbol *s_filterset
        t_symbol *s_find
        t_symbol *s_find_enableselectall
        t_symbol *s_findall
        t_symbol *s_finddoneclicked
        t_symbol *s_findfirst
        t_symbol *s_findmaster
        t_symbol *s_findnextclicked
        t_symbol *s_findprevclicked
        t_symbol *s_findreturnkeypressed
        t_symbol *s_findselectallclicked
        t_symbol *s_findsize
        t_symbol *s_findtextchanged
        t_symbol *s_first
        t_symbol *s_firstline
        t_symbol *s_firstobject
        t_symbol *s_firstview
        t_symbol *s_five
        t_symbol *s_fixed
        t_symbol *s_fixwidth
        t_symbol *s_flags
        t_symbol *s_flat
        t_symbol *s_float
        t_symbol *s_float32
        t_symbol *s_float64
        t_symbol *s_floating
        t_symbol *s_flonum
        t_symbol *s_flush
        t_symbol *s_focusgained
        t_symbol *s_focuslost
        t_symbol *s_focusonvis
        t_symbol *s_fold
        t_symbol *s_folder
        t_symbol *s_folderdropped
        t_symbol *s_folderpath
        t_symbol *s_font
        t_symbol *s_fontchanged
        t_symbol *s_fontface
        t_symbol *s_fontfixwidth
        t_symbol *s_fontinfochanged
        t_symbol *s_fontname
        t_symbol *s_fontnamechanged
        t_symbol *s_fontpanel_isclientwindow
        t_symbol *s_fontpanelfontcolor
        t_symbol *s_fontpanelfontface
        t_symbol *s_fontpanelfontname
        t_symbol *s_fontpanelfontsize
        t_symbol *s_fontsize
        t_symbol *s_fontsizechanged
        t_symbol *s_fonttarget
        t_symbol *s_forbidclose
        t_symbol *s_forward
        t_symbol *s_four
        t_symbol *s_fpic
        t_symbol *s_free
        t_symbol *s_freebang
        t_symbol *s_freekeys
        t_symbol *s_freepatcherview
        t_symbol *s_frgb
        t_symbol *s_frgba
        t_symbol *s_fromdictionary
        t_symbol *s_fromgworld
        t_symbol *s_frommatrix
        t_symbol *s_frommatrix_trunc
        t_symbol *s_front
        t_symbol *s_frozen
        t_symbol *s_frozen_box_attributes
        t_symbol *s_frozen_object_attributes
        t_symbol *s_frozen_pendingattrs
        t_symbol *s_frozen_text
        t_symbol *s_fsaa
        t_symbol *s_fullname
        t_symbol *s_fullscreen
        t_symbol *s_funall
        t_symbol *s_function
        t_symbol *s_g_inout_binlet
        t_symbol *s_g_max_newest
        t_symbol *s_g_max_newest_box
        t_symbol *s_gb
        t_symbol *s_genframe
        t_symbol *s_get
        t_symbol *s_get_jit_ob
        t_symbol *s_getargumentlabel
        t_symbol *s_getassoc
        t_symbol *s_getatoms
        t_symbol *s_getattrtext
        t_symbol *s_getbounds
        t_symbol *s_getboxlayer
        t_symbol *s_getcaptioninfo
        t_symbol *s_getcellcolor
        t_symbol *s_getcelldescription
        t_symbol *s_getcelleditable
        t_symbol *s_getcellfiletypes
        t_symbol *s_getcellicon
        t_symbol *s_getcellindentlevel
        t_symbol *s_getcellmenu
        t_symbol *s_getcelltext
        t_symbol *s_getcelltextlength
        t_symbol *s_getcellunits
        t_symbol *s_getcellunitsyms
        t_symbol *s_getcellvalue
        t_symbol *s_getcolumnnames
        t_symbol *s_getcomparisonlabel
        t_symbol *s_getcomponent
        t_symbol *s_getcontainedcomponent
        t_symbol *s_getdata
        t_symbol *s_getdefaultfocuscomponent
        t_symbol *s_getdefext
        t_symbol *s_getdeftype
        t_symbol *s_getdrawparams
        t_symbol *s_getdst
        t_symbol *s_getfieldlabel
        t_symbol *s_getfindtext
        t_symbol *s_getflags
        t_symbol *s_getfolderpath
        t_symbol *s_getfonttarget
        t_symbol *s_getfontview
        t_symbol *s_getformat
        t_symbol *s_gethintdelay
        t_symbol *s_getholder
        t_symbol *s_getimage
        t_symbol *s_getindex
        t_symbol *s_getinfo
        t_symbol *s_getinports
        t_symbol *s_getinput
        t_symbol *s_getinputlist
        t_symbol *s_getioproc
        t_symbol *s_getkeys
        t_symbol *s_getlastinsertid
        t_symbol *s_getlayoutinfo
        t_symbol *s_getlogical
        t_symbol *s_getmatrix
        t_symbol *s_getmethod
        t_symbol *s_getname
        t_symbol *s_getnamed
        t_symbol *s_getnamedbox
        t_symbol *s_getnextrecord
        t_symbol *s_getnthview
        t_symbol *s_getobject
        t_symbol *s_getoffset
        t_symbol *s_getoutports
        t_symbol *s_getoutput
        t_symbol *s_getoutputlist
        t_symbol *s_getprobevalue
        t_symbol *s_getptr
        t_symbol *s_getptr_forview
        t_symbol *s_getquantifierlabel
        t_symbol *s_getquery
        t_symbol *s_getquerydict
        t_symbol *s_getquerynames
        t_symbol *s_getquerytype
        t_symbol *s_getrect
        t_symbol *s_getrowcolor
        t_symbol *s_getrowobject
        t_symbol *s_getselected
        t_symbol *s_getsize
        t_symbol *s_getsort
        t_symbol *s_getspecial
        t_symbol *s_getsrc
        t_symbol *s_getstackbase
        t_symbol *s_getsyswind
        t_symbol *s_gettextptr
        t_symbol *s_gettitle
        t_symbol *s_gettype
        t_symbol *s_gettypelist
        t_symbol *s_getunitstext
        t_symbol *s_getunitsyms
        t_symbol *s_getvalueof
        t_symbol *s_getvisiblecanvasrect
        t_symbol *s_getwind
        t_symbol *s_getwindowrect
        t_symbol *s_gl_line_loop
        t_symbol *s_gl_line_strip
        t_symbol *s_gl_lines
        t_symbol *s_gl_points
        t_symbol *s_gl_polygon
        t_symbol *s_gl_quad_grid
        t_symbol *s_gl_quad_strip
        t_symbol *s_gl_quads
        t_symbol *s_gl_tri_fan
        t_symbol *s_gl_tri_grid
        t_symbol *s_gl_tri_strip
        t_symbol *s_gl_triangles
        t_symbol *s_global
        t_symbol *s_globalpatchername
        t_symbol *s_go
        t_symbol *s_grabfocus
        t_symbol *s_greaterthan
        t_symbol *s_green
        t_symbol *s_grid
        t_symbol *s_gridonopen
        t_symbol *s_gridsize
        t_symbol *s_gridsnap
        t_symbol *s_gridsnaponopen
        t_symbol *s_group
        t_symbol *s_grow
        t_symbol *s_growboth
        t_symbol *s_growy
        t_symbol *s_hasclose
        t_symbol *s_hasdatatype
        t_symbol *s_hasgrow
        t_symbol *s_hashorizscroll
        t_symbol *s_hashtab_entry_free
        t_symbol *s_hashtab_entry_new
        t_symbol *s_hashtab_free
        t_symbol *s_hasmenu
        t_symbol *s_hasminimize
        t_symbol *s_hastitlebar
        t_symbol *s_hasvertscroll
        t_symbol *s_haszoom
        t_symbol *s_head
        t_symbol *s_height
        t_symbol *s_help
        t_symbol *s_helpfile
        t_symbol *s_helpmenu
        t_symbol *s_helpname
        t_symbol *s_hidden
        t_symbol *s_hiddenconnect
        t_symbol *s_hide
        t_symbol *s_hideablechanged
        t_symbol *s_hidecaption
        t_symbol *s_hidewindow
        t_symbol *s_hint
        t_symbol *s_hint_disabled
        t_symbol *s_hinttrack
        t_symbol *s_history
        t_symbol *s_history_entry
        t_symbol *s_hittest
        t_symbol *s_holderoptions
        t_symbol *s_hz
        t_symbol *s_icon
        t_symbol *s_id
        t_symbol *s_identifier
        t_symbol *s_ignoreclick
        t_symbol *s_image
        t_symbol *s_imagefile
        t_symbol *s_imbed
        t_symbol *s_imprint
        t_symbol *s_includebg
        t_symbol *s_index
        t_symbol *s_info
        t_symbol *s_init
        t_symbol *s_inlet
        t_symbol *s_inletinfo
        t_symbol *s_inletnum
        t_symbol *s_inletoutlet
        t_symbol *s_inletscaleratio
        t_symbol *s_inputcount
        t_symbol *s_insert
        t_symbol *s_insertboxtext
        t_symbol *s_insertindex
        t_symbol *s_insertpatcher
        t_symbol *s_insertsegment
        t_symbol *s_insetchanged
        t_symbol *s_insp
        t_symbol *s_inspectee
        t_symbol *s_inspectees
        t_symbol *s_inspector
        t_symbol *s_inspector_clueclass
        t_symbol *s_inspector_color
        t_symbol *s_inspector_customize
        t_symbol *s_inspector_editor
        t_symbol *s_inspector_fontattr
        t_symbol *s_inspector_rect
        t_symbol *s_inspector_tab
        t_symbol *s_inspector_tabheight
        t_symbol *s_inspector_title
        t_symbol *s_inspector_toolbarid
        t_symbol *s_inspectorchange
        t_symbol *s_instance_attributes
        t_symbol *s_instanceattr
        t_symbol *s_int
        t_symbol *s_int16
        t_symbol *s_int24
        t_symbol *s_int32
        t_symbol *s_int8
        t_symbol *s_interface
        t_symbol *s_interp
        t_symbol *s_interp_arg
        t_symbol *s_interval
        t_symbol *s_invalidate
        t_symbol *s_invalidateallboxlayers
        t_symbol *s_invalidateboxlayer
        t_symbol *s_invalidatetoolbar
        t_symbol *s_invis
        t_symbol *s_invisaction
        t_symbol *s_invisible
        t_symbol *s_invlabel
        t_symbol *s_invmsg
        t_symbol *s_ioname
        t_symbol *s_ioproc
        t_symbol *s_is
        t_symbol *s_iscolumnvisible
        t_symbol *s_isfile
        t_symbol *s_isfirstin
        t_symbol *s_isfolder
        t_symbol *s_iso_8859_1
        t_symbol *s_isproto
        t_symbol *s_isselectedqueryremovable
        t_symbol *s_isselectionvalid
        t_symbol *s_issystemquery
        t_symbol *s_italic
        t_symbol *s_items
        t_symbol *s_iterate
        t_symbol *s_javascript
        t_symbol *s_jbogus
        t_symbol *s_jbox
        t_symbol *s_jbox_bytecount
        t_symbol *s_jbox_instances
        t_symbol *s_jboxattr
        t_symbol *s_jdrag
        t_symbol *s_jed
        t_symbol *s_jgraphics
        t_symbol *s_jit_attr_offset
        t_symbol *s_jit_attr_offset_array
        t_symbol *s_jit_attribute
        t_symbol *s_jit_gl_texture
        t_symbol *s_jit_linklist
        t_symbol *s_jit_matrix
        t_symbol *s_jit_mop
        t_symbol *s_jit_namespace
        t_symbol *s_jpatcher
        t_symbol *s_jpatcher_bytecount
        t_symbol *s_jpatcher_instances
        t_symbol *s_jpatchercontroller
        t_symbol *s_jpatcherholder
        t_symbol *s_jpatchline
        t_symbol *s_jpg
        t_symbol *s_jsave
        t_symbol *s_JSON
        t_symbol *s_jsonreader
        t_symbol *s_jsonwriter
        t_symbol *s_juibogus
        t_symbol *s_juiobject
        t_symbol *s_jundo_command
        t_symbol *s_jundo_commandgroup
        t_symbol *s_jweb_history
        t_symbol *s_jwind
        t_symbol *s_kb
        t_symbol *s_key
        t_symbol *s_key_backslash
        t_symbol *s_key_backspace
        t_symbol *s_key_clear
        t_symbol *s_key_delete
        t_symbol *s_key_downarrow
        t_symbol *s_key_end
        t_symbol *s_key_enter
        t_symbol *s_key_esc
        t_symbol *s_key_grave
        t_symbol *s_key_helpkey
        t_symbol *s_key_home
        t_symbol *s_key_insert
        t_symbol *s_key_leftarrow
        t_symbol *s_key_optionspace
        t_symbol *s_key_pagedown
        t_symbol *s_key_pageup
        t_symbol *s_key_return
        t_symbol *s_key_rightarrow
        t_symbol *s_key_spacebar
        t_symbol *s_key_tab
        t_symbol *s_key_tilde
        t_symbol *s_key_uparrow
        t_symbol *s_key_vertbar
        t_symbol *s_keyaction
        t_symbol *s_keyfilter
        t_symbol *s_keyfocuschanged
        t_symbol *s_keymessage
        t_symbol *s_kind
        t_symbol *s_kindenum
        t_symbol *s_kindis
        t_symbol *s_kindmenu
        t_symbol *s_label
        t_symbol *s_labels
        t_symbol *s_last
        t_symbol *s_last_access
        t_symbol *s_lastmessage
        t_symbol *s_lastmodified
        t_symbol *s_lastobject
        t_symbol *s_latency
        t_symbol *s_lessthan
        t_symbol *s_linear
        t_symbol *s_linechanged
        t_symbol *s_linecontextmenu
        t_symbol *s_linecount
        t_symbol *s_linenotify
        t_symbol *s_lines
        t_symbol *s_lineup
        t_symbol *s_list
        t_symbol *s_list_rowcomponent
        t_symbol *s_list_setvalue
        t_symbol *s_listboxprototype
        t_symbol *s_listfiles
        t_symbol *s_listwindow
        t_symbol *s_loadbang
        t_symbol *s_local
        t_symbol *s_lock
        t_symbol *s_locked
        t_symbol *s_locked_iocolor
        t_symbol *s_lockeddragscroll
        t_symbol *s_lockedpatchercontextmenu
        t_symbol *s_log
        t_symbol *s_long
        t_symbol *s_lookup
        t_symbol *s_lookupcommand
        t_symbol *s_loop
        t_symbol *s_macroman
        t_symbol *s_mainsearchentry
        t_symbol *s_makearray
        t_symbol *s_makeview
        t_symbol *s_margin
        t_symbol *s_matchdragrole
        t_symbol *s_matchinspectees
        t_symbol *s_matrix
        t_symbol *s_matrix_calc
        t_symbol *s_matrixctrl
        t_symbol *s_matrixname
        t_symbol *s_max
        t_symbol *s_max_jit_classex
        t_symbol *s_maxapplication
        t_symbol *s_maxclass
        t_symbol *s_maxdebug
        t_symbol *s_maxdim
        t_symbol *s_maximize
        t_symbol *s_maxmessage
        t_symbol *s_maxplanecount
        t_symbol *s_maxwindow
        t_symbol *s_mb
        t_symbol *s_measuretext
        t_symbol *s_menu
        t_symbol *s_menubar
        t_symbol *s_menus
        t_symbol *s_menus_runtime
        t_symbol *s_message
        t_symbol *s_messages
        t_symbol *s_metadata
        t_symbol *s_metadatalist
        t_symbol *s_metadatawindow
        t_symbol *s_methodall
        t_symbol *s_methodindex
        t_symbol *s_methods
        t_symbol *s_midpoints
        t_symbol *s_min
        t_symbol *s_mindim
        t_symbol *s_minimize
        t_symbol *s_minplanecount
        t_symbol *s_minus
        t_symbol *s_mode
        t_symbol *s_modified
        t_symbol *s_monitor
        t_symbol *s_months
        t_symbol *s_mousedoubleclick
        t_symbol *s_mousedown
        t_symbol *s_mousedownonchar
        t_symbol *s_mousedrag
        t_symbol *s_mousedragdelta
        t_symbol *s_mouseenter
        t_symbol *s_mouseleave
        t_symbol *s_mousemove
        t_symbol *s_mousescale
        t_symbol *s_mouseup
        t_symbol *s_mousewheel
        t_symbol *s_move
        t_symbol *s_moveboxes
        t_symbol *s_moved
        t_symbol *s_moveifoffdisplay
        t_symbol *s_movelines
        t_symbol *s_movesegment
        t_symbol *s_moviedim
        t_symbol *s_moviefile
        t_symbol *s_ms
        t_symbol *s_ms_ansi
        t_symbol *s_mulaw
        t_symbol *s_mult
        t_symbol *s_multiboxcontextmenu
        t_symbol *s_multilinecontextmenu
        t_symbol *s_mute
        t_symbol *s_name
        t_symbol *s_name_changed
        t_symbol *s_name_rowcomponent
        t_symbol *s_name_setvalue
        t_symbol *s_name_textcolor
        t_symbol *s_name_textstyle
        t_symbol *s_name_width
        t_symbol *s_nameinspector
        t_symbol *s_nativewindow
        t_symbol *s_navkey
        t_symbol *s_new
        t_symbol *s_newcopy
        t_symbol *s_newdefault
        t_symbol *s_newex
        t_symbol *s_newfilebrowser
        t_symbol *s_newfolder
        t_symbol *s_newlines
        t_symbol *s_newobj
        t_symbol *s_newobject
        t_symbol *s_newobjects
        t_symbol *s_newpatcherview
        t_symbol *s_newpatchline
        t_symbol *s_newquery
        t_symbol *s_next
        t_symbol *s_nextline
        t_symbol *s_nextobject
        t_symbol *s_nextrecord
        t_symbol *s_nextview
        t_symbol *s_nfilters
        t_symbol *s_No
        t_symbol *s_noactivate
        t_symbol *s_nobox
        t_symbol *s_noclipse
        t_symbol *s_noclose
        t_symbol *s_noedit
        t_symbol *s_noeval
        t_symbol *s_nofloat
        t_symbol *s_nofontpanel
        t_symbol *s_nogrow
        t_symbol *s_nomad
        t_symbol *s_nomenu
        t_symbol *s_nominimize
        t_symbol *s_none
        t_symbol *s_nonomad
        t_symbol *s_normalheight
        t_symbol *s_normalwidth
        t_symbol *s_noscroll
        t_symbol *s_not
        t_symbol *s_notevalues
        t_symbol *s_nothing
        t_symbol *s_notify
        t_symbol *s_notifyall
        t_symbol *s_notifyall_delete
        t_symbol *s_notifyall_new
        t_symbol *s_notitle
        t_symbol *s_nozoom
        t_symbol *s_nth
        t_symbol *s_number
        t_symbol *s_number_paint
        t_symbol *s_number_rowcomponent
        t_symbol *s_number_setvalue
        t_symbol *s_numfields
        t_symbol *s_nummidpoints
        t_symbol *s_numrecords
        t_symbol *s_numresults
        t_symbol *s_numrowschanged
        t_symbol *s_numtabs
        t_symbol *s_numviews
        t_symbol *s_numwindowviews
        t_symbol *s_ob_sym
        t_symbol *s_obex_container
        t_symbol *s_objargs
        t_symbol *s_object
        t_symbol *s_object_id
        t_symbol *s_objectcount
        t_symbol *s_objectfilename
        t_symbol *s_objectlist
        t_symbol *s_objectpalette
        t_symbol *s_objectview_doubleclick
        t_symbol *s_objectview_selected
        t_symbol *s_objptr2index
        t_symbol *s_objtype
        t_symbol *s_obtrusiveerror
        t_symbol *s_offset
        t_symbol *s_offset_rowcomponent
        t_symbol *s_offset_setvalue
        t_symbol *s_offsetfrom
        t_symbol *s_offsetmidpoints
        t_symbol *s_okclose
        t_symbol *s_oksize
        t_symbol *s_one
        t_symbol *s_onoff
        t_symbol *s_opaque
        t_symbol *s_open
        t_symbol *s_openassoc
        t_symbol *s_openfile
        t_symbol *s_openinpresentation
        t_symbol *s_openquery
        t_symbol *s_openrect
        t_symbol *s_openweb
        t_symbol *s_optional
        t_symbol *s_optionsdirty
        t_symbol *s_order
        t_symbol *s_order_by
        t_symbol *s_orderbefore
        t_symbol *s_orderfront
        t_symbol *s_orientation
        t_symbol *s_outlet
        t_symbol *s_outletnum
        t_symbol *s_outline
        t_symbol *s_outmode
        t_symbol *s_output
        t_symbol *s_outputcount
        t_symbol *s_outputmatrix
        t_symbol *s_outputmode
        t_symbol *s_overdrive
        t_symbol *s_owner
        t_symbol *s_ownervis
        t_symbol *s_p
        t_symbol *s_paint
        t_symbol *s_paintsbackground
        t_symbol *s_palette_action
        t_symbol *s_palette_caption
        t_symbol *s_palette_category
        t_symbol *s_palette_numerical_order
        t_symbol *s_palette_order
        t_symbol *s_palette_protocount
        t_symbol *s_palette_tab_action
        t_symbol *s_parameter_enable
        t_symbol *s_parent
        t_symbol *s_parentclass
        t_symbol *s_parentpatcher
        t_symbol *s_parse
        t_symbol *s_parsefile
        t_symbol *s_paste
        t_symbol *s_pastefileintoobject
        t_symbol *s_pastefrom
        t_symbol *s_pastereplace
        t_symbol *s_patcher
        t_symbol *s_patchercomponent
        t_symbol *s_patchercontextmenu
        t_symbol *s_patchereditor
        t_symbol *s_patchername
        t_symbol *s_patchernotify
        t_symbol *s_patcherview
        t_symbol *s_patcherview_instances
        t_symbol *s_patcherview_invis
        t_symbol *s_patcherview_notify_locked
        t_symbol *s_patcherview_notify_presentation
        t_symbol *s_patcherview_vis
        t_symbol *s_patching_position
        t_symbol *s_patching_rect
        t_symbol *s_patching_size
        t_symbol *s_patchline
        t_symbol *s_path
        t_symbol *s_pattrstorage
        t_symbol *s_pclose
        t_symbol *s_pending
        t_symbol *s_pic
        t_symbol *s_pictctrl
        t_symbol *s_plane
        t_symbol *s_planecount
        t_symbol *s_planelink
        t_symbol *s_plugconfig
        t_symbol *s_plus
        t_symbol *s_png
        t_symbol *s_pointer
        t_symbol *s_popupmenu
        t_symbol *s_portenable
        t_symbol *s_position
        t_symbol *s_postname
        t_symbol *s_pound_B
        t_symbol *s_pound_D
        t_symbol *s_pound_N
        t_symbol *s_pound_P
        t_symbol *s_pound_X
        t_symbol *s_preferences
        t_symbol *s_preload
        t_symbol *s_presentation
        t_symbol *s_presentation_linecount
        t_symbol *s_presentation_position
        t_symbol *s_presentation_rect
        t_symbol *s_presentation_size
        t_symbol *s_preset
        t_symbol *s_preset_data
        t_symbol *s_preview
        t_symbol *s_preview_image
        t_symbol *s_previewheight
        t_symbol *s_previewimagedata
        t_symbol *s_prevobject
        t_symbol *s_print
        t_symbol *s_priority
        t_symbol *s_prototype
        t_symbol *s_prototype_binbuf
        t_symbol *s_prototypename
        t_symbol *s_psave
        t_symbol *s_pulsate
        t_symbol *s_pupdate
        t_symbol *s_quantifier
        t_symbol *s_quantifier_exists
        t_symbol *s_quantifiers
        t_symbol *s_quantize
        t_symbol *s_queries
        t_symbol *s_query
        t_symbol *s_query_selected
        t_symbol *s_querycontroller
        t_symbol *s_queryid
        t_symbol *s_querylist
        t_symbol *s_queryname
        t_symbol *s_querypreview
        t_symbol *s_querysearch
        t_symbol *s_quickmap
        t_symbol *s_quit
        t_symbol *s_quitting
        t_symbol *s_radial
        t_symbol *s_range
        t_symbol *s_rawfind
        t_symbol *s_rawwindsave
        t_symbol *s_rawwindsaveas
        t_symbol *s_read
        t_symbol *s_readonly
        t_symbol *s_realclass
        t_symbol *s_rebuilding
        t_symbol *s_recordbyindex
        t_symbol *s_recreate_invis
        t_symbol *s_recreate_vis
        t_symbol *s_rect
        t_symbol *s_rectangle
        t_symbol *s_rectangular
        t_symbol *s_red
        t_symbol *s_redostack
        t_symbol *s_redraw
        t_symbol *s_redrawcontents
        t_symbol *s_reference
        t_symbol *s_reffile
        t_symbol *s_refinequery
        t_symbol *s_refresh
        t_symbol *s_register
        t_symbol *s_remove
        t_symbol *s_removeattr
        t_symbol *s_removeattr_enable
        t_symbol *s_removeboxlayer
        t_symbol *s_removeclient
        t_symbol *s_removefrompresentation
        t_symbol *s_removelines
        t_symbol *s_removeobjects
        t_symbol *s_removesegment
        t_symbol *s_removeslot
        t_symbol *s_removewiretap
        t_symbol *s_rename
        t_symbol *s_renumberslot
        t_symbol *s_replace
        t_symbol *s_replaced_args
        t_symbol *s_reschedule
        t_symbol *s_reset
        t_symbol *s_resize
        t_symbol *s_resizeaction
        t_symbol *s_resized
        t_symbol *s_resizelimits
        t_symbol *s_resizenotify
        t_symbol *s_resolve_name
        t_symbol *s_resolve_raw
        t_symbol *s_resort
        t_symbol *s_resource
        t_symbol *s_respondtoclick
        t_symbol *s_restore
        t_symbol *s_restrict_dim
        t_symbol *s_restrict_planecount
        t_symbol *s_restrict_type
        t_symbol *s_result
        t_symbol *s_retain
        t_symbol *s_revealinfinder
        t_symbol *s_reverse
        t_symbol *s_rgb
        t_symbol *s_rgba
        t_symbol *s_rolename
        t_symbol *s_rotate
        t_symbol *s_rounded
        t_symbol *s_rowcolorchanged
        t_symbol *s_rowcomponent
        t_symbol *s_rowenabled
        t_symbol *s_rowhead
        t_symbol *s_rowheightchanged
        t_symbol *s_safebang
        t_symbol *s_safeclear
        t_symbol *s_samples
        t_symbol *s_save
        t_symbol *s_save2
        t_symbol *s_saveas
        t_symbol *s_saveboxprototype
        t_symbol *s_saved_object_attributes
        t_symbol *s_savefilepath
        t_symbol *s_savelasttab
        t_symbol *s_savequery
        t_symbol *s_saveto
        t_symbol *s_savewindow
        t_symbol *s_savingdefault
        t_symbol *s_scale
        t_symbol *s_script
        t_symbol *s_scrollanimatetime
        t_symbol *s_scrolloffset
        t_symbol *s_scrollorigin
        t_symbol *s_scrollposition
        t_symbol *s_scrollselectedobjectsintoview
        t_symbol *s_scrollto
        t_symbol *s_scrollviewtoshow
        t_symbol *s_search
        t_symbol *s_searchterm
        t_symbol *s_searchtext
        t_symbol *s_select
        t_symbol *s_selectallonedit
        t_symbol *s_selectbox
        t_symbol *s_selectcategory
        t_symbol *s_selectcell
        t_symbol *s_selectdropped
        t_symbol *s_selected
        t_symbol *s_selectedboxes
        t_symbol *s_selectedlines
        t_symbol *s_selectedrow
        t_symbol *s_selectedrow_contextual
        t_symbol *s_selfsave
        t_symbol *s_selmode
        t_symbol *s_send
        t_symbol *s_sendbackward
        t_symbol *s_sendbox
        t_symbol *s_sendboxmsg
        t_symbol *s_senderclasssym
        t_symbol *s_sendtoback
        t_symbol *s_session_usage_count
        t_symbol *s_set
        t_symbol *s_setall
        t_symbol *s_setassoc
        t_symbol *s_setatoms
        t_symbol *s_setattr
        t_symbol *s_setboxrect
        t_symbol *s_setcellunits
        t_symbol *s_setcellvalue
        t_symbol *s_setcontainedcomponent
        t_symbol *s_setdata
        t_symbol *s_setdefaults
        t_symbol *s_setdirty
        t_symbol *s_seteditboxcaretposition
        t_symbol *s_seteditboxhighlightedregion
        t_symbol *s_seteditview
        t_symbol *s_setfilter
        t_symbol *s_setflags
        t_symbol *s_setfont
        t_symbol *s_setglobalcoords
        t_symbol *s_setinfo
        t_symbol *s_setinfo_ex
        t_symbol *s_setmethod
        t_symbol *s_setmidpoint
        t_symbol *s_setpatcherattr
        t_symbol *s_setptr
        t_symbol *s_setrect
        t_symbol *s_setspecialrow
        t_symbol *s_settext
        t_symbol *s_setunitsym
        t_symbol *s_setvalue
        t_symbol *s_setvalueof
        t_symbol *s_setvaluetext
        t_symbol *s_setwindowrect
        t_symbol *s_setwindowsize
        t_symbol *s_setzorder
        t_symbol *s_shortcut
        t_symbol *s_show
        t_symbol *s_showcaption
        t_symbol *s_showdoc
        t_symbol *s_showfind
        t_symbol *s_showpreview
        t_symbol *s_showrecent
        t_symbol *s_showrow
        t_symbol *s_showsaveable
        t_symbol *s_showtarget
        t_symbol *s_shuffle
        t_symbol *s_signal
        t_symbol *s_sinceyesterday
        t_symbol *s_singleinspector
        t_symbol *s_size
        t_symbol *s_sizeboxes
        t_symbol *s_slot_definition
        t_symbol *s_slot_modified
        t_symbol *s_slots
        t_symbol *s_smpte
        t_symbol *s_snaptogrid
        t_symbol *s_sort
        t_symbol *s_sortablechanged
        t_symbol *s_sortcolumn
        t_symbol *s_sortdata
        t_symbol *s_sorted
        t_symbol *s_sorted_by_column
        t_symbol *s_source
        t_symbol *s_spacing
        t_symbol *s_special
        t_symbol *s_specialclick
        t_symbol *s_specialcount
        t_symbol *s_sql
        t_symbol *s_sql2
        t_symbol *s_sqlite
        t_symbol *s_sqlite_result
        t_symbol *s_sqlstring
        t_symbol *s_sr
        t_symbol *s_start
        t_symbol *s_startdrag
        t_symbol *s_startmoveboxes
        t_symbol *s_startpoint
        t_symbol *s_startprobe
        t_symbol *s_starttransaction
        t_symbol *s_startwindow
        t_symbol *s_state
        t_symbol *s_sticky_attr
        t_symbol *s_sticky_method
        t_symbol *s_stop
        t_symbol *s_store
        t_symbol *s_straighten
        t_symbol *s_straightend
        t_symbol *s_straightstart
        t_symbol *s_straightthresh
        t_symbol *s_string
        t_symbol *s_style
        t_symbol *s_sub
        t_symbol *s_subpatcher
        t_symbol *s_surfacebuffer
        t_symbol *s_svg
        t_symbol *s_swap
        t_symbol *s_swatches
        t_symbol *s_symbol
        t_symbol *s_sysqelem
        t_symbol *s_t
        t_symbol *s_tab_bgcolor
        t_symbol *s_table
        t_symbol *s_tabledata
        t_symbol *s_tag
        t_symbol *s_tags
        t_symbol *s_tapcontroller_show
        t_symbol *s_tapwindow
        t_symbol *s_target
        t_symbol *s_template
        t_symbol *s_tempo
        t_symbol *s_text
        t_symbol *s_text_commaseparated
        t_symbol *s_text_large
        t_symbol *s_text_onesymbol
        t_symbol *s_textchanged
        t_symbol *s_textcolor
        t_symbol *s_textfield
        t_symbol *s_textfile
        t_symbol *s_textmargins
        t_symbol *s_textstyle
        t_symbol *s_thismonth
        t_symbol *s_thisweek
        t_symbol *s_threadpriority
        t_symbol *s_threadsafe
        t_symbol *s_three
        t_symbol *s_ticks
        t_symbol *s_time
        t_symbol *s_timeobj
        t_symbol *s_timesigchanged
        t_symbol *s_timeupdate
        t_symbol *s_tinge
        t_symbol *s_title
        t_symbol *s_titleassoc
        t_symbol *s_tobinbuf
        t_symbol *s_toc
        t_symbol *s_today
        t_symbol *s_todictionary
        t_symbol *s_togworld
        t_symbol *s_tool
        t_symbol *s_tool_paint
        t_symbol *s_tool_rowcomponent
        t_symbol *s_tool_setvalue
        t_symbol *s_tool_sort
        t_symbol *s_toolbarheight
        t_symbol *s_toolbarid
        t_symbol *s_toolbars
        t_symbol *s_toolbarvisible
        t_symbol *s_top_inset
        t_symbol *s_topmost
        t_symbol *s_toppatcher
        t_symbol *s_topredoname
        t_symbol *s_topundoname
        t_symbol *s_trackaction
        t_symbol *s_transparent
        t_symbol *s_transport
        t_symbol *s_traverse
        t_symbol *s_triangle
        t_symbol *s_tutorial
        t_symbol *s_two
        t_symbol *s_twobytecomment
        t_symbol *s_type
        t_symbol *s_typed_usage_count
        t_symbol *s_typedwrapper
        t_symbol *s_typelink
        t_symbol *s_types
        t_symbol *s_underline
        t_symbol *s_understands
        t_symbol *s_undo
        t_symbol *s_undostack
        t_symbol *s_unimbed
        t_symbol *s_uninitialized
        t_symbol *s_unique
        t_symbol *s_unit
        t_symbol *s_units
        t_symbol *s_unlocked_iocolor
        t_symbol *s_unlockedinteraction
        t_symbol *s_unset
        t_symbol *s_up
        t_symbol *s_update
        t_symbol *s_update_metadata
        t_symbol *s_updatecommand
        t_symbol *s_updatecontent
        t_symbol *s_updatefind
        t_symbol *s_updatelookandfeel
        t_symbol *s_updatemenu
        t_symbol *s_updateprototypes
        t_symbol *s_updatequery
        t_symbol *s_updatequerydict
        t_symbol *s_updaterect
        t_symbol *s_url
        t_symbol *s_us_ascii
        t_symbol *s_usage_count
        t_symbol *s_usedslotlist
        t_symbol *s_useellipsis
        t_symbol *s_useimagebuffer
        t_symbol *s_usemax
        t_symbol *s_usemin
        t_symbol *s_user
        t_symbol *s_usercanget
        t_symbol *s_usercanset
        t_symbol *s_utf_16
        t_symbol *s_utf_16be
        t_symbol *s_utf_16le
        t_symbol *s_utf_8
        t_symbol *s_val
        t_symbol *s_value
        t_symbol *s_value_endchange
        t_symbol *s_value_rowcomponent
        t_symbol *s_valuebyindex
        t_symbol *s_varname
        t_symbol *s_varname_set
        t_symbol *s_varname_unset
        t_symbol *s_version
        t_symbol *s_videofile
        t_symbol *s_views
        t_symbol *s_viewspan
        t_symbol *s_vignette
        t_symbol *s_vis
        t_symbol *s_visibilitychanged
        t_symbol *s_visible
        t_symbol *s_visiblecanvasrect
        t_symbol *s_vol
        t_symbol *s_vpicture
        t_symbol *s_vpreset
        t_symbol *s_wantfree
        t_symbol *s_wantsreturn
        t_symbol *s_wantstab
        t_symbol *s_watch
        t_symbol *s_watchpoint_flags
        t_symbol *s_watchpoint_id
        t_symbol *s_wclose
        t_symbol *s_webpage
        t_symbol *s_weeks
        t_symbol *s_wind
        t_symbol *s_window
        t_symbol *s_windowbounds
        t_symbol *s_windowrect
        t_symbol *s_windsave
        t_symbol *s_wiretap
        t_symbol *s_withinlast
        t_symbol *s_wobjectname
        t_symbol *s_wordwrap
        t_symbol *s_workspace
        t_symbol *s_write
        t_symbol *s_writedictionary
        t_symbol *s_xmlfile
        t_symbol *s_years
        t_symbol *s_zero
        t_symbol *s_zoom
        t_symbol *s_zoombox
        t_symbol *s_zoomfactor
        t_symbol *s_zorder
        t_symbol *s_zzz
        t_symbol *s_Sans_Serif
    cdef t_common_symbols_table *_common_symbols
    cdef void common_symbols_init()
    cdef t_common_symbols_table *common_symbols_gettable()

    # these are globals
    cdef t_symbol* _sym__preset
    cdef t_symbol* _sym_abbrev
    cdef t_symbol* _sym_abbrev_rowcomponent
    cdef t_symbol* _sym_abbrev_setvalue
    cdef t_symbol* _sym_acceptsdrag
    cdef t_symbol* _sym_acceptsdrag_locked
    cdef t_symbol* _sym_acceptsdrag_unlocked
    cdef t_symbol* _sym_action
    cdef t_symbol* _sym_action_rowcomponent
    cdef t_symbol* _sym_action_setvalue
    cdef t_symbol* _sym_activate
    cdef t_symbol* _sym_active
    cdef t_symbol* _sym_activetab
    cdef t_symbol* _sym_activetabname
    cdef t_symbol* _sym_activewindow
    cdef t_symbol* _sym_adapt
    cdef t_symbol* _sym_add
    cdef t_symbol* _sym_addattr
    cdef t_symbol* _sym_addattr_enable
    cdef t_symbol* _sym_addclient
    cdef t_symbol* _sym_addfolder
    cdef t_symbol* _sym_addfolderandsave
    cdef t_symbol* _sym_addquerydict
    cdef t_symbol* _sym_addquerydictfromfile
    cdef t_symbol* _sym_addslot
    cdef t_symbol* _sym_addtopresentation
    cdef t_symbol* _sym_addwiretap
    cdef t_symbol* _sym_adornments
    cdef t_symbol* _sym_alias
    cdef t_symbol* _sym_alignboxes
    cdef t_symbol* _sym_alignconnections
    cdef t_symbol* _sym_alignlines
    cdef t_symbol* _sym_all
    cdef t_symbol* _sym_allkinds
    cdef t_symbol* _sym_allowmod
    cdef t_symbol* _sym_alpha
    cdef t_symbol* _sym_annotation
    cdef t_symbol* _sym_annotation_name
    cdef t_symbol* _sym_anydate
    cdef t_symbol* _sym_anykind
    cdef t_symbol* _sym_anything
    cdef t_symbol* _sym_append
    cdef t_symbol* _sym_append_sql
    cdef t_symbol* _sym_appendatoms
    cdef t_symbol* _sym_appendtodictionary
    cdef t_symbol* _sym_apply
    cdef t_symbol* _sym_applyboxprototype
    cdef t_symbol* _sym_applydeep
    cdef t_symbol* _sym_applydeepif
    cdef t_symbol* _sym_applyif
    cdef t_symbol* _sym_args
    cdef t_symbol* _sym_argument
    cdef t_symbol* _sym_arguments
    cdef t_symbol* _sym_argv
    cdef t_symbol* _sym_ascending
    cdef t_symbol* _sym_aspect
    cdef t_symbol* _sym_assist
    cdef t_symbol* _sym_assoc
    cdef t_symbol* _sym_atbclick
    cdef t_symbol* _sym_atom
    cdef t_symbol* _sym_atomarray
    cdef t_symbol* _sym_attach
    cdef t_symbol* _sym_attr_filter_clip
    cdef t_symbol* _sym_attr_get
    cdef t_symbol* _sym_attr_getnames
    cdef t_symbol* _sym_attr_gettarget
    cdef t_symbol* _sym_attr_modified
    cdef t_symbol* _sym_attr_offset_array
    cdef t_symbol* _sym_attr_renamed
    cdef t_symbol* _sym_attr_setdisabled
    cdef t_symbol* _sym_attr_setinvisible
    cdef t_symbol* _sym_attribute
    cdef t_symbol* _sym_attributes
    cdef t_symbol* _sym_attrname
    cdef t_symbol* _sym_audiofile
    cdef t_symbol* _sym_audioplugin
    cdef t_symbol* _sym_author
    cdef t_symbol* _sym_autocompletion
    cdef t_symbol* _sym_autocompletion_query
    cdef t_symbol* _sym_autofixwidth
    cdef t_symbol* _sym_autoheightchanged
    cdef t_symbol* _sym_autoscroll
    cdef t_symbol* _sym_back
    cdef t_symbol* _sym_background
    cdef t_symbol* _sym_bang
    cdef t_symbol* _sym_bbu
    cdef t_symbol* _sym_bclear
    cdef t_symbol* _sym_bcopy
    cdef t_symbol* _sym_bcut
    cdef t_symbol* _sym_begineditbox
    cdef t_symbol* _sym_beginswith
    cdef t_symbol* _sym_beginswithorcontains
    cdef t_symbol* _sym_bfixwidth
    cdef t_symbol* _sym_bfont
    cdef t_symbol* _sym_bgcolor
    cdef t_symbol* _sym_bgcount
    cdef t_symbol* _sym_bghidden
    cdef t_symbol* _sym_bglocked
    cdef t_symbol* _sym_bgmode
    cdef t_symbol* _sym_blue
    cdef t_symbol* _sym_bogus
    cdef t_symbol* _sym_bold
    cdef t_symbol* _sym_border
    cdef t_symbol* _sym_borderchanged
    cdef t_symbol* _sym_bottom_inset
    cdef t_symbol* _sym_boundingbox
    cdef t_symbol* _sym_bounds
    cdef t_symbol* _sym_box
    cdef t_symbol* _sym_box1
    cdef t_symbol* _sym_box2
    cdef t_symbol* _sym_boxalpha
    cdef t_symbol* _sym_boxanimatetime
    cdef t_symbol* _sym_boxcomponent
    cdef t_symbol* _sym_boxcontextitems
    cdef t_symbol* _sym_boxcontextmenu
    cdef t_symbol* _sym_boxes
    cdef t_symbol* _sym_boxlayer
    cdef t_symbol* _sym_boxnotify
    cdef t_symbol* _sym_boxscreenrectchanged
    cdef t_symbol* _sym_bpaste
    cdef t_symbol* _sym_bpastepic
    cdef t_symbol* _sym_bpatcher
    cdef t_symbol* _sym_bpatcher_holder
    cdef t_symbol* _sym_bpm
    cdef t_symbol* _sym_bracket_default
    cdef t_symbol* _sym_bracket_none
    cdef t_symbol* _sym_break
    cdef t_symbol* _sym_bredo
    cdef t_symbol* _sym_brgba
    cdef t_symbol* _sym_bringforward
    cdef t_symbol* _sym_bringtofront
    cdef t_symbol* _sym_bubblesize
    cdef t_symbol* _sym_build
    cdef t_symbol* _sym_buildcolumns
    cdef t_symbol* _sym_bundo
    cdef t_symbol* _sym_button
    cdef t_symbol* _sym_c74object
    cdef t_symbol* _sym_canback
    cdef t_symbol* _sym_cancopychanged
    cdef t_symbol* _sym_candropfiles
    cdef t_symbol* _sym_canforward
    cdef t_symbol* _sym_canhilite
    cdef t_symbol* _sym_canmovebackward
    cdef t_symbol* _sym_canmoveforward
    cdef t_symbol* _sym_canpastechanged
    cdef t_symbol* _sym_canremove
    cdef t_symbol* _sym_cansave
    cdef t_symbol* _sym_canschedule
    cdef t_symbol* _sym_canselectchanged
    cdef t_symbol* _sym_canvastoscreen
    cdef t_symbol* _sym_caption
    cdef t_symbol* _sym_catcolors
    cdef t_symbol* _sym_category
    cdef t_symbol* _sym_category_first
    cdef t_symbol* _sym_cell
    cdef t_symbol* _sym_cell_clue
    cdef t_symbol* _sym_cellclue
    cdef t_symbol* _sym_cellenabled
    cdef t_symbol* _sym_cellschanged
    cdef t_symbol* _sym_char
    cdef t_symbol* _sym_char_comma
    cdef t_symbol* _sym_char_minus
    cdef t_symbol* _sym_char_semi
    cdef t_symbol* _sym_char_space
    cdef t_symbol* _sym_charset_converter
    cdef t_symbol* _sym_checkbox
    cdef t_symbol* _sym_choose
    cdef t_symbol* _sym_chord
    cdef t_symbol* _sym_chuck
    cdef t_symbol* _sym_chuckindex
    cdef t_symbol* _sym_class
    cdef t_symbol* _sym_class_jit_attribute
    cdef t_symbol* _sym_class_jit_matrix
    cdef t_symbol* _sym_class_jit_namespace
    cdef t_symbol* _sym_classname
    cdef t_symbol* _sym_classsym
    cdef t_symbol* _sym_clear
    cdef t_symbol* _sym_clearactions
    cdef t_symbol* _sym_clearcolumns
    cdef t_symbol* _sym_clearitem
    cdef t_symbol* _sym_clearslots
    cdef t_symbol* _sym_click
    cdef t_symbol* _sym_clickaction
    cdef t_symbol* _sym_clientcontext
    cdef t_symbol* _sym_clipboard
    cdef t_symbol* _sym_clipping
    cdef t_symbol* _sym_clock
    cdef t_symbol* _sym_close
    cdef t_symbol* _sym_closebang
    cdef t_symbol* _sym_clue_cell
    cdef t_symbol* _sym_clue_header
    cdef t_symbol* _sym_clueclass
    cdef t_symbol* _sym_cluelookupattr
    cdef t_symbol* _sym_cluename
    cdef t_symbol* _sym_clues
    cdef t_symbol* _sym_colhead
    cdef t_symbol* _sym_coll
    cdef t_symbol* _sym_collectfiles
    cdef t_symbol* _sym_collective
    cdef t_symbol* _sym_color
    cdef t_symbol* _sym_colorvalue
    cdef t_symbol* _sym_columnadded
    cdef t_symbol* _sym_columnclue
    cdef t_symbol* _sym_columndeleted
    cdef t_symbol* _sym_columnheaderclue
    cdef t_symbol* _sym_columnminmaxchanged
    cdef t_symbol* _sym_columnnamechanged
    cdef t_symbol* _sym_columns
    cdef t_symbol* _sym_columnwidthchanged
    cdef t_symbol* _sym_com
    cdef t_symbol* _sym_command
    cdef t_symbol* _sym_command_enable
    cdef t_symbol* _sym_commandgroup
    cdef t_symbol* _sym_commands
    cdef t_symbol* _sym_comment
    cdef t_symbol* _sym_comparison
    cdef t_symbol* _sym_comparisons
    cdef t_symbol* _sym_completeconnection
    cdef t_symbol* _sym_connect
    cdef t_symbol* _sym_connectcolor
    cdef t_symbol* _sym_containersizechange
    cdef t_symbol* _sym_contains
    cdef t_symbol* _sym_contextmenu
    cdef t_symbol* _sym_contextualpaste
    cdef t_symbol* _sym_controller
    cdef t_symbol* _sym_convert
    cdef t_symbol* _sym_copy
    cdef t_symbol* _sym_copyatoms
    cdef t_symbol* _sym_copyjundo
    cdef t_symbol* _sym_coremidi
    cdef t_symbol* _sym_count
    cdef t_symbol* _sym_create_backup
    cdef t_symbol* _sym_createbpatcher
    cdef t_symbol* _sym_createsubpatcher
    cdef t_symbol* _sym_current
    cdef t_symbol* _sym_cut
    cdef t_symbol* _sym_data
    cdef t_symbol* _sym_date_created
    cdef t_symbol* _sym_date_lastaccessed
    cdef t_symbol* _sym_date_modified
    cdef t_symbol* _sym_days
    cdef t_symbol* _sym_dblclick
    cdef t_symbol* _sym_debugwindow_show
    cdef t_symbol* _sym_decorator
    cdef t_symbol* _sym_default
    cdef t_symbol* _sym_default_fontface
    cdef t_symbol* _sym_default_fontname
    cdef t_symbol* _sym_default_fontsize
    cdef t_symbol* _sym_default_matrixplcolor
    cdef t_symbol* _sym_default_plcolor
    cdef t_symbol* _sym_default_query
    cdef t_symbol* _sym_default_sigplcolor
    cdef t_symbol* _sym_defaultcommand
    cdef t_symbol* _sym_defaultfocusbox
    cdef t_symbol* _sym_defaultname
    cdef t_symbol* _sym_defaultquery
    cdef t_symbol* _sym_defaults
    cdef t_symbol* _sym_defaultvaluechanged
    cdef t_symbol* _sym_definearg
    cdef t_symbol* _sym_defineargument
    cdef t_symbol* _sym_definecomparison
    cdef t_symbol* _sym_definefield
    cdef t_symbol* _sym_definefolder
    cdef t_symbol* _sym_definequantifier
    cdef t_symbol* _sym_definequery
    cdef t_symbol* _sym_defineslot
    cdef t_symbol* _sym_definesort
    cdef t_symbol* _sym_defrect
    cdef t_symbol* _sym_delete
    cdef t_symbol* _sym_deleteindex
    cdef t_symbol* _sym_deletemetadata
    cdef t_symbol* _sym_deletequery
    cdef t_symbol* _sym_depthbuffer
    cdef t_symbol* _sym_descending
    cdef t_symbol* _sym_description
    cdef t_symbol* _sym_dest_closing
    cdef t_symbol* _sym_destination
    cdef t_symbol* _sym_destrect
    cdef t_symbol* _sym_destroy
    cdef t_symbol* _sym_detach
    cdef t_symbol* _sym_devicerects
    cdef t_symbol* _sym_dictionary
    cdef t_symbol* _sym_digest
    cdef t_symbol* _sym_dim
    cdef t_symbol* _sym_dimlink
    cdef t_symbol* _sym_dirty
    cdef t_symbol* _sym_disabled
    cdef t_symbol* _sym_disablewiretap
    cdef t_symbol* _sym_disconnect
    cdef t_symbol* _sym_do_get_shared_context
    cdef t_symbol* _sym_document
    cdef t_symbol* _sym_docwindow_docrect
    cdef t_symbol* _sym_docwindow_refrect
    cdef t_symbol* _sym_docwindow_searchrect
    cdef t_symbol* _sym_docwindow_tocrect
    cdef t_symbol* _sym_docwindow_tutrect
    cdef t_symbol* _sym_domain
    cdef t_symbol* _sym_done
    cdef t_symbol* _sym_donewobj
    cdef t_symbol* _sym_dontsave
    cdef t_symbol* _sym_doublebuffer
    cdef t_symbol* _sym_doubleclick
    cdef t_symbol* _sym_doubleclickaction
    cdef t_symbol* _sym_doubleclicked
    cdef t_symbol* _sym_down
    cdef t_symbol* _sym_downcaption
    cdef t_symbol* _sym_downicon
    cdef t_symbol* _sym_drag
    cdef t_symbol* _sym_dragactive
    cdef t_symbol* _sym_dragdrop
    cdef t_symbol* _sym_dragenter
    cdef t_symbol* _sym_dragexit
    cdef t_symbol* _sym_draggablechanged
    cdef t_symbol* _sym_dragmove
    cdef t_symbol* _sym_dragrole
    cdef t_symbol* _sym_dragtarget
    cdef t_symbol* _sym_drawfirstin
    cdef t_symbol* _sym_drawinlast
    cdef t_symbol* _sym_drawsresizer
    cdef t_symbol* _sym_dropaction
    cdef t_symbol* _sym_dropaction_addapplyprototype
    cdef t_symbol* _sym_dropaction_addconnectedmessage
    cdef t_symbol* _sym_dropaction_addcustom
    cdef t_symbol* _sym_dropaction_addcustom_extended
    cdef t_symbol* _sym_dropaction_addobjectcreation
    cdef t_symbol* _sym_dropaction_addopeninnewwindow
    cdef t_symbol* _sym_dropaction_addprototypeobjectcreation
    cdef t_symbol* _sym_dropfiles
    cdef t_symbol* _sym_droprole
    cdef t_symbol* _sym_droprole_locked
    cdef t_symbol* _sym_droprole_unlocked
    cdef t_symbol* _sym_dsp
    cdef t_symbol* _sym_dump
    cdef t_symbol* _sym_dumpboxes
    cdef t_symbol* _sym_dumpout
    cdef t_symbol* _sym_duplicate
    cdef t_symbol* _sym_edit
    cdef t_symbol* _sym_edit_framecolor
    cdef t_symbol* _sym_editactive
    cdef t_symbol* _sym_editbox
    cdef t_symbol* _sym_editcell
    cdef t_symbol* _sym_editing_bgcolor
    cdef t_symbol* _sym_editmetadata
    cdef t_symbol* _sym_editonclick
    cdef t_symbol* _sym_editor
    cdef t_symbol* _sym_elements
    cdef t_symbol* _sym_embed
    cdef t_symbol* _sym_emptytext
    cdef t_symbol* _sym_enable
    cdef t_symbol* _sym_enable_rowcomponent
    cdef t_symbol* _sym_enable_setvalue
    cdef t_symbol* _sym_enablehscroll
    cdef t_symbol* _sym_enabler
    cdef t_symbol* _sym_enablevscroll
    cdef t_symbol* _sym_enddrag
    cdef t_symbol* _sym_endmoveboxes
    cdef t_symbol* _sym_endpoint
    cdef t_symbol* _sym_endprobe
    cdef t_symbol* _sym_endswith
    cdef t_symbol* _sym_endtransaction
    cdef t_symbol* _sym_enter
    cdef t_symbol* _sym_entertext
    cdef t_symbol* _sym_enum
    cdef t_symbol* _sym_enumindex
    cdef t_symbol* _sym_enummsg
    cdef t_symbol* _sym_enumtarget
    cdef t_symbol* _sym_enumvals
    cdef t_symbol* _sym_equalto
    cdef t_symbol* _sym_error
    cdef t_symbol* _sym_eventinterval
    cdef t_symbol* _sym_everything
    cdef t_symbol* _sym_excludebg
    cdef t_symbol* _sym_exec
    cdef t_symbol* _sym_execstring
    cdef t_symbol* _sym_execstring_safe
    cdef t_symbol* _sym_execute
    cdef t_symbol* _sym_executefind
    cdef t_symbol* _sym_extension
    cdef t_symbol* _sym_extra
    cdef t_symbol* _sym_ezquery
    cdef t_symbol* _sym_fadetinge
    cdef t_symbol* _sym_fgcolor
    cdef t_symbol* _sym_fghidden
    cdef t_symbol* _sym_field
    cdef t_symbol* _sym_fieldnamebyindex
    cdef t_symbol* _sym_fields
    cdef t_symbol* _sym_file
    cdef t_symbol* _sym_filefolder
    cdef t_symbol* _sym_fileformat
    cdef t_symbol* _sym_filename
    cdef t_symbol* _sym_filepath
    cdef t_symbol* _sym_filetypemessage
    cdef t_symbol* _sym_fileusage
    cdef t_symbol* _sym_filter
    cdef t_symbol* _sym_filterget
    cdef t_symbol* _sym_filterset
    cdef t_symbol* _sym_find
    cdef t_symbol* _sym_find_enableselectall
    cdef t_symbol* _sym_findall
    cdef t_symbol* _sym_finddoneclicked
    cdef t_symbol* _sym_findfirst
    cdef t_symbol* _sym_findmaster
    cdef t_symbol* _sym_findnextclicked
    cdef t_symbol* _sym_findprevclicked
    cdef t_symbol* _sym_findreturnkeypressed
    cdef t_symbol* _sym_findselectallclicked
    cdef t_symbol* _sym_findsize
    cdef t_symbol* _sym_findtextchanged
    cdef t_symbol* _sym_first
    cdef t_symbol* _sym_firstline
    cdef t_symbol* _sym_firstobject
    cdef t_symbol* _sym_firstview
    cdef t_symbol* _sym_five
    cdef t_symbol* _sym_fixed
    cdef t_symbol* _sym_fixwidth
    cdef t_symbol* _sym_flags
    cdef t_symbol* _sym_flat
    cdef t_symbol* _sym_float
    cdef t_symbol* _sym_float32
    cdef t_symbol* _sym_float64
    cdef t_symbol* _sym_floating
    cdef t_symbol* _sym_flonum
    cdef t_symbol* _sym_flush
    cdef t_symbol* _sym_focusgained
    cdef t_symbol* _sym_focuslost
    cdef t_symbol* _sym_focusonvis
    cdef t_symbol* _sym_fold
    cdef t_symbol* _sym_folder
    cdef t_symbol* _sym_folderdropped
    cdef t_symbol* _sym_folderpath
    cdef t_symbol* _sym_font
    cdef t_symbol* _sym_fontchanged
    cdef t_symbol* _sym_fontface
    cdef t_symbol* _sym_fontfixwidth
    cdef t_symbol* _sym_fontinfochanged
    cdef t_symbol* _sym_fontname
    cdef t_symbol* _sym_fontnamechanged
    cdef t_symbol* _sym_fontpanel_isclientwindow
    cdef t_symbol* _sym_fontpanelfontcolor
    cdef t_symbol* _sym_fontpanelfontface
    cdef t_symbol* _sym_fontpanelfontname
    cdef t_symbol* _sym_fontpanelfontsize
    cdef t_symbol* _sym_fontsize
    cdef t_symbol* _sym_fontsizechanged
    cdef t_symbol* _sym_fonttarget
    cdef t_symbol* _sym_forbidclose
    cdef t_symbol* _sym_forward
    cdef t_symbol* _sym_four
    cdef t_symbol* _sym_fpic
    cdef t_symbol* _sym_free
    cdef t_symbol* _sym_freebang
    cdef t_symbol* _sym_freekeys
    cdef t_symbol* _sym_freepatcherview
    cdef t_symbol* _sym_frgb
    cdef t_symbol* _sym_frgba
    cdef t_symbol* _sym_fromdictionary
    cdef t_symbol* _sym_fromgworld
    cdef t_symbol* _sym_frommatrix
    cdef t_symbol* _sym_frommatrix_trunc
    cdef t_symbol* _sym_front
    cdef t_symbol* _sym_frozen
    cdef t_symbol* _sym_frozen_box_attributes
    cdef t_symbol* _sym_frozen_object_attributes
    cdef t_symbol* _sym_frozen_pendingattrs
    cdef t_symbol* _sym_frozen_text
    cdef t_symbol* _sym_fsaa
    cdef t_symbol* _sym_fullname
    cdef t_symbol* _sym_fullscreen
    cdef t_symbol* _sym_funall
    cdef t_symbol* _sym_function
    cdef t_symbol* _sym_g_inout_binlet
    cdef t_symbol* _sym_g_max_newest
    cdef t_symbol* _sym_g_max_newest_box
    cdef t_symbol* _sym_gb
    cdef t_symbol* _sym_genframe
    cdef t_symbol* _sym_get
    cdef t_symbol* _sym_get_jit_ob
    cdef t_symbol* _sym_getargumentlabel
    cdef t_symbol* _sym_getassoc
    cdef t_symbol* _sym_getatoms
    cdef t_symbol* _sym_getattrtext
    cdef t_symbol* _sym_getbounds
    cdef t_symbol* _sym_getboxlayer
    cdef t_symbol* _sym_getcaptioninfo
    cdef t_symbol* _sym_getcellcolor
    cdef t_symbol* _sym_getcelldescription
    cdef t_symbol* _sym_getcelleditable
    cdef t_symbol* _sym_getcellfiletypes
    cdef t_symbol* _sym_getcellicon
    cdef t_symbol* _sym_getcellindentlevel
    cdef t_symbol* _sym_getcellmenu
    cdef t_symbol* _sym_getcelltext
    cdef t_symbol* _sym_getcelltextlength
    cdef t_symbol* _sym_getcellunits
    cdef t_symbol* _sym_getcellunitsyms
    cdef t_symbol* _sym_getcellvalue
    cdef t_symbol* _sym_getcolumnnames
    cdef t_symbol* _sym_getcomparisonlabel
    cdef t_symbol* _sym_getcomponent
    cdef t_symbol* _sym_getcontainedcomponent
    cdef t_symbol* _sym_getdata
    cdef t_symbol* _sym_getdefaultfocuscomponent
    cdef t_symbol* _sym_getdefext
    cdef t_symbol* _sym_getdeftype
    cdef t_symbol* _sym_getdrawparams
    cdef t_symbol* _sym_getdst
    cdef t_symbol* _sym_getfieldlabel
    cdef t_symbol* _sym_getfindtext
    cdef t_symbol* _sym_getflags
    cdef t_symbol* _sym_getfolderpath
    cdef t_symbol* _sym_getfonttarget
    cdef t_symbol* _sym_getfontview
    cdef t_symbol* _sym_getformat
    cdef t_symbol* _sym_gethintdelay
    cdef t_symbol* _sym_getholder
    cdef t_symbol* _sym_getimage
    cdef t_symbol* _sym_getindex
    cdef t_symbol* _sym_getinfo
    cdef t_symbol* _sym_getinports
    cdef t_symbol* _sym_getinput
    cdef t_symbol* _sym_getinputlist
    cdef t_symbol* _sym_getioproc
    cdef t_symbol* _sym_getkeys
    cdef t_symbol* _sym_getlastinsertid
    cdef t_symbol* _sym_getlayoutinfo
    cdef t_symbol* _sym_getlogical
    cdef t_symbol* _sym_getmatrix
    cdef t_symbol* _sym_getmethod
    cdef t_symbol* _sym_getname
    cdef t_symbol* _sym_getnamed
    cdef t_symbol* _sym_getnamedbox
    cdef t_symbol* _sym_getnextrecord
    cdef t_symbol* _sym_getnthview
    cdef t_symbol* _sym_getobject
    cdef t_symbol* _sym_getoffset
    cdef t_symbol* _sym_getoutports
    cdef t_symbol* _sym_getoutput
    cdef t_symbol* _sym_getoutputlist
    cdef t_symbol* _sym_getprobevalue
    cdef t_symbol* _sym_getptr
    cdef t_symbol* _sym_getptr_forview
    cdef t_symbol* _sym_getquantifierlabel
    cdef t_symbol* _sym_getquery
    cdef t_symbol* _sym_getquerydict
    cdef t_symbol* _sym_getquerynames
    cdef t_symbol* _sym_getquerytype
    cdef t_symbol* _sym_getrect
    cdef t_symbol* _sym_getrowcolor
    cdef t_symbol* _sym_getrowobject
    cdef t_symbol* _sym_getselected
    cdef t_symbol* _sym_getsize
    cdef t_symbol* _sym_getsort
    cdef t_symbol* _sym_getspecial
    cdef t_symbol* _sym_getsrc
    cdef t_symbol* _sym_getstackbase
    cdef t_symbol* _sym_getsyswind
    cdef t_symbol* _sym_gettextptr
    cdef t_symbol* _sym_gettitle
    cdef t_symbol* _sym_gettype
    cdef t_symbol* _sym_gettypelist
    cdef t_symbol* _sym_getunitstext
    cdef t_symbol* _sym_getunitsyms
    cdef t_symbol* _sym_getvalueof
    cdef t_symbol* _sym_getvisiblecanvasrect
    cdef t_symbol* _sym_getwind
    cdef t_symbol* _sym_getwindowrect
    cdef t_symbol* _sym_gl_line_loop
    cdef t_symbol* _sym_gl_line_strip
    cdef t_symbol* _sym_gl_lines
    cdef t_symbol* _sym_gl_points
    cdef t_symbol* _sym_gl_polygon
    cdef t_symbol* _sym_gl_quad_grid
    cdef t_symbol* _sym_gl_quad_strip
    cdef t_symbol* _sym_gl_quads
    cdef t_symbol* _sym_gl_tri_fan
    cdef t_symbol* _sym_gl_tri_grid
    cdef t_symbol* _sym_gl_tri_strip
    cdef t_symbol* _sym_gl_triangles
    cdef t_symbol* _sym_global
    cdef t_symbol* _sym_globalpatchername
    cdef t_symbol* _sym_go
    cdef t_symbol* _sym_grabfocus
    cdef t_symbol* _sym_greaterthan
    cdef t_symbol* _sym_green
    cdef t_symbol* _sym_grid
    cdef t_symbol* _sym_gridonopen
    cdef t_symbol* _sym_gridsize
    cdef t_symbol* _sym_gridsnap
    cdef t_symbol* _sym_gridsnaponopen
    cdef t_symbol* _sym_group
    cdef t_symbol* _sym_grow
    cdef t_symbol* _sym_growboth
    cdef t_symbol* _sym_growy
    cdef t_symbol* _sym_hasclose
    cdef t_symbol* _sym_hasdatatype
    cdef t_symbol* _sym_hasgrow
    cdef t_symbol* _sym_hashorizscroll
    cdef t_symbol* _sym_hashtab_entry_free
    cdef t_symbol* _sym_hashtab_entry_new
    cdef t_symbol* _sym_hashtab_free
    cdef t_symbol* _sym_hasmenu
    cdef t_symbol* _sym_hasminimize
    cdef t_symbol* _sym_hastitlebar
    cdef t_symbol* _sym_hasvertscroll
    cdef t_symbol* _sym_haszoom
    cdef t_symbol* _sym_head
    cdef t_symbol* _sym_height
    cdef t_symbol* _sym_help
    cdef t_symbol* _sym_helpfile
    cdef t_symbol* _sym_helpmenu
    cdef t_symbol* _sym_helpname
    cdef t_symbol* _sym_hidden
    cdef t_symbol* _sym_hiddenconnect
    cdef t_symbol* _sym_hide
    cdef t_symbol* _sym_hideablechanged
    cdef t_symbol* _sym_hidecaption
    cdef t_symbol* _sym_hidewindow
    cdef t_symbol* _sym_hint
    cdef t_symbol* _sym_hint_disabled
    cdef t_symbol* _sym_hinttrack
    cdef t_symbol* _sym_history
    cdef t_symbol* _sym_history_entry
    cdef t_symbol* _sym_hittest
    cdef t_symbol* _sym_holderoptions
    cdef t_symbol* _sym_hz
    cdef t_symbol* _sym_icon
    cdef t_symbol* _sym_id
    cdef t_symbol* _sym_identifier
    cdef t_symbol* _sym_ignoreclick
    cdef t_symbol* _sym_image
    cdef t_symbol* _sym_imagefile
    cdef t_symbol* _sym_imbed
    cdef t_symbol* _sym_imprint
    cdef t_symbol* _sym_includebg
    cdef t_symbol* _sym_index
    cdef t_symbol* _sym_info
    cdef t_symbol* _sym_init
    cdef t_symbol* _sym_inlet
    cdef t_symbol* _sym_inletinfo
    cdef t_symbol* _sym_inletnum
    cdef t_symbol* _sym_inletoutlet
    cdef t_symbol* _sym_inletscaleratio
    cdef t_symbol* _sym_inputcount
    cdef t_symbol* _sym_insert
    cdef t_symbol* _sym_insertboxtext
    cdef t_symbol* _sym_insertindex
    cdef t_symbol* _sym_insertpatcher
    cdef t_symbol* _sym_insertsegment
    cdef t_symbol* _sym_insetchanged
    cdef t_symbol* _sym_insp
    cdef t_symbol* _sym_inspectee
    cdef t_symbol* _sym_inspectees
    cdef t_symbol* _sym_inspector
    cdef t_symbol* _sym_inspector_clueclass
    cdef t_symbol* _sym_inspector_color
    cdef t_symbol* _sym_inspector_customize
    cdef t_symbol* _sym_inspector_editor
    cdef t_symbol* _sym_inspector_fontattr
    cdef t_symbol* _sym_inspector_rect
    cdef t_symbol* _sym_inspector_tab
    cdef t_symbol* _sym_inspector_tabheight
    cdef t_symbol* _sym_inspector_title
    cdef t_symbol* _sym_inspector_toolbarid
    cdef t_symbol* _sym_inspectorchange
    cdef t_symbol* _sym_instance_attributes
    cdef t_symbol* _sym_instanceattr
    cdef t_symbol* _sym_int
    cdef t_symbol* _sym_int16
    cdef t_symbol* _sym_int24
    cdef t_symbol* _sym_int32
    cdef t_symbol* _sym_int8
    cdef t_symbol* _sym_interface
    cdef t_symbol* _sym_interp
    cdef t_symbol* _sym_interp_arg
    cdef t_symbol* _sym_interval
    cdef t_symbol* _sym_invalidate
    cdef t_symbol* _sym_invalidateallboxlayers
    cdef t_symbol* _sym_invalidateboxlayer
    cdef t_symbol* _sym_invalidatetoolbar
    cdef t_symbol* _sym_invis
    cdef t_symbol* _sym_invisaction
    cdef t_symbol* _sym_invisible
    cdef t_symbol* _sym_invlabel
    cdef t_symbol* _sym_invmsg
    cdef t_symbol* _sym_ioname
    cdef t_symbol* _sym_ioproc
    cdef t_symbol* _sym_is
    cdef t_symbol* _sym_iscolumnvisible
    cdef t_symbol* _sym_isfile
    cdef t_symbol* _sym_isfirstin
    cdef t_symbol* _sym_isfolder
    cdef t_symbol* _sym_iso_8859_1
    cdef t_symbol* _sym_isproto
    cdef t_symbol* _sym_isselectedqueryremovable
    cdef t_symbol* _sym_isselectionvalid
    cdef t_symbol* _sym_issystemquery
    cdef t_symbol* _sym_italic
    cdef t_symbol* _sym_items
    cdef t_symbol* _sym_iterate
    cdef t_symbol* _sym_javascript
    cdef t_symbol* _sym_jbogus
    cdef t_symbol* _sym_jbox
    cdef t_symbol* _sym_jbox_bytecount
    cdef t_symbol* _sym_jbox_instances
    cdef t_symbol* _sym_jboxattr
    cdef t_symbol* _sym_jdrag
    cdef t_symbol* _sym_jed
    cdef t_symbol* _sym_jgraphics
    cdef t_symbol* _sym_jit_attr_offset
    cdef t_symbol* _sym_jit_attr_offset_array
    cdef t_symbol* _sym_jit_attribute
    cdef t_symbol* _sym_jit_gl_texture
    cdef t_symbol* _sym_jit_linklist
    cdef t_symbol* _sym_jit_matrix
    cdef t_symbol* _sym_jit_mop
    cdef t_symbol* _sym_jit_namespace
    cdef t_symbol* _sym_jpatcher
    cdef t_symbol* _sym_jpatcher_bytecount
    cdef t_symbol* _sym_jpatcher_instances
    cdef t_symbol* _sym_jpatchercontroller
    cdef t_symbol* _sym_jpatcherholder
    cdef t_symbol* _sym_jpatchline
    cdef t_symbol* _sym_jpg
    cdef t_symbol* _sym_jsave
    cdef t_symbol* _sym_JSON
    cdef t_symbol* _sym_jsonreader
    cdef t_symbol* _sym_jsonwriter
    cdef t_symbol* _sym_juibogus
    cdef t_symbol* _sym_juiobject
    cdef t_symbol* _sym_jundo_command
    cdef t_symbol* _sym_jundo_commandgroup
    cdef t_symbol* _sym_jweb_history
    cdef t_symbol* _sym_jwind
    cdef t_symbol* _sym_kb
    cdef t_symbol* _sym_key
    cdef t_symbol* _sym_key_backslash
    cdef t_symbol* _sym_key_backspace
    cdef t_symbol* _sym_key_clear
    cdef t_symbol* _sym_key_delete
    cdef t_symbol* _sym_key_downarrow
    cdef t_symbol* _sym_key_end
    cdef t_symbol* _sym_key_enter
    cdef t_symbol* _sym_key_esc
    cdef t_symbol* _sym_key_grave
    cdef t_symbol* _sym_key_helpkey
    cdef t_symbol* _sym_key_home
    cdef t_symbol* _sym_key_insert
    cdef t_symbol* _sym_key_leftarrow
    cdef t_symbol* _sym_key_optionspace
    cdef t_symbol* _sym_key_pagedown
    cdef t_symbol* _sym_key_pageup
    cdef t_symbol* _sym_key_return
    cdef t_symbol* _sym_key_rightarrow
    cdef t_symbol* _sym_key_spacebar
    cdef t_symbol* _sym_key_tab
    cdef t_symbol* _sym_key_tilde
    cdef t_symbol* _sym_key_uparrow
    cdef t_symbol* _sym_key_vertbar
    cdef t_symbol* _sym_keyaction
    cdef t_symbol* _sym_keyfilter
    cdef t_symbol* _sym_keyfocuschanged
    cdef t_symbol* _sym_keymessage
    cdef t_symbol* _sym_kind
    cdef t_symbol* _sym_kindenum
    cdef t_symbol* _sym_kindis
    cdef t_symbol* _sym_kindmenu
    cdef t_symbol* _sym_label
    cdef t_symbol* _sym_labels
    cdef t_symbol* _sym_last
    cdef t_symbol* _sym_last_access
    cdef t_symbol* _sym_lastmessage
    cdef t_symbol* _sym_lastmodified
    cdef t_symbol* _sym_lastobject
    cdef t_symbol* _sym_latency
    cdef t_symbol* _sym_lessthan
    cdef t_symbol* _sym_linear
    cdef t_symbol* _sym_linechanged
    cdef t_symbol* _sym_linecontextmenu
    cdef t_symbol* _sym_linecount
    cdef t_symbol* _sym_linenotify
    cdef t_symbol* _sym_lines
    cdef t_symbol* _sym_lineup
    cdef t_symbol* _sym_list
    cdef t_symbol* _sym_list_rowcomponent
    cdef t_symbol* _sym_list_setvalue
    cdef t_symbol* _sym_listboxprototype
    cdef t_symbol* _sym_listfiles
    cdef t_symbol* _sym_listwindow
    cdef t_symbol* _sym_loadbang
    cdef t_symbol* _sym_local
    cdef t_symbol* _sym_lock
    cdef t_symbol* _sym_locked
    cdef t_symbol* _sym_locked_iocolor
    cdef t_symbol* _sym_lockeddragscroll
    cdef t_symbol* _sym_lockedpatchercontextmenu
    cdef t_symbol* _sym_log
    cdef t_symbol* _sym_long
    cdef t_symbol* _sym_lookup
    cdef t_symbol* _sym_lookupcommand
    cdef t_symbol* _sym_loop
    cdef t_symbol* _sym_macroman
    cdef t_symbol* _sym_mainsearchentry
    cdef t_symbol* _sym_makearray
    cdef t_symbol* _sym_makeview
    cdef t_symbol* _sym_margin
    cdef t_symbol* _sym_matchdragrole
    cdef t_symbol* _sym_matchinspectees
    cdef t_symbol* _sym_matrix
    cdef t_symbol* _sym_matrix_calc
    cdef t_symbol* _sym_matrixctrl
    cdef t_symbol* _sym_matrixname
    cdef t_symbol* _sym_max
    cdef t_symbol* _sym_max_jit_classex
    cdef t_symbol* _sym_maxapplication
    cdef t_symbol* _sym_maxclass
    cdef t_symbol* _sym_maxdebug
    cdef t_symbol* _sym_maxdim
    cdef t_symbol* _sym_maximize
    cdef t_symbol* _sym_maxmessage
    cdef t_symbol* _sym_maxplanecount
    cdef t_symbol* _sym_maxwindow
    cdef t_symbol* _sym_mb
    cdef t_symbol* _sym_measuretext
    cdef t_symbol* _sym_menu
    cdef t_symbol* _sym_menubar
    cdef t_symbol* _sym_menus
    cdef t_symbol* _sym_menus_runtime
    cdef t_symbol* _sym_message
    cdef t_symbol* _sym_messages
    cdef t_symbol* _sym_metadata
    cdef t_symbol* _sym_metadatalist
    cdef t_symbol* _sym_metadatawindow
    cdef t_symbol* _sym_methodall
    cdef t_symbol* _sym_methodindex
    cdef t_symbol* _sym_methods
    cdef t_symbol* _sym_midpoints
    cdef t_symbol* _sym_min
    cdef t_symbol* _sym_mindim
    cdef t_symbol* _sym_minimize
    cdef t_symbol* _sym_minplanecount
    cdef t_symbol* _sym_minus
    cdef t_symbol* _sym_mode
    cdef t_symbol* _sym_modified
    cdef t_symbol* _sym_monitor
    cdef t_symbol* _sym_months
    cdef t_symbol* _sym_mousedoubleclick
    cdef t_symbol* _sym_mousedown
    cdef t_symbol* _sym_mousedownonchar
    cdef t_symbol* _sym_mousedrag
    cdef t_symbol* _sym_mousedragdelta
    cdef t_symbol* _sym_mouseenter
    cdef t_symbol* _sym_mouseleave
    cdef t_symbol* _sym_mousemove
    cdef t_symbol* _sym_mousescale
    cdef t_symbol* _sym_mouseup
    cdef t_symbol* _sym_mousewheel
    cdef t_symbol* _sym_move
    cdef t_symbol* _sym_moveboxes
    cdef t_symbol* _sym_moved
    cdef t_symbol* _sym_moveifoffdisplay
    cdef t_symbol* _sym_movelines
    cdef t_symbol* _sym_movesegment
    cdef t_symbol* _sym_moviedim
    cdef t_symbol* _sym_moviefile
    cdef t_symbol* _sym_ms
    cdef t_symbol* _sym_ms_ansi
    cdef t_symbol* _sym_mulaw
    cdef t_symbol* _sym_mult
    cdef t_symbol* _sym_multiboxcontextmenu
    cdef t_symbol* _sym_multilinecontextmenu
    cdef t_symbol* _sym_mute
    cdef t_symbol* _sym_name
    cdef t_symbol* _sym_name_changed
    cdef t_symbol* _sym_name_rowcomponent
    cdef t_symbol* _sym_name_setvalue
    cdef t_symbol* _sym_name_textcolor
    cdef t_symbol* _sym_name_textstyle
    cdef t_symbol* _sym_name_width
    cdef t_symbol* _sym_nameinspector
    cdef t_symbol* _sym_nativewindow
    cdef t_symbol* _sym_navkey
    cdef t_symbol* _sym_new
    cdef t_symbol* _sym_newcopy
    cdef t_symbol* _sym_newdefault
    cdef t_symbol* _sym_newex
    cdef t_symbol* _sym_newfilebrowser
    cdef t_symbol* _sym_newfolder
    cdef t_symbol* _sym_newlines
    cdef t_symbol* _sym_newobj
    cdef t_symbol* _sym_newobject
    cdef t_symbol* _sym_newobjects
    cdef t_symbol* _sym_newpatcherview
    cdef t_symbol* _sym_newpatchline
    cdef t_symbol* _sym_newquery
    cdef t_symbol* _sym_next
    cdef t_symbol* _sym_nextline
    cdef t_symbol* _sym_nextobject
    cdef t_symbol* _sym_nextrecord
    cdef t_symbol* _sym_nextview
    cdef t_symbol* _sym_nfilters
    cdef t_symbol* _sym_No
    cdef t_symbol* _sym_noactivate
    cdef t_symbol* _sym_nobox
    cdef t_symbol* _sym_noclipse
    cdef t_symbol* _sym_noclose
    cdef t_symbol* _sym_noedit
    cdef t_symbol* _sym_noeval
    cdef t_symbol* _sym_nofloat
    cdef t_symbol* _sym_nofontpanel
    cdef t_symbol* _sym_nogrow
    cdef t_symbol* _sym_nomad
    cdef t_symbol* _sym_nomenu
    cdef t_symbol* _sym_nominimize
    cdef t_symbol* _sym_none
    cdef t_symbol* _sym_nonomad
    cdef t_symbol* _sym_normalheight
    cdef t_symbol* _sym_normalwidth
    cdef t_symbol* _sym_noscroll
    cdef t_symbol* _sym_not
    cdef t_symbol* _sym_notevalues
    cdef t_symbol* _sym_nothing
    cdef t_symbol* _sym_notify
    cdef t_symbol* _sym_notifyall
    cdef t_symbol* _sym_notifyall_delete
    cdef t_symbol* _sym_notifyall_new
    cdef t_symbol* _sym_notitle
    cdef t_symbol* _sym_nozoom
    cdef t_symbol* _sym_nth
    cdef t_symbol* _sym_number
    cdef t_symbol* _sym_number_paint
    cdef t_symbol* _sym_number_rowcomponent
    cdef t_symbol* _sym_number_setvalue
    cdef t_symbol* _sym_numfields
    cdef t_symbol* _sym_nummidpoints
    cdef t_symbol* _sym_numrecords
    cdef t_symbol* _sym_numresults
    cdef t_symbol* _sym_numrowschanged
    cdef t_symbol* _sym_numtabs
    cdef t_symbol* _sym_numviews
    cdef t_symbol* _sym_numwindowviews
    cdef t_symbol* _sym_ob_sym
    cdef t_symbol* _sym_obex_container
    cdef t_symbol* _sym_objargs
    cdef t_symbol* _sym_object
    cdef t_symbol* _sym_object_id
    cdef t_symbol* _sym_objectcount
    cdef t_symbol* _sym_objectfilename
    cdef t_symbol* _sym_objectlist
    cdef t_symbol* _sym_objectpalette
    cdef t_symbol* _sym_objectview_doubleclick
    cdef t_symbol* _sym_objectview_selected
    cdef t_symbol* _sym_objptr2index
    cdef t_symbol* _sym_objtype
    cdef t_symbol* _sym_obtrusiveerror
    cdef t_symbol* _sym_offset
    cdef t_symbol* _sym_offset_rowcomponent
    cdef t_symbol* _sym_offset_setvalue
    cdef t_symbol* _sym_offsetfrom
    cdef t_symbol* _sym_offsetmidpoints
    cdef t_symbol* _sym_okclose
    cdef t_symbol* _sym_oksize
    cdef t_symbol* _sym_one
    cdef t_symbol* _sym_onoff
    cdef t_symbol* _sym_opaque
    cdef t_symbol* _sym_open
    cdef t_symbol* _sym_openassoc
    cdef t_symbol* _sym_openfile
    cdef t_symbol* _sym_openinpresentation
    cdef t_symbol* _sym_openquery
    cdef t_symbol* _sym_openrect
    cdef t_symbol* _sym_openweb
    cdef t_symbol* _sym_optional
    cdef t_symbol* _sym_optionsdirty
    cdef t_symbol* _sym_order
    cdef t_symbol* _sym_order_by
    cdef t_symbol* _sym_orderbefore
    cdef t_symbol* _sym_orderfront
    cdef t_symbol* _sym_orientation
    cdef t_symbol* _sym_outlet
    cdef t_symbol* _sym_outletnum
    cdef t_symbol* _sym_outline
    cdef t_symbol* _sym_outmode
    cdef t_symbol* _sym_output
    cdef t_symbol* _sym_outputcount
    cdef t_symbol* _sym_outputmatrix
    cdef t_symbol* _sym_outputmode
    cdef t_symbol* _sym_overdrive
    cdef t_symbol* _sym_owner
    cdef t_symbol* _sym_ownervis
    cdef t_symbol* _sym_p
    cdef t_symbol* _sym_paint
    cdef t_symbol* _sym_paintsbackground
    cdef t_symbol* _sym_palette_action
    cdef t_symbol* _sym_palette_caption
    cdef t_symbol* _sym_palette_category
    cdef t_symbol* _sym_palette_numerical_order
    cdef t_symbol* _sym_palette_order
    cdef t_symbol* _sym_palette_protocount
    cdef t_symbol* _sym_palette_tab_action
    cdef t_symbol* _sym_parameter_enable
    cdef t_symbol* _sym_parent
    cdef t_symbol* _sym_parentclass
    cdef t_symbol* _sym_parentpatcher
    cdef t_symbol* _sym_parse
    cdef t_symbol* _sym_parsefile
    cdef t_symbol* _sym_paste
    cdef t_symbol* _sym_pastefileintoobject
    cdef t_symbol* _sym_pastefrom
    cdef t_symbol* _sym_pastereplace
    cdef t_symbol* _sym_patcher
    cdef t_symbol* _sym_patchercomponent
    cdef t_symbol* _sym_patchercontextmenu
    cdef t_symbol* _sym_patchereditor
    cdef t_symbol* _sym_patchername
    cdef t_symbol* _sym_patchernotify
    cdef t_symbol* _sym_patcherview
    cdef t_symbol* _sym_patcherview_instances
    cdef t_symbol* _sym_patcherview_invis
    cdef t_symbol* _sym_patcherview_notify_locked
    cdef t_symbol* _sym_patcherview_notify_presentation
    cdef t_symbol* _sym_patcherview_vis
    cdef t_symbol* _sym_patching_position
    cdef t_symbol* _sym_patching_rect
    cdef t_symbol* _sym_patching_size
    cdef t_symbol* _sym_patchline
    cdef t_symbol* _sym_path
    cdef t_symbol* _sym_pattrstorage
    cdef t_symbol* _sym_pclose
    cdef t_symbol* _sym_pending
    cdef t_symbol* _sym_pic
    cdef t_symbol* _sym_pictctrl
    cdef t_symbol* _sym_plane
    cdef t_symbol* _sym_planecount
    cdef t_symbol* _sym_planelink
    cdef t_symbol* _sym_plugconfig
    cdef t_symbol* _sym_plus
    cdef t_symbol* _sym_png
    cdef t_symbol* _sym_pointer
    cdef t_symbol* _sym_popupmenu
    cdef t_symbol* _sym_portenable
    cdef t_symbol* _sym_position
    cdef t_symbol* _sym_postname
    cdef t_symbol* _sym_pound_B
    cdef t_symbol* _sym_pound_D
    cdef t_symbol* _sym_pound_N
    cdef t_symbol* _sym_pound_P
    cdef t_symbol* _sym_pound_X
    cdef t_symbol* _sym_preferences
    cdef t_symbol* _sym_preload
    cdef t_symbol* _sym_presentation
    cdef t_symbol* _sym_presentation_linecount
    cdef t_symbol* _sym_presentation_position
    cdef t_symbol* _sym_presentation_rect
    cdef t_symbol* _sym_presentation_size
    cdef t_symbol* _sym_preset
    cdef t_symbol* _sym_preset_data
    cdef t_symbol* _sym_preview
    cdef t_symbol* _sym_preview_image
    cdef t_symbol* _sym_previewheight
    cdef t_symbol* _sym_previewimagedata
    cdef t_symbol* _sym_prevobject
    cdef t_symbol* _sym_print
    cdef t_symbol* _sym_priority
    cdef t_symbol* _sym_prototype
    cdef t_symbol* _sym_prototype_binbuf
    cdef t_symbol* _sym_prototypename
    cdef t_symbol* _sym_psave
    cdef t_symbol* _sym_pulsate
    cdef t_symbol* _sym_pupdate
    cdef t_symbol* _sym_quantifier
    cdef t_symbol* _sym_quantifier_exists
    cdef t_symbol* _sym_quantifiers
    cdef t_symbol* _sym_quantize
    cdef t_symbol* _sym_queries
    cdef t_symbol* _sym_query
    cdef t_symbol* _sym_query_selected
    cdef t_symbol* _sym_querycontroller
    cdef t_symbol* _sym_queryid
    cdef t_symbol* _sym_querylist
    cdef t_symbol* _sym_queryname
    cdef t_symbol* _sym_querypreview
    cdef t_symbol* _sym_querysearch
    cdef t_symbol* _sym_quickmap
    cdef t_symbol* _sym_quit
    cdef t_symbol* _sym_quitting
    cdef t_symbol* _sym_radial
    cdef t_symbol* _sym_range
    cdef t_symbol* _sym_rawfind
    cdef t_symbol* _sym_rawwindsave
    cdef t_symbol* _sym_rawwindsaveas
    cdef t_symbol* _sym_read
    cdef t_symbol* _sym_readonly
    cdef t_symbol* _sym_realclass
    cdef t_symbol* _sym_rebuilding
    cdef t_symbol* _sym_recordbyindex
    cdef t_symbol* _sym_recreate_invis
    cdef t_symbol* _sym_recreate_vis
    cdef t_symbol* _sym_rect
    cdef t_symbol* _sym_rectangle
    cdef t_symbol* _sym_rectangular
    cdef t_symbol* _sym_red
    cdef t_symbol* _sym_redostack
    cdef t_symbol* _sym_redraw
    cdef t_symbol* _sym_redrawcontents
    cdef t_symbol* _sym_reference
    cdef t_symbol* _sym_reffile
    cdef t_symbol* _sym_refinequery
    cdef t_symbol* _sym_refresh
    cdef t_symbol* _sym_register
    cdef t_symbol* _sym_remove
    cdef t_symbol* _sym_removeattr
    cdef t_symbol* _sym_removeattr_enable
    cdef t_symbol* _sym_removeboxlayer
    cdef t_symbol* _sym_removeclient
    cdef t_symbol* _sym_removefrompresentation
    cdef t_symbol* _sym_removelines
    cdef t_symbol* _sym_removeobjects
    cdef t_symbol* _sym_removesegment
    cdef t_symbol* _sym_removeslot
    cdef t_symbol* _sym_removewiretap
    cdef t_symbol* _sym_rename
    cdef t_symbol* _sym_renumberslot
    cdef t_symbol* _sym_replace
    cdef t_symbol* _sym_replaced_args
    cdef t_symbol* _sym_reschedule
    cdef t_symbol* _sym_reset
    cdef t_symbol* _sym_resize
    cdef t_symbol* _sym_resizeaction
    cdef t_symbol* _sym_resized
    cdef t_symbol* _sym_resizelimits
    cdef t_symbol* _sym_resizenotify
    cdef t_symbol* _sym_resolve_name
    cdef t_symbol* _sym_resolve_raw
    cdef t_symbol* _sym_resort
    cdef t_symbol* _sym_resource
    cdef t_symbol* _sym_respondtoclick
    cdef t_symbol* _sym_restore
    cdef t_symbol* _sym_restrict_dim
    cdef t_symbol* _sym_restrict_planecount
    cdef t_symbol* _sym_restrict_type
    cdef t_symbol* _sym_result
    cdef t_symbol* _sym_retain
    cdef t_symbol* _sym_revealinfinder
    cdef t_symbol* _sym_reverse
    cdef t_symbol* _sym_rgb
    cdef t_symbol* _sym_rgba
    cdef t_symbol* _sym_rolename
    cdef t_symbol* _sym_rotate
    cdef t_symbol* _sym_rounded
    cdef t_symbol* _sym_rowcolorchanged
    cdef t_symbol* _sym_rowcomponent
    cdef t_symbol* _sym_rowenabled
    cdef t_symbol* _sym_rowhead
    cdef t_symbol* _sym_rowheightchanged
    cdef t_symbol* _sym_safebang
    cdef t_symbol* _sym_safeclear
    cdef t_symbol* _sym_samples
    cdef t_symbol* _sym_save
    cdef t_symbol* _sym_save2
    cdef t_symbol* _sym_saveas
    cdef t_symbol* _sym_saveboxprototype
    cdef t_symbol* _sym_saved_object_attributes
    cdef t_symbol* _sym_savefilepath
    cdef t_symbol* _sym_savelasttab
    cdef t_symbol* _sym_savequery
    cdef t_symbol* _sym_saveto
    cdef t_symbol* _sym_savewindow
    cdef t_symbol* _sym_savingdefault
    cdef t_symbol* _sym_scale
    cdef t_symbol* _sym_script
    cdef t_symbol* _sym_scrollanimatetime
    cdef t_symbol* _sym_scrolloffset
    cdef t_symbol* _sym_scrollorigin
    cdef t_symbol* _sym_scrollposition
    cdef t_symbol* _sym_scrollselectedobjectsintoview
    cdef t_symbol* _sym_scrollto
    cdef t_symbol* _sym_scrollviewtoshow
    cdef t_symbol* _sym_search
    cdef t_symbol* _sym_searchterm
    cdef t_symbol* _sym_searchtext
    cdef t_symbol* _sym_select
    cdef t_symbol* _sym_selectallonedit
    cdef t_symbol* _sym_selectbox
    cdef t_symbol* _sym_selectcategory
    cdef t_symbol* _sym_selectcell
    cdef t_symbol* _sym_selectdropped
    cdef t_symbol* _sym_selected
    cdef t_symbol* _sym_selectedboxes
    cdef t_symbol* _sym_selectedlines
    cdef t_symbol* _sym_selectedrow
    cdef t_symbol* _sym_selectedrow_contextual
    cdef t_symbol* _sym_selfsave
    cdef t_symbol* _sym_selmode
    cdef t_symbol* _sym_send
    cdef t_symbol* _sym_sendbackward
    cdef t_symbol* _sym_sendbox
    cdef t_symbol* _sym_sendboxmsg
    cdef t_symbol* _sym_senderclasssym
    cdef t_symbol* _sym_sendtoback
    cdef t_symbol* _sym_session_usage_count
    cdef t_symbol* _sym_set
    cdef t_symbol* _sym_setall
    cdef t_symbol* _sym_setassoc
    cdef t_symbol* _sym_setatoms
    cdef t_symbol* _sym_setattr
    cdef t_symbol* _sym_setboxrect
    cdef t_symbol* _sym_setcellunits
    cdef t_symbol* _sym_setcellvalue
    cdef t_symbol* _sym_setcontainedcomponent
    cdef t_symbol* _sym_setdata
    cdef t_symbol* _sym_setdefaults
    cdef t_symbol* _sym_setdirty
    cdef t_symbol* _sym_seteditboxcaretposition
    cdef t_symbol* _sym_seteditboxhighlightedregion
    cdef t_symbol* _sym_seteditview
    cdef t_symbol* _sym_setfilter
    cdef t_symbol* _sym_setflags
    cdef t_symbol* _sym_setfont
    cdef t_symbol* _sym_setglobalcoords
    cdef t_symbol* _sym_setinfo
    cdef t_symbol* _sym_setinfo_ex
    cdef t_symbol* _sym_setmethod
    cdef t_symbol* _sym_setmidpoint
    cdef t_symbol* _sym_setpatcherattr
    cdef t_symbol* _sym_setptr
    cdef t_symbol* _sym_setrect
    cdef t_symbol* _sym_setspecialrow
    cdef t_symbol* _sym_settext
    cdef t_symbol* _sym_setunitsym
    cdef t_symbol* _sym_setvalue
    cdef t_symbol* _sym_setvalueof
    cdef t_symbol* _sym_setvaluetext
    cdef t_symbol* _sym_setwindowrect
    cdef t_symbol* _sym_setwindowsize
    cdef t_symbol* _sym_setzorder
    cdef t_symbol* _sym_shortcut
    cdef t_symbol* _sym_show
    cdef t_symbol* _sym_showcaption
    cdef t_symbol* _sym_showdoc
    cdef t_symbol* _sym_showfind
    cdef t_symbol* _sym_showpreview
    cdef t_symbol* _sym_showrecent
    cdef t_symbol* _sym_showrow
    cdef t_symbol* _sym_showsaveable
    cdef t_symbol* _sym_showtarget
    cdef t_symbol* _sym_shuffle
    cdef t_symbol* _sym_signal
    cdef t_symbol* _sym_sinceyesterday
    cdef t_symbol* _sym_singleinspector
    cdef t_symbol* _sym_size
    cdef t_symbol* _sym_sizeboxes
    cdef t_symbol* _sym_slot_definition
    cdef t_symbol* _sym_slot_modified
    cdef t_symbol* _sym_slots
    cdef t_symbol* _sym_smpte
    cdef t_symbol* _sym_snaptogrid
    cdef t_symbol* _sym_sort
    cdef t_symbol* _sym_sortablechanged
    cdef t_symbol* _sym_sortcolumn
    cdef t_symbol* _sym_sortdata
    cdef t_symbol* _sym_sorted
    cdef t_symbol* _sym_sorted_by_column
    cdef t_symbol* _sym_source
    cdef t_symbol* _sym_spacing
    cdef t_symbol* _sym_special
    cdef t_symbol* _sym_specialclick
    cdef t_symbol* _sym_specialcount
    cdef t_symbol* _sym_sql
    cdef t_symbol* _sym_sql2
    cdef t_symbol* _sym_sqlite
    cdef t_symbol* _sym_sqlite_result
    cdef t_symbol* _sym_sqlstring
    cdef t_symbol* _sym_sr
    cdef t_symbol* _sym_start
    cdef t_symbol* _sym_startdrag
    cdef t_symbol* _sym_startmoveboxes
    cdef t_symbol* _sym_startpoint
    cdef t_symbol* _sym_startprobe
    cdef t_symbol* _sym_starttransaction
    cdef t_symbol* _sym_startwindow
    cdef t_symbol* _sym_state
    cdef t_symbol* _sym_sticky_attr
    cdef t_symbol* _sym_sticky_method
    cdef t_symbol* _sym_stop
    cdef t_symbol* _sym_store
    cdef t_symbol* _sym_straighten
    cdef t_symbol* _sym_straightend
    cdef t_symbol* _sym_straightstart
    cdef t_symbol* _sym_straightthresh
    cdef t_symbol* _sym_string
    cdef t_symbol* _sym_style
    cdef t_symbol* _sym_sub
    cdef t_symbol* _sym_subpatcher
    cdef t_symbol* _sym_surfacebuffer
    cdef t_symbol* _sym_svg
    cdef t_symbol* _sym_swap
    cdef t_symbol* _sym_swatches
    cdef t_symbol* _sym_symbol
    cdef t_symbol* _sym_sysqelem
    cdef t_symbol* _sym_t
    cdef t_symbol* _sym_tab_bgcolor
    cdef t_symbol* _sym_table
    cdef t_symbol* _sym_tabledata
    cdef t_symbol* _sym_tag
    cdef t_symbol* _sym_tags
    cdef t_symbol* _sym_tapcontroller_show
    cdef t_symbol* _sym_tapwindow
    cdef t_symbol* _sym_target
    cdef t_symbol* _sym_template
    cdef t_symbol* _sym_tempo
    cdef t_symbol* _sym_text
    cdef t_symbol* _sym_text_commaseparated
    cdef t_symbol* _sym_text_large
    cdef t_symbol* _sym_text_onesymbol
    cdef t_symbol* _sym_textchanged
    cdef t_symbol* _sym_textcolor
    cdef t_symbol* _sym_textfield
    cdef t_symbol* _sym_textfile
    cdef t_symbol* _sym_textmargins
    cdef t_symbol* _sym_textstyle
    cdef t_symbol* _sym_thismonth
    cdef t_symbol* _sym_thisweek
    cdef t_symbol* _sym_threadpriority
    cdef t_symbol* _sym_threadsafe
    cdef t_symbol* _sym_three
    cdef t_symbol* _sym_ticks
    cdef t_symbol* _sym_time
    cdef t_symbol* _sym_timeobj
    cdef t_symbol* _sym_timesigchanged
    cdef t_symbol* _sym_timeupdate
    cdef t_symbol* _sym_tinge
    cdef t_symbol* _sym_title
    cdef t_symbol* _sym_titleassoc
    cdef t_symbol* _sym_tobinbuf
    cdef t_symbol* _sym_toc
    cdef t_symbol* _sym_today
    cdef t_symbol* _sym_todictionary
    cdef t_symbol* _sym_togworld
    cdef t_symbol* _sym_tool
    cdef t_symbol* _sym_tool_paint
    cdef t_symbol* _sym_tool_rowcomponent
    cdef t_symbol* _sym_tool_setvalue
    cdef t_symbol* _sym_tool_sort
    cdef t_symbol* _sym_toolbarheight
    cdef t_symbol* _sym_toolbarid
    cdef t_symbol* _sym_toolbars
    cdef t_symbol* _sym_toolbarvisible
    cdef t_symbol* _sym_top_inset
    cdef t_symbol* _sym_topmost
    cdef t_symbol* _sym_toppatcher
    cdef t_symbol* _sym_topredoname
    cdef t_symbol* _sym_topundoname
    cdef t_symbol* _sym_trackaction
    cdef t_symbol* _sym_transparent
    cdef t_symbol* _sym_transport
    cdef t_symbol* _sym_traverse
    cdef t_symbol* _sym_triangle
    cdef t_symbol* _sym_tutorial
    cdef t_symbol* _sym_two
    cdef t_symbol* _sym_twobytecomment
    cdef t_symbol* _sym_type
    cdef t_symbol* _sym_typed_usage_count
    cdef t_symbol* _sym_typedwrapper
    cdef t_symbol* _sym_typelink
    cdef t_symbol* _sym_types
    cdef t_symbol* _sym_underline
    cdef t_symbol* _sym_understands
    cdef t_symbol* _sym_undo
    cdef t_symbol* _sym_undostack
    cdef t_symbol* _sym_unimbed
    cdef t_symbol* _sym_uninitialized
    cdef t_symbol* _sym_unique
    cdef t_symbol* _sym_unit
    cdef t_symbol* _sym_units
    cdef t_symbol* _sym_unlocked_iocolor
    cdef t_symbol* _sym_unlockedinteraction
    cdef t_symbol* _sym_unset
    cdef t_symbol* _sym_up
    cdef t_symbol* _sym_update
    cdef t_symbol* _sym_update_metadata
    cdef t_symbol* _sym_updatecommand
    cdef t_symbol* _sym_updatecontent
    cdef t_symbol* _sym_updatefind
    cdef t_symbol* _sym_updatelookandfeel
    cdef t_symbol* _sym_updatemenu
    cdef t_symbol* _sym_updateprototypes
    cdef t_symbol* _sym_updatequery
    cdef t_symbol* _sym_updatequerydict
    cdef t_symbol* _sym_updaterect
    cdef t_symbol* _sym_url
    cdef t_symbol* _sym_us_ascii
    cdef t_symbol* _sym_usage_count
    cdef t_symbol* _sym_usedslotlist
    cdef t_symbol* _sym_useellipsis
    cdef t_symbol* _sym_useimagebuffer
    cdef t_symbol* _sym_usemax
    cdef t_symbol* _sym_usemin
    cdef t_symbol* _sym_user
    cdef t_symbol* _sym_usercanget
    cdef t_symbol* _sym_usercanset
    cdef t_symbol* _sym_utf_16
    cdef t_symbol* _sym_utf_16be
    cdef t_symbol* _sym_utf_16le
    cdef t_symbol* _sym_utf_8
    cdef t_symbol* _sym_val
    cdef t_symbol* _sym_value
    cdef t_symbol* _sym_value_endchange
    cdef t_symbol* _sym_value_rowcomponent
    cdef t_symbol* _sym_valuebyindex
    cdef t_symbol* _sym_varname
    cdef t_symbol* _sym_varname_set
    cdef t_symbol* _sym_varname_unset
    cdef t_symbol* _sym_version
    cdef t_symbol* _sym_videofile
    cdef t_symbol* _sym_views
    cdef t_symbol* _sym_viewspan
    cdef t_symbol* _sym_vignette
    cdef t_symbol* _sym_vis
    cdef t_symbol* _sym_visibilitychanged
    cdef t_symbol* _sym_visible
    cdef t_symbol* _sym_visiblecanvasrect
    cdef t_symbol* _sym_vol
    cdef t_symbol* _sym_vpicture
    cdef t_symbol* _sym_vpreset
    cdef t_symbol* _sym_wantfree
    cdef t_symbol* _sym_wantsreturn
    cdef t_symbol* _sym_wantstab
    cdef t_symbol* _sym_watch
    cdef t_symbol* _sym_watchpoint_flags
    cdef t_symbol* _sym_watchpoint_id
    cdef t_symbol* _sym_wclose
    cdef t_symbol* _sym_webpage
    cdef t_symbol* _sym_weeks
    cdef t_symbol* _sym_wind
    cdef t_symbol* _sym_window
    cdef t_symbol* _sym_windowbounds
    cdef t_symbol* _sym_windowrect
    cdef t_symbol* _sym_windsave
    cdef t_symbol* _sym_wiretap
    cdef t_symbol* _sym_withinlast
    cdef t_symbol* _sym_wobjectname
    cdef t_symbol* _sym_wordwrap
    cdef t_symbol* _sym_workspace
    cdef t_symbol* _sym_write
    cdef t_symbol* _sym_writedictionary
    cdef t_symbol* _sym_xmlfile
    cdef t_symbol* _sym_years
    cdef t_symbol* _sym_zero
    cdef t_symbol* _sym_zoom
    cdef t_symbol* _sym_zoombox
    cdef t_symbol* _sym_zoomfactor
    cdef t_symbol* _sym_zorder
    cdef t_symbol* _sym_zzz
    cdef t_symbol* _sym_Sans_Serif






cdef extern from "ext_wind.h":
    ctypedef enum e_max_wind_advise_result:
        aaYes = 1
        aaNo
        aaCancel

    cdef int ADVISE_SAVE
    cdef int ADVISE_DISCARD
    cdef int ADVISE_CANCEL
    cdef int ADVISE_FIRST
    cdef int ADVISE_SECOND

    cdef short wind_advise(t_object *w, char *s, ...)
    cdef short wind_advise_explain(t_object *w, char *note, char *explanation, char *b1, char *b2, char *b3)
    cdef void wind_setcursor(short which)




cdef extern from "jpatcher_api.h":

    cdef int JPATCHER_API_CURRENT_FILE_VERSION

    ctypedef enum t_clipboard_datatype:
        JCLIPBOARD_TYPE_TEXT = 1
        JCLIPBOARD_TYPE_BINBUF = 2
        JCLIPBOARD_TYPE_JSON = 4
        JCLIPBOARD_TYPE_IMAGE = 8
        JCLIPBOARD_TYPE_JSON_ATTRIBUTES = 16
        JCLIPBOARD_TYPE_UNKNOWN = 256

    cdef t_symbol *fontmap_getmapping(t_symbol *from_, char *mapped)
    cdef double fontinfo_getsize(short oldsize)
    cdef t_symbol *fontinfo_getname(short number)
    cdef short fontinfo_getnumber(t_symbol *s)
    ctypedef struct t_rect:
        double x
        double y
        double width
        double height
    ctypedef struct t_pt:
        double x
        double y
    ctypedef struct t_size:
        double width
        double height        
    ctypedef struct t_jrgb:
        double red
        double green
        double blue
    ctypedef struct t_jrgba:
        double red
        double green
        double blue
        double alpha
    ctypedef struct t_jboxdrawparams: pass

    cdef int JBOX_SPOOL_CONTENTS
    cdef int JBOX_SPOOL_WHOLEBOX
    #// cdef int JBOX_RELINE_DEFER
    cdef int JBOX_FLAG_READ
    cdef int JBOX_FLAG_FIRST_PAINT

    ctypedef struct t_jbox: pass
    ctypedef struct t_pvselinfo: pass

    cdef t_max_err object_attr_get_rect(t_object *o, t_symbol *name, t_rect *rect)
    cdef t_max_err object_attr_set_rect(t_object *o, t_symbol *name, t_rect *rect)
    cdef void object_attr_set_xywh(t_object *o, t_symbol *attr, double x, double y, double w, double h)
    cdef t_max_err object_attr_getpt(t_object *o, t_symbol *name, t_pt *pt)
    cdef t_max_err object_attr_setpt(t_object *o, t_symbol *name, t_pt *pt)
    cdef t_max_err object_attr_getsize(t_object *o, t_symbol *name, t_size *size)
    cdef t_max_err object_attr_setsize(t_object *o, t_symbol *name, t_size *size)
    cdef t_max_err object_attr_getcolor(t_object *b, t_symbol *attrname, t_jrgba *prgba)
    cdef t_max_err object_attr_setcolor(t_object *b, t_symbol *attrname, t_jrgba *prgba)
    cdef t_max_err jrgba_attr_get(t_jrgba *jrgba, long *argc, t_atom **argv)
    cdef t_max_err jrgba_attr_set(t_jrgba *jrgba, long argc, t_atom *argv)
    cdef void set_jrgba_from_palette_index(short index, t_jrgba *jrgba)
    cdef void set_jrgba_from_boxcolor_index(short index, t_jrgba *jrgba)
    cdef short get_boxcolor_index_from_jrgba(t_jrgba *jrgba)
    cdef void jgraphics_clip_rgba(t_jrgba *rgba)
    cdef void object_openhelp(t_object *x)
    cdef void object_openrefpage(t_object *x)
    cdef void object_openquery(t_object *x)
    cdef void classname_openhelp(char *classname)
    cdef void classname_openrefpage(char *classname)
    cdef void classname_openquery(char *classname)
    cdef t_object* patcherview_findpatcherview(int x, int y)
    cdef void patcherview_makepalette()
    cdef int jpatcher_is_patcher(t_object *p)
    cdef t_object* jpatcher_get_box(t_object *p)
    cdef long jpatcher_get_count(t_object *p)
    cdef char jpatcher_get_locked(t_object *p)
    cdef t_max_err jpatcher_set_locked(t_object *p, char c)
    cdef char jpatcher_get_presentation(t_object *p)
    cdef t_max_err jpatcher_set_presentation(t_object *p, char c)
    cdef t_object* jpatcher_get_firstobject(t_object *p)
    cdef t_object* jpatcher_get_lastobject(t_object *p)
    cdef t_object* jpatcher_get_firstline(t_object *p)
    cdef t_object* jpatcher_get_firstview(t_object *p)
    cdef t_symbol* jpatcher_get_title(t_object *p)
    cdef t_max_err jpatcher_set_title(t_object *p, t_symbol *ps)
    cdef t_symbol* jpatcher_get_name(t_object *p)
    cdef t_symbol* jpatcher_get_filepath(t_object *p)
    cdef t_symbol* jpatcher_get_filename(t_object *p)
    cdef char jpatcher_get_dirty(t_object *p)
    cdef t_max_err jpatcher_set_dirty(t_object *p, char c)
    cdef char jpatcher_get_bglocked(t_object *p)
    cdef t_max_err jpatcher_set_bglocked(t_object *p, char c)
    cdef char jpatcher_get_bghidden(t_object *p)
    cdef t_max_err jpatcher_set_bghidden(t_object *p, char c)
    cdef char jpatcher_get_fghidden(t_object *p)
    cdef t_max_err jpatcher_set_fghidden(t_object *p, char c)
    cdef t_max_err jpatcher_get_editing_bgcolor(t_object *p, t_jrgba *prgba)
    cdef t_max_err jpatcher_set_editing_bgcolor(t_object *p, t_jrgba *prgba)
    cdef t_max_err jpatcher_get_bgcolor(t_object *p, t_jrgba *prgba)
    cdef t_max_err jpatcher_get_locked_bgcolor(t_object *p, t_jrgba *prgba)
    cdef t_max_err jpatcher_set_bgcolor(t_object *p, t_jrgba *prgba)
    cdef t_max_err jpatcher_set_locked_bgcolor(t_object *p, t_jrgba *prgba)
    cdef t_max_err jpatcher_get_gridsize(t_object *p, double *gridsizeX, double *gridsizeY)
    cdef t_max_err jpatcher_set_gridsize(t_object *p, double gridsizeX, double gridsizeY)
    cdef t_object* jpatcher_get_controller(t_object *p)
    cdef void jpatcher_deleteobj(t_object *p, t_jbox *b)
    cdef t_object* jpatcher_get_parentpatcher(t_object *p)
    cdef t_object* jpatcher_get_toppatcher(t_object *p)
    cdef t_object* jpatcher_get_hubholder(t_object *p)
    cdef t_symbol* jpatcher_get_maxclass(t_object *p)
    cdef t_symbol* jpatcher_get_parentclass(t_object *p)
    cdef t_max_err jpatcher_get_rect(t_object *p, t_rect *pr)
    cdef t_max_err jpatcher_set_rect(t_object *p, t_rect *pr)
    cdef t_max_err jpatcher_get_defrect(t_object *p, t_rect *pr)
    cdef t_max_err jpatcher_set_defrect(t_object *p, t_rect *pr)
    cdef char jpatcher_get_noedit(t_object *p)
    cdef t_object *jpatcher_get_collective(t_object *p)
    cdef char jpatcher_get_cansave(t_object *p)
    cdef t_symbol *jpatcher_uniqueboxname(t_object *p, t_symbol *classname)
    cdef short jpatcher_getboxfont(t_object *p, short fnum, double *fsize, t_symbol **fontname)
    cdef t_symbol *jpatcher_get_default_fontname(t_object *p)
    cdef float jpatcher_get_default_fontsize(t_object *p)
    cdef long jpatcher_get_default_fontface(t_object *p)
    cdef t_max_err jpatcher_set_imprint(t_object *p, char c)
    cdef char jpatcher_get_imprint(t_object *p)
    cdef void jpatcher_addboxlistener(t_object *p, t_object *listener)
    cdef void jpatcher_removeboxlistener(t_object *p, t_object *listener)
    cdef long jpatcher_get_fileversion(t_object *p)
    cdef long jpatcher_get_currentfileversion()
    cdef t_max_err jbox_get_rect_for_view(t_object *box, t_object *patcherview, t_rect *rect)
    cdef t_max_err jbox_set_rect_for_view(t_object *box, t_object *patcherview, t_rect *rect)
    cdef t_max_err jbox_get_rect_for_sym(t_object *box, t_symbol *which, t_rect *pr)
    cdef t_max_err jbox_set_rect_for_sym(t_object *box, t_symbol *which, t_rect *pr)
    cdef t_max_err jbox_set_rect(t_object *box, t_rect *pr)
    cdef t_max_err jbox_get_patching_rect(t_object *box, t_rect *pr)
    cdef t_max_err jbox_set_patching_rect(t_object *box, t_rect *pr)
    cdef t_max_err jbox_get_presentation_rect(t_object *box, t_rect *pr)
    cdef t_max_err jbox_set_presentation_rect(t_object *box, t_rect *pr)
    cdef t_max_err jbox_set_position(t_object *box, t_pt *pos)
    cdef t_max_err jbox_get_patching_position(t_object *box, t_pt *pos)
    cdef t_max_err jbox_set_patching_position(t_object *box, t_pt *pos)
    cdef t_max_err jbox_get_presentation_position(t_object *box, t_pt *pos)
    cdef t_max_err jbox_set_presentation_position(t_object *box, t_pt *pos)
    cdef t_max_err jbox_set_size(t_object *box, t_size *size)
    cdef t_max_err jbox_get_patching_size(t_object *box, t_size *size)
    cdef t_max_err jbox_set_patching_size(t_object *box, t_size *size)
    cdef t_max_err jbox_get_presentation_size(t_object *box, t_size *size)
    cdef t_max_err jbox_set_presentation_size(t_object *box, t_size *size)
    cdef t_symbol* jbox_get_maxclass(t_object *b)
    cdef t_object* jbox_get_object(t_object *b)
    cdef t_object* jbox_get_patcher(t_object *b)
    cdef char jbox_get_hidden(t_object *b)
    cdef t_max_err jbox_set_hidden(t_object *b, char c)
    cdef t_symbol* jbox_get_fontname(t_object *b)
    cdef t_max_err jbox_set_fontname(t_object *b, t_symbol *ps)
    cdef double jbox_get_fontsize(t_object *b)
    cdef t_max_err jbox_set_fontsize(t_object *b, double d)
    cdef t_max_err jbox_get_color(t_object *b, t_jrgba *prgba)
    cdef t_max_err jbox_set_color(t_object *b, t_jrgba *prgba)
    cdef t_symbol *jbox_get_hint(t_object *b)
    cdef t_max_err jbox_set_hint(t_object *b, t_symbol *s)
    cdef char *jbox_get_hintstring(t_object *bb)
    cdef void jbox_set_hintstring(t_object *bb, char *s)
    cdef char jbox_get_hinttrack(t_object *b)
    cdef t_max_err jbox_set_hinttrack(t_object *b, char h)
    cdef char *jbox_get_annotation(t_object *bb)
    cdef void jbox_set_annotation(t_object *bb, char *s)
    cdef t_object* jbox_get_nextobject(t_object *b)
    cdef t_object* jbox_get_prevobject(t_object *b)
    cdef t_symbol* jbox_get_varname(t_object *b)
    cdef t_max_err jbox_set_varname(t_object *b, t_symbol *ps)
    cdef t_symbol* jbox_get_id(t_object *b)
    cdef char jbox_get_canhilite(t_object *b)
    cdef char jbox_get_background(t_object *b)
    cdef t_max_err jbox_set_background(t_object *b, char c)
    cdef char jbox_get_ignoreclick(t_object *b)
    cdef t_max_err jbox_set_ignoreclick(t_object *b, char c)
    cdef char jbox_get_drawfirstin(t_object *b)
    cdef char jbox_get_outline(t_object *b)
    cdef t_max_err jbox_set_outline(t_object *b, char c)
    cdef char jbox_get_growy(t_object *b)
    cdef char jbox_get_growboth(t_object *b)
    cdef char jbox_get_nogrow(t_object *b)
    cdef char jbox_get_drawinlast(t_object *b)
    cdef char jbox_get_mousedragdelta(t_object *b)
    cdef t_max_err jbox_set_mousedragdelta(t_object *b, char c)
    cdef t_object* jbox_get_textfield(t_object *b)
    cdef long jbox_get_understanding(t_object *b, t_symbol *msg)
    cdef char jbox_get_presentation(t_object *b)
    cdef t_max_err jbox_set_presentation(t_object *b, char c)
    cdef t_object *jbox_get_autocompletion(t_object *b)
    cdef t_symbol *jbox_get_prototypename(t_object *b)
    cdef void jbox_set_prototypename(t_object *b, t_symbol *name)
    cdef t_atom_long jclipboard_datatypes()
    cdef void jbox_validaterects(t_jbox *b)

    ctypedef enum t_patchline_updatetype:
        JPATCHLINE_DISCONNECT = 0
        JPATCHLINE_CONNECT = 1
        JPATCHLINE_ORDER = 2


    cdef t_max_err jpatchline_get_startpoint(t_object *l, double *x, double *y)
    cdef t_max_err jpatchline_get_endpoint(t_object *l, double *x, double *y)
    cdef long jpatchline_get_nummidpoints(t_object *l)
    cdef char jpatchline_get_pending(t_object *l)
    cdef t_object* jpatchline_get_box1(t_object *l)
    cdef long jpatchline_get_outletnum(t_object *l)
    cdef t_object* jpatchline_get_box2(t_object *l)
    cdef long jpatchline_get_inletnum(t_object *l)
    cdef double jpatchline_get_straightthresh(t_object *l)
    cdef t_max_err jpatchline_set_straightthresh(t_object *l, double d)
    cdef char jpatchline_get_straightstart(t_object *l)
    cdef char jpatchline_get_straightend(t_object *l)
    cdef t_max_err jpatchline_set_straightstart(t_object *l, char c)
    cdef t_max_err jpatchline_set_straightend(t_object *l, char c)
    cdef t_object* jpatchline_get_nextline(t_object *b)
    cdef char jpatchline_get_hidden(t_object *l)
    cdef t_max_err jpatchline_set_hidden(t_object *l, char c)
    cdef t_max_err jpatchline_get_color(t_object *l, t_jrgba *prgba)
    cdef t_max_err jpatchline_set_color(t_object *l, t_jrgba *prgba)
    cdef t_object *jpatchline_get_wiretap(t_object *l)
    cdef long wiretap_get_id(t_object *w)
    cdef long wiretap_get_flags(t_object *w)
    cdef void wiretap_set_flags(t_object *w, long n)
    cdef char patcherview_get_visible(t_object *pv)
    cdef t_max_err patcherview_set_visible(t_object *pv, char c)
    cdef t_max_err patcherview_get_rect(t_object *pv, t_rect *pr)
    cdef t_max_err patcherview_set_rect(t_object *pv, t_rect *pr)
    cdef void patcherview_canvas_to_screen(t_object *pv, double cx, double cy, long *sx, long *sy)
    cdef void patcherview_screen_to_canvas(t_object *pv, long sx, long sy, double *cx, double *cy)
    cdef char patcherview_get_locked(t_object *p)
    cdef t_max_err patcherview_set_locked(t_object *p, char c)
    cdef char patcherview_get_presentation(t_object *pv)
    cdef t_max_err patcherview_set_presentation(t_object *p, char c)
    cdef double patcherview_get_zoomfactor(t_object *pv)
    cdef t_max_err patcherview_set_zoomfactor(t_object *pv, double d)
    cdef t_object* patcherview_get_nextview(t_object *pv)
    cdef t_object* patcherview_get_jgraphics(t_object *pv)
    cdef t_max_err patcherview_set_jgraphics(t_object *pv, t_object *po)
    cdef t_object* patcherview_get_patcher(t_object *pv)
    cdef t_object* patcherview_get_topview(t_object *pv)
    cdef t_object* textfield_get_owner(t_object *tf)
    cdef t_max_err textfield_get_textcolor(t_object *tf, t_jrgba *prgba)
    cdef t_max_err textfield_set_textcolor(t_object *tf, t_jrgba *prgba)
    cdef t_max_err textfield_get_bgcolor(t_object *tf, t_jrgba *prgba)
    cdef t_max_err textfield_set_bgcolor(t_object *tf, t_jrgba *prgba)
    cdef t_max_err textfield_get_textmargins(t_object *tf, double *pleft, double *ptop, double *pright, double *pbottom)
    cdef t_max_err textfield_set_textmargins(t_object *tf, double left, double top, double right, double bottom)
    cdef char textfield_get_editonclick(t_object *tf)
    cdef t_max_err textfield_set_editonclick(t_object *tf, char c)
    cdef char textfield_get_selectallonedit(t_object *tf)
    cdef t_max_err textfield_set_selectallonedit(t_object *tf, char c)
    cdef char textfield_get_noactivate(t_object *tf)
    cdef t_max_err textfield_set_noactivate(t_object *tf, char c)
    cdef char textfield_get_readonly(t_object *tf)
    cdef t_max_err textfield_set_readonly(t_object *tf, char c)
    cdef char textfield_get_wordwrap(t_object *tf)
    cdef t_max_err textfield_set_wordwrap(t_object *tf, char c)
    cdef char textfield_get_useellipsis(t_object *tf)
    cdef t_max_err textfield_set_useellipsis(t_object *tf, char c)
    cdef char textfield_get_autoscroll(t_object *tf)
    cdef t_max_err textfield_set_autoscroll(t_object *tf, char c)
    cdef char textfield_get_wantsreturn(t_object *tf)
    cdef t_max_err textfield_set_wantsreturn(t_object *tf, char c)
    cdef char textfield_get_wantstab(t_object *tf)
    cdef t_max_err textfield_set_wantstab(t_object *tf, char c)
    cdef char textfield_get_underline(t_object *tf)
    cdef t_max_err textfield_set_underline(t_object *tf, char c)
    cdef char textfield_get_justification(t_object *tf)
    cdef t_max_err textfield_set_justification(t_object *tf, char c)
    cdef char textfield_get_autofixwidth(t_object *tf)
    cdef t_max_err textfield_set_autofixwidth(t_object *tf, char c)
    cdef t_max_err textfield_set_emptytext(t_object *tf, t_symbol *txt)
    cdef t_symbol *textfield_get_emptytext(t_object *tf)


    cdef int TEXTFIELD_DEF_LEFTMARGIN
    cdef int TEXTFIELD_DEF_TOPMARGIN
    cdef int TEXTFIELD_DEF_RIGHTMARGIN
    cdef int TEXTFIELD_DEF_BOTTOMMARGIN
    cdef int JBOX_DRAWFIRSTIN
    cdef int JBOX_NODRAWBOX
    cdef int JBOX_DRAWINLAST
    cdef int JBOX_TRANSPARENT
    cdef int JBOX_NOGROW
    cdef int JBOX_GROWY
    cdef int JBOX_GROWBOTH
    cdef int JBOX_IGNORELOCKCLICK
    cdef int JBOX_HILITE
    cdef int JBOX_BACKGROUND
    cdef int JBOX_NOFLOATINSPECTOR
    cdef int JBOX_TEXTFIELD
    cdef int JBOX_FIXWIDTH
    cdef int JBOX_FONTATTR
    cdef int JBOX_TEXTJUSTIFICATIONATTR
    cdef int JBOX_BINBUF
    cdef int JBOX_MOUSEDRAGDELTA
    cdef int JBOX_COLOR
    cdef int JBOX_DRAWIOLOCKED
    cdef int JBOX_DRAWBACKGROUND
    cdef int JBOX_NOINSPECTFIRSTIN

    cdef int JBOX_FOCUS
    cdef int JBOX_BOXVIEW
    cdef int JBOX_LEGACYCOLOR
    cdef int JBOX_COPYLEGACYDEFAULT
    cdef int JBOX_NOLEGACYDEFAULT

    ctypedef enum:
        JBOX_FONTFACE_REGULAR = 0
        JBOX_FONTFACE_BOLD = 1
        JBOX_FONTFACE_ITALIC = 2
        JBOX_FONTFACE_BOLDITALIC = 3

    ctypedef enum HitTestResult:
        HitNothing = 0
        HitBox = 1
        HitInlet = 2
        HitOutlet = 3
        HitGrowBox = 4
        HitLine = 5
        HitLineLocked = 6

    ctypedef enum DecoratorPaintFlags:
        BoxSelected = 1 << 0
        DrawFirstIn = 1 << 1
        NoGrow = 1 << 2
        Outline = 1 << 3
        Locked = 1 << 4
        InletHighlighted = 1 << 5
        OutletHighlighted = 1 << 6

    cdef void jbox_initclass(t_class *c, long flags)
    cdef t_max_err jbox_new(t_jbox *b, long flags, long argc, t_atom *argv)
    cdef void jbox_free(t_jbox *b)
    cdef void jbox_ready(t_jbox *b)
    cdef void jbox_redraw(t_jbox *b)
    cdef void jbox_redrawcontents(t_jbox *b)
    cdef void *jbox_getoutlet(t_jbox *x, long index)
    cdef void *jbox_getinlet(t_jbox *x, long index)
    cdef void jbox_updatetextfield(t_jbox *b)
    cdef int LEGACYDEFAULTS_FLAGS_FORCE

    cdef void jbox_processlegacydefaults(t_jbox *b, t_dictionary *d, long flags)
    cdef t_max_err jbox_notify(t_jbox *b, t_symbol *s, t_symbol *msg, void *sender, void *data)
    cdef t_max_err jbox_set_to_defaultsize(t_jbox *b, t_symbol *s, short argc, t_atom *argv)
    cdef void jbox_grabfocus(t_jbox *b)
    cdef void jbox_redrawpeers(t_jbox *b)
    cdef long jbox_getinletindex(t_jbox *b, void *inlet)
    cdef long jbox_getoutletindex(t_jbox *b, void *outlet)
    cdef void jbox_show_caption(t_jbox *b)
    cdef void jbox_hide_caption(t_jbox *b)

    cdef int DICT_JRGBA

    cdef t_max_err dictionary_appendjrgba(t_dictionary *d, t_symbol *key, t_jrgba *jc)
    cdef t_max_err dictionary_getdefjrgba(t_dictionary *d, t_symbol *key, t_jrgba *jc, t_jrgba *def_)
    cdef t_max_err dictionary_gettrect(t_dictionary *d, t_symbol *key, t_rect *rect)
    cdef t_max_err dictionary_appendtrect(t_dictionary *d, t_symbol *key, t_rect *rect)
    cdef t_max_err dictionary_gettpt(t_dictionary *d, t_symbol *key, t_pt *pt)
    cdef t_max_err dictionary_appendtpt(t_dictionary *d, t_symbol *key, t_pt *pt)
    cdef void atomstojrgba(long argc, t_atom *argv, t_jrgba *dest)
    cdef void jrgbatoatoms(t_jrgba *src, t_atom *argv)
    cdef t_max_err dictionary_read(char *filename, short path, t_dictionary **d)
    cdef t_max_err dictionary_write(t_dictionary *d, char *filename, short path)
    cdef t_max_err dictionary_read_yaml(const char *filename, const short path, t_dictionary **d)
    cdef t_max_err dictionary_write_yaml(const t_dictionary *d, const char *filename, const short path)
    #define newobject_fromdictionary_delete(p,d) newobject_fromdictionary(p,d), freeobject((t_object *)d)

    ctypedef enum t_modifiers:
        eCommandKey = 1
        eShiftKey = 2
        eControlKey = 4
        eAltKey = 8
        eLeftButton = 16
        eRightButton = 32
        eMiddleButton = 64
        ePopupMenu = 128
        eCapsLock = 256
        eAutoRepeat = 512

    cdef t_modifiers jkeyboard_getcurrentmodifiers()
    cdef t_modifiers jkeyboard_getcurrentmodifiers_realtime()

    ctypedef enum t_keycode:


        JKEY_NONE               = -1
        JKEY_SPACEBAR           = -2
        JKEY_ESC                = -3
        JKEY_RETURN             = -4
        JKEY_ENTER              = -4
        JKEY_TAB                = -5
        JKEY_DELETE             = -6
        JKEY_BACKSPACE          = -7
        JKEY_INSERT             = -8
        JKEY_UPARROW            = -9
        JKEY_DOWNARROW          = -10
        JKEY_LEFTARROW          = -11
        JKEY_RIGHTARROW         = -12
        JKEY_PAGEUP             = -13
        JKEY_PAGEDOWN           = -14
        JKEY_HOME               = -15
        JKEY_END                = -16
        JKEY_F1                 = -17
        JKEY_F2                 = -18
        JKEY_F3                 = -19
        JKEY_F4                 = -20
        JKEY_F5                 = -21
        JKEY_F6                 = -22
        JKEY_F7                 = -23
        JKEY_F8                 = -24
        JKEY_F9                 = -25
        JKEY_F10                = -26
        JKEY_F11                = -27
        JKEY_F12                = -28
        JKEY_F13                = -29
        JKEY_F14                = -30
        JKEY_F15                = -31
        JKEY_F16                = -32
        JKEY_NUMPAD0            = -33
        JKEY_NUMPAD1            = -34
        JKEY_NUMPAD2            = -35
        JKEY_NUMPAD3            = -36
        JKEY_NUMPAD4            = -37
        JKEY_NUMPAD5            = -38
        JKEY_NUMPAD6            = -39
        JKEY_NUMPAD7            = -40
        JKEY_NUMPAD8            = -41
        JKEY_NUMPAD9            = -42
        JKEY_NUMPADADD          = -43
        JKEY_NUMPADSUBTRACT     = -44
        JKEY_NUMPADMULTIPLY     = -45
        JKEY_NUMPADDIVIDE       = -46
        JKEY_NUMPADSEPARATOR    = -47
        JKEY_NUMPADDECIMALPOINT = -48
        JKEY_NUMPADEQUALS       = -49
        JKEY_NUMPADDELETE       = -50
        JKEY_PLAYPAUSE          = -51
        JKEY_STOP               = -52
        JKEY_NEXTTRACK          = -53
        JKEY_PREVTRACK          = -54
        JKEY_HELP               = -55

    cdef void jmouse_getposition_global(int *x, int *y)
    cdef void jmouse_setposition_global(int x, int y)
    cdef void jmouse_setposition_view(t_object *patcherview, double cx, double cy)
    cdef void jmouse_setposition_box(t_object *patcherview, t_object *box, double bx, double by)
    cdef void *jmouse_getobject()

    ctypedef enum t_jmouse_cursortype:
        JMOUSE_CURSOR_NONE
        JMOUSE_CURSOR_ARROW
        JMOUSE_CURSOR_WAIT
        JMOUSE_CURSOR_IBEAM
        JMOUSE_CURSOR_CROSSHAIR
        JMOUSE_CURSOR_COPYING
        JMOUSE_CURSOR_POINTINGHAND
        JMOUSE_CURSOR_DRAGGINGHAND
        JMOUSE_CURSOR_RESIZE_LEFTRIGHT
        JMOUSE_CURSOR_RESIZE_UPDOWN
        JMOUSE_CURSOR_RESIZE_FOURWAY
        JMOUSE_CURSOR_RESIZE_TOPEDGE
        JMOUSE_CURSOR_RESIZE_BOTTOMEDGE
        JMOUSE_CURSOR_RESIZE_LEFTEDGE
        JMOUSE_CURSOR_RESIZE_RIGHTEDGE
        JMOUSE_CURSOR_RESIZE_TOPLEFTCORNER
        JMOUSE_CURSOR_RESIZE_TOPRIGHTCORNER
        JMOUSE_CURSOR_RESIZE_BOTTOMLEFTCORNER
        JMOUSE_CURSOR_RESIZE_BOTTOMRIGHTCORNER

    cdef void jmouse_setcursor(t_object *patcherview, t_object *box, t_jmouse_cursortype type)
    cdef t_object* jwind_getactive()
    cdef long jwind_getcount()
    cdef t_object* jwind_getat(long index)
    cdef long jmonitor_getnumdisplays()
    cdef void jmonitor_getdisplayrect(long workarea, long displayindex, t_rect *rect)
    cdef void jmonitor_getdisplayrect_foralldisplays(long workarea, t_rect *rect)
    cdef void jmonitor_getdisplayrect_forpoint(long workarea, t_pt pt, t_rect *rect)
    cdef void swatches_init()
    cdef void swatches_shutdown()
    cdef void *jpatcher_load(char *name, short volume, short ac, t_atom *av)
    cdef void *jpatcher_load_frombuffer(char *name, short vol, const char *json, long len, short ac, t_atom *av)
    cdef void *jpatcher_load_fromdictionary(char *name, short vol, t_object *rd, short ac, t_atom *av)
    cdef void *jpatcher_load_namespace(char *name, short volume, short ac, t_atom *av, t_symbol *classnamespace)
    cdef void *jpatcher_load_frombuffer_namespace(char *name, short vol, const char *json, long len, short ac, t_atom *av, t_symbol *classnamespace)
    cdef void *jpatcher_load_fromdictionary_namespace(char *name, short vol, t_object *rd, short ac, t_atom *av, t_symbol *classnamespace)
    cdef void classname_openrefpage_ext(t_symbol *classnamespace, char *classname)
    cdef long jpatcher_is_box_namespace(t_object *p)
    cdef t_object *jbox_get_dragtarget(t_jbox *b, char locked)
    cdef long jpatcher_inc_maxsendcontext()
    cdef long jbox_is_selected_in_view(t_object *box, t_object *view)
    cdef t_atom_long jpatcher_dictionary_modernui(t_dictionary *d)
    cdef t_atom_long jpatcher_dictionary_version()
    cdef t_dictionary *jpatcher_fallback_version()
    cdef long jbox_isdefaultattribute(t_jbox *x, t_symbol *attrname)
    cdef const char *systemfontname()
    cdef const char *systemfontname_bold()
    cdef const char *systemfontname_light()
    cdef t_symbol *systemfontsym()
    #//#define JPATCHER_DEFAULT_EXTENSION ".maxpat"

cdef extern from "jpatcher_utils.h":

    cdef void atom_copy(long argc1, t_atom *argv1, t_atom *argv2)
    cdef void postargs(long argc, t_atom *argv)
    cdef void postdictionary(t_object *d)
    cdef t_max_err atom_arg_getobjclass(t_object **x, long idx, long argc, t_atom *argv, t_symbol *cls)
    cdef void *atom_getobjclass(t_atom *av, t_symbol *cls)
    cdef method my_object_getmethod(void *obj, t_symbol *s)


cdef extern from "jgraphics.h":
    ctypedef struct t_jgraphics: pass
    ctypedef struct t_jpath: pass
    ctypedef struct t_jpattern: pass
    ctypedef struct t_jfont: pass
    ctypedef struct t_jtextlayout: pass
    ctypedef struct t_jtransform: pass
    ctypedef struct t_jsurface: pass
    ctypedef struct t_jdesktopui: pass
    ctypedef struct t_jpopupmenu: pass
    ctypedef struct t_jsvg: pass
    ctypedef struct t_jsvg_remap: pass

    cdef int JGRAPHICS_RECT_BOTTOM(t_rect *rect)
    cdef int JGRAPHICS_RECT_RIGHT(t_rect *rect)
    cdef int JGRAPHICS_PI
    cdef int JGRAPHICS_2PI
    cdef int JGRAPHICS_PIOVER2
    cdef int JGRAPHICS_3PIOVER2

    ctypedef enum t_jgraphics_line_join:
        JGRAPHICS_LINE_JOIN_MITER
        JGRAPHICS_LINE_JOIN_ROUND
        JGRAPHICS_LINE_JOIN_BEVEL


    ctypedef enum t_jgraphics_line_cap:
        JGRAPHICS_LINE_CAP_BUTT
        JGRAPHICS_LINE_CAP_ROUND
        JGRAPHICS_LINE_CAP_SQUARE


    ctypedef enum t_jgraphics_bubble_side:
        JGRAPHICS_BUBBLE_SIDE_TOP
        JGRAPHICS_BUBBLE_SIDE_LEFT
        JGRAPHICS_BUBBLE_SIDE_BOTTOM
        JGRAPHICS_BUBBLE_SIDE_RIGHT


    ctypedef enum t_jgraphics_path_type:
        JGRAPHICS_PATH_STARTNEWSUBPATH
        JGRAPHICS_PATH_LINETO
        JGRAPHICS_PATH_QUADRATICTO
        JGRAPHICS_PATH_CUBICTO
        JGRAPHICS_PATH_CLOSEPATH


    cdef int jgraphics_round(double d)

    ctypedef enum t_jgraphics_format:
        JGRAPHICS_FORMAT_ARGB32
        JGRAPHICS_FORMAT_RGB24
        JGRAPHICS_FORMAT_A8

    ctypedef enum t_jgraphics_fileformat:
        JGRAPHICS_FILEFORMAT_PNG
        JGRAPHICS_FILEFORMAT_JPEG

    cdef t_jsurface* jgraphics_image_surface_create(t_jgraphics_format format, int width, int height)
    cdef t_jsurface *jgraphics_image_surface_create_referenced(const char *filename, short path)
    cdef t_jsurface* jgraphics_image_surface_create_from_file(const char *filename, short path)
    cdef t_jsurface* jgraphics_image_surface_create_for_data(unsigned char *data, t_jgraphics_format format,  int width, int height, int stride, method freefun, void *freearg)
    cdef t_jsurface* jgraphics_image_surface_create_for_data_premult(unsigned char *data, t_jgraphics_format format, int width, int height, int stride, method freefun, void *freearg)
    cdef t_jsurface* jgraphics_image_surface_create_from_filedata(const void *data, unsigned long datalen)
    cdef t_jsurface* jgraphics_image_surface_create_from_resource(const void* moduleRef, const char *resname)
    cdef t_max_err jgraphics_get_resource_data(const void *moduleRef, const char *resname, long extcount, t_atom *exts, void **data, unsigned long *datasize)
    cdef t_jsurface* jgraphics_surface_reference(t_jsurface *s)
    cdef void        jgraphics_surface_destroy(t_jsurface *s)
    cdef t_max_err   jgraphics_image_surface_writepng(t_jsurface *surface, const char *filename, short path, long dpi)
    cdef t_max_err   jgraphics_image_surface_writejpeg(t_jsurface *surface, const char *filename, short path)
    cdef void        jgraphics_surface_set_device_offset(t_jsurface *s, double x_offset, double y_offset)
    cdef void        jgraphics_surface_get_device_offset(t_jsurface *s, double *x_offset, double *y_offset)
    cdef int         jgraphics_image_surface_get_width(t_jsurface *s)
    cdef int         jgraphics_image_surface_get_height(t_jsurface *s)
    cdef void        jgraphics_image_surface_set_pixel(t_jsurface *s, int x, int y, t_jrgba color)
    cdef void        jgraphics_image_surface_get_pixel(t_jsurface *s, int x, int y, t_jrgba *color)
    cdef void        jgraphics_image_surface_scroll(t_jsurface *s, int x, int y, int width, int height, int dx, int dy, t_jpath **path)
    cdef const unsigned char* jgraphics_image_surface_lockpixels_readonly(t_jsurface *s, int x, int y, int width, int height, int *linestride, int *pixelstride)
    cdef void                 jgraphics_image_surface_unlockpixels_readonly(t_jsurface *s, const unsigned char *data)
    cdef unsigned char*       jgraphics_image_surface_lockpixels(t_jsurface *s, int x, int y, int width, int height, int *linestride, int *pixelstride)
    cdef void                 jgraphics_image_surface_unlockpixels(t_jsurface *s, unsigned char *data)
    cdef void        jgraphics_image_surface_draw(t_jgraphics *g, t_jsurface *s, t_rect srcRect, t_rect destRect)
    cdef void        jgraphics_image_surface_draw_fast(t_jgraphics *g, t_jsurface *s)
    cdef void jgraphics_write_image_surface_to_filedata(t_jsurface *surf, long fmt, void **data, long *size)
    cdef t_jsurface* jgraphics_image_surface_create_from_base64(const char *base64, unsigned long datalen)
    cdef void jgraphics_write_image_surface_to_base64(t_jsurface *surf, long fmt, char **base64, long *size)
    cdef void jgraphics_image_surface_clear(t_jsurface *s, int x, int y, int width, int height)
    cdef t_jsvg*     jsvg_create_from_file(const char *filename, short path)
    cdef t_jsvg*     jsvg_create_from_resource(const void *moduleRef, const char *resname)
    cdef t_jsvg*     jsvg_create_from_xmlstring(const char *svgXML)
    cdef void        jsvg_get_size(t_jsvg *svg, double *width, double *height)
    cdef void        jsvg_destroy(t_jsvg *svg)
    cdef void        jsvg_render(t_jsvg *svg, t_jgraphics *g)
    cdef void jsvg_load_cached(t_symbol *name, t_jsvg **psvg)
    cdef t_jsvg_remap *jsvg_remap_create(t_jsvg *svg)
    cdef void jsvg_remap_addcolor(t_jsvg_remap *r, t_jrgba *src, t_jrgba *dst)
    cdef void jsvg_remap_perform(t_jsvg_remap *r, t_jsvg **remapped)
    cdef void jsvg_remap_destroy(t_jsvg_remap *r)
    cdef void jgraphics_draw_jsvg(t_jgraphics *g, t_jsvg *svg, t_rect *r, int flags, double opacity)
    cdef t_jgraphics*    jgraphics_create(t_jsurface *target)
    cdef t_jgraphics*    jgraphics_reference(t_jgraphics *g)
    cdef void            jgraphics_destroy(t_jgraphics *g)

    ctypedef struct t_jgraphics_path_elem: pass

    cdef void        jgraphics_new_path(t_jgraphics *g)
    cdef t_jpath*    jgraphics_copy_path(t_jgraphics *g)
    cdef t_jpath*    jgraphics_path_createstroked(t_jpath *p, double thickness, t_jgraphics_line_join join, t_jgraphics_line_cap cap)
    cdef void        jgraphics_path_destroy(t_jpath *path)
    cdef void        jgraphics_append_path(t_jgraphics *g, t_jpath *path)
    cdef void        jgraphics_close_path(t_jgraphics *g)
    cdef void        jgraphics_path_roundcorners(t_jgraphics *g, double cornerRadius)
    cdef long        jgraphics_path_contains(t_jpath *path, double x, double y)
    cdef long        jgraphics_path_intersectsline(t_jpath *path, double x1, double y1, double x2, double y2)
    cdef double      jgraphics_path_getlength(t_jpath *path)
    cdef void        jgraphics_path_getpointalongpath(t_jpath *path, double distancefromstart, double *x, double *y)
    cdef double      jgraphics_path_getnearestpoint(t_jpath *path, double x, double y, double *path_x, double *path_y)
    cdef long        jgraphics_path_getpathelems(t_jpath *path, t_jgraphics_path_elem **elems)
    cdef void        jgraphics_get_current_point(t_jgraphics *g, double *x, double *y)
    cdef void        jgraphics_arc(t_jgraphics *g, double xc, double yc, double radius, double angle1, double angle2)

    cdef void jgraphics_piesegment(t_jgraphics *g, double xc, double yc, double radius, double angle1, double angle2, double innercircleproportionalsize)
    cdef void jgraphics_ovalarc(t_jgraphics *g, double xc, double yc, double radiusx, double radiusy, double angle1, double angle2)
    cdef void jgraphics_arc_negative(t_jgraphics *g, double xc, double yc, double radius, double angle1, double angle2)
    cdef void jgraphics_curve_to(t_jgraphics *g,double x1, double y1, double x2, double y2, double x3, double y3)
    cdef void jgraphics_rel_curve_to(t_jgraphics *g, double x1, double y1, double x2, double y2, double x3, double y3)
    cdef void jgraphics_line_to(t_jgraphics *g, double x, double y)
    cdef void jgraphics_rel_line_to(t_jgraphics *g, double x, double y)
    cdef void jgraphics_move_to(t_jgraphics *g, double x, double y)
    cdef void jgraphics_rel_move_to(t_jgraphics *g, double x, double y)
    cdef void jgraphics_rectangle(t_jgraphics *g, double x, double y, double width, double height)
    cdef void jgraphics_oval(t_jgraphics *g,    double x, double y, double width, double height)
    cdef void jgraphics_rectangle_rounded(t_jgraphics *g, double x, double y, double width, double height, double ovalwidth, double ovalheight)
    cdef void jgraphics_ellipse(t_jgraphics *g,  double x, double y, double width, double height)
    cdef void jgraphics_bubble(t_jgraphics *g, double bodyx, double bodyy,  double bodywidth, double bodyheight, double cornersize, double arrowtipx, double arrowtipy, t_jgraphics_bubble_side whichside, double arrowedgeprop, double arrowwidth)
    cdef void jgraphics_triangle(t_jgraphics *g, double x1, double y1, double x2, double y2, double x3, double y3)
    cdef void jgraphics_diagonal_line_fill(t_jgraphics *g, double pixels, double x, double y, double width, double height)

    ctypedef enum t_jgraphics_font_slant:
        JGRAPHICS_FONT_SLANT_NORMAL,
        JGRAPHICS_FONT_SLANT_ITALIC

    ctypedef enum t_jgraphics_font_weight:
        JGRAPHICS_FONT_WEIGHT_NORMAL
        JGRAPHICS_FONT_WEIGHT_BOLD

    cdef void jgraphics_select_font_face(t_jgraphics *g, const char *family, t_jgraphics_font_slant slant, t_jgraphics_font_weight weight)
    cdef void jgraphics_select_jfont(t_jgraphics *g, t_jfont *jfont)
    cdef void jgraphics_set_font_size(t_jgraphics *g, double size)
    cdef void jgraphics_set_underline(t_jgraphics *g, char underline)
    cdef void jgraphics_show_text(t_jgraphics *g, const char *utf8)
    cdef void jgraphics_text_path(t_jgraphics *g, const char *utf8)

    ctypedef struct t_jgraphics_font_extents: pass

    cdef void jgraphics_font_extents(t_jgraphics *g, t_jgraphics_font_extents *extents)
    cdef void jgraphics_text_measure(t_jgraphics *g, const char *utf8, double *width, double *height)
    cdef void jgraphics_text_measuretext_wrapped(t_jgraphics *g, const char *utf8, double wrapwidth, long includewhitespace, double *width, double *height, long *numlines)
    cdef double jgraphics_getfontscale()
    cdef t_jfont* jfont_create_from_maxfont(short number, short size)
    cdef t_jfont* jfont_create(const char *family, t_jgraphics_font_slant slant, t_jgraphics_font_weight weight, double size)
    cdef t_jfont* jfont_reference(t_jfont *font)
    cdef void jfont_destroy(t_jfont *font)
    cdef long jfont_isequalto(t_jfont *font, t_jfont *other)
    cdef void jfont_set_family(t_jfont *font, t_symbol *family)
    cdef t_symbol* jfont_get_family(t_jfont *font)
    cdef void jfont_set_slant(t_jfont *font, t_jgraphics_font_slant slant)
    cdef t_jgraphics_font_slant jfont_get_slant(t_jfont *font)
    cdef void jfont_set_weight(t_jfont *font, t_jgraphics_font_weight weight)
    cdef t_jgraphics_font_weight jfont_get_weight(t_jfont *font)
    cdef void jfont_set_font_size(t_jfont *font, double size)
    cdef double jfont_get_font_size(t_jfont *font)
    cdef void jfont_set_underline(t_jfont *font, char ul)
    cdef char jfont_get_underline(t_jfont *font)
    cdef double jfont_get_heighttocharheightratio(t_jfont *font)
    cdef void jfont_extents(t_jfont *font, t_jgraphics_font_extents *extents)
    cdef void jfont_text_measure(t_jfont *font, const char *utf8, double *width, double *height)
    cdef void jfont_text_measuretext_wrapped(t_jfont *font, const char *utf8, double wrapwidth, long includewhitespace, double *width, double *height, long *numlines)
    cdef void jfont_get_em_dimensions(t_jfont *font, double *width, double *height)
    cdef t_max_err  jfont_getfontlist(long *count, t_symbol ***list)
    cdef long jfont_isfixedwidth(const char *name)
    cdef const char *jfont_get_default_fixedwidth_name()
    cdef void jfont_set_juce_default_fontname(char *s)
    cdef void jfont_copy_juce_default_fontname(char *s, long maxlen)
    cdef void jfont_copy_juce_platform_fontname(char *s, long maxlen)
    cdef void jfont_set_juce_fallback_fontname(char *s)
    cdef void jfont_copy_juce_fallback_fontname(char *s, long maxlen)
    cdef long jgraphics_system_canantialiastexttotransparentbg()
    cdef long jgraphics_fontname_hasglyph(char *name, long code)
    cdef t_jtextlayout* jtextlayout_create()
    cdef t_jtextlayout* jtextlayout_withbgcolor(t_jgraphics *g, t_jrgba *bgcolor)
    cdef void jtextlayout_destroy(t_jtextlayout* textlayout)

    ctypedef enum t_jgraphics_text_justification:
        JGRAPHICS_TEXT_JUSTIFICATION_LEFT = 1
        JGRAPHICS_TEXT_JUSTIFICATION_RIGHT = 2
        JGRAPHICS_TEXT_JUSTIFICATION_HCENTERED = 4
        JGRAPHICS_TEXT_JUSTIFICATION_TOP = 8
        JGRAPHICS_TEXT_JUSTIFICATION_BOTTOM = 16
        JGRAPHICS_TEXT_JUSTIFICATION_VCENTERED = 32
        JGRAPHICS_TEXT_JUSTIFICATION_HJUSTIFIED = 64
        JGRAPHICS_TEXT_JUSTIFICATION_CENTERED = JGRAPHICS_TEXT_JUSTIFICATION_HCENTERED + JGRAPHICS_TEXT_JUSTIFICATION_VCENTERED

    ctypedef enum t_jgraphics_textlayout_flags:
        JGRAPHICS_TEXTLAYOUT_NOWRAP = 1
        JGRAPHICS_TEXTLAYOUT_USEELLIPSIS = 3

    cdef void jtextlayout_set(t_jtextlayout *textlayout, const char *utf8,  t_jfont *jfont, double x, double y, double width, double height, t_jgraphics_text_justification justification, t_jgraphics_textlayout_flags flags)
    cdef void jtextlayout_settext(t_jtextlayout *textlayout, const char *utf8, t_jfont *jfont)
    cdef void jtextlayout_settextcolor(t_jtextlayout *textlayout, t_jrgba *textcolor)
    cdef void jtextlayout_measuretext(t_jtextlayout *textlayout, long startindex, long numchars, long includewhitespace, double *width, double *height, long *numlines)
    cdef void jtextlayout_draw(t_jtextlayout *tl, t_jgraphics *g)
    cdef long jtextlayout_getnumchars(t_jtextlayout *tl)
    cdef t_max_err jtextlayout_getcharbox(t_jtextlayout *tl, long index, t_rect *rect)
    cdef t_max_err jtextlayout_getchar(t_jtextlayout *tl, long index, long *pch)
    cdef t_jpath* jtextlayout_createpath(t_jtextlayout *tl)

    ctypedef struct t_jmatrix: pass


    cdef void jgraphics_matrix_init(t_jmatrix *x, double xx, double yx, double xy, double yy, double x0, double y0)
    cdef void jgraphics_matrix_init_identity(t_jmatrix *x)
    cdef void jgraphics_matrix_init_translate(t_jmatrix *x, double tx, double ty)
    cdef void jgraphics_matrix_init_scale(t_jmatrix *x, double sx, double sy)
    cdef void jgraphics_matrix_init_rotate(t_jmatrix *x, double radians)
    cdef void jgraphics_matrix_translate(t_jmatrix *x, double tx, double ty)
    cdef void jgraphics_matrix_scale(t_jmatrix *x, double sx, double sy)
    cdef void jgraphics_matrix_rotate(t_jmatrix *x, double radians)
    cdef void jgraphics_matrix_invert(t_jmatrix *x)
    cdef void jgraphics_matrix_multiply(t_jmatrix *result, const t_jmatrix *a, const t_jmatrix *b)
    cdef void jgraphics_matrix_transform_point(const t_jmatrix *matrix, double *x, double *y)
    cdef t_jpattern*    jgraphics_pattern_create_rgba(double red, double green, double blue, double alpha)
    cdef t_jpattern*    jgraphics_pattern_create_for_surface(t_jsurface *surface)
    cdef t_jpattern* jgraphics_pattern_create_linear(double x0, double y0, double x1, double y1)
    cdef t_jpattern* jgraphics_pattern_create_radial(double cx0, double cy0, double radius0, double cx1, double cy1, double radius1)
    cdef void jgraphics_pattern_add_color_stop_rgba(t_jpattern* pattern, double offset, double red, double green, double blue, double alpha)
    cdef void jgraphics_pattern_add_color_for_proportion(t_jpattern* pattern, double proportion)
    cdef t_jpattern *jgraphics_pattern_reference(t_jpattern *pattern)
    cdef void jgraphics_pattern_destroy(t_jpattern *pattern)

    ctypedef enum t_jgraphics_pattern_type:
        JGRAPHICS_PATTERN_TYPE_SOLID
        JGRAPHICS_PATTERN_TYPE_SURFACE
        JGRAPHICS_PATTERN_TYPE_LINEAR
        JGRAPHICS_PATTERN_TYPE_RADIAL

    t_jgraphics_pattern_type jgraphics_pattern_get_type(t_jpattern *pattern)

    ctypedef enum t_jgraphics_extend:
        JGRAPHICS_EXTEND_NONE
        JGRAPHICS_EXTEND_REPEAT
        JGRAPHICS_EXTEND_REFLECT
        JGRAPHICS_EXTEND_PAD

    ##define JGRAPHICS_EXTEND_GRADIENT_DEFAULT JGRAPHICS_EXTEND_PAD
    ##define JGRAPHICS_EXTEND_SURFACE_DEFAULT JGRAPHICS_EXTEND_NONE

    cdef void jgraphics_pattern_set_extend(t_jpattern *pattern, t_jgraphics_extend extend)
    cdef t_jgraphics_extend jgraphics_pattern_get_extend(t_jpattern *pattern)
    cdef void jgraphics_pattern_set_matrix(t_jpattern *pattern, const t_jmatrix *matrix)
    cdef void jgraphics_pattern_get_matrix(t_jpattern *pattern, t_jmatrix *matrix)
    cdef void jgraphics_pattern_translate(t_jpattern *pattern, double tx, double ty)
    cdef void jgraphics_pattern_scale(t_jpattern *pattern, double sx, double sy)
    cdef void jgraphics_pattern_rotate(t_jpattern *pattern, double angle)
    cdef void jgraphics_pattern_transform(t_jpattern *pattern, const t_jmatrix *matrix)
    cdef void jgraphics_pattern_identity_matrix(t_jpattern *pattern)
    cdef t_jsurface *jgraphics_pattern_get_surface(t_jpattern *pattern)
    cdef void jgraphics_translate(t_jgraphics *g, double tx, double ty)
    cdef void jgraphics_scale(t_jgraphics *g, double sx, double sy)
    cdef void jgraphics_rotate(t_jgraphics *g, double angle)
    cdef void jgraphics_transform(t_jgraphics *g, const t_jmatrix *matrix)
    cdef void jgraphics_set_matrix(t_jgraphics *g, const t_jmatrix *matrix)
    cdef void jgraphics_get_matrix(t_jgraphics *g, t_jmatrix *matrix)
    cdef void jgraphics_identity_matrix(t_jgraphics *g)
    cdef void jgraphics_user_to_device(t_jgraphics *g, double *x, double *y)
    cdef void jgraphics_device_to_user(t_jgraphics *g, double *x, double *y)
    cdef void jgraphics_save(t_jgraphics* g)
    cdef void jgraphics_restore(t_jgraphics *g)
    cdef t_jsurface* jgraphics_get_target(t_jgraphics *g)
    cdef void jgraphics_push_group(t_jgraphics *g)
    cdef t_jpattern* jgraphics_pop_group(t_jgraphics *g)
    cdef void jgraphics_pop_group_to_source(t_jgraphics *g)
    cdef t_jsurface* jgraphics_get_group_target(t_jgraphics *g)
    cdef t_jsurface* jgraphics_pop_group_surface(t_jgraphics *g)
    cdef void jgraphics_set_source_rgba(t_jgraphics *g,double red, double green, double blue, double alpha)
    cdef void jgraphics_set_source_jrgba(t_jgraphics *g, t_jrgba *rgba)
    cdef void jgraphics_set_source_rgb(t_jgraphics *g, double red, double green, double blue)
    cdef void jgraphics_set_source(t_jgraphics *g, t_jpattern *source)
    cdef void jgraphics_set_source_surface(t_jgraphics *g, t_jsurface *surface, double x, double y)


    ctypedef enum t_jgraphics_pattern_shared:
        JGRAPHICS_PATTERN_GRAY = 0
        JGRAPHICS_NUM_SHARED_PATTERNS



    cdef void jgraphics_set_source_shared(t_jgraphics *g, t_jgraphics_pattern_shared patindex)
    cdef void jgraphics_scale_source_rgba(t_jgraphics *g, double redscale, double greenscale, double bluescale, double alphascale)
    cdef void jgraphics_translate_source_rgba(t_jgraphics *g, double redoffset, double greenoffset, double blueoffset, double alphaoffset)
    cdef void jgraphics_set_dash(t_jgraphics *g, double *dashes, int numdashes, double offset)

    ctypedef enum t_jgraphics_fill_rule:
        JGRAPHICS_FILL_RULE_WINDING
        JGRAPHICS_FILL_RULE_EVEN_ODD


    cdef void jgraphics_set_fill_rule(t_jgraphics *g, t_jgraphics_fill_rule fill_rule)
    cdef t_jgraphics_fill_rule jgraphics_get_fill_rule(t_jgraphics *g)
    cdef void jgraphics_set_line_cap(t_jgraphics *g, t_jgraphics_line_cap line_cap)
    cdef t_jgraphics_line_cap jgraphics_get_line_cap(t_jgraphics *g)
    cdef void jgraphics_set_line_join(t_jgraphics *g, t_jgraphics_line_join line_join)
    cdef t_jgraphics_line_join  jgraphics_get_line_join(t_jgraphics *g)
    cdef void jgraphics_set_line_width(t_jgraphics *g, double width)
    cdef double jgraphics_get_line_width(t_jgraphics *g)
    cdef void jgraphics_fill(t_jgraphics *g)
    cdef void jgraphics_fill_preserve(t_jgraphics *g)
    cdef void jgraphics_fill_with_alpha(t_jgraphics *g, double alpha)
    cdef void jgraphics_fill_preserve_with_alpha(t_jgraphics *g, double alpha)
    cdef int jgraphics_in_fill(t_jgraphics *g, double x, double y)
    cdef int    jgraphics_path_intersects_line(t_jgraphics *g, double x1, double y1, double x2, double y2)
    cdef int jgraphics_ptinrect(t_pt pt, t_rect rect)
    cdef int jgraphics_lines_intersect(double l1x1, double l1y1, double l1x2, double l1y2, double l2x1, double l2y1, double l2x2, double l2y2, double *ix, double *iy)
    cdef int jgraphics_line_intersects_rect(double linex1, double liney1, double linex2, double liney2, t_rect r, double *ix, double *iy)
    cdef int jgraphics_ptaboveline(t_pt pt, double lx1, double ly1, double lx2, double ly2)
    cdef int jgraphics_points_on_same_side_of_line(t_pt a, t_pt b, double lx1, double ly1, double lx2, double ly2)
    cdef int jgraphics_ptinroundedrect(t_pt pt, t_rect rect, double ovalwidth, double ovalheight)


    cdef void jgraphics_fill_extents(t_jgraphics *g, double *x1, double *y1, double *x2, double *y2)
    cdef void jgraphics_paint(t_jgraphics *g)
    cdef void jgraphics_paint_with_alpha(t_jgraphics *g, double alpha)
    cdef void jgraphics_stroke(t_jgraphics *g)
    cdef void jgraphics_stroke_preserve(t_jgraphics *g)
    cdef void jgraphics_stroke_with_alpha(t_jgraphics *g, double alpha)
    cdef void jgraphics_stroke_preserve_with_alpha(t_jgraphics *g, double alpha)
    cdef void jgraphics_rectangle_fill_fast(t_jgraphics *g, double x, double y, double width, double height)
    cdef void jgraphics_rectangle_draw_fast(t_jgraphics *g, double x, double y, double width, double height, double border)
    cdef void jgraphics_line_draw_fast(t_jgraphics *g, double x1, double y1, double x2, double y2, double linewidth)

    ctypedef enum t_jdesktopui_flags:
        JDESKTOPUI_FLAGS_FIRSTFLAG = 1

    cdef t_jdesktopui* jdesktopui_new(t_object *owner, t_jdesktopui_flags flags, t_rect rect)
    cdef void jdesktopui_destroy(t_jdesktopui *x)
    cdef void jdesktopui_setvisible(t_jdesktopui *x, long way)
    cdef void jdesktopui_setalwaysontop(t_jdesktopui *x, long way)
    cdef void jdesktopui_setrect(t_jdesktopui *x, t_rect rect)
    cdef void jdesktopui_getrect(t_jdesktopui *x, t_rect *rect)
    cdef void jdesktopui_setposition(t_jdesktopui *x, t_pt pt)
    cdef void jdesktopui_setfadetimes(t_jdesktopui *x, int fade_in_ms, int fade_out_ms)
    cdef t_jgraphics* jdesktopui_get_jgraphics(t_jdesktopui *x)
    cdef void jdesktopui_redraw(t_jdesktopui *x)
    cdef void jdesktopui_redrawrect(t_jdesktopui *x, t_rect rect)
    cdef double jdesktopui_getopacity(t_jdesktopui *x)
    cdef void *jdesktopui_createtimer(t_jdesktopui *x, t_symbol *msg, void *arg)
    cdef void jdesktopui_starttimer(void *ref, int interval)
    cdef void jdesktopui_stoptimer(void *ref, int alsodelete)
    cdef void jdesktopui_destroytimer(void *ref)
    cdef t_jrgba jgraphics_jrgba_contrasting(t_jrgba *c, double amount)
    cdef t_jrgba jgraphics_jrgba_contrastwith(t_jrgba *c1, t_jrgba *c2)
    cdef t_jrgba jgraphics_jrgba_darker(t_jrgba *c, double amount)
    cdef t_jrgba jgraphics_jrgba_brighter(t_jrgba *c, double amount)
    cdef t_jrgba jgraphics_jrgba_overlay(t_jrgba *c1, t_jrgba *c2)
    cdef t_jrgba jgraphics_jrgba_interpolate(t_jrgba *c1, t_jrgba *c2, double proportion)
    cdef void jgraphics_jrgba_gethsb(t_jrgba *c, double *h, double *s, double *b)
    cdef t_jrgba jgraphics_jrgba_fromhsb(double h, double s, double b, double a)
    cdef long jcolor_getcolor(t_symbol *name, t_jrgba *on, t_jrgba *off)
    cdef t_jpopupmenu* jpopupmenu_create()
    cdef void jpopupmenu_destroy(t_jpopupmenu *menu)
    cdef void jpopupmenu_clear(t_jpopupmenu *menu)
    cdef void jpopupmenu_setitemcallback(method fun, void *arg)

    cdef void jpopupmenu_setcolors(t_jpopupmenu *menu, t_jrgba text, t_jrgba bg, t_jrgba highlightedtext, t_jrgba highlightedbg)
    cdef void jpopupmenu_setheadercolor(t_jpopupmenu *menu, t_jrgba *hc)
    cdef void jpopupmenu_setfont(t_jpopupmenu *menu, t_jfont *font)
    cdef void jpopupmenu_additem(t_jpopupmenu *menu, int itemid, const char *utf8Text, t_jrgba *textColor, int checked, int disabled, t_jsurface *icon)
    cdef void jpopupmenu_addsubmenu(t_jpopupmenu *menu, const char *utf8Name, t_jpopupmenu *submenu, int disabled)
    cdef void jpopupmenu_addseperator(t_jpopupmenu *menu)
    cdef void jpopupmenu_addheader(t_jpopupmenu *menu, const char *utf8Text)
    cdef void jpopupmenu_addownerdrawitem(t_jpopupmenu *menu, int itemid, t_object *owner)
    cdef int jpopupmenu_popup(t_jpopupmenu *menu, t_pt screen, int defitemid)
    cdef int jpopupmenu_popup_abovebox(t_jpopupmenu *menu, t_object *box, t_object *view, int offset, int defitemid)
    cdef int jpopupmenu_popup_nearbox(t_jpopupmenu *menu, t_object *box, t_object *view, int defitemid)
    cdef int jpopupmenu_popup_belowrect(t_jpopupmenu *menu, t_rect rect, int defitemid)
    cdef int jpopupmenu_popup_aboverect(t_jpopupmenu *menu, t_rect rect, int defitemid)
    cdef int jpopupmenu_popup_leftofpt(t_jpopupmenu *menu, t_pt pt, int defitemid, int flags)
    cdef void jpopupmenu_estimatesize(t_jpopupmenu *menu, int *width, int *height)
    cdef void jpopupmenu_setitemtooltip(void *itemref, char *tip)

    ctypedef enum:
        JPOPUPMENU_DARKSTYLE = 1

    cdef void jpopupmenu_setstandardstyle(t_jpopupmenu *menu, long styleindex, double fontsize, int margin)
    cdef void jpopupmenu_setstandardstyle_forjucemenu(void *jpm, long styleindex, double fontsize, t_jrgba *headertextcolor)
    cdef void jpopupmenu_closeall()
    cdef void jmouse_setcursor_surface(t_object *patcherview, t_object *box, t_jsurface *surface, int xHotSpot, int yHotSpot)
    cdef void jbox_fontface_to_weight_slant(t_object *b, long *weight, long *slant)
    cdef long jbox_get_font_weight(t_object *b)
    cdef long jbox_get_font_slant(t_object *b)
    cdef t_jfont *jbox_createfont(t_object *b)
    cdef void jgraphics_jrgba_set_brightness(t_jrgba *c, double amt)
    cdef t_max_err jgraphics_attr_setrgba(t_object *x, t_object *attr, long argc, t_atom *argv)
    cdef t_max_err jgraphics_attr_setrgb_alias(t_object *x, t_object *attr, long argc, t_atom *argv)

    # #define CLASS_ATTR_RGBA(c,attrname,flags,structname,structmember) \
    #     {   CLASS_ATTR_DOUBLE_ARRAY(c,attrname,flags,structname,structmember,4) \
    #         CLASS_ATTR_ACCESSORS(c,attrname,NULL,jgraphics_attr_setrgba) \
    #         CLASS_ATTR_PAINT(c,attrname,0) }
    # #define CLASS_ATTR_RGBA_LEGACY(c,attrname,aliasname,flags,structname,structmember) \
    #     {   CLASS_ATTR_RGBA(c,attrname,flags,structname,structmember) \
    #         CLASS_ATTR_ALIAS(c,attrname,aliasname) \
    #         CLASS_ATTR_INVISIBLE(c,aliasname,0) \
    #         CLASS_ATTR_ACCESSORS(c,aliasname,NULL,jgraphics_attr_setrgb_alias) }

    cdef t_max_err object_attr_getjrgba(void *ob, t_symbol *s, t_jrgba *c)
    cdef t_max_err object_attr_setjrgba(void *ob, t_symbol *s, t_jrgba *c)
    cdef void jrgba_to_atoms(t_jrgba *c, t_atom *argv)
    cdef t_max_err atoms_to_jrgba(long argc, t_atom *argv, t_jrgba *c)
    cdef void jrgba_set(t_jrgba *prgba, double r, double g, double b, double a)
    cdef void jrgba_copy(t_jrgba *dest, t_jrgba *src)
    cdef long jrgba_compare(t_jrgba *rgba1, t_jrgba *rgba2)
    cdef void jgraphics_getfiletypes(void *dummy, long *count, t_fourcc **filetypes, char *alloc)
    cdef t_max_err jbox_invalidate_layer(t_object *b, t_object *view, t_symbol *name)
    cdef t_max_err jbox_remove_layer(t_object *b, t_object *view, t_symbol *name)
    cdef t_jgraphics* jbox_start_layer(t_object *b, t_object *view, t_symbol *name, double width, double height)
    cdef t_max_err jbox_end_layer(t_object *b, t_object *view, t_symbol *name)
    cdef t_max_err jbox_paint_layer(t_object *b, t_object *view, t_symbol *name, double x, double y)
    cdef long jgraphics_rectintersectsrect(t_rect *r1, t_rect *r2)
    cdef long jgraphics_rectcontainsrect(t_rect *outer, t_rect *inner)
    cdef void jgraphics_position_one_rect_near_another_rect_but_keep_inside_a_third_rect( t_rect *positioned_rect, const t_rect *positioned_near_this_rect, const t_rect *keep_inside_this_rect)
    cdef void jgraphics_clip(t_jgraphics *g, double x, double y, double width, double height)



cdef extern from "ext_boxstyle.h":

    cdef void class_attr_setstyle(t_class *c, const char *s)
    cdef void class_attr_style_alias(t_class *c, const char *name, const char *aliasname, long legacy)
    cdef int FILL_ATTR_SAVE
    cdef void class_attr_setfill(t_class *c, const char *name, long flags)
    cdef void jgraphics_attr_fillrect(t_object *b, t_jgraphics *g, t_symbol *attrname, t_rect *area)
    cdef t_jpattern *jgraphics_attr_setfill(t_object *b, t_jgraphics *g, t_symbol *attrname, t_rect *area)
    cdef void object_attr_getfillcolor_atposition(t_object *b, const char *attrname, double pos, t_jrgba *c)
    cdef long object_attr_getfill(t_object *obj, t_symbol *attrname)
    cdef void object_style_setfillattribute(t_object *x, t_symbol *fillattr, t_symbol *entry, long argc, t_atom *argv)
    cdef void class_attr_stylemap(t_class *c, char *attrname, char *mapname)
    cdef t_symbol *object_attr_attrname_forstylemap(t_object *x, t_symbol *mapname)
    cdef t_symbol *object_attr_stylemapname(t_object *x, t_symbol *attrname)
    cdef t_jpopupmenu *style_getmenu(t_object *context, t_symbol *current, long mask, long *selecteditem, long *headercount)
    cdef void style_handlemenu(t_object *context, long itemindex, t_symbol **current)

    cdef void CLASS_ATTR_STYLE_RGBA_NOSAVE(c,attrname,flags,structname,structmember,label)
    cdef void CLASS_ATTR_STYLE_RGBA(c,attrname,flags,structname,structmember,label)
    cdef void CLASS_ATTR_STYLE_RGBA_PREVIEW(c,attrname,flags,structname,structmember,label,previewtype)
    cdef void CLASS_ATTR_STYLE_ALIAS_NOSAVE(c,attrname,aliasname)
    cdef void CLASS_ATTR_STYLE_ALIAS_COMPATIBILITY(c,attrname,aliasname)
    cdef void CLASS_ATTR_STYLE_ALIAS_RGBA_LEGACY(c,attrname,aliasname)



cdef extern from "jdataview.h":

    cdef int JDATAVIEW_CELLMAX

    cdef int JCOLUMN_DEFAULT_WIDTH
    cdef int JCOLUMN_DEFAULT_MINWIDTH
    cdef int JCOLUMN_DEFAULT_MAXWIDTH
    cdef int JCOLUMN_DEFAULT_FONTSIZE
    cdef int JCOLUMN_DEFAULT_MAXTEXTLEN

    ctypedef enum:
        JCOLUMN_ALIGN_LEFT = 1
        JCOLUMN_ALIGN_CENTER
        JCOLUMN_ALIGN_RIGHT

    ctypedef enum:
        JCOLUMN_STYLE_PLAIN = 0
        JCOLUMN_STYLE_BOLD = 1
        JCOLUMN_STYLE_ITALIC = 2

    ctypedef enum:
        JCOLUMN_MOUSE_ENTER = 0
        JCOLUMN_MOUSE_MOVE = 1
        JCOLUMN_MOUSE_EXIT = 2

    ctypedef enum:
        JCOLUMN_SORTDIRECTION_FORWARD = 1
        JCOLUMN_SORTDIRECTION_BACKWARD = 0

    ctypedef enum:
        JCOLUMN_INITIALLYSORTED_NONE = 0
        JCOLUMN_INITIALLYSORTED_FORWARDS = 1
        JCOLUMN_INITIALLYSORTED_BACKWARDS = 2

    ctypedef enum:
        JCOLUMN_COMPONENT_NONE = 0
        JCOLUMN_COMPONENT_CHECKBOX = 1
        JCOLUMN_COMPONENT_TEXTEDITOR = 2
        JCOLUMN_COMPONENT_SLIDER = 3
        JCOLUMN_COMPONENT_COLOR = 4
        JCOLUMN_COMPONENT_MENU = 5

    ctypedef enum:
        JCOLUMN_MENU_INDEX = 1
        JCOLUMN_MENU_FONTMENU = 2
        JCOLUMN_MENU_PAINT = 4
        JCOLUMN_MENU_SELECT = 8
        JCOLUMN_MENU_NOPANEL = 16
        JCOLUMN_MENU_CLEARITEM = 32
        JCOLUMN_MENU_STYLEMENU = 64

    ctypedef enum:
        JCOLUMN_TEXT_ONESYMBOL = 1
        JCOLUMN_TEXT_COMMASEPARATED = 2
        JCOLUMN_TEXT_ICON = 4
        JCOLUMN_TEXT_HASBUTTON = 8
        JCOLUMN_TEXT_FILECHOOSEBUTTON = 16
        JCOLUMN_TEXT_VIEWBUTTON = 32
        JCOLUMN_TEXT_EDITBUTTON = 64
        JCOLUMN_TEXT_TIME = 128
        JCOLUMN_TEXT_FLOAT = 256
        JCOLUMN_TEXT_INT = 512
        JCOLUMN_TEXT_CANTEDIT = 1024
        JCOLUMN_TEXT_FONTSIZE = 2048

        JCOLUMN_TEXT_FILTERED = 8192
        JCOLUMN_TEXT_STRINGOBJECT = 16384
        JCOLUMN_TEXT_PITCH = 32768

    ctypedef enum:
        JCOLUMN_SLIDER_NOTEXTBOX = 1,
        JCOLUMN_SLIDER_ZERO_TO_ONE = 2

    cdef int JCOLUMN_DISABLED

    ctypedef struct t_celldesc: pass
    ctypedef struct t_jcolumn: pass
    ctypedef struct  t_jdataview: pass
    ctypedef struct t_jdv_notifier: pass
    ctypedef struct t_privatesortrec: pass
    ctypedef struct t_jsection: pass

    cdef void jdataview_initclass()
    cdef void *jdataview_new()
    cdef void jdataview_setclient(t_object *dv, t_object *client)
    cdef t_object *jdataview_getclient(t_object *dv)
    cdef void *jdataview_newsection(t_object *dv, char *name, void *assoc, t_jsurface *icon, char initiallyopen, char headervisible)
    cdef int jdataview_numsections(t_object *dv)
    cdef void *jdataview_getnthsection(t_object *dv, long index)
    cdef int jdataview_section_getnumrows(t_object *dv, void *section)
    cdef long jdataview_section_isopen(t_object *dv, void *section)
    cdef void jdataview_section_setopen(t_object *dv, void *section, long way)
    cdef void jdataview_getsectionopenness(t_object *dv, char **state)
    cdef void jdataview_setsectionopenness(t_object *dv, char *state)
    cdef long jdataview_section_headervisible(t_object *dv, void *section)
    cdef void jdataview_section_setheadervisible(t_object *dv, void *section, long way)
    cdef t_symbol *jdataview_section_getname(t_object *dv, void *section)
    cdef t_jsurface *jdataview_section_geticon(t_object *dv, void *section)
    cdef void jdataview_patchervis(t_object *dv, t_object *pv, t_object *box)
    cdef void jdataview_patcherinvis(t_object *dv, t_object *pv)
    cdef void jdataview_obscuring(t_object *dv, t_object *pv)
    cdef void jdataview_setheight(t_object *dv, long height)
    cdef long jdataview_getheight(t_object *dv)
    cdef void jdataview_setautoheight(t_object *dv, long way)
    cdef short jdataview_getautoheight(t_object *dv)
    cdef void jdataview_setcolumnheaderheight(t_object *dv, double height)
    cdef double jdataview_getcolumnheaderheight(t_object *dv)
    cdef void jdataview_setrowcolor1(t_object *dv, t_jrgba *c)
    cdef void jdataview_getrowcolor1(t_object *dv, t_jrgba *c)
    cdef void jdataview_setrowcolor2(t_object *dv, t_jrgba *c)
    cdef void jdataview_getrowcolor2(t_object *dv, t_jrgba *c)
    cdef void jdataview_getselectcolor(t_object *dv, t_jrgba *c)
    cdef void jdataview_setselectcolor(t_object *dv, t_jrgba *c)
    cdef void jdataview_setusegradient(t_object *dv, long way)
    cdef long jdataview_getusegradient(t_object *dv)
    cdef void jdataview_setcanselectmultiple(t_object *dv, long way)
    cdef short jdataview_getcanselectmultiple(t_object *dv)
    cdef void jdataview_setcancopy(t_object *dv, long way)
    cdef short jdataview_getcancopy(t_object *dv)
    cdef void jdataview_setcanpaste(t_object *dv, long way)
    cdef short jdataview_getcanpaste(t_object *dv)
    cdef void jdataview_setinset(t_object *dv, long inset)
    cdef long jdataview_getinset(t_object *dv)
    cdef void jdataview_setautosizeright(t_object *dv, long way)
    cdef long jdataview_getautosizeright(t_object *dv)
    cdef void jdataview_setautosizebottom(t_object *dv, long way)
    cdef long jdataview_getautosizebottom(t_object *dv)
    cdef void jdataview_setautosizerightcolumn(t_object *dv, long way)
    cdef long jdataview_getautosizerightcolumn(t_object *dv)
    cdef void jdataview_setusecharheightfont(t_object *dv, long way)
    ctypedef long (*t_containersizechange_fun)(t_object *x, double cw, double ch, double *width, double *height, int asr, int asb)
    cdef t_atom_long jdataview_containersizechange(t_object *x, double cw, double ch, double *width, double *height, int asr, int asb)
    cdef t_max_err jdataview_gethorizscrollvalues(t_object *x, double *min, double *max, double *start, double *size)
    cdef void jdataview_sethorizscrollvalues(t_object *x, double start, double size)
    cdef t_max_err jdataview_getvertscrollvalues(t_object *x, double *min, double *max, double *start, double *size)
    cdef void jdataview_setvertscrollvalues(t_object *x, double start, double size)
    cdef t_max_err jdataview_setscrollvisible(t_object *x, long vbar, long hbar)
    cdef void jdataview_setborderthickness(t_object *dv, long val)
    cdef long jdataview_getborderthickness(t_object *dv)
    cdef void jdataview_setkeyfocusable(t_object *x, long val)
    cdef long jdataview_getkeyfocusable(t_object *x)
    cdef void jdataview_setenabledeletekey(t_object *dv, long way)
    cdef long jdataview_getenabledeletekey(t_object *dv)
    cdef void jdataview_setfontname(t_object *dv, t_symbol *fontname)
    cdef t_symbol *jdataview_getfontname(t_object *dv)
    cdef void jdataview_setfontsize(t_object *dv, double fsize)
    cdef double jdataview_getfontsize(t_object *dv)
    cdef double jdataview_getclientfontsize(t_object *dv)
    cdef void jdataview_columnheadermouse(t_object *dv, t_object *col, long msg)
    cdef int jdataview_getdragenabled(t_object *dv)
    cdef void jdataview_setdragenabled(t_object *dv, long way)
    cdef void jdataview_setcolumnheadercluemsg(t_object *dv, t_symbol *msg)
    cdef t_symbol *jdataview_getcolumnheadercluemsg(t_object *dv)
    cdef int jdataview_getdrawgrid(t_object *dv)
    cdef void jdataview_setdrawgrid(t_object *dv, int way)
    cdef void jdataview_setrowinset(t_object *dv, long top, long bottom)
    cdef void jdataview_getrowinset(t_object *dv, long *top, long *bottom)
    cdef t_object *jdataview_getsearchcolumn(t_object *dv)
    cdef void jdataview_setsearchcolumn(t_object *dv, t_object *col)
    cdef void jdataview_setoverridefocus(t_object *dv, long way)
    cdef long jdataview_getoverridefocus(t_object *dv)
    cdef void jdataview_setreturnkeycolumn(t_object *dv, t_object *col)
    cdef t_object *jdataview_getreturnkeycolumn(t_object *dv)
    cdef int jdataview_keynavigate(t_object *dv, char *buffer)
    cdef void jdataview_setheaderbgcolor(t_object *dv, t_jrgba *c)
    cdef void jdataview_setheadertextcolor(t_object *dv, t_jrgba *c)
    cdef void jdataview_getheaderbgcolor(t_object *dv, t_jrgba *c)
    cdef void jdataview_getheadertextcolor(t_object *dv, t_jrgba *c)
    cdef long jdataview_getheadersolidcolor(t_object *dv)
    cdef void jdataview_setheadersolidcolor(t_object *dv, long way)
    cdef void jdataview_setcontext(t_object *dv, void *context)
    cdef void *jdataview_getcontext(t_object *dv)
    cdef t_object *jdataview_addcolumn(t_object *dv, t_symbol *name, t_symbol *before, short unused)
    cdef t_object *jdataview_addcolumn_hidden(t_object *dv, t_symbol *name, t_symbol *before, short unused)
    cdef void *jcolumn_new()
    cdef void jcolumn_setdataview(t_object *c, t_object *dv)
    cdef void jdataview_colname_delete(t_object *dv, t_symbol *name)
    cdef void jdataview_deletecolumn(t_object *dv, t_object *col)
    cdef t_object *jdataview_getnamedcolumn(t_object *dv, t_symbol *name)
    cdef t_object *jdataview_getnthcolumn(t_object *dv, long index)
    cdef int jdataview_colname2index(t_object *dv, t_symbol *name)
    cdef void jdataview_colname_setvisible(t_object *dv, t_symbol *name, long way)
    cdef short jdataview_colname_getvisible(t_object *dv, t_symbol *name)
    cdef int jdataview_getnumcolumns(t_object *dv)
    cdef int jcolumn_getwidth(t_object *col)
    cdef void jcolumn_setwidth(t_object *col, long width)
    cdef int jcolumn_getmaxwidth(t_object *col)
    cdef void jcolumn_setmaxwidth(t_object *col, long width)
    cdef int jcolumn_getminwidth(t_object *col)
    cdef void jcolumn_setminwidth(t_object *col, long width)
    cdef long jcolumn_getid(t_object *col)
    cdef int jcolumn_getautosize(t_object *col)
    cdef void jcolumn_setdataview(t_object *col, t_object *dv)
    cdef t_symbol *jcolumn_getname(t_object *col)
    cdef void jcolumn_setname(t_object *col, t_symbol *name)
    cdef void jcolumn_setlabel(t_object *col, t_symbol *label)
    cdef t_symbol *jcolumn_getlabel(t_object *col)
    cdef void jcolumn_setinsertbefore(t_object *col, t_symbol *before)
    cdef t_symbol *jcolumn_getinsertbefore(t_object *col)
    cdef void jcolumn_setnumeric(t_object *col, long way)
    cdef int jcolumn_getnumeric(t_object *col)
    cdef void jcolumn_setcustomsort(t_object *col, t_symbol *msg)
    cdef t_symbol *jcolumn_getcustomsort(t_object *col)
    cdef void jcolumn_setoverridesort(t_object *col, char val)
    cdef char jcolumn_getoverridesort(t_object *col)
    cdef void jcolumn_setcustompaint(t_object *col, t_symbol *msg)
    cdef t_symbol *jcolumn_getcustompaint(t_object *col)
    cdef void jcolumn_setcustommenu(t_object *col, t_symbol *setmsg, t_symbol *resultmsg)
    cdef t_symbol *jcolumn_getcustommenu_setmsg(t_object *col)
    cdef t_symbol *jcolumn_getcustommenu_resultmsg(t_object *col)
    cdef void jcolumn_setsortable(t_object *col, long way)
    cdef int jcolumn_getsortable(t_object *col)
    cdef void jcolumn_setdraggable(t_object *col, long way)
    cdef int jcolumn_getdraggable(t_object *col)
    cdef void jcolumn_setinitiallysorted(t_object *col, long way)
    cdef int jcolumn_getinitiallysorted(t_object *col)
    cdef void jcolumn_sethideable(t_object *col, long way)
    cdef int jcolumn_gethideable(t_object *col)
    cdef void jcolumn_setvisible(t_object *col, long way)
    cdef int jcolumn_getvisible(t_object *col)
    cdef void jcolumn_setcasesensitive(t_object *col, long way)
    cdef int jcolumn_getcasesensitive(t_object *col)
    cdef void jcolumn_setreference(t_object *col, void *ref)
    cdef void *jcolumn_getreference(t_object *col)
    cdef void jcolumn_setcheckbox(t_object *col, t_symbol *msg)
    cdef void jcolumn_setvaluemsg(t_object *col, t_symbol *msg, t_symbol *beginmsg, t_symbol *endmsg)
    cdef t_symbol *jcolumn_getvaluemsg(t_object *col)
    cdef t_symbol *jcolumn_getbeginchangemsg(t_object *col)
    cdef t_symbol *jcolumn_getendchangemsg(t_object *col)
    cdef int jcolumn_getcomponent(t_object *col)
    cdef void jcolumn_setrowcomponentmsg(t_object *col, t_symbol *msg)
    cdef t_symbol *jcolumn_getrowcomponentmsg(t_object *col)
    cdef double jcolumn_getindentspacing(t_object *col)
    cdef void jcolumn_setindentspacing(t_object *col, double spacing)
    cdef void jcolumn_setcellcluemsg(t_object *col, t_symbol *msg)
    cdef t_symbol *jcolumn_getcellcluemsg(t_object *col)
    cdef t_symbol *jcolumn_getcelltextcolormsg(t_object *col)
    cdef void jcolumn_setcelltextcolormsg(t_object *col, t_symbol *msg)
    cdef t_symbol *jcolumn_getcelltextstylemsg(t_object *col)
    cdef void jcolumn_setcelltextstylemsg(t_object *col, t_symbol *msg)
    cdef void jcolumn_setcellentermsg(t_object *col, t_symbol *msg)
    cdef void jcolumn_setcellexitmsg(t_object *col, t_symbol *msg)
    cdef void jcolumn_setcellmovedmsg(t_object *col, t_symbol *msg)
    cdef void jcolumn_setcellclickmsg(t_object *col, t_symbol *msg)
    cdef void jcolumn_setshowinfobutton(t_object *col, long way)
    cdef long jcolumn_getshowinfobutton(t_object *col)
    cdef t_symbol *jcolumn_getcellentermsg(t_object *col)
    cdef t_symbol *jcolumn_getcellexitmsg(t_object *col)
    cdef t_symbol *jcolumn_getcellmovedmsg(t_object *col)
    cdef t_symbol *jcolumn_getcellclickmsg(t_object *col)
    cdef void jcolumn_update(t_object *col, t_symbol *msg)
    cdef t_object *jdataview_addcolumnfromdictionary(t_object *dv, t_object *d)
    ctypedef void *t_rowref
    cdef void jdataview_addrowtosection(t_object *dv, void *section, t_rowref rr)
    cdef void jdataview_addrow(t_object *dv, t_rowref rr)
    cdef void jdataview_addrowstosection(t_object *dv, void *section, long count, t_rowref *rrs)
    cdef void jdataview_addrows(t_object *dv, long count, t_rowref *rrs)
    cdef void jdataview_deleterowfromsection(t_object *dv, void *section, t_rowref rr)
    cdef void jdataview_deleterow(t_object *dv, t_rowref rr)
    cdef void jdataview_deleterowsfromsection(t_object *dv, void *section, long count, t_rowref *rrs)
    cdef void jdataview_deleterows(t_object *dv, long count, t_rowref *rrs)
    cdef void jdataview_deleteselectedrows(t_object *dv)
    cdef void jdataview_deleteselectedrowsforview(t_object *dv, t_object *patcherview)
    cdef void jdataview_clear(t_object *dv)
    cdef int jdataview_getnumrows(t_object *dv)
    cdef void jdataview_gettextinrows(t_object *dv, t_rowref *rows, char *cellsep, char **text)
    cdef int jdataview_selectedrowcountforview(t_object *dv, t_object *patcherview)
    cdef int jdataview_selectedrowcount(t_object *dv)
    cdef t_rowref *jdataview_getallrows(t_object *dv)
    cdef t_rowref *jdataview_section_getallrows(t_object *dv, void *section, long *count)
    cdef t_rowref *jdataview_getselectedrowsforview(t_object *dv, t_object *patcherview)
    cdef t_rowref *jdataview_getselectedrows(t_object *dv)
    cdef void jdataview_applytoselectedrows(t_object *dv, t_symbol *msg, long bycell)
    cdef void jdataview_applytorows(t_object *dv, t_symbol *msg, long bycell, t_rowref *srs)
    cdef void jdataview_enablerow(t_object *dv, t_rowref rr, long way)
    cdef void jdataview_selectall(t_object *dv)
    cdef void jdataview_selectallforview(t_object *dv, t_object *patcherview)
    cdef void jdataview_selectnone(t_object *dv)
    cdef void jdataview_selectnoneforview(t_object *dv, t_object *patcherview)
    cdef t_object *jdataview_id2column(t_object *dv, int id)
    cdef t_symbol *jdataview_id2colname(t_object *dv, int id)
    cdef int jdataview_colname2id(t_object *dv, t_symbol *name)
    cdef int jdataview_column2id(t_object *dv, t_object *col)
    cdef int jdataview_row2id(t_object *dv, t_rowref rr, void **section)
    cdef t_rowref jdataview_id2row(t_object *dv, void *section, int id)
    cdef void jdataview_showrow(t_object *dv, t_rowref rr)
    cdef void jdataview_celldesc(t_jdataview *x, t_symbol *colname, t_rowref rr, t_celldesc *desc)
    cdef void jdataview_selectcellinview(t_object *dv, t_object *pv, t_symbol *colname, t_rowref rr)
    cdef void jdataview_selectcell(t_object *dv, t_symbol *colname, t_rowref rr)
    cdef int jdataview_getcelltextlength(t_object *dv, t_symbol *colname, t_rowref rr, long *length)
    cdef int jdataview_getcelltext(t_object *dv, t_symbol *colname, t_rowref rr, char *text, long maxlen)
    cdef int jdataview_getcellunits(t_object *dv, t_symbol *colname, t_rowref rr, char *text, long maxlen)
    cdef int jdataview_setcellunits(t_object *dv, t_symbol *colname, t_rowref rr, t_symbol *val)
    cdef int jdataview_getcellunitsyms(t_object *dv, t_symbol *colname, t_rowref rr, long *argc, t_atom **argv)
    cdef int jdataview_getcelldescription(t_object *dv, t_symbol *colname, t_rowref rr, char *text)
    cdef int jdataview_getcellvalue(t_object *dv, t_symbol *colname, t_rowref rr, long *argc, t_atom *argv)
    cdef void jdataview_getcelltextcolor(t_object *dv, t_symbol *colname, t_rowref rr, t_jrgba *color)
    cdef void jdataview_getcelltextstyle(t_object *dv, t_symbol *colname, t_rowref rr, long *style, long *align)
    cdef int jdataview_getcellmenu(t_object *dv, t_symbol *colname, t_rowref rr, long *argc, t_atom *argv, char **enabled, long *currentitemindex)
    cdef int jdataview_getcelltooltip(t_object *dv, t_symbol *colname, t_rowref rr, t_rect *cellrect, char *text, long maxlen)
    cdef void jdataview_setcellvalue(t_object *dv, t_symbol *colname, t_rowref rr, long argc, t_atom *argv)
    cdef void jdataview_editcell(t_object *dv, t_symbol *colname, t_rowref rr)
    cdef int jdataview_iscelltextselected(t_object *dv, char justfocused)
    cdef void jdataview_cellclear(t_object *dv)
    cdef void jdataview_cellcut(t_object *dv)
    cdef void jdataview_cellcopy(t_object *dv)
    cdef void jdataview_cellpaste(t_object *dv)
    cdef int jdataview_getcellcomponent(t_object *dv, t_symbol *colname, t_rowref rr, long *options, t_symbol **label)
    cdef int jdataview_getcellfiletypes(t_object *dv, t_symbol *colname, t_rowref rr, long *count, t_fourcc **types, char *alloc)
    cdef t_symbol *jdataview_getcellfilterval(t_object *dv, t_symbol *colname, t_rowref rr)
    cdef void jdataview_redrawcell(t_object *dv, t_symbol *colname, t_rowref rr)
    cdef void jdataview_begincellchange(t_object *dv, t_symbol *colname, t_rowref rr)
    cdef void jdataview_endcellchange(t_object *dv, t_symbol *colname, t_rowref rr)
    cdef void jdataview_selected(t_object *dv, t_symbol *colname, t_rowref rr)
    cdef void jdataview_selectedrow(t_object *dv, t_rowref rr)
    cdef void jdataview_doubleclick(t_object *dv, t_symbol *colname, t_rowref rr)
    cdef void jdataview_contextualclick(t_object *dv, t_symbol *colname, t_rowref rr)
    cdef void jdataview_getcellicon(t_object *dv, t_symbol *colname, t_rowref rr, t_jsurface **surf)
    cdef void jdataview_getrowcolor(t_object *dv, t_rowref rr, long isoddrow, t_jrgba *c)
    cdef int jdataview_colorbycell(t_object *dv)
    cdef void jdataview_getcellcolor(t_object *dv, t_symbol *colname, t_rowref rr, long isoddrow, t_jrgba *c)
    cdef int jdataview_getcustomselectcolor(t_object *dv)
    cdef void jdataview_setcustomselectcolor(t_object *dv, int way)
    cdef double jdataview_getcellindent(t_object *dv, t_symbol *colname, t_rowref rr)
    cdef void jdataview_cellenter(t_object *dv, t_symbol *colname, t_rowref rr, int px, int py)
    cdef void jdataview_cellexit(t_object *dv, t_symbol *colname, t_rowref rr, int px, int py)
    cdef void jdataview_cellmove(t_object *dv, t_symbol *colname, t_rowref rr, int px, int py)
    cdef t_atom_long jdataview_getcelleditable(t_object *dv, t_symbol *colname, t_rowref rr)
    cdef long jdataview_getbordercolor(t_object *dv, t_jrgba *c)
    cdef void jdataview_setbordercolor(t_object *dv, t_jrgba *c)
    cdef long jdataview_getusesystemfont(t_object *dv)
    cdef void jdataview_setusesystemfont(t_object *dv, long way)
    cdef void jdataview_enablecell(t_object *dv, t_symbol *colname, t_rowref rr, long way)
    cdef void jdataview_forcecellvisible(t_object *x, t_symbol *colname, t_rowref rr)
    cdef void jdataview_scrolltosection(t_object *dv, void *section)
    cdef void jdataview_scrolltotop(t_object *dv)
    cdef int jdataview_getpaintcellseparator(t_object *dv)
    cdef void jdataview_setpaintcellseparator(t_object *dv, int way)
    cdef void jdataview_getcellclue(t_object *dv, t_symbol *colname, t_rowref rr, char **str)
    cdef void jdataview_clientpaintcell(t_object *dv, t_symbol *msg, t_object *pv, t_symbol *colname, t_rowref rr, int width, int height, int rowIsSelected, int rowNumber)
    cdef void jdataview_getcolumnmenuforcell(t_object *dv, t_symbol *colname, t_rowref rr, long *argc, t_atom **argv, char **enabled)
    cdef void jdataview_cellcolumnmenuresult(t_object *dv, t_symbol *colname, t_rowref rr, long result)
    cdef void jdataview_sortcolumn(t_object *x, t_symbol *colname, int fwd)
    cdef void jdataview_sort(t_object *x, t_symbol *colname, int fwd)
    cdef void jdataview_resort(t_object *x)
    cdef long jdataview_getsortcolumn(t_object *x)
    cdef void jdataview_selectcell(t_object *dv, t_symbol *colname, t_rowref rr)
    cdef void jdataview_selectrow(t_jdataview *x, t_rowref rr)
    cdef void jdataview_selectcolumn(t_jdataview *x, t_symbol *colname)
    cdef void jdataview_selectallrows(t_jdataview *x)
    cdef long jdataview_iscellselected(t_jdataview *x, t_symbol *colname, t_rowref rr)
    cdef long jdataview_isrowselected(t_jdataview *x, t_rowref rr)
    cdef long jdataview_iscolumnselected(t_jdataview *x, t_symbol *colname)
    cdef void jdataview_savecolumnwidths(t_object *dv, t_dictionary **d)
    cdef void jdataview_restorecolumnwidths(t_object *dv, t_dictionary *d)
    cdef void jdataview_updatefontpanelforclient(t_object *dv, long show)
    cdef void jdataview_redrawrow(t_object *dv, t_rowref rr)
    cdef void jdataview_getcellrect(t_jdataview *x, t_symbol *colname, t_rowref rr, t_rect *r)
    cdef t_max_err jdataview_jgraphics_from_paintcontext(t_jdataview *x, void *ctx, t_jgraphics **g)
    cdef long jdataview_containersize(t_object *dv, t_object *box, double cw, double ch, double *width, double *height)
    cdef void jdataview_forcecellvisible(t_object *x, t_symbol *colname, t_rowref rr)
    cdef t_object *jdataview_getrowobject(t_object *dv, t_rowref rr)
    cdef void jdataview_entermodalstate(t_object *dv, long way)
    cdef void *jdataview_getdragreceiver(t_object *dv)


# -------------------------------------------------------------------------------------------------
# inlined functions

# for eg
# cdef inline int my_min(int a, int b):
#     return b if b < a else a

