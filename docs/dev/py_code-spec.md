# py_code: implementation spec

Behavioural specification for `py_code`, an in-patch Python editor for Max
that sends code to the `py` external. Written so the editor can be implemented
from scratch. Everything here is MIT-licensed new work.

Directory: `source/projects/py_code/`. Language: C++17 (ObjC++ for two macOS
files). Target: macOS 11+, arm64 and x86_64, Max 8/9 SDK.

Existing, keep as is (MIT, already written and tested):

- `py_syntax.{h,cpp}`: tree-sitter-python analysis. API: `Syntax::update(text,
  revision)`, `tokens(line)` (line-relative tokens covering the line, roles in
  `enum TokenType`), `match_bracket(caret)`, `indent_at(caret, tabwidth)`
  (spaces for a new line), `reindent_at(caret, tabwidth)` (new indent for the
  caret line after a closer or `else:`-style colon was typed at caret-1, -1 =
  leave), `eval_span(caret, sel_start, sel_end, mode)`, `is_expression(code)`,
  free functions `dedent(code)` and `leading_ws(line)`.
- `tests/test.h`: minimal harness (`TEST`, `CHECK`, `CHECK_EQ`, `TEST_MAIN`),
  `tests/main.cpp`, `tests/syntax_test.cpp`.
- `thirdparty/tree-sitter`, `thirdparty/tree-sitter-python`,
  `thirdparty/nlohmann` (json 3.12.0, `#include <nlohmann/json.hpp>`).

Conventions for all new code:

- Namespace `pyc`. Offsets are UTF-8 byte offsets on code point boundaries.
  "Cells" are code points (the font is monospace, one code point = one cell).
- No Max headers outside `py_code*.cpp`. Core and LSP modules are headless and
  tested under `make test`.
- ASCII only in source. No emoji. Comments: 1-2 lines; header docs short.
- SPDX header `// SPDX-License-Identifier: MIT` on every file.
- Warnings clean under `-Wall -Wextra -Wno-unused-parameter`.

---

## 1. Core (`core/`, headless)

### 1.1 `core/editor.{h,cpp}`: `pyc::Editor`

Owns the document text, the selection and undo history.

Text and lines:

- `text()`, `length()`, `revision()` (increments on every text change, never
  on selection change).
- Lines are separated by `\n`. `line_count() == count('\n') + 1`; a line
  excludes its `\n`. `line_start(l)`, `line_end(l)`, `line_of(offset)`,
  `line_text(l)`.
- `cells_before(offset)`: code points from the line start to `offset`.
  `offset_at_cell(line, cell)`: inverse, clamped to the line.
- Invalid UTF-8 never hangs: a stray byte counts as one code point.

Selection: `anchor` and `caret`; selected text is `[min, max)`.
`set_caret(off, extend)`, `select(anchor, caret)`, `select_all()`,
`select_word_at(off)` (run of `[A-Za-z0-9_]`; on other non-newline
characters, the run of non-word non-space characters; on whitespace, the
whitespace run; a `\n` selects nothing), `select_line_at(off)` (line without
its `\n`). Offsets are clamped and snapped to code point boundaries.

Motion `move(Move, extend)`: `Left`, `Right` (by code point; with a selection
and no extend, collapse to that side), `WordLeft`, `WordRight` (skip
whitespace then a word or punctuation run), `Up`, `Down` (logical lines,
keeping a goal cell column across consecutive vertical moves), `LineStart`
(first press: first non-blank of the line; pressed again there: column 0),
`LineEnd`, `DocStart`, `DocEnd`. Vertical motion past the first/last line goes
to the start/end of the document.

Editing (each returns true if the text changed):

- `insert(str)`: replaces the selection.
- `type_char(codepoint, autoclose)`: see auto-close rules below.
- `backspace(autoclose, word)`, `delete_forward(word)`: delete the selection,
  else one code point (or to the previous/next word boundary when `word`).
  Backspace in leading whitespace (only spaces before the caret on the line)
  deletes back to the previous multiple of `tabwidth` (pass `tabwidth` in).
