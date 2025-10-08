/**
 * @file test-server.c
 * @brief Standalone test server for zedit web interface
 *
 * Tests web app <-> server communication without Max/MSP dependencies.
 * Uses Python C API directly for code execution with output capture.
 */

#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "mongoose.h"

// Global flag for graceful shutdown
static volatile int s_shutdown_requested = 0;

// Server configuration
static const char* s_listening_address = "http://localhost:8000";
static const char* s_root_dir = "./web/public";

// Authentication token
static char s_auth_token[64] = {0};

// Rate limiting
#define MAX_REQUESTS_PER_MINUTE 60
typedef struct {
    char ip[64];
    int count;
    time_t window_start;
} rate_limit_entry_t;
static rate_limit_entry_t rate_limits[10] = {0};

// Python globals dictionary
static PyObject* py_globals = NULL;

// Generate random auth token
void generate_auth_token(void) {
    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    for (int i = 0; i < 32; i++) {
        int index = rand() % (sizeof(charset) - 1);
        s_auth_token[i] = charset[index];
    }
    s_auth_token[32] = '\0';
}

// Validate auth token
int validate_auth_token(const char* token) {
    if (token == NULL || s_auth_token[0] == '\0') return 0;
    return (strcmp(token, s_auth_token) == 0);
}

// Check rate limit
int check_rate_limit(const char* ip) {
    if (ip == NULL) return 0;
    time_t now = time(NULL);
    int oldest_idx = 0;
    time_t oldest_time = rate_limits[0].window_start;

    for (int i = 0; i < 10; i++) {
        if (strcmp(rate_limits[i].ip, ip) == 0) {
            if (now - rate_limits[i].window_start >= 60) {
                rate_limits[i].count = 1;
                rate_limits[i].window_start = now;
                return 1;
            } else if (rate_limits[i].count >= MAX_REQUESTS_PER_MINUTE) {
                return 0;
            } else {
                rate_limits[i].count++;
                return 1;
            }
        }
        if (rate_limits[i].window_start < oldest_time) {
            oldest_time = rate_limits[i].window_start;
            oldest_idx = i;
        }
    }

    strncpy(rate_limits[oldest_idx].ip, ip, sizeof(rate_limits[oldest_idx].ip) - 1);
    rate_limits[oldest_idx].count = 1;
    rate_limits[oldest_idx].window_start = now;
    return 1;
}

/**
 * Execute Python code and capture stdout/stderr
 * Returns captured output (caller must free)
 */
