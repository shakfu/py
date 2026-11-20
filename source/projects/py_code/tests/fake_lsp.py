#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""fake_lsp.py -- deterministic LSP server on stdio for the py_code client tests.

Mode (argv[1]): normal (default), utf8 (picks utf-8 positions), env (asks for
the Python settings sections and reports initializationOptions), crash (exits 3
after `initialized`), init-error (initialize fails), immortal (ignores exit, EOF
and SIGTERM: only SIGKILL stops it).

Canned behaviour, keyed on the document text:
  - diagnostics: every `ERR` (severity 1) and `WARN` (2), message `<tok> v<version>`;
  - completion: alpha, beta, gamma (textEdit over the word before the caret) and
    `pos:<line>:<character>`; a line with SLOW answers after 0.4 s and honours
    $/cancelRequest, STUBBORN answers after 0.4 s and ignores it;
  - signatureHelp: `nosig` -> null, `sig_offsets` -> label offsets, else string labels.
Side channel to the test: window/showMessage and logMessage with type 1.
"""

import json
import os
import select
import signal
import sys
import time

MODE = sys.argv[1] if len(sys.argv) > 1 else "normal"
IN = sys.stdin.buffer.fileno()
OUT = sys.stdout.buffer

state = {"utf8": False, "shutdown": False, "buf": b"", "init": None}
docs = {}  # uri -> (text, version)
pending = []  # [due, id, result, stubborn]
replies = {}  # our request id -> reply result
WANT = {"cfg": "cfg", 1001: "reg", 1002: "prog"}


def send(msg):
    msg["jsonrpc"] = "2.0"
    body = json.dumps(msg).encode()
    OUT.write(b"Content-Length: %d\r\n\r\n" % len(body) + body)
    OUT.flush()


def error_message(text):
    send({"method": "window/logMessage", "params": {"type": 1, "message": text}})


def units(s):
    return len(s.encode()) if state["utf8"] else len(s.encode("utf-16-le")) // 2


def index_of(line, character):
    """Python index in `line` of an LSP character offset."""
    n = 0
    for i, ch in enumerate(line):
        if n >= character:
            return i
        n += units(ch)
    return len(line)


def publish(uri):
    text, version = docs[uri]
    diags = []
    for ln, line in enumerate(text.split("\n")):
        for tok, sev in (("ERR", 1), ("WARN", 2)):
            k = line.find(tok)
            while k >= 0:
                rng = {"start": {"line": ln, "character": units(line[:k])},
                       "end": {"line": ln, "character": units(line[:k + len(tok)])}}
                diags.append({"range": rng, "severity": sev, "message": "%s v%d" % (tok, version)})
                k = line.find(tok, k + 1)
    send({"method": "textDocument/publishDiagnostics",
          "params": {"uri": uri, "version": version, "diagnostics": diags}})


def line_at(params):
    text = docs.get(params["textDocument"]["uri"], ("", 0))[0]
    lines = text.split("\n")
    pos = params["position"]
    return (lines[pos["line"]] if pos["line"] < len(lines) else ""), pos["line"], pos["character"]


def completion(params):
    line, ln, ch = line_at(params)
    i = index_of(line, ch)
    ws = i
    while ws > 0 and (line[ws - 1].isalnum() or line[ws - 1] == "_"):
        ws -= 1
    rng = {"start": {"line": ln, "character": units(line[:ws])}, "end": {"line": ln, "character": ch}}
    items = [
        {"label": "alpha", "kind": 6, "detail": "var", "sortText": "2"},
        {"label": "beta", "kind": 3, "insertText": "beta()"},
        {"label": "gamma", "textEdit": {"range": rng, "newText": "gamma_x"}},
        {"label": "pos:%d:%d" % (ln, ch)},
    ]
    return {"isIncomplete": False, "items": items}, line


def signature(params):
    line, _, _ = line_at(params)
    if "nosig" in line:
        return None
    if "sig_offsets" in line:
        label = "f(\u00e4: int, b)"
        a, b = label.index("\u00e4"), label.index("b)")
        return {"signatures": [{"label": label,
                                "parameters": [{"label": [units(label[:a]), units(label[:a + 6])]},
                                               {"label": [units(label[:b]), units(label[:b + 1])]}],
                                "activeParameter": 1}],
                "activeSignature": 0, "activeParameter": 0}
    return {"signatures": [{"label": "g(x, y=1)", "parameters": [{"label": "x"}, {"label": "y=1"}]}],
            "activeSignature": 0, "activeParameter": 1}


def check_initialize(p):
    """Returns a problem description, or '' when the params match the spec."""
    caps = p.get("capabilities", {})
    td = caps.get("textDocument", {})
    if not isinstance(p.get("processId"), int):
        return "processId"
    if p.get("clientInfo", {}).get("name") != "py_code":
        return "clientInfo"
    if caps.get("general", {}).get("positionEncodings") != ["utf-8", "utf-16"]:
        return "positionEncodings"
    item = td.get("completion", {}).get("completionItem", {})
    if item.get("snippetSupport") is not False or item.get("documentationFormat") != ["plaintext"]:
        return "completion"
    sig = td.get("signatureHelp", {}).get("signatureInformation", {})
    if sig.get("parameterInformation", {}).get("labelOffsetSupport") is not True:
        return "signatureHelp"
    if "publishDiagnostics" not in td or td.get("synchronization", {}).get("didSave") is not False:
        return "sync"
    root, folders = p.get("rootUri"), p.get("workspaceFolders")
    if root is None:
        return "" if folders is None else "folders"
    if not root.startswith("file:///") or " " in root or folders[0]["uri"] != root:
        return "rootUri"
    return ""


def handle(msg):
    method, mid, params = msg.get("method"), msg.get("id"), msg.get("params")
    if method is None:  # a reply to one of our requests
        if mid in WANT:
            replies[WANT[mid]] = msg.get("result", "missing")
            if len(replies) == 3:
                dump = lambda k: json.dumps(replies[k], separators=(",", ":"))
                text = "replies: cfg=%s reg=%s prog=%s" % (dump("cfg"), dump("reg"), dump("prog"))
                if MODE == "env":
                    text += " init=" + json.dumps(state["init"], separators=(",", ":"), sort_keys=True)
                send({"method": "window/showMessage", "params": {"type": 1, "message": text}})
        return
    if method == "initialize":
        if MODE == "init-error":
            send({"id": mid, "error": {"code": -32603, "message": "nope"}})
            return
        encs = params["capabilities"].get("general", {}).get("positionEncodings", [])
        state["utf8"] = MODE == "utf8" and "utf-8" in encs
        state["init"] = params.get("initializationOptions")
        problem = check_initialize(params)
        caps = {"textDocumentSync": 1, "completionProvider": {"triggerCharacters": ["."]},
                "signatureHelpProvider": {"triggerCharacters": ["(", ","]}}
        if state["utf8"]:
            caps["positionEncoding"] = "utf-8"
        send({"id": mid, "result": {"capabilities": caps,
                                    "serverInfo": {"name": "fake-lsp" + ("-bad:" + problem if problem else "")}}})
    elif method == "initialized":
        if MODE == "crash":
            sys.stderr.write("fatal: boom\n")
            sys.stderr.flush()
            sys.exit(3)
        send({"method": "window/logMessage", "params": {"type": 3, "message": "info noise"}})
        items = [{"section": "a"}, {"section": "b"}]
        if MODE == "env":
            items = [{"section": "python"}, {"section": "basedpyright.analysis"},
                     {"section": "python.missing"}, {}]
        send({"id": "cfg", "method": "workspace/configuration", "params": {"items": items}})
        send({"id": 1001, "method": "client/registerCapability", "params": {"registrations": []}})
        send({"id": 1002, "method": "window/workDoneProgress/create", "params": {"token": "t"}})
    elif method == "textDocument/didOpen":
        td = params["textDocument"]
        if td.get("languageId") != "python":
            error_message("bad languageId")
        docs[td["uri"]] = (td["text"], td["version"])
        publish(td["uri"])
    elif method == "textDocument/didChange":
        td = params["textDocument"]
        docs[td["uri"]] = (params["contentChanges"][-1]["text"], td["version"])
        publish(td["uri"])
    elif method == "textDocument/didClose":
        docs.pop(params["textDocument"]["uri"], None)
    elif method == "textDocument/completion":
        result, line = completion(params)
        if "SLOW" in line or "STUBBORN" in line:
            pending.append([time.time() + 0.4, mid, result, "STUBBORN" in line])
        else:
            send({"id": mid, "result": result})
    elif method == "textDocument/signatureHelp":
        send({"id": mid, "result": signature(params)})
    elif method == "$/cancelRequest":
        for p in list(pending):
            if p[1] == params["id"] and not p[3]:
                pending.remove(p)
                send({"id": p[1], "error": {"code": -32800, "message": "cancelled"}})
                error_message("cancelled %d" % p[1])
    elif method == "shutdown":
        state["shutdown"] = True
        send({"id": mid, "result": None})
    elif method == "exit":
        if MODE == "immortal":
            return
        sys.exit(0 if state["shutdown"] else 1)
    elif mid is not None:
        send({"id": mid, "error": {"code": -32601, "message": "unknown " + method}})


def read_messages(timeout):
    r, _, _ = select.select([IN], [], [], timeout)
    if r:
        chunk = os.read(IN, 65536)
        if not chunk:
            while MODE == "immortal":
                time.sleep(1)
            sys.exit(0 if state["shutdown"] else 1)
        state["buf"] += chunk
    out = []
    while True:
        buf = state["buf"]
        i = buf.find(b"\r\n\r\n")
        if i < 0:
            break
        n = int(buf[:i].split(b":")[1])
        if len(buf) < i + 4 + n:
            break
        out.append(json.loads(buf[i + 4:i + 4 + n]))
        state["buf"] = buf[i + 4 + n:]
    return out


def main():
    if MODE == "immortal":
        signal.signal(signal.SIGTERM, signal.SIG_IGN)
    OUT.write(b"login shell noise\n")  # the client must skip bytes before a header
    OUT.flush()
    while True:
        now = time.time()
        timeout = max(0.0, min(p[0] for p in pending) - now) if pending else None
        for msg in read_messages(timeout):
            handle(msg)
        now = time.time()
        for p in [p for p in pending if p[0] <= now]:
            pending.remove(p)
            send({"id": p[1], "result": p[2]})
            if p[3]:
                error_message("late %d" % p[1])


if __name__ == "__main__":
    main()