- `newline(indent)`: replaces the selection with `\n` + `indent`. If the caret
  sits between an opener and its closer (`(|)`, `[|]`, `{|}`, only spaces
  between), the closer moves to its own line indented like the opener's line:
  `f(|)` with indent 4 gives `f(\n    |\n)`.
- `indent(tabwidth)`: no selection or a selection inside one line: insert
  spaces up to the next multiple of `tabwidth` (in cells). A selection
  spanning lines: add `tabwidth` spaces to every line it touches (a line whose
  selection starts at column 0 of the last line is excluded) and keep the
  lines selected.
- `outdent(tabwidth)`: remove up to `tabwidth` leading spaces from the caret
  line or from every selected line.
- `set_line_indent(line, spaces)`: replace the line's leading whitespace.
  Caret keeps its position relative to the text. Joins the current undo group.
- `reset(text)`: replace everything, clear undo, caret at 0. Used for loads.

Auto-close (`type_char` with `autoclose == true`):

- `(` `[` `{` insert the pair, caret between, unless the next character is a
  word character (`[A-Za-z0-9_]`); then only the opener.
- `'` `"`: if the next character is the same quote, move past it. Else if the
  two characters before the caret are that quote, insert three more
  (`""` + `"` gives `"""|"""`). Else pair it, unless the caret follows a word:
  the run of word characters before the caret pairs only if it is 1-2
  characters, all from `rRbBuUfFtT` (string prefixes `f"`, `rb'`); otherwise
  (`don't`) insert the quote alone.
- `)` `]` `}` with the same character at the caret: move past it.
- A selection with an opener or quote typed: wrap the selection
  (`(sel)`), keep the inner text selected.
- `backspace` with autoclose, caret between an empty pair (`(|)`, `'|'` etc.):
  delete both.

Undo (`undo()`, `redo()`, `can_undo()`, `can_redo()`): an edit log, not
snapshots. Each record stores the replaced range, removed text, inserted text
and the selection before and after. Grouping:

- consecutive `type_char` without caret movement in between form one group,
  except that a group ends after a whitespace character following a
  non-whitespace one (so undo works word by word);
- consecutive backspaces form one group; consecutive forward deletes form one;
- `set_line_indent` joins the group of the edit just before it;
- every other edit (insert/paste, newline, indent, outdent, auto-close pair,
  wrap) is its own group;
- any `move`, `set_caret`, `select*` ends the current group.

Undo restores text and the selection before the group; redo re-applies and
restores the selection after. A new edit clears redo. History limit: 1000
records; dropping the oldest whole group.

### 1.2 `core/layout.{h,cpp}`: `pyc::Layout`

Soft wrap, vertical scroll and hit testing in one class. Units: cells
horizontally, pixels vertically.

- `update(const Editor&, int cols)`: rebuild rows if the revision or `cols`
  changed (`cols <= 0` = no wrap: one row per line).
- Wrap rule per line: a row holds at most `cols` cells. Break after the last
  space or one of `( [ { , ;` that lies in the second half of the row
  (break column >= cols/2); otherwise break hard at `cols`. Never inside a
  code point. An empty line is one row.
- Rows are numbered globally. `row_count()`, `row(r) -> {line, cell_begin,
  cell_end}`, `row_of(offset)` (an offset exactly at a wrap break belongs to
  the later row; the end of a line belongs to its last row),
  `offset_at(row, cell)` (clamped to the row; on a non-last row of a line the
  last valid position is before the break).
- Scroll state: `scroll_y` in pixels; `set_view(view_h, line_h, pad)`.
  Content height = `row_count * line_h + 2 * pad`. `clamp()`,
  `scroll_by(dy)`, `ensure_row_visible(r)` (scroll the minimum),
  `first_visible_row()`, `last_visible_row()`, `row_y(r)` (top of the row in
  view coordinates, `pad + r * line_h - scroll_y`), `row_at_y(y)` (clamped),
  `page_rows()` (whole rows that fit, minus one, at least 1).
- Scroll bar: `thumb(track_h) -> {visible, y, h}`: visible only when content
  exceeds the view; thumb height proportional, minimum 24 px; `drag_thumb(y,
  track_h)` maps a thumb top position back to `scroll_y`.

### 1.3 `core/palette.{h,cpp}`: themes

