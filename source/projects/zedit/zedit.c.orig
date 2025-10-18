/**
 * @defgroup   zedit external
 *
 * @brief      This external borrows from the implementation max-sdk simplethread
 *             example external and embeds an http webserver using the mongoose
 *             webserver library. ref: https://github.com/cesanta/mongoose
 *
 * @author     sa
 * @date       2023
 */

#include "ext.h"
#include "ext_obex.h"
#include "ext_systhread.h"

#define PY_IMPLEMENTATION // <-- activate the implementation
#include "py.h"           // <-- include this

#include "mongoose.h"

#define PY_MAX_ELEMS 1024

// static global constants
#if defined RELEASE
static const char* s_listening_address = "http://localhost:8000";
static const char* s_subpath = "/Contents/Resources/public";
#else
static const char* s_listening_address = "http://0.0.0.0:8000";
static const char* s_subpath = "/source/projects/zedit/webroot";
#endif
static const   int s_debug_level = 2;
// static const char* s_enable_hexdump = "no";
static const char* s_ssi_pattern = "#.shtml";

// mutable constants
static const char* s_root_dir = NULL;


typedef struct _zedit {
    t_object x_ob;             // standard max object
    t_systhread x_systhread;   // thread reference
    t_systhread_mutex x_mutex; // mutual exclusion lock for threadsafety
    int x_systhread_cancel;    // thread cancel flag
    void* x_qelem;             // for message passing between threads
    void* x_outlet;            // the only outlet
    long x_foo;                // simple data to pass between threads
    int x_sleeptime;           // how many milliseconds to sleep
    int x_is_running;          // status of zediter
    t_string* x_root_dir;      // root path to statically serve from
    t_py* py;                  // python interpreter type instance
} t_zedit;


// *-structors
void* zedit_new(void);
void zedit_free(t_zedit* x);

// thread
void zedit_start(t_zedit* x);
void zedit_foo(t_zedit* x, long foo);
void zedit_sleeptime(t_zedit* x, long sleeptime);
void zedit_stop(t_zedit* x);
void zedit_cancel(t_zedit* x);
void* zedit_threadproc(t_zedit* x);
void zedit_qfn(t_zedit* x);

// doc
void zedit_assist(t_zedit* x, void* b, long m, long a, char* s);

// message
void zedit_bang(t_zedit* x);

// core py methods
t_max_err zedit_import(t_zedit* x, t_symbol* s);
t_max_err zedit_eval(t_zedit* x, t_symbol* s);
t_max_err zedit_exec(t_zedit* x, t_symbol* s);
t_max_err zedit_execfile(t_zedit* x, t_symbol* s);

// extra py methods
t_max_err zedit_call(t_zedit* x, t_symbol* s, long argc, t_atom* argv);
t_max_err zedit_assign(t_zedit* x, t_symbol* s, long argc, t_atom* argv);
t_max_err zedit_code(t_zedit* x, t_symbol* s, long argc, t_atom* argv);
t_max_err zedit_anything(t_zedit* x, t_symbol* s, long argc, t_atom* argv);
t_max_err zedit_pipe(t_zedit* x, t_symbol* s, long argc, t_atom* argv);

// code evaluation methods
t_max_err zedit_exec_file_input(t_zedit* x, const char* code);
t_max_err zedit_exec_single_input(t_zedit* x, const char* code);


// web
// void do_build_objects(t_zedit* x, t_symbol *s, short argc, t_atom *argv);
void handle_event_http_message(struct mg_connection *c, int ev, void *ev_data, void *fn_data);
static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data);
t_string* get_path_to_webroot(t_class* klass);



// -------------------------------------------------------------------------------------




// void do_build_objects(t_zedit* x, t_symbol *s, short argc, t_atom *argv)
// {

//     t_object *patcher;

//     if (object_obex_lookup(x, gensym("#P"), &patcher) == MAX_ERR_NONE) {
//         if (1) {
//             t_object *toggle, *metro;
//             newobject_sprintf(patcher, 
//                 "@maxclass toggle @patching_position %.2f %.2f", 50.0, 300.0); 
//             newobject_sprintf(patcher, 
//                 "@maxclass newobj @text \"metro 400\" @patching_position %.2f %.2f", 200.0, 400.0);        
//         } else {
//             newobject_fromboxtext(patcher, "cycle~ 440");
//         } 
//     }
// }

