import { EditorView, basicSetup } from "codemirror";
import { python } from "@codemirror/lang-python";
import { oneDark } from "@codemirror/theme-one-dark";

// Global auth token
let authToken = null;

// Fetch auth token on load
fetch("/api/auth")
    .then((response) => response.json())
    .then((data) => {
        authToken = data.token;
        console.log("Authentication token received");
    })
    .catch((error) => {
        console.error("Failed to fetch auth token:", error);
    });

let editor = new EditorView({
    extensions: [basicSetup, python(), oneDark],
    doc: "# python code here\n",
    parent: document.getElementById("editor-container"),
});

function log_msg(msg) {
    document.getElementById("msg").innerHTML = msg;
    $("#msg").fadeIn(2000, "linear");
    $("#msg").fadeOut(5000, "linear");
}

function display_output(output, isError = false) {
    const outputContent = document.getElementById("output-content");
    const timestamp = new Date().toLocaleTimeString();
    const prefix = isError ? "[ERROR]" : "[OUTPUT]";
    const newOutput = `${prefix} ${timestamp}\n${output}\n\n`;
    outputContent.textContent += newOutput;
    // Scroll to bottom
    outputContent.parentElement.scrollTop =
        outputContent.parentElement.scrollHeight;
}

function clear_output() {
    document.getElementById("output-content").textContent = "";
    log_msg("Output cleared");
}

function openCode() {
    var input = document.createElement("input");
    input.type = "file";

    input.onchange = (e) => {
        // getting a hold of the file reference
        var file = e.target.files[0];

        // setting up the reader
        var reader = new FileReader();
        reader.readAsText(file, "UTF-8");

        // here we tell the reader what to do when it's done reading...
        reader.onload = (readerEvent) => {
            var content = readerEvent.target.result; // this is the content!
            // console.log(content);
            let transaction = editor.state.update({
                changes: {
                    from: 0,
                    to: editor.state.doc.length,
                    insert: content,
                },
            });
            // console.log(transaction.state.doc.toString()); // "0123"
            // At this point the view still shows the old state.
            editor.dispatch(transaction);
        };
    };

    input.click();

    log_msg("file opened");
}
var open_btn = document.getElementById("open_btn");
open_btn.addEventListener("click", openCode);

function saveCode() {
    if (!authToken) {
        log_msg("Error: Not authenticated");
        return;
    }

    fetch("/api/code/save", {
        method: "POST",
        body: JSON.stringify({
            file_id: 1,
            content: editor.state.doc.toString(),
        }),

        headers: {
            "Content-type": "application/json; charset=UTF-8",
            "X-Auth-Token": authToken,
        },
    })
        .then((response) => {
            if (!response.ok) {
                throw new Error("Save failed: " + response.status);
            }
            return response.json();
        })
        .then((json) => {
            console.log("[DEBUG] Save response received:", json);
            if (json.output) {
                console.log("[DEBUG] Displaying output:", json.output);
                display_output(json.output);
            } else {
                console.log("[DEBUG] No output in response");
            }
            log_msg("file saved");
        })
        .catch((error) => {
            console.error("[DEBUG] Save error:", error);
            display_output(error.message, true);
            log_msg("Error: " + error.message);
        });
}
var save_btn = document.getElementById("save_btn");
save_btn.addEventListener("click", saveCode);

function runCode() {
    if (!authToken) {
        log_msg("Error: Not authenticated");
        return;
    }

    fetch("/api/code/run", {
        method: "POST",
        body: JSON.stringify({
            file_id: 1,
            content: editor.state.doc.toString(),
        }),

        headers: {
            "Content-type": "application/json; charset=UTF-8",
            "X-Auth-Token": authToken,
        },
    })
        .then((response) => {
            if (!response.ok) {
                throw new Error("Run failed: " + response.status);
            }
            return response.json();
        })
        .then((json) => {
            console.log("[DEBUG] Run response received:", json);
            if (json.output) {
                console.log("[DEBUG] Displaying output:", json.output);
                display_output(json.output);
            } else {
                console.log("[DEBUG] No output in response");
            }
            log_msg("code run");
        })
        .catch((error) => {
            console.error("[DEBUG] Run error:", error);
            display_output(error.message, true);
            log_msg("Error: " + error.message);
        });
}
var run_btn = document.getElementById("run_btn");
run_btn.addEventListener("click", runCode);

var clear_output_btn = document.getElementById("clear_output_btn");
clear_output_btn.addEventListener("click", clear_output);

// ----------------------------------------------------------------------
// terminal