23 color roles, in this order: `bg text sel caret curline keyword builtin
function class decorator string escape number self comment bracket operator
match blink bar barhighlight error warning`. Colors are RGBA doubles 0..1.
Table (hex, `@a` = alpha, else 1):

| name | aliases | 21 roles in order (error and warning listed separately below) |
|-|-|-|
| onedark | atom | #282c34 #abb2bf #3e4451 #528bff #2c313a #c678dd #56b6c2 #e06c75 #e5c07b #d19a66 #98c379 #98c379 #d19a66 #e06c75 #5c6370 #7f848e #abb2bf #61afef@0.30 #61afef@0.40 #21252b #61afef |
| catppuccin | mocha catppuccinmocha | #1e1e2e #cdd6f4 #89b4fa@0.30 #f5e0dc #313244 #cba6f7 #94e2d5 #f5c2e7 #89b4fa #fab387 #a6e3a1 #f9e2af #74c7ec #eba0ac #6c7086 #9399b2 #bac2de #b4befe@0.35 #fab387@0.40 #181825 #fab387 |
| dracula | - | #282a36 #f8f8f2 #44475a #f8f8f2 #323443 #ff79c6 #8be9fd #ffb86c #8be9fd #50fa7b #f1fa8c #f1fa8c #bd93f9 #ffb86c #6272a4 #f8f8f2 #ff79c6 #8be9fd@0.25 #ff79c6@0.35 #21222c #bd93f9 |
| nord | - | #2e3440 #d8dee9 #434c5e #88c0d0 #3b4252 #81a1c1 #8fbcbb #b48ead #8fbcbb #a3be8c #a3be8c #ebcb8b #b48ead #d08770 #616e88 #d8dee9 #81a1c1 #88c0d0@0.30 #ebcb8b@0.35 #242933 #88c0d0 |
| gruvbox | gruvboxdark | #282828 #ebdbb2 #504945 #fe8019 #32302f #fb4934 #8ec07c #d3869b #fabd2f #fe8019 #b8bb26 #b8bb26 #d3869b #fe8019 #928374 #a89984 #ebdbb2 #83a598@0.35 #fe8019@0.35 #1d2021 #fe8019 |
| monokai | - | #272822 #f8f8f2 #49483e #f8f8f2 #3e3d32 #f92672 #ae81ff #fd971f #66d9ef #a6e22e #e6db74 #e6db74 #ae81ff #fd971f #75715e #f8f8f2 #f92672 #66d9ef@0.28 #f92672@0.35 #1e1f1c #a6e22e |
| solarized | solarizeddark solarised | #002b36 #839496 #073642 #93a1a1 #073642 #859900 #b58900 #6c71c4 #268bd2 #cb4b16 #2aa198 #2aa198 #d33682 #cb4b16 #586e75 #93a1a1 #859900 #268bd2@0.35 #b58900@0.35 #00252e #268bd2 |

error / warning: onedark #e06c75 #e5c07b; catppuccin #f38ba8 #f9e2af;
dracula #ff5555 #f1fa8c; nord #bf616a #ebcb8b; gruvbox #fb4934 #fabd2f;
monokai #f92672 #e6db74; solarized #dc322f #b58900.

API: `theme_count()`, `theme_at(i)`, `theme_find(name)` (case, spaces, `-`,
`_` ignored; aliases accepted; null if unknown), `role_count()`,
`role_name(i)` (the names above), and a theme's colors indexable by role.
Store the table as hex strings and parse once.

### 1.4 `core/snippets.{h,cpp}`

File format (UTF-8, no escapes): a line starting `### ` followed by a key
`[A-Za-z0-9_]+` (rest of the line trimmed) starts an entry; the body is every
following line up to the next header, verbatim. Text before the first header
is ignored. CRLF and CR become LF; a UTF-8 BOM is dropped. Leading and
trailing blank lines of a body are removed; indentation is kept.
`parse(text) -> Snippets` with `find(key)`, `keys()` (in file order),
`duplicates` (keys seen again: first wins) and `empty` (keys with no body:
not stored). `match(code, &key)`: true when `code`, trimmed of whitespace, is
exactly `snip.<key>` with a valid key.

### 1.5 `core/paths.{h,cpp}`