// http message handler
void handle_event_http_message(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
    post("MG_EV_HTTP_MSG");
    struct mg_http_message *hm = (struct mg_http_message*)ev_data;
    struct mg_http_message tmp = {0};
    struct mg_str unknown = mg_str_n("?", 1);
    struct mg_str *cl;
    // On /api/hello requests, send dynamic JSON response
    if (mg_http_match_uri(hm, "/api/hello")) {
        object_post((t_object*)c->fn_data, "/api/hello");
        mg_http_reply(c, 200, "", "{%Q:%d}\n", "status", 1);

    } else if (mg_http_match_uri(hm, "/api/code/save")) {
        object_post((t_object*)c->fn_data, "/api/code/save");


        // Expecting JSON array in the HTTP body, e.g. [ 1, "..." ]
        long file_id;
        char *code;

        if ((file_id = mg_json_get_long(hm->body, "$.file_id", 100) &&
            (code = mg_json_get_str(hm->body, "$.content")))) {
            // Success! create JSON response
            mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                          "{%Q:%g}\n",
                          "result", "OK SAVED");

            zedit_exec_file_input((t_zedit*)c->fn_data, code);
            post("code: %s", code);
            free(code);
        } else {
            mg_http_reply(c, 500, NULL, "Parameters missing\n");
        }

    } else if (mg_http_match_uri(hm, "/api/code/run")) {
        object_post((t_object*)c->fn_data, "/api/code/run");
    } else if (mg_http_match_uri(hm, "/api/repl/send")) {
        object_post((t_object*)c->fn_data, "/api/repl/run");
        char *code;

        if ((code = mg_json_get_str(hm->body, "$.content"))) {
            // Success! create JSON response
            mg_http_reply(c, 200, "Content-Type: application/json\r\n",
                          "{%Q:%Q}\n",
                          "result", "OK");
            zedit_exec_single_input((t_zedit*)c->fn_data, code);
            post("code: %s", code);
            free(code);
        } else {
            mg_http_reply(c, 500, NULL, "Parameters missing\n");
        }

    } else if (mg_http_match_uri(hm, "/api/items/*")) {
        mg_http_reply(c, 200, "", "{\"result\": \"%.*s\"}\n", (int) hm->uri.len,
                hm->uri.ptr);
    } else {
        // mg_http_reply(c, 500, NULL, "\n");
        // OR
        // static file server configuration
        struct mg_http_serve_opts opts = {
            .root_dir = s_root_dir,
            .ssi_pattern = s_ssi_pattern
        };
        mg_http_serve_dir(c, hm, &opts);
    }
    mg_http_parse((char *) c->send.buf, c->send.len, &tmp);
    cl = mg_http_get_header(&tmp, "Content-Length");
    if (cl == NULL) cl = &unknown;
    post("%.*s %.*s %.*s %.*s", (int) hm->method.len, hm->method.ptr,
             (int) hm->uri.len, hm->uri.ptr, (int) tmp.uri.len, tmp.uri.ptr,
             (int) cl->len, cl->ptr);
}