$(function () {
    var code = "";
    var env = {};
    var multiLineBuffer = ""; // Buffer for multi-line input
    var inMultiLine = false; // Flag for multi-line mode

    let repl_respond = function (json, term) {
        if (json.output && json.output.trim()) {
            // Check if output looks like an error (contains "Traceback" or "Error:")
            if (
                json.output.includes("Traceback") ||
                json.output.includes("Error:")
            ) {
                term.error(json.output);
            } else {
                term.echo(json.output);
            }
        }
        if (json.error) {
            term.error(json.error);
        }
    };

    let repl_send = function (line, term) {
        if (!authToken) {
            term.error("Error: Not authenticated");
            return;
        }

        // Handle multi-line input at frontend
        if (inMultiLine) {
            if (line.trim() === "") {
                // Empty line in multi-line mode = send accumulated code
                let codeToExecute = multiLineBuffer;

                // Reset state immediately (before async fetch)
                multiLineBuffer = "";
                inMultiLine = false;
                term.set_prompt(">>> ");

                // Send complete block to backend
                fetch("/api/repl/send", {
                    method: "POST",
                    body: JSON.stringify({
                        content: codeToExecute,
                    }),
                    headers: {
                        "Content-type": "application/json; charset=UTF-8",
                        "X-Auth-Token": authToken,
                    },
                })
                    .then((response) => {
                        if (!response.ok) {
                            throw new Error(
                                "REPL request failed: " + response.status
                            );
                        }
                        return response.json();
                    })
                    .then((json) => repl_respond(json, term))
                    .catch((error) => {
                        term.error("Error: " + error.message);
                    });
            } else {
                // Add to buffer and continue
                multiLineBuffer += line + "\n";
                term.set_prompt("... ");
            }
            return;
        }

        // Check if line starts multi-line statement (ends with : or contains keywords)
        let trimmed = line.trim();
        if (
            trimmed.endsWith(":") ||
            trimmed.startsWith("def ") ||
            trimmed.startsWith("class ") ||
            trimmed.startsWith("if ") ||
            trimmed.startsWith("elif ") ||
            trimmed.startsWith("else:") ||
            trimmed.startsWith("for ") ||
            trimmed.startsWith("while ") ||
            trimmed.startsWith("with ") ||
            trimmed.startsWith("try:") ||
            trimmed.startsWith("except ") ||
            trimmed.startsWith("finally:")
        ) {
            // Start multi-line mode
            multiLineBuffer = line + "\n";
            inMultiLine = true;
            term.set_prompt("... ");
            return;
        }

        // Single-line statement - send immediately to backend
        fetch("/api/repl/send", {
            method: "POST",
            body: JSON.stringify({
                content: line,
            }),

            headers: {
                "Content-type": "application/json; charset=UTF-8",
                "X-Auth-Token": authToken,
            },
        })
            .then((response) => {
                if (!response.ok) {
                    throw new Error("REPL request failed: " + response.status);
                }
                return response.json();
            })
            .then((json) => repl_respond(json, term))
            .catch((error) => {
                term.error("Error: " + error.message);
            });
    };

    $("#terminal").terminal(
        [
            {
                hello: function (what) {
                    this.echo(
                        "Hello, " + what + ". Wellcome to this terminal."
                    );
                },

                cat: function (width, height) {
                    return $(
                        '<img src="https://placekitten.com/' +
                            width +
                            "/" +
                            height +
                            '">'
                    );
                },

                title: function () {
                    return fetch("https://terminal.jcubic.pl")
                        .then((r) => r.text())
                        .then(
                            (html) => html.match(/<title>([^>]+)<\/title>/)[1]
                        );
                },

                // opts like argument parsing (-a / --a)
                demo: function (...args) {
                    const options = $.terminal.parse_options(args);
                    return options;
                },

                clear: function () {
                    this.clear;
                },

                py: {
                    eval: function (arg) {},
                    exec: function (arg) {},
                    run: function (arg) {},
                    load: function (arg) {},
                    save: function (arg) {},
                },

                name: function (name) {
                    this.push(
                        function (last_name) {
                            if (last_name) {
                                this.echo(
                                    "Your name is " + name + " " + last_name
                                ).pop();
                            }
                        },
                        {
                            prompt: "last name: ",
                        }
                    );
                },
            },
            function (command) {
                repl_send(command, this);
                console.log(command);
            },
        ],
        {
            keymap: {
                "CTRL-C": function (e, original) {
                    this.echo("my shortcut");
                },
                TAB: function (e, original) {
                    this.insert("    ");
                },
                ENTER: function (e, original) {
                    // Get current command line
                    let command = this.get_command();

                    if (inMultiLine && command.trim() === "") {
                        // Empty line in multi-line mode - execute buffer
                        e.preventDefault();

                        let codeToExecute = multiLineBuffer;
                        multiLineBuffer = "";
                        inMultiLine = false;
                        this.set_prompt(">>> ");

                        // Send to backend
                        fetch("/api/repl/send", {
                            method: "POST",
                            body: JSON.stringify({
                                content: codeToExecute,
                            }),
                            headers: {
                                "Content-type":
                                    "application/json; charset=UTF-8",
                                "X-Auth-Token": authToken,
                            },
                        })
                            .then((response) => {
                                if (!response.ok) {
                                    throw new Error(
                                        "REPL request failed: " +
                                            response.status
                                    );
                                }
                                return response.json();
                            })
                            .then((json) => repl_respond(json, this))
                            .catch((error) => {
                                this.error("Error: " + error.message);
                            });

                        return false; // Prevent default Enter behavior
                    }

                    // Default behavior
                    return original(e);
                },
            },
            checkArity: false,
            completion: true,
            greetings: "Python 3.11.3\n",
            prompt: ">>> ",
        }
    );
});
$.terminal.syntax("python");
$.terminal.prism_formatters = {
    prompt: true,
    echo: true,
    animation: true, // will be supported in version >= 2.32.0
    command: true,
};

document.getElementById("default-tab").click();