- `posix_path(s)`: Max's volume form `Macintosh HD:/Users/a/x.wav` ->
  `/Users/a/x.wav` (strip up to the first `:/` when `s` does not start with
  `/`). Other input unchanged.
- `expand_home(home, s)`: `~` or `~/...` -> home + rest (trailing `/` of home
  ignored; empty home = no expansion). `~user/...` unchanged.
- `unquote(s)`: trim whitespace; drop one pair of surrounding `'...'` or
  `"..."`; a lone or mismatched quote stays.
- `python_string(s)`: double-quoted Python literal, escaping `\` and `"`.
- `drop_text(home, names)`: each name through posix_path + expand_home +
  python_string, joined with a space.

### 1.6 `core/words.{h,cpp}`: local completion source

`Words::sync(text, revision)` (rescan only on a new revision) collects
identifiers `[A-Za-z_][A-Za-z0-9_]*` with occurrence counts, plus Python
keywords and the builtins in `builtins` (frequency 0). `query(prefix,
limit) -> vector<CompletionItem>`: names starting with `prefix`
(case-sensitive), excluding `prefix` itself, ranked by frequency desc, length
asc, then alphabetical. Items have `label == insert == name`, a `kind`
(keyword 14, function 3 for builtins, text 1 for document words) and
`replace_start = replace_end = -1` (replace the word prefix).

### 1.7 `core/completion.{h,cpp}`: popup model (source agnostic)

```cpp
struct CompletionItem {          // core/completion_item.h, shared with lsp/
    std::string label;           // shown
    std::string insert;          // inserted text (plain, no snippets)
    std::string detail;          // shown dimmed after the label
    std::string filter;          // text matched against the prefix ("" = label)
    std::string sort;            // server order key ("" = keep order)
    int kind = 0;                // LSP CompletionItemKind
    int replace_start = -1;      // byte range to replace; -1 = word prefix
    int replace_end = -1;
};
```

`CompletionSession`:

- `open(text, caret, items)`: word start = start of the `[A-Za-z0-9_]` run
  ending at the caret. Keeps the items, filters by the prefix
  `text[word_start, caret)`.
- `update(text, caret)`: refilter; closes (returns false) when the caret is
  before the word start, the line changed, or nothing matches.
- Filtering: case-insensitive. Prefix matches first, then subsequence
  matches. Within a class: `sort` key when present, else original order.
  An empty prefix matches everything in original order.
- `selected()`, `select_next()`, `select_prev()` (wrap), `visible(rows) ->
  {first, count}` (window that keeps the selection visible),
  `accept() -> {start, end, text}`: the range to replace, from the item's
  `replace_start/replace_end` when set (extended to the caret if the user
  typed past its end), else `[word_start, caret)`.
- `close()`, `is_open()`.

### 1.8 Tests (`tests/`)

One `*_test.cpp` per module, registered in `tests/main.cpp`'s binary. Cover
every rule above with tables. The existing `tests/buffer_test.cpp` tests the
old buffer API; port its cases to `Editor` (behaviour is the same: pairs,
type-over, quotes after words, string prefixes, triple quotes, backspace in a
pair, selection wrap, newline between brackets) and rename it
`editor_test.cpp`.

---

## 2. LSP client (`lsp/`, headless)

### 2.1 Process (`lsp/process.{h,cpp}`)

- Start the server through the user's login shell so `PATH` matches the
  terminal (Max launched from the Dock has `/usr/bin:/bin:/usr/sbin:/sbin`):
  `$SHELL -l -c 'exec <command>'` (`/bin/zsh` if `SHELL` is unset).
- Command `auto` (the default): one shell run that execs the first found of
  `pylsp`, `basedpyright-langserver --stdio`, `pyright-langserver --stdio`,
  `jedi-language-server`, `ty server`; exit 127 if none.
- `posix_spawn` with three pipes. `POSIX_SPAWN_CLOEXEC_DEFAULT` so no Max file
  descriptor leaks into the child. `SIGPIPE` must not kill Max: write with
  `MSG_NOSIGNAL`-equivalent handling (`fcntl(F_SETNOSIGPIPE)` on macOS).