// zedit main function
static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
    switch(ev) {
    
    case MG_EV_ERROR: { // Error -> char *error_message
        char *error_message = strdup((char*)ev_data);
        error("MG_EV_ERROR: %s", error_message);
    }   break;

    case MG_EV_OPEN:    // Connection created -> NULL
        post("MG_EV_OPEN");
        // c->is_hexdumping = 1;
        break;

    case MG_EV_POLL: {  // mg_mgr_poll iteration -> uint64_t *milliseconds
        // uint64_t *milliseconds = (uint64_t*)ev_data;
        // post("MG_EV_POLL: %d", milliseconds);
    }   break;

    case MG_EV_RESOLVE: // host name is resolved
        post("MG_EV_RESOLVE");
        break;

    case MG_EV_CONNECT: // connection established
        post("MG_EV_CONNECT");
        break;

    case MG_EV_ACCEPT:  // connection accepted
        post("MG_EV_ACCEPT");
        break;

    case MG_EV_TLS_HS:  // TLS handshake succeeded
        post("MG_EV_TLS_HS");
        break;

    case MG_EV_READ:    // data received from socket -> long *bytes_read
        post("MG_EV_READ");
        break;

    case MG_EV_WRITE:   // data written to socket -> long *bytes_written
        post("MG_EV_WRITE");
        break;

    case MG_EV_CLOSE:   // connection closed
        post("MG_EV_CLOSE");
        break;

    case MG_EV_HTTP_MSG: // http request/respose -> struct mg_http_message *
        handle_event_http_message(c, ev, ev_data, fn_data);
        break;

    case MG_EV_HTTP_CHUNK: // HTTP chunk (partial msg) -> struct mg_http_message *
        post("MG_EV_HTTP_CHUNK");
        break;

    case MG_EV_WS_OPEN: // Websocket handshake done -> struct mg_http_message *
        post("MG_EV_WS_OPEN");
        break;

    case MG_EV_WS_MSG: // Websocket msg, text or bin ->sstruct mg_ws_message *
        post("MG_EV_WS_MSG");
        break;

    case MG_EV_WS_CTL: // Websocket control msg -> struct mg_ws_message *
        post("MG_EV_WS_MSG");
        break;

    case MG_EV_MQTT_CMD: // /MQTT low-level command -> struct mg_mqtt_message *
        post("MG_EV_MQTT_CMD");
        break;

    case MG_EV_MQTT_MSG: // MQTT PUBLISH received -> struct mg_mqtt_message *
        post("MG_EV_MQTT_MSG");
        break;

    case MG_EV_MQTT_OPEN: // MQTT CONNACK received -> int *connack_status_code
        post("MG_EV_MQTT_OPEN");
        break;

    case MG_EV_SNTP_TIME: // SNTP time received -> uint64_t *milliseconds
        post("MG_EV_SNTP_TIME");
        break;

    case MG_EV_USER: // Starting ID for user events
        post("MG_EV_USER");
        break;

    default:
        break;

    }
    (void) fn_data;
}


// // zediter main function
// static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
// {
//     if (ev == MG_EV_OPEN) {
//         // c->is_hexdumping = 1;
//     }
//     else if (ev == MG_EV_HTTP_MSG) {
//         struct mg_http_message *hm = (struct mg_http_message*)ev_data;
//         struct mg_http_message tmp = {0};
//         struct mg_str unknown = mg_str_n("?", 1);
//         struct mg_str *cl;
//         // On /api/hello requests, send dynamic JSON response
//         if (mg_http_match_uri(hm, "/api/hello")) {
//             mg_http_reply(c, 200, "", "{%Q:%d}\n", "status", 1);
//         } else {
//             // static file server configuration
//             struct mg_http_serve_opts opts = {
//                 .root_dir = s_root_dir,
//                 .ssi_pattern = s_ssi_pattern
//             };
//             mg_http_serve_dir(c, hm, &opts);
//         }
//         mg_http_parse((char *) c->send.buf, c->send.len, &tmp);
//         cl = mg_http_get_header(&tmp, "Content-Length");
//         if (cl == NULL) cl = &unknown;
//         post("%.*s %.*s %.*s %.*s", (int) hm->method.len, hm->method.ptr,
//                  (int) hm->uri.len, hm->uri.ptr, (int) tmp.uri.len, tmp.uri.ptr,
//                  (int) cl->len, cl->ptr);
//   }
//   (void) fn_data;
// }


// t_string* get_path_to_webroot(t_class* klass)
// {
//     char external_path[MAX_PATH_CHARS];
//     char external_name[MAX_PATH_CHARS];
//     char conform_path[MAX_PATH_CHARS];

//     short path_id = class_getpath(klass);
//     snprintf_zero(external_name, PY_MAX_ELEMS, "%s.mxo", klass->c_sym->s_name);
//     path_toabsolutesystempath(path_id, external_name, external_path);
//     path_nameconform(external_path, conform_path, PATH_STYLE_NATIVE,
//                      PATH_TYPE_TILDE);
// #if defined RELEASE
//     t_string* webroot_path = string_new(external_path);
// #else
//     t_string* webroot_path = string_new(dirname(dirname(external_path)));
// #endif
//     string_append(webroot_path, s_subpath);
//     return webroot_path;
// }