char* execute_python_with_output(const char* code, int is_repl_mode) {
    PyGILState_STATE gstate = PyGILState_Ensure();

    PyObject *sys_module = NULL;
    PyObject *io_module = NULL;
    PyObject *string_io = NULL;
    PyObject *old_stdout = NULL;
    PyObject *old_stderr = NULL;
    PyObject *result = NULL;
    char *output_str = NULL;

    printf("[DEBUG] Executing code (mode=%s):\n%s\n", is_repl_mode ? "REPL" : "FILE", code);

    // Import sys and io modules
    sys_module = PyImport_ImportModule("sys");
    if (!sys_module) {
        printf("[ERROR] Failed to import sys module\n");
        PyErr_Print();
        PyGILState_Release(gstate);
        return strdup("ERROR: Failed to import sys");
    }

    io_module = PyImport_ImportModule("io");
    if (!io_module) {
        printf("[ERROR] Failed to import io module\n");
        PyErr_Print();
        Py_DECREF(sys_module);
        PyGILState_Release(gstate);
        return strdup("ERROR: Failed to import io");
    }

    // Create StringIO
    PyObject* string_io_class = PyObject_GetAttrString(io_module, "StringIO");
    string_io = PyObject_CallObject(string_io_class, NULL);
    Py_DECREF(string_io_class);

    if (!string_io) {
        printf("[ERROR] Failed to create StringIO\n");
        PyErr_Print();
        Py_DECREF(sys_module);
        Py_DECREF(io_module);
        PyGILState_Release(gstate);
        return strdup("ERROR: Failed to create StringIO");
    }

    // Save old stdout/stderr
    old_stdout = PyObject_GetAttrString(sys_module, "stdout");
    old_stderr = PyObject_GetAttrString(sys_module, "stderr");

    // Redirect stdout/stderr to StringIO
    PyObject_SetAttrString(sys_module, "stdout", string_io);
    PyObject_SetAttrString(sys_module, "stderr", string_io);

    // Execute code
    // Py_single_input: for REPL - shows expression results, interactive interpreter loop
    // Py_file_input: for editor - doesn't show expression results, runs as script
    int exec_mode = is_repl_mode ? Py_single_input : Py_file_input;
    result = PyRun_String(code, exec_mode, py_globals, py_globals);

    // Restore stdout/stderr immediately
    if (old_stdout) PyObject_SetAttrString(sys_module, "stdout", old_stdout);
    if (old_stderr) PyObject_SetAttrString(sys_module, "stderr", old_stderr);

    // Check for execution errors
    if (!result) {
        printf("[ERROR] Code execution failed\n");
        // Capture error from StringIO
        PyObject* getvalue = PyObject_GetAttrString(string_io, "getvalue");
        if (getvalue) {
            PyObject* output_obj = PyObject_CallObject(getvalue, NULL);
            if (output_obj && PyUnicode_Check(output_obj)) {
                const char* temp = PyUnicode_AsUTF8(output_obj);
                if (temp && strlen(temp) > 0) {
                    output_str = strdup(temp);
                }
                Py_DECREF(output_obj);
            }
            Py_DECREF(getvalue);
        }

        // Also print to stderr for debugging
        PyErr_Print();
    } else {
        // Get output from StringIO
        PyObject* getvalue = PyObject_GetAttrString(string_io, "getvalue");
        if (getvalue) {
            PyObject* output_obj = PyObject_CallObject(getvalue, NULL);
            if (output_obj && PyUnicode_Check(output_obj)) {
                const char* temp = PyUnicode_AsUTF8(output_obj);
                if (temp) {
                    output_str = strdup(temp);
                    printf("[DEBUG] Captured output (%zu bytes): %s\n",
                           strlen(output_str), output_str);
                }
                Py_DECREF(output_obj);
            }
            Py_DECREF(getvalue);
        }
    }

    // Cleanup
    Py_XDECREF(result);
    Py_XDECREF(string_io);
    Py_XDECREF(old_stdout);
    Py_XDECREF(old_stderr);
    Py_DECREF(sys_module);
    Py_DECREF(io_module);

    PyGILState_Release(gstate);

    return output_str ? output_str : strdup("");
}