- Threads: a reader (stdout -> framer -> inbound queue), a writer (outbound
  queue -> stdin), a stderr drain keeping the last 20 lines for error
  reports. The main thread never blocks on the pipes.
- Stop: send `shutdown` request, then `exit` notification; close stdin; after
  2 s `SIGTERM`, after 2 more `SIGKILL`; join threads; `waitpid`. Stopping
  must not block Max for more than 4.5 s worst case, and normally returns at
  once (the waiting happens on a detached reaper thread).

### 2.2 Framing (`lsp/framing.{h,cpp}`)

`Framer::feed(bytes, len)` then `next(&json_text)`. Headers are
`Content-Length: N` lines ended by `\r\n\r\n` (also accept `\n\n`), other
headers ignored, case-insensitive name. Bytes before a header start are
skipped (a login shell may print to stdout before `exec`). Handles split
chunks and several messages per chunk. `encode(json_text)` produces a framed
message.

### 2.3 Client (`lsp/client.{h,cpp}`)

One server process per distinct (command, root, Python env) triple, shared by all
documents using it; reference counted; stopped when its last document
closes.

```cpp
namespace pyc::lsp {
enum class State { Starting, Ready, Failed, Stopped };
struct Diagnostic { int start, end; int severity; std::string message; };  // bytes, 1 error .. 4 hint
struct Signature { std::string label; std::vector<std::pair<int,int>> params; int active = -1; }; // byte ranges in label

class Document {
  public:
    // Opens (or joins) a server. Never blocks: spawn happens on a thread.
    static std::shared_ptr<Document> open(const std::string &command, const std::string &root_dir,
                                          const std::string &path,   // absolute path of the .py (may not exist)
                                          const PythonEnv &env = {}); // see core/python_env.h
    ~Document();                                                     // didClose, release the server
    void set_text(const std::string &text, int revision);           // didOpen first time, then full didChange
    bool complete(int offset, std::function<void(int revision, std::vector<CompletionItem>)> cb);
    bool signature(int offset, std::function<void(int revision, Signature)> cb);   // empty label = none
    const std::vector<Diagnostic> &diagnostics() const;  int diagnostics_serial() const;
    State state() const;  std::string server_name() const;  std::string error() const;
};
void set_wakeup(std::function<void()> fn);   // called from any thread when inbound work is queued
void poll();                                   // main thread: dispatch inbound messages and callbacks
void shutdown_all();                           // on Max quit / class free
}
```

Behaviour:

- `initialize`: `processId` = getpid, `rootUri` from root (null if empty),
  `workspaceFolders` likewise, `clientInfo {name: "py_code"}`, capabilities:
  `general.positionEncodings ["utf-8","utf-16"]`; `textDocument.completion`
  with `snippetSupport false`, `documentationFormat ["plaintext"]`;
  `signatureHelp` with `parameterInformation.labelOffsetSupport true`;
  `publishDiagnostics`; `synchronization` with `didSave false`;
  `workspace.configuration true`. A non-empty env adds `initializationOptions`
  (`pylsp.plugins.jedi`, and `workspace` for jedi-language-server). Then
  `initialized`, then `didOpen` for documents opened meanwhile.
- `server_name()` = `serverInfo.name` from the initialize result, else the
  command.
- Positions: use the server's `positionEncoding` (default `utf-16`). Convert
  byte offset <-> (line, character) against the text last sent for that
  document. Provide and test the conversions as free functions
  (`to_position(text, offset, utf16)`, `to_offset(text, line, character,
  utf16)`, clamped; surrogate pairs count 2 in UTF-16).
- `set_text`: `didOpen` (languageId `python`, version = revision) the first
  time, `didChange` with one full-content change afterwards. The caller
  throttles; the client sends what it is given.
- `complete` / `signature` return false when the server is not Ready. A new
  request of the same kind cancels the previous one (`$/cancelRequest`, and
  its callback is never invoked). Callbacks run only inside `poll()`.
- Completion result: `CompletionItem[]` or `CompletionList`. Map: `label`;
  `insert` = `textEdit.newText`, else `insertText`, else `label`; range from
  `textEdit.range` (or `.insert` of an InsertReplaceEdit); `detail`; `kind`;
  `sortText`; `filterText`. Limit 200 items.