t_string* get_path_to_webroot(t_class* klass)
{
    char external_path[MAX_PATH_CHARS];
    char external_name[MAX_PATH_CHARS];
    char conform_path[MAX_PATH_CHARS];

    char _dummy[MAX_PATH_CHARS];
    char externals_folder[MAX_PATH_CHARS];
    char package_folder[MAX_PATH_CHARS];

    short path_id = class_getpath(klass);
    t_string* webroot_path;

#ifdef __APPLE__
    const char* ext_filename = "%s.mxo";
#else
    const char* ext_filename = "%s.mxe64";
#endif

    snprintf_zero(external_name, MAX_FILENAME_CHARS, ext_filename, klass->c_sym->s_name);
    path_toabsolutesystempath(path_id, external_name, external_path);
    path_nameconform(external_path, conform_path, PATH_STYLE_NATIVE,
                     PATH_TYPE_TILDE);
#if defined RELEASE
    webroot_path = string_new(external_path);
#else
    path_splitnames(external_path, externals_folder, _dummy); // ignore filename
    path_splitnames(externals_folder, package_folder, _dummy); // ignore filename
    webroot_path = string_new((char*)package_folder);
#endif
    string_append(webroot_path, s_subpath);
    return webroot_path;
}


t_class* zedit_class;

void ext_main(void* r)
{
    t_class* c;

    c = class_new("zedit", (method)zedit_new, (method)zedit_free,
                  sizeof(t_zedit), 0L, 0);

    class_addmethod(c, (method)zedit_bang,      "bang",                 0);
    class_addmethod(c, (method)zedit_start,     "start",                0);
    class_addmethod(c, (method)zedit_foo,       "foo",       A_DEFLONG, 0);
    class_addmethod(c, (method)zedit_sleeptime, "sleeptime", A_DEFLONG, 0);
    class_addmethod(c, (method)zedit_cancel,    "cancel",               0);
    class_addmethod(c, (method)zedit_assist,    "assist",   A_CANT,     0);

    class_addmethod(c, (method)zedit_import,    "import",   A_SYM,      0);
    class_addmethod(c, (method)zedit_eval,      "eval",     A_SYM,      0);
    class_addmethod(c, (method)zedit_exec,      "exec",     A_SYM,      0);
    class_addmethod(c, (method)zedit_execfile,  "execfile", A_SYM,      0);

    class_addmethod(c, (method)zedit_call,      "call",     A_GIMME,    0);
    class_addmethod(c, (method)zedit_code,      "code",     A_GIMME,    0);
    class_addmethod(c, (method)zedit_pipe,      "pipe",     A_GIMME,    0);
    class_addmethod(c, (method)zedit_anything,  "anything", A_GIMME,    0);

    class_register(CLASS_BOX, c);
    zedit_class = c;
}



void zedit_start(t_zedit* x)
{
    if (x->x_is_running)
        zedit_stop(x); // kill thread if, any

    // create new thread + begin execution
    if (x->x_systhread == NULL && !x->x_is_running) {
        // post("starting a zediter on port: %d", x->x_port);

        post("Mongoose version : v%s", MG_VERSION);
        post("Listening on     : %s", s_listening_address);
        post("Web root         : [%s]", s_root_dir);
        systhread_create((method)zedit_threadproc, x, 0, 0, 0,
                         &x->x_systhread);
        x->x_is_running = true;
    }
}

void zedit_foo(t_zedit* x, long foo)
{
    systhread_mutex_lock(x->x_mutex);
    x->x_foo = foo; // override our current value
    systhread_mutex_unlock(x->x_mutex);
}

void zedit_sleeptime(t_zedit* x, long sleeptime)
{
    if (sleeptime < 10)
        sleeptime = 10;
    x->x_sleeptime = sleeptime; // no need to lock since we are readonly in
                                // worker thread
}


void zedit_stop(t_zedit* x)
{
    unsigned int ret;

    if (x->x_systhread) {
        post("stopping webserver thread");
        x->x_systhread_cancel = true;         // tell the thread to stop
        systhread_join(x->x_systhread, &ret); // wait for the thread to stop
        x->x_systhread = NULL;
        x->x_is_running = false;
    }
}


void zedit_cancel(t_zedit* x)
{
    zedit_stop(x); // kill thread if, any
    post("Exiting on message cancel");
    outlet_anything(x->x_outlet, gensym("cancelled"), 0, NULL);
}