// HTTP event handler
void handle_http_message(struct mg_connection *c, struct mg_http_message *hm) {
    const char* sec_headers =
        "X-Frame-Options: DENY\r\n"
        "X-Content-Type-Options: nosniff\r\n"
        "Content-Security-Policy: default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'\r\n";

    // Rate limiting
    if (mg_http_match_uri(hm, "/api/*")) {
        char client_ip[64] = {0};
        mg_snprintf(client_ip, sizeof(client_ip), "%M", mg_print_ip, &c->rem);
        if (!check_rate_limit(client_ip)) {
            mg_http_reply(c, 429, sec_headers, "{%Q:%Q}\n", "error", "Rate limit exceeded");
            return;
        }
    }

    // Authentication check
    if (mg_http_match_uri(hm, "/api/*") && !mg_http_match_uri(hm, "/api/auth")) {
        struct mg_str *auth_header = mg_http_get_header(hm, "X-Auth-Token");
        char auth_token_buf[64] = {0};
        if (auth_header && auth_header->len < sizeof(auth_token_buf)) {
            snprintf(auth_token_buf, sizeof(auth_token_buf), "%.*s", (int)auth_header->len, auth_header->ptr);
        }
        if (!validate_auth_token(auth_token_buf)) {
            char headers[512];
            snprintf(headers, sizeof(headers), "%sContent-Type: application/json\r\n", sec_headers);
            mg_http_reply(c, 401, headers, "{%Q:%Q}\n", "error", "Unauthorized");
            return;
        }
    }

    // /api/auth endpoint
    if (mg_http_match_uri(hm, "/api/auth")) {
        char headers[512];
        snprintf(headers, sizeof(headers), "%sContent-Type: application/json\r\n", sec_headers);
        mg_http_reply(c, 200, headers, "{%Q:%Q}\n", "token", s_auth_token);
        printf("[AUTH] Token requested\n");
        return;
    }

    // /api/code/save endpoint
    if (mg_http_match_uri(hm, "/api/code/save")) {
        char headers[512];
        snprintf(headers, sizeof(headers), "%sContent-Type: application/json\r\n", sec_headers);
        char *code = mg_json_get_str(hm->body, "$.content");

        if (code) {
            printf("[SAVE] Executing code: %s\n", code);
            char* output = execute_python_with_output(code, 0);
            printf("[SAVE] Output: %s\n", output);

            mg_http_reply(c, 200, headers, "{%Q:%Q, %Q:%Q}\n",
                          "result", "OK SAVED", "output", output);

            free(code);
            free(output);
        } else {
            mg_http_reply(c, 400, headers, "{%Q:%Q}\n", "error", "Parameters missing");
        }
        return;
    }

    // /api/code/run endpoint
    if (mg_http_match_uri(hm, "/api/code/run")) {
        char headers[512];
        snprintf(headers, sizeof(headers), "%sContent-Type: application/json\r\n", sec_headers);
        char *code = mg_json_get_str(hm->body, "$.content");

        if (code) {
            printf("[RUN] Executing code: %s\n", code);
            char* output = execute_python_with_output(code, 0);
            printf("[RUN] Output: %s\n", output);

            mg_http_reply(c, 200, headers, "{%Q:%Q, %Q:%Q}\n",
                          "result", "OK", "output", output);

            free(code);
            free(output);
        } else {
            mg_http_reply(c, 400, headers, "{%Q:%Q}\n", "error", "Parameters missing");
        }
        return;
    }

    // /api/repl/send endpoint
    if (mg_http_match_uri(hm, "/api/repl/send")) {
        char headers[512];
        snprintf(headers, sizeof(headers), "%sContent-Type: application/json\r\n", sec_headers);
        char *code = mg_json_get_str(hm->body, "$.content");

        if (code) {
            printf("[REPL] Executing code: %s\n", code);
            char* output = execute_python_with_output(code, 1);
            printf("[REPL] Output: %s\n", output);

            mg_http_reply(c, 200, headers, "{%Q:%Q, %Q:%Q}\n",
                          "result", "OK", "output", output);

            free(code);
            free(output);
        } else {
            mg_http_reply(c, 400, headers, "{%Q:%Q}\n", "error", "Parameters missing");
        }
        return;
    }

    // Static file server
    struct mg_http_serve_opts opts = {.root_dir = s_root_dir};
    mg_http_serve_dir(c, hm, &opts);
}

// Main event handler
static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
    if (ev == MG_EV_HTTP_MSG) {
        handle_http_message(c, (struct mg_http_message*)ev_data);
    }
    (void)fn_data;
}

// Signal handler for graceful shutdown
static void signal_handler(int signo) {
    printf("\n[SIGNAL] Received signal %d, shutting down...\n", signo);
    s_shutdown_requested = 1;
}

int main(void) {
    printf("=== Standalone Test Server for zedit ===\n\n");

    // Register signal handlers
    signal(SIGINT, signal_handler);   // Ctrl+C
    signal(SIGTERM, signal_handler);  // kill command

    // Initialize random seed
    srand(time(NULL));
    generate_auth_token();
    printf("Auth token: %s\n", s_auth_token);
    printf("Use this in X-Auth-Token header for API requests\n\n");

    // Initialize Python
    printf("Initializing Python...\n");
    Py_Initialize();

    // Create global namespace
    py_globals = PyDict_New();
    PyDict_SetItemString(py_globals, "__builtins__", PyEval_GetBuiltins());
    printf("Python initialized\n\n");

    // Start HTTP server
    struct mg_mgr mgr;
    mg_log_set(2);
    mg_mgr_init(&mgr);

    printf("Starting HTTP server on %s\n", s_listening_address);
    printf("Web root: %s\n\n", s_root_dir);

    if (!mg_http_listen(&mgr, s_listening_address, fn, NULL)) {
        fprintf(stderr, "Failed to start server\n");
        return 1;
    }

    printf("Server running. Press Ctrl+C to stop.\n");
    printf("Open http://localhost:8000 in your browser\n\n");

    // Event loop with shutdown check
    while (!s_shutdown_requested) {
        mg_mgr_poll(&mgr, 1000);
    }

    // Cleanup
    printf("[SHUTDOWN] Cleaning up...\n");
    mg_mgr_free(&mgr);
    printf("[SHUTDOWN] HTTP server stopped\n");

    Py_XDECREF(py_globals);
    Py_Finalize();
    printf("[SHUTDOWN] Python finalized\n");

    printf("[SHUTDOWN] Server stopped cleanly\n");
    return 0;
}