- Signature result: the `activeSignature` (default 0); params from label
  offsets or by locating the string label in the signature label;
  `activeParameter` of the signature overrides the top-level one.
- `textDocument/publishDiagnostics` for the document's URI replaces its
  diagnostics and increments the serial. Ranges converted to bytes.
- Server requests (`workspace/configuration`, `client/registerCapability`,
  `window/workDoneProgress/create`, ...): reply with `null` result.
  `workspace/configuration` gets one value per item from the env's settings
  (`python.pythonPath`, `python.analysis.extraPaths`,
  `basedpyright.analysis.extraPaths`), null where absent. `window/showMessage` / `logMessage`:
  ignore except errors, kept in `error()`.
- Process exit or initialize error: State Failed, `error()` has the reason
  and the stderr tail; documents fall back to local completion. No automatic
  restart; reopening (attribute set again) retries.
- URIs: `file://` + percent-encoded absolute path.

### 2.4 Tests

- Framing: split chunks, garbage preamble, two messages in one chunk,
  `\n\n` separators, encode round trip.
- Position conversions with ASCII, 2-, 3- and 4-byte characters.
- Result mapping from canned JSON (completion list and array, textEdit,
  signature with offsets and with string labels, diagnostics).
- Integration with `tests/fake_lsp.py` (a small Python script speaking LSP on
  stdio with canned replies; launched as `python3 <path>`): open, change,
  completion, signature, diagnostics, cancel, shutdown. Integration with the
  real `pylsp` through the login shell when available; the test exits with
  77 when not found (ctest `SKIP_RETURN_CODE 77`), as a separate ctest entry.

---

## 3. Max object (`py_code*.cpp`, `platform/*.mm`)

### 3.1 macOS helpers (`platform/`)

- `clipboard.mm`: `clipboard_text()` (UTF-8 plain text or ""),
  `set_clipboard_text(s)`. NSPasteboard, no ARC, explicit autorelease pools.
- `filedialog.mm`: `save_dialog(default_name)` restricted to `.py`
  (UTType on macOS 11+, extension list before), `open_dialog()` (`.py`
  preferred, others allowed). Return the POSIX path or "". Modal, main thread.

### 3.2 The object

Class `py_code`, a UI box (`CLASS_BOX`, `CLASS_FLAG_NEWDICTIONARY`,
`jbox_initclass`). Call `jpatcher_syms_init()` in `ext_main` and compile
`${MAX_SDK_INCLUDES}/common/jpatcher_syms.c` into the bundle (UI boxes crash in
`jbox_new` without it). Box flags: draw first in, transparent, draw inlet
last, hilite (takes focus on click; no focus, no keys), grow both. Default
rect 420 x 140. One inlet, one outlet.

Facts about Max measured on this kind of box (requirements, not choices):

- Keys arrive through the `key` method only; `keyfilter` is never called on a
  jbox without a textfield. With Cmd held, `textcharacter` is 0 and the
  keycode is the ASCII letter. Use the SDK `JKEY_*` constants for special
  keys. Backspace and Delete share textcharacter 127: decide by keycode.
- Ctrl+Space and Alt+Space do not reach `key`. Cmd+. is taken by Max.
- Right click arrives as `mousedown` with `eRightButton`.
- `jpopupmenu_popup` wants screen coordinates: convert the box rect origin
  and far corner with `patcherview_canvas_to_screen` and scale the box-local
  point by their ratio (zoomed patchers).
- Finder drops: register `acceptsdrag_locked` and `acceptsdrag_unlocked`
  (called on every mouse move: keep cheap), call `jdrag_add(drag, x,
  gensym("dropfile"), "Insert path", NULL)` and return 1; Max then calls the
  `dropfile` method with the drag object; read items with
  `jdrag_itemcount` / `jdrag_getitemstring`. Accept only when the first item
  is an existing file (stat after `posix_path`), which rejects object drags.
- Attribute application order from the box dictionary is not guaranteed: a
  setter must not assume another attribute was already applied.