void* zedit_threadproc(t_zedit* x)
{
    // char listening_address[100];
    // int max_len = sizeof listening_address;
    // snprintf_zero(listening_address, max_len, s_listening_address, x->x_port);

    // loop until told to stop
    while (1) {

        struct mg_mgr mgr;
        mg_log_set(s_debug_level);
        mg_mgr_init(&mgr); // Init manager
        // mg_http_listen(&mgr, s_listening_address, fn, &mgr); // Setup listener
        mg_http_listen(&mgr, s_listening_address, fn, x); // Setup listener

        for (;;) {
            mg_mgr_poll(&mgr, 1000); // Event loop
            if (x->x_systhread_cancel)
                break;
        }
        mg_mgr_free(&mgr); // Cleanup
        break;

        // systhread_mutex_lock(x->x_mutex);
        // x->x_foo++; // fiddle with shared data
        // systhread_mutex_unlock(x->x_mutex);

        // qelem_set(x->x_qelem); // notify main thread using qelem mechanism

        // systhread_sleep(x->x_sleeptime); // sleep a bit
    }

    // reset cancel flag for next time, in case the thread is created again
    x->x_systhread_cancel = false;

    systhread_exit(0); // this can return a value to systhread_join();
    return NULL;
}

// triggered by the helper thread
void zedit_qfn(t_zedit* x)
{
    int myfoo;

    systhread_mutex_lock(x->x_mutex);
    myfoo = x->x_foo; // access shared data
    systhread_mutex_unlock(x->x_mutex);

    // *never* wrap outlet calls with systhread_mutex_lock()
    outlet_int(x->x_outlet, myfoo);
}

void zedit_assist(t_zedit* x, void* b, long m, long a, char* s)
{
    if (m == 1)
        sprintf(s, "start starts a new thread");
    else if (m == 2)
        sprintf(s, "report when done/cancelled");
}

void zedit_free(t_zedit* x)
{
    // stop our thread if it is still running
    zedit_stop(x);

    // free our qelem
    if (x->x_qelem)
        qelem_free(x->x_qelem);

    // free out mutex
    if (x->x_mutex)
        systhread_mutex_free(x->x_mutex);

    // cleanup python
    py_free(x->py);
}


void* zedit_new(void)
{
    t_zedit* x;

    x = (t_zedit*)object_alloc(zedit_class);
    x->x_outlet = outlet_new(x, NULL);
    x->x_qelem = qelem_new(x, (method)zedit_qfn);
    x->x_systhread = NULL;
    systhread_mutex_new(&x->x_mutex, 0);
    x->x_foo = 0;
    x->x_sleeptime = 1000;
    // x->x_port = 8000;
    x->x_is_running = false;
    x->x_root_dir = get_path_to_webroot(zedit_class);

    x->py = py_init(zedit_class); // This is all that is need to init the `py` obj

    // set global
    s_root_dir = string_getptr(x->x_root_dir);
    post("webroot: %s", s_root_dir);

    return (x);
}

void zedit_bang(t_zedit* x) {
    outlet_bang(x->x_outlet); 
}


t_max_err zedit_import(t_zedit* x, t_symbol* s)
{
    return py_import(x->py, s); // returns t_max_err
}

t_max_err zedit_eval(t_zedit* x, t_symbol* s)
{
    return py_eval(x->py, s, x->x_outlet);
}


t_max_err zedit_exec(t_zedit* x, t_symbol* s)
{
    return py_exec(x->py, s);
}


t_max_err zedit_exec_file_input(t_zedit* x, const char* code)
{
    return py_exec_file_input(x->py, code);
}


t_max_err zedit_exec_single_input(t_zedit* x, const char* code)
{
    return py_exec_single_input(x->py, code);
}


t_max_err zedit_execfile(t_zedit* x, t_symbol* s)
{
    return py_execfile(x->py, s);
}


t_max_err zedit_call(t_zedit* x, t_symbol* s, long argc, t_atom* argv)
{
    return py_call(x->py, s, argc, argv, x->x_outlet);
}


t_max_err zedit_assign(t_zedit* x, t_symbol* s, long argc, t_atom* argv)
{
    return py_assign(x->py, s, argc, argv);
}


t_max_err zedit_code(t_zedit* x, t_symbol* s, long argc, t_atom* argv)
{
    return py_code(x->py, s, argc, argv, x->x_outlet);
}


t_max_err zedit_anything(t_zedit* x, t_symbol* s, long argc, t_atom* argv)
{
    return py_anything(x->py, s, argc, argv, x->x_outlet);
}


t_max_err zedit_pipe(t_zedit* x, t_symbol* s, long argc, t_atom* argv)
{
    return py_pipe(x->py, s, argc, argv, x->x_outlet);
}