Output: one message per evaluation on the outlet, the code as ONE symbol
(`gensym(code)`): `eval <code>` when `Syntax::is_expression(code)`, else
`exec <code>`. Before sending: `eval_span` from the mode, the selection
winning; `dedent`; if `snippets::match` -> replace the span by the snippet
body (one undo group, nothing sent, post `-> key` to the console with
`post`), with an error naming the available keys if missing. Empty span:
post "nothing to evaluate".

Keys (focused box):

| key | action |
|-|-|
| Shift+Return | evaluate line |
| Cmd+Return | evaluate block (selection, top-level statement, line) |
| Cmd+Shift+Return | evaluate document |
| Return | newline with `Syntax::indent_at`; accepts the completion when the popup is open |
| Tab | completion popup open: accept. Else, the caret follows `[A-Za-z0-9_.]` and there is no selection and `@completion` is on: open completion. Else indent. |
| Shift+Tab | popup open: previous item. Else outdent. |
| Up / Down | popup open: move selection. Else caret by visual row (via Layout), keeping the goal cell. |
| Esc | close the popup; else not consumed |
| Left/Right, Alt (word), Cmd (line start/end), Shift extends | motion |
| Cmd+Up/Down, Alt+Up/Down | document start/end |
| PageUp/PageDown | caret by `page_rows()` visual rows, keeping the cell |
| Home/End | line start/end |
| Backspace/Delete, Alt = word | delete |
| Cmd+A/C/X/V/Z, Cmd+Shift+Z | select all, copy, cut, paste, undo, redo |
| printable | `type_char` (autoclose per `@autoclose`); after `) ] } :` apply `reindent_at` via `set_line_indent`; `.` opens completion when the LSP is Ready and `@completion` is on |
| other Cmd/Ctrl combos | not consumed (Max shortcuts) |

`@readonly` blocks every edit; navigation, copy and evaluation still work.
After every key: caret row kept visible (minimal scroll), repaint.

Completion: requests go to the LSP when Ready, else to `Words`. LSP results
arrive later: open the popup only if the caret and revision are unchanged
since the request (else drop). The popup: up to 8 rows, drawn inside the box
under the caret row (above it if it does not fit), left edge at the word
start, width = longest label + detail in cells, clamped to the box; colors
`bar` (background), `sel` (selected row), `text` (label), `text` at 0.5 alpha
(detail); a 1 px border in `text` at 0.15 alpha. Typing word characters or
Backspace refilters (`update`); a caret move, click or wheel closes it.
Accept = one undo group replacing the range with the item text.

Status line (bottom, reserved when `@statusbar` is on, height = line height
+ 4, so text never hides under it): shows, by priority, the signature help
for the call at the caret (active parameter in `barhighlight`, the rest in
`text` at 0.55 alpha), else the first diagnostic message on the caret line
(in `error`/`warning`), else nothing. Signature help: request (when Ready)
150 ms after the last edit or caret move; drop results for a stale revision.

Diagnostics: draw a 1 px line under each diagnostic range (2 px below the
text baseline) in `error` (severity 1) or `warning` (2-4); zero-length ranges
get one cell.

LSP lifecycle: document path = `@file` if set, else
`$TMPDIR/py_code/<unique>.py` (never written). Root = the directory of
`@file`, else "". Env = `bundled_python` of `py.mxo`, located from the loaded
`py` class, else beside `py_code.mxo`. Open when the box is created and `@lsp` is non-empty; reopen
when `@lsp` or `@file` changes; send `set_text` 300 ms after the last edit
(a clock), and once at open. Wake-up: `lsp::set_wakeup` triggers a `qelem`
(thread safe) whose handler calls `lsp::poll()` and repaints boxes whose
diagnostics changed. Title strip right side: `python`, plus ` | <server>`
when Ready, ` | lsp starting` or ` | lsp failed` (and post the error once).

Painting (`paint`): background; title strip (`@titlebar`, one line high,
background = bg lightened by 0.07, text right-aligned at 0.55 alpha, 1 px
separator at 0.12); current line band (`curline`, the caret's visual row,
under the gutter too; alpha 0 disables); clip to the text area; selection;
evaluation flash (`blink` color, alpha fading 1 -> 0 with an InCubic curve
over `@blinkduration` ms, repainted every 25 ms by a clock, the evaluated
span); matching brackets (`match` background on both cells,
`@matchbrackets`); text per visible row from `Syntax::tokens`, each token in
its role color, trimmed to the row's cells; diagnostics underlines; caret
(1.6 px bar in `caret`, when focused); placeholder when empty:
`shift-ret line | cmd-ret block | cmd-shift-ret all` at 0.45 alpha. Outside
the clip: gutter (`@linenumbers`: right-aligned numbers on the first row of
each line, 0.70 alpha for the caret line, 0.30 others, 1 px separator at
0.10); scroll bar (7 px wide, track at 0.10 and thumb at 0.30, 0.55 while
dragging, in `#6c7086`); status line; popup last. Font Menlo at
`@fontsize`; measure the cell width and line height once per font size. The
wrap width always reserves the scroll bar so its visibility cannot change
the wrap. Focus ring: 2 px in `caret` via `getdrawparams`.

Mouse: click places the caret; drag selects (autoscroll when dragged above or
below the text area, speed proportional to the distance); double click
selects a word, triple a line (400 ms, 4 px); Shift+click extends. Clicks on
the title strip or status line do nothing; on the scroll bar they grab the
thumb (or jump so the thumb centres on the click) and drag scrolls. Wheel:
`scroll_by(-delta_y * line_h * @scrollspeed)`. Cursor: I-beam on enter.
Right click: menu Undo, Redo, | Cut, Copy, Paste, Delete, Select All, | Save
to .py (`...` when no `@file`), Load from .py..., with items disabled by
state. Save with `@file` writes it; without, the save dialog, then sets
`@file` without reloading. Load: the open dialog, `reset` the editor, set
`@file` without reloading.

Persistence: `jsave` writes `py_code_text`, `py_code_caret`,
`py_code_scroll` into the box dictionary; `jload` (called at the end of
`new`) restores them; saved text wins over `@file`.

Attributes (all saved with the patch unless noted; `onoff` style for flags;
color attributes `rgba` style, defaults from theme 0):

| name | type | default | notes |
|-|-|-|-|
| theme | sym (enum of theme names) | onedark | copies a preset over the 23 colors. Applied at creation after the box dictionary; a color present in the creation dictionary keeps its value. |
| fontsize | float | 12 | clamped 6..48 |
| tabwidth | long | 4 | clamped 1..16 |
| linenumbers, titlebar, matchbrackets, wrap, blink, completion, statusbar, autoclose, autoload | long | 1 | |
| readonly, autowrite | long | 0 | |
| blinkduration | long | 500 | clamped 50..5000 |
| scrollspeed | float | 3 | clamped 0.05..50; also a `scrollspeed <f>` message |
| file | sym | - | setting it reads the file when `@autoload` |
| snippets | sym | - | path (`~`, quotes); load and post `snippets: N entries from <path>`, duplicates and empty keys posted once. Hot reload when mtime or size changed, checked when a `snip.` evaluation happens. Also a `snippets <path...>` message (atoms joined with spaces). Empty = unload. |
| lsp | sym | auto | server command; empty = off |
| 23 colors | float[4] | theme | `bgcolor textcolor selcolor caretcolor curlinecolor keywordcolor builtincolor functioncolor classcolor decoratorcolor stringcolor escapecolor numbercolor selfcolor commentcolor bracketcolor operatorcolor matchcolor blinkcolor barcolor barhighlight errorcolor warningcolor` |

Messages: `theme <name>`, `themes` (post names and the one in use),
`snippets`, `scrollspeed`, `assist`. Unknown theme: error listing names.

Files: `@file` read in full (error on failure, document unchanged); write is
the exact text. `@autowrite`: write 400 ms after the last edit (a clock) and
on free if one is pending.

`free`: stop clocks, close the LSP document, write a pending autowrite, free
everything. The class registers a `quittask` that calls
`lsp::shutdown_all()`.

### 3.3 Build

`CMakeLists.txt`: static libs for tree-sitter (warnings off) and the core +
lsp (C++17, PIC), the MODULE linking AppKit, UniformTypeIdentifiers (weak is
fine), exporting only `_ext_main` (`-Wl,-exported_symbol,_ext_main`), and the
`py_code_tests` executable plus the skip-capable pylsp integration test.
`make test` at the repo root builds and runs ctest.
