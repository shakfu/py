// SPDX-License-Identifier: GPL-3.0-only
/* py_code.cpp -- `py_code`: in-patch Python editor for the `py` object.
 *
 * UI and glue only. Text, caret and undo live in `py_buffer`; parsing,
 * highlight, indentation, bracket pairs and the eval span live in `py_syntax`
 * (tree-sitter-python). The object never runs Python: it emits
 *
 *     exec <code>   statements                 -> [py] runs them
 *     eval <code>   a single expression        -> [py] outputs the value
 *
 * with the code as ONE symbol, so commas, semicolons, quotes and newlines pass
 * the patch cord untouched (no Max message parsing in between).
 *
 * Keys: Shift-Return line, Cmd-Return top-level block, Cmd-Shift-Return all.
 * A selection always wins.
 *
 * Derived from `sc_code` by Gustavo (gus74vv/sc3-tilde, GPL-3.0-only); see
 * LICENSE.md in this directory. The SuperCollider parts (sclang lexer,
 * regions, class index, call aid) are replaced or dropped.
 */

#include "ext.h"
#include "ext_obex.h"
#include "ext_drag.h"
#include "jpatcher_api.h"
#include "jpatcher_syms.h" /* jpatcher_syms_init(): jbox_new() crashes without it */
#include "jgraphics.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/stat.h>

#include "py_blink.h"
#include "py_buffer.h"
#include "py_clipboard.h"
#include "py_complete.h"
#include "py_filedialog.h"
#include "py_path.h"
#include "py_scroll.h"
#include "py_snip.h"
#include "py_syntax.h"
#include "py_theme.h"
#include "py_wrap.h"

#define PY_CODE_VERSION "0.1.0"

#define COL_BAR_R 0.424 /* scroll bar */
#define COL_BAR_G 0.439
#define COL_BAR_B 0.525

#define PAD_X 6.
#define PAD_Y 4.
#define BAR_W 7.      /* vertical scroll bar width */
#define GUTTER_GAP 8. /* space right of the line numbers */
#define PY_TITLE_TEXT "python"
#define PY_NO_WRAP (1 << 28) /* `@wrap 0`: one row per line */
#define PY_COMPL_MIN_PREFIX 3

/* Heap-held: object_alloc returns zeroed memory, not constructed C++ objects. */
struct PySnipState {
	py_code::Snippets dict;
	std::string path; /* resolved (`~` expanded) */
	long mtime;       /* 0 = not read yet */
	long long size;   /* -1 = last attempt failed: retry */
};

typedef struct _py_code {
	t_jbox j_box; /* must be first */
	py_code::Buffer *buf;
	py_code::Syntax *syn;
	void *out_eval; /* exec/eval <code> */
	long focused;
	long tabwidth;
	long click_count; /* 1 caret, 2 word, 3 line */
	long last_click_x, last_click_y;
	double last_click_ms;
	long dragging;
	double font_size;
	double char_w, line_h, ascent; /* monospace metrics, measured once per size */
	double metrics_for;
	py_code::ScrollView scroll;
	py_code::WrapCache wraps;
	long wrap;
	int text_cols; /* cells per row at the last paint (0 = not measured) */
	double view_w, view_h;
	long sb_drag;
	double sb_grab;

	t_jrgba bgcolor, textcolor, selcolor, caretcolor, curlinecolor;
	t_symbol *theme;
	t_jrgba keywordcolor, builtincolor, functioncolor, classcolor;
	t_jrgba decoratorcolor, stringcolor, escapecolor, numbercolor;
	t_jrgba selfcolor, commentcolor, bracketcolor, operatorcolor;
	long linenumbers;
	long readonly;
	long autoclose;
	long autowrite;
	long autoload;
	t_symbol *filename;
	double gutter_w;
	t_clock *autowrite_clock;

	long titlebar;
	double title_h;
	long matchbrackets;
	t_jrgba matchcolor;

	long evalblink;
	long blinkduration;
	t_jrgba blinkcolor;
	t_clock *blink_clock;
	py_code::Blink blink_state;

	t_symbol *snipfile;
	PySnipState *snips;

	long completion; /* 1 = reserved bottom bar showing the completion candidate */
	double bar_h;
	long bar_grab;
	t_jrgba barcolor, barhighlight;
	py_code::Completer *completer;
	long compl_open;
	int compl_cycle;
	int compl_start, compl_end;
	std::string *compl_word;
	int compl_alternatives;

	double scrollspeed;
} t_py_code;

static t_class *s_py_code_class = NULL;

static double py_code_now_ms() {
	using namespace std::chrono;
	return (double)duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count() / 1000.;
}

static void py_code_sync(t_py_code *x) { x->syn->update(x->buf->text(), x->buf->revision()); }

/**********************************************************************/
/* evaluation                                                          */

static void py_code_blink_tick(t_py_code *x) {
	if (x->blink_state.expired(py_code_now_ms()))
		x->blink_state.stop();
	jbox_redraw(&x->j_box);
	if (x->blink_state.active() && x->blink_clock)
		clock_fdelay(x->blink_clock, py_code::Blink::kStep);
}

static void py_code_blink_poke(t_py_code *x, int start, int end) {
	if (!x->evalblink || end <= start)
		return;
	x->blink_state.start(start, end, py_code_now_ms());
	if (x->blink_clock) {
		clock_unset(x->blink_clock);
		clock_fdelay(x->blink_clock, py_code::Blink::kStep);
	}
	jbox_redraw(&x->j_box);
}

static void py_code_blink_cancel(t_py_code *x) {
	if (x->blink_clock)
		clock_unset(x->blink_clock);
	x->blink_state.stop();
}

static void py_code_snip_expand(t_py_code *x, const py_code::EvalSpan &span, const std::string &key);

static void py_code_eval_with_mode(t_py_code *x, py_code::EvalMode mode) {
	py_code_sync(x);
	const std::string &text = x->buf->text();
	const py_code::EvalSpan span =
		x->syn->eval_span(x->buf->caret(), x->buf->selection_start(), x->buf->selection_end(), mode);
	if (!span.has_value()) {
		object_post((t_object *)x, "nothing to evaluate (empty line)");
		return;
	}
	/* An indented line or selection is dedented: `exec` rejects leading indentation. */
	const std::string code = py_code::dedent(text.substr((size_t)span.start, (size_t)(span.end - span.start)));

	std::string key;
	if (py_code::snip_matches(code, key)) {
		py_code_snip_expand(x, span, key);
		return;
	}

	py_code_blink_poke(x, span.start, span.end);
	const bool expr = x->syn->is_expression(code);
	t_atom a;
	atom_setsym(&a, gensym(code.c_str()));
	outlet_anything(x->out_eval, gensym(expr ? "eval" : "exec"), 1, &a);
}

/**********************************************************************/
/* @file and patch persistence                                         */

static bool py_code_has_file(t_py_code *x) { return x->filename && x->filename->s_name[0]; }

static bool py_code_read_all(const char *path, std::string *out) {
	FILE *f = fopen(path, "rb");
	if (!f)
		return false;
	char chunk[4096];
	size_t n;
	while ((n = fread(chunk, 1, sizeof(chunk), f)) > 0)
		out->append(chunk, n);
	const bool bad = ferror(f) != 0;
	fclose(f);
	return !bad;
}

static void py_code_reset_view(t_py_code *x) {
	x->wraps.reset();
	py_code_blink_cancel(x);
	x->scroll.y = 0.;
	jbox_redraw(&x->j_box);
}

/* A load, not an edit: resets undo, caret and scroll. On failure the document is kept. */
static bool py_code_load_from_path(t_py_code *x, const char *path) {
	std::string text;
	if (!py_code_read_all(path, &text)) {
		object_error((t_object *)x, "cannot read '%s'", path);
		return false;
	}
	x->buf->set_text(text);
	py_code_reset_view(x);
	object_post((t_object *)x, "%d bytes read from %s", (int)text.size(), path);
	return true;
}

static bool py_code_save_to_path(t_py_code *x, const char *path) {
	FILE *f = fopen(path, "wb");
	if (!f) {
		object_error((t_object *)x, "cannot write '%s'", path);
		return false;
	}
	const std::string &text = x->buf->text();
	const size_t wrote = text.empty() ? 0 : fwrite(text.data(), 1, text.size(), f);
	const bool ok = wrote == text.size() && fclose(f) == 0;
	if (wrote != text.size())
		fclose(f);
	if (!ok)
		object_error((t_object *)x, "incomplete write to '%s'", path);
	return ok;
}

static bool py_code_read_file(t_py_code *x) {
	return py_code_has_file(x) && py_code_load_from_path(x, x->filename->s_name);
}

static bool py_code_write_file(t_py_code *x) {
	return py_code_has_file(x) && py_code_save_to_path(x, x->filename->s_name);
}

/* Debounced (400 ms): the last change reaches the disk, not every keystroke. */
static void py_code_autowrite_poke(t_py_code *x) {
	if (!x->autowrite || !py_code_has_file(x) || !x->autowrite_clock)
		return;
	clock_unset(x->autowrite_clock);
	clock_delay(x->autowrite_clock, 400);
}

static void py_code_autowrite_tick(t_py_code *x) { py_code_write_file(x); }

/**********************************************************************/
/* snippets: evaluating exactly `snip.KEY` inserts the body of KEY      */

static void py_code_after_edit(t_py_code *x);

/* `~` expanded and one pair of surrounding quotes dropped (a message box keeps them). */
static std::string py_code_expand_path(const char *path) {
	const std::string p = py_code::unquote_path(path ? path : "");
	if (p == "~" || (p.size() > 1 && p[0] == '~' && p[1] == '/')) {
		if (const char *home = getenv("HOME"))
			return std::string(home) + p.substr(1);
	}
	return p;
}

static bool py_code_file_stamp(const std::string &path, long *mtime, long long *size) {
	struct stat st;
	if (stat(path.c_str(), &st) != 0)
		return false;
	*mtime = (long)st.st_mtime;
	*size = (long long)st.st_size;
	return true;
}

static bool py_code_snip_load(t_py_code *x, const char *path) {
	PySnipState *st = x->snips;
	st->path = py_code_expand_path(path);
	std::string text;
	if (!py_code_read_all(st->path.c_str(), &text)) {
		object_error((t_object *)x, "snippets: cannot read '%s'", st->path.c_str());
		st->mtime = 0;
		st->size = -1;
		return false;
	}
	st->dict = py_code::parse_snippets(text);
	if (!py_code_file_stamp(st->path, &st->mtime, &st->size))
		st->size = (long long)text.size();
	for (const std::string &k : st->dict.duplicates)
		object_post((t_object *)x, "snippets: duplicate `### %s`, the first one wins", k.c_str());
	for (const std::string &k : st->dict.empty)
		object_post((t_object *)x, "snippets: `### %s` has no body, ignored", k.c_str());
	object_post((t_object *)x, "snippets: %d entries from %s", (int)st->dict.size(), st->path.c_str());
	return true;
}

/* Hot reload on mtime/size change, checked only when a `snip.KEY` is evaluated. */
static void py_code_snip_maybe_reload(t_py_code *x) {
	PySnipState *st = x->snips;
	long mtime = 0;
	long long size = 0;
	if (st->path.empty() || !py_code_file_stamp(st->path, &mtime, &size))
		return;
	if (mtime != st->mtime || size != st->size)
		py_code_snip_load(x, st->path.c_str());
}

/* Replaces the evaluated span with the body: text only, nothing is sent, one undo step. */
static void py_code_snip_expand(t_py_code *x, const py_code::EvalSpan &span, const std::string &key) {
	py_code_snip_maybe_reload(x);
	PySnipState *st = x->snips;
	const std::string *body = st->dict.find(key);
	if (!body) {
		const std::string have = st->dict.keys_list();
		if (st->path.empty())
			object_error((t_object *)x, "snip.%s: no snippets file (@snippets <path>)", key.c_str());
		else
			object_error((t_object *)x, "snip.%s: not found in %s (has: %s)", key.c_str(), st->path.c_str(),
						 have.empty() ? "nothing" : have.c_str());
		return;
	}
	if (x->readonly) {
		object_error((t_object *)x, "snip.%s: the editor is read-only", key.c_str());
		return;
	}
	x->buf->set_selection(span.start, span.end);
	if (!x->buf->insert_text(*body))
		return;
	py_code_after_edit(x);
	post("-> %s", key.c_str());
	jbox_redraw(&x->j_box);
}

/* A message box splits a path on spaces: the atoms are joined back. */
static t_max_err py_code_set_snippets(t_py_code *x, t_object *attr, long argc, t_atom *argv) {
	std::string path;
	for (long i = 0; i < argc && argv; ++i) {
		if (atom_gettype(argv + i) != A_SYM)
			continue;
		if (!path.empty())
			path += ' ';
		path += atom_getsym(argv + i)->s_name;
	}
	if (path.empty()) {
		x->snipfile = NULL;
		*x->snips = PySnipState();
		return MAX_ERR_NONE;
	}
	const std::string stored = py_code::unquote_path(path); /* `~` kept: portable patch */
	x->snipfile = gensym(stored.c_str());
	py_code_snip_load(x, stored.c_str());
	return MAX_ERR_NONE;
}

/* Registered as a method too: the attribute alone does not reliably create the message. */
static void py_code_snippets_msg(t_py_code *x, t_symbol *s, long argc, t_atom *argv) {
	py_code_set_snippets(x, NULL, argc, argv);
}

/* jsave: the text is document state, not an attribute. Keys are prefixed because
 * the box dictionary already has `text`. */
#define PY_KEY_TEXT "py_code_text"
#define PY_KEY_CARET "py_code_caret"
#define PY_KEY_SCROLL "py_code_scroll"

static t_max_err py_code_jsave(t_py_code *x, t_dictionary *d) {
	dictionary_appendstring(d, gensym(PY_KEY_TEXT), x->buf->text().c_str());
	dictionary_appendlong(d, gensym(PY_KEY_CARET), x->buf->caret());
	dictionary_appendlong(d, gensym(PY_KEY_SCROLL), (t_atom_long)std::lround(x->scroll.y));
	return MAX_ERR_NONE;
}

static t_max_err py_code_jload(t_py_code *x, t_dictionary *d) {
	const char *text = NULL;
	if (dictionary_getstring(d, gensym(PY_KEY_TEXT), &text) == MAX_ERR_NONE && text) {
		x->buf->set_text(std::string(text));
		x->wraps.reset();
	}
	t_atom_long caret = 0, scroll = 0;
	if (dictionary_getlong(d, gensym(PY_KEY_CARET), &caret) == MAX_ERR_NONE)
		x->buf->set_caret((int)caret);
	if (dictionary_getlong(d, gensym(PY_KEY_SCROLL), &scroll) == MAX_ERR_NONE)
		x->scroll.y = (double)scroll;
	jbox_redraw(&x->j_box);
	return MAX_ERR_NONE;
}

/* --- attribute setters ---------------------------------------------------- */

static t_max_err py_code_set_fontsize(t_py_code *x, t_object *attr, long argc, t_atom *argv) {
	if (!argc || !argv)
		return MAX_ERR_NONE;
	const double v = atom_getfloat(argv);
	x->font_size = v < 6. ? 6. : (v > 48. ? 48. : v);
	x->metrics_for = -1.;
	jbox_redraw(&x->j_box);
	return MAX_ERR_NONE;
}

static t_max_err py_code_set_tabwidth(t_py_code *x, t_object *attr, long argc, t_atom *argv) {
	if (!argc || !argv)
		return MAX_ERR_NONE;
	const long v = atom_getlong(argv);
	x->tabwidth = v < 1 ? 1 : (v > 16 ? 16 : v);
	return MAX_ERR_NONE;
}

static t_max_err py_code_set_autoclose(t_py_code *x, t_object *attr, long argc, t_atom *argv) {
	if (!argc || !argv)
		return MAX_ERR_NONE;
	x->autoclose = atom_getlong(argv) != 0;
	x->buf->set_auto_close(x->autoclose != 0);
	return MAX_ERR_NONE;
}

static t_max_err py_code_set_autowrite(t_py_code *x, t_object *attr, long argc, t_atom *argv) {
	if (!argc || !argv)
		return MAX_ERR_NONE;
	x->autowrite = atom_getlong(argv) != 0;
	if (x->autowrite && py_code_has_file(x))
		py_code_autowrite_poke(x);
	return MAX_ERR_NONE;
}

static t_max_err py_code_set_blinkduration(t_py_code *x, t_object *attr, long argc, t_atom *argv) {
	if (!argc || !argv)
		return MAX_ERR_NONE;
	x->blink_state.set_duration(atom_getfloat(argv));
	x->blinkduration = (long)(x->blink_state.duration() + 0.5);
	return MAX_ERR_NONE;
}

/* Multiplies the wheel/trackpad delta. Clamped to [0.05, 50]. */
static t_max_err py_code_set_scrollspeed(t_py_code *x, t_object *attr, long argc, t_atom *argv) {
	if (!argc || !argv)
		return MAX_ERR_NONE;
	const double v = atom_getfloat(argv);
	x->scrollspeed = v < 0.05 ? 0.05 : (v > 50. ? 50. : v);
	return MAX_ERR_NONE;
}

static void py_code_scrollspeed_msg(t_py_code *x, double v) {
	t_atom a;
	atom_setfloat(&a, v);
	py_code_set_scrollspeed(x, NULL, 1, &a);
}

static t_max_err py_code_set_file(t_py_code *x, t_object *attr, long argc, t_atom *argv) {
	x->filename = (argc && argv && atom_gettype(argv) == A_SYM) ? atom_getsym(argv) : NULL;
	if (py_code_has_file(x) && x->autoload)
		py_code_read_file(x);
	return MAX_ERR_NONE;
}

/* --- @theme ---------------------------------------------------------------
 * Copies a preset over the 21 color attributes. A color present in the
 * creation dictionary `keep` was set by hand and survives the preset (Max does
 * not guarantee the order it applies dictionary attributes in). */

static t_jrgba py_to_jrgba(const py_code::RGBA &c) {
	t_jrgba v;
	v.red = c.r;
	v.green = c.g;
	v.blue = c.b;
	v.alpha = c.a;
	return v;
}

static void py_code_apply_theme(t_py_code *x, const py_code::Theme *t, t_dictionary *keep) {
	if (!t)
		return;
	struct Role {
		const char *attr;
		t_jrgba *dst;
		const py_code::RGBA *src;
	} roles[] = {
		{"bgcolor", &x->bgcolor, &t->bg},
		{"textcolor", &x->textcolor, &t->text},
		{"selcolor", &x->selcolor, &t->sel},
		{"caretcolor", &x->caretcolor, &t->caret},
		{"curlinecolor", &x->curlinecolor, &t->curline},
		{"keywordcolor", &x->keywordcolor, &t->keyword},
		{"builtincolor", &x->builtincolor, &t->builtin},
		{"functioncolor", &x->functioncolor, &t->function},
		{"classcolor", &x->classcolor, &t->klass},
		{"decoratorcolor", &x->decoratorcolor, &t->decorator},
		{"stringcolor", &x->stringcolor, &t->string},
		{"escapecolor", &x->escapecolor, &t->escape},
		{"numbercolor", &x->numbercolor, &t->number},
		{"selfcolor", &x->selfcolor, &t->self},
		{"commentcolor", &x->commentcolor, &t->comment},
		{"bracketcolor", &x->bracketcolor, &t->bracket},
		{"operatorcolor", &x->operatorcolor, &t->op},
		{"matchcolor", &x->matchcolor, &t->match},
		{"blinkcolor", &x->blinkcolor, &t->blink},
		{"barcolor", &x->barcolor, &t->bar},
		{"barhighlight", &x->barhighlight, &t->barhighlight},
	};
	for (const Role &r : roles)
		if (!(keep && dictionary_hasentry(keep, gensym(r.attr))))
			*r.dst = py_to_jrgba(*r.src);
	x->theme = gensym(t->name);
	if (!keep)
		jbox_redraw((t_jbox *)x);
}

static void py_code_theme_names(std::string *out) {
	for (int i = 0; i < py_code::theme_count(); ++i) {
		if (i)
			out->append(" ");
		out->append(py_code::theme_at(i)->name);
	}
}

static t_max_err py_code_set_theme(t_py_code *x, t_object *attr, long argc, t_atom *argv) {
	if (!argc || !argv)
		return MAX_ERR_NONE;
	const char *name = atom_getsym(argv)->s_name;
	const py_code::Theme *t = py_code::theme_find(name);
	if (!t) {
		std::string list;
		py_code_theme_names(&list);
		object_error((t_object *)x, "theme: no \"%s\" (available: %s)", name, list.c_str());
		return MAX_ERR_NONE;
	}
	py_code_apply_theme(x, t, NULL);
	return MAX_ERR_NONE;
}

static void py_code_theme_msg(t_py_code *x, t_symbol *s) {
	t_atom a;
	atom_setsym(&a, s);
	py_code_set_theme(x, NULL, 1, &a);
}

static void py_code_themes_msg(t_py_code *x) {
	std::string list;
	py_code_theme_names(&list);
	object_post((t_object *)x, "themes: %s (in use: %s)", list.c_str(), x->theme ? x->theme->s_name : "?");
}

/**********************************************************************/
/* Finder drops: insert the absolute path as a quoted Python string     */

/* Called on every mouse move of a drag: must stay cheap. The first item must be an
 * existing file, which separates a Finder drop from dragging a Max object. */
static long py_code_acceptsdrag(t_py_code *x, t_object *drag, t_object *view) {
	if (x->readonly || !drag || jdrag_itemcount(drag) <= 0)
		return 0;
	const char *item = jdrag_getitemstring(drag, 0, 0, 0);
	if (!item || !item[0])
		return 0;
	struct stat st;
	if (stat(py_code::path_to_posix(item).c_str(), &st) != 0)
		return 0;
	jdrag_add(drag, (t_object *)x, gensym("dropfile"), "Insert path", NULL);
	return 1;
}

static void py_code_dropfile(t_py_code *x, t_object *drag) {
	if (x->readonly || !drag)
		return;
	std::vector<std::string> names;
	const long count = jdrag_itemcount(drag);
	for (long i = 0; i < count; ++i) {
		const char *name = jdrag_getitemstring(drag, i, 0, 0);
		if (name && name[0])
			names.push_back(name);
	}
	const char *home = getenv("HOME");
	const std::string text = py_code::dropped_text(home ? home : "", names);
	if (text.empty())
		return;
	x->buf->insert_text(text);
	py_code_after_edit(x);
	jbox_redraw(&x->j_box);
}

/**********************************************************************/
/* life cycle                                                          */

static t_py_code *py_code_new(t_symbol *s, long argc, t_atom *argv);
static void py_code_free(t_py_code *x);
static void py_code_paint(t_py_code *x, t_object *view);
static long py_code_key(t_py_code *x, t_object *patcherview, long keycode, long modifiers, long textcharacter);
static void py_code_focusgained(t_py_code *x, t_object *patcherview);
static void py_code_focuslost(t_py_code *x, t_object *patcherview);
static void py_code_mouseenter(t_py_code *x, t_object *patcherview, t_pt pt, long modifiers);
static long py_code_mousedown(t_py_code *x, t_object *patcherview, t_pt pt, long modifiers);
static void py_code_mousedrag(t_py_code *x, t_object *patcherview, t_pt pt, long modifiers);
static void py_code_mouseup(t_py_code *x, t_object *patcherview, t_pt pt, long modifiers);
static void py_code_mousewheel(t_py_code *x, t_object *patcherview, t_pt pt, long modifiers, double dx, double dy);
static void py_code_getdrawparams(t_py_code *x, t_object *patcherview, t_jboxdrawparams *params);
static void py_code_assist(t_py_code *x, void *b, long m, long a, char *s);

#define PY_COLOR_ATTR(c, name, field, def)                                                                             \
	CLASS_ATTR_RGBA(c, name, 0, t_py_code, field);                                                                     \
	CLASS_ATTR_STYLE_LABEL(c, name, 0, "rgba", name);                                                                  \
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, name, 0, def)

C74_EXPORT void ext_main(void *r) {
	jpatcher_syms_init();

	std::string theme_menu;
	py_code_theme_names(&theme_menu);

	t_class *c = class_new("py_code", (method)py_code_new, (method)py_code_free, sizeof(t_py_code), (method)NULL,
						   A_GIMME, 0L);
	c->c_flags |= CLASS_FLAG_NEWDICTIONARY;
	jbox_initclass(c, 0);

	class_addmethod(c, (method)py_code_paint, "paint", A_CANT, 0);
	class_addmethod(c, (method)py_code_key, "key", A_CANT, 0);
	class_addmethod(c, (method)py_code_focusgained, "focusgained", A_CANT, 0);
	class_addmethod(c, (method)py_code_focuslost, "focuslost", A_CANT, 0);
	class_addmethod(c, (method)py_code_mouseenter, "mouseenter", A_CANT, 0);
	class_addmethod(c, (method)py_code_mousedown, "mousedown", A_CANT, 0);
	class_addmethod(c, (method)py_code_mousedrag, "mousedrag", A_CANT, 0);
	class_addmethod(c, (method)py_code_mouseup, "mouseup", A_CANT, 0);
	class_addmethod(c, (method)py_code_mousewheel, "mousewheel", A_CANT, 0);
	class_addmethod(c, (method)py_code_getdrawparams, "getdrawparams", A_CANT, 0);
	class_addmethod(c, (method)py_code_jsave, "jsave", A_CANT, 0);
	class_addmethod(c, (method)py_code_jload, "jload", A_CANT, 0);
	class_addmethod(c, (method)py_code_assist, "assist", A_CANT, 0);
	class_addmethod(c, (method)py_code_acceptsdrag, "acceptsdrag_locked", A_CANT, 0);
	class_addmethod(c, (method)py_code_acceptsdrag, "acceptsdrag_unlocked", A_CANT, 0);
	class_addmethod(c, (method)py_code_dropfile, "dropfile", A_CANT, 0);

	CLASS_ATTR_DEFAULT(c, "rect", 0, "0. 0. 420. 140.");

	CLASS_ATTR_DOUBLE(c, "fontsize", 0, t_py_code, font_size);
	CLASS_ATTR_ACCESSORS(c, "fontsize", NULL, (method)py_code_set_fontsize);
	CLASS_ATTR_DEFAULT_SAVE(c, "fontsize", 0, "12.");

	CLASS_ATTR_LONG(c, "tabwidth", 0, t_py_code, tabwidth);
	CLASS_ATTR_ACCESSORS(c, "tabwidth", NULL, (method)py_code_set_tabwidth);
	CLASS_ATTR_DEFAULT_SAVE(c, "tabwidth", 0, "4");

	CLASS_ATTR_LONG(c, "linenumbers", 0, t_py_code, linenumbers);
	CLASS_ATTR_STYLE(c, "linenumbers", 0, "onoff");
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "linenumbers", 0, "1");

	CLASS_ATTR_LONG(c, "readonly", 0, t_py_code, readonly);
	CLASS_ATTR_STYLE(c, "readonly", 0, "onoff");
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "readonly", 0, "0");

	CLASS_ATTR_LONG(c, "autoclose", 0, t_py_code, autoclose);
	CLASS_ATTR_STYLE(c, "autoclose", 0, "onoff");
	CLASS_ATTR_ACCESSORS(c, "autoclose", NULL, (method)py_code_set_autoclose);
	CLASS_ATTR_DEFAULT_SAVE(c, "autoclose", 0, "1");

	CLASS_ATTR_LONG(c, "autowrite", 0, t_py_code, autowrite);
	CLASS_ATTR_STYLE(c, "autowrite", 0, "onoff");
	CLASS_ATTR_ACCESSORS(c, "autowrite", NULL, (method)py_code_set_autowrite);
	CLASS_ATTR_DEFAULT_SAVE(c, "autowrite", 0, "0");

	CLASS_ATTR_LONG(c, "autoload", 0, t_py_code, autoload);
	CLASS_ATTR_STYLE(c, "autoload", 0, "onoff");
	CLASS_ATTR_DEFAULT_SAVE(c, "autoload", 0, "1");

	CLASS_ATTR_SYM(c, "file", 0, t_py_code, filename);
	CLASS_ATTR_ACCESSORS(c, "file", NULL, (method)py_code_set_file);

	CLASS_ATTR_SYM(c, "snippets", 0, t_py_code, snipfile);
	CLASS_ATTR_ACCESSORS(c, "snippets", NULL, (method)py_code_set_snippets);
	CLASS_ATTR_SAVE(c, "snippets", 0);
	class_addmethod(c, (method)py_code_snippets_msg, "snippets", A_GIMME, 0);

	CLASS_ATTR_LONG(c, "completion", 0, t_py_code, completion);
	CLASS_ATTR_STYLE(c, "completion", 0, "onoff");
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "completion", 0, "1");

	CLASS_ATTR_LONG(c, "titlebar", 0, t_py_code, titlebar);
	CLASS_ATTR_STYLE(c, "titlebar", 0, "onoff");
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "titlebar", 0, "1");

	CLASS_ATTR_LONG(c, "matchbrackets", 0, t_py_code, matchbrackets);
	CLASS_ATTR_STYLE(c, "matchbrackets", 0, "onoff");
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "matchbrackets", 0, "1");

	CLASS_ATTR_LONG(c, "wrap", 0, t_py_code, wrap);
	CLASS_ATTR_STYLE(c, "wrap", 0, "onoff");
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "wrap", 0, "1");

	CLASS_ATTR_LONG(c, "blink", 0, t_py_code, evalblink);
	CLASS_ATTR_STYLE(c, "blink", 0, "onoff");
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "blink", 0, "1");

	CLASS_ATTR_LONG(c, "blinkduration", 0, t_py_code, blinkduration);
	CLASS_ATTR_ACCESSORS(c, "blinkduration", NULL, (method)py_code_set_blinkduration);
	CLASS_ATTR_DEFAULT_SAVE(c, "blinkduration", 0, "500");

	CLASS_ATTR_DOUBLE(c, "scrollspeed", 0, t_py_code, scrollspeed);
	CLASS_ATTR_ACCESSORS(c, "scrollspeed", NULL, (method)py_code_set_scrollspeed);
	CLASS_ATTR_DEFAULT_SAVE(c, "scrollspeed", 0, "3.");
	class_addmethod(c, (method)py_code_scrollspeed_msg, "scrollspeed", A_FLOAT, 0);

	CLASS_ATTR_SYM(c, "theme", 0, t_py_code, theme);
	CLASS_ATTR_ACCESSORS(c, "theme", NULL, (method)py_code_set_theme);
	CLASS_ATTR_DEFAULT_SAVE(c, "theme", 0, "onedark");
	CLASS_ATTR_ENUM(c, "theme", 0, theme_menu.c_str());
	class_addmethod(c, (method)py_code_theme_msg, "theme", A_SYM, 0);
	class_addmethod(c, (method)py_code_themes_msg, "themes", 0);

	/* Defaults are the One Dark macros, the same strings the `onedark` theme row parses. */
	PY_COLOR_ATTR(c, "bgcolor", bgcolor, PY_THEME_ONEDARK_BG);
	PY_COLOR_ATTR(c, "textcolor", textcolor, PY_THEME_ONEDARK_TEXT);
	PY_COLOR_ATTR(c, "selcolor", selcolor, PY_THEME_ONEDARK_SEL);
	PY_COLOR_ATTR(c, "caretcolor", caretcolor, PY_THEME_ONEDARK_CARET);
	PY_COLOR_ATTR(c, "curlinecolor", curlinecolor, PY_THEME_ONEDARK_CURLINE);
	PY_COLOR_ATTR(c, "keywordcolor", keywordcolor, PY_THEME_ONEDARK_KEYWORD);
	PY_COLOR_ATTR(c, "builtincolor", builtincolor, PY_THEME_ONEDARK_BUILTIN);
	PY_COLOR_ATTR(c, "functioncolor", functioncolor, PY_THEME_ONEDARK_FUNCTION);
	PY_COLOR_ATTR(c, "classcolor", classcolor, PY_THEME_ONEDARK_CLASS);
	PY_COLOR_ATTR(c, "decoratorcolor", decoratorcolor, PY_THEME_ONEDARK_DECORATOR);
	PY_COLOR_ATTR(c, "stringcolor", stringcolor, PY_THEME_ONEDARK_STRING);
	PY_COLOR_ATTR(c, "escapecolor", escapecolor, PY_THEME_ONEDARK_ESCAPE);
	PY_COLOR_ATTR(c, "numbercolor", numbercolor, PY_THEME_ONEDARK_NUMBER);
	PY_COLOR_ATTR(c, "selfcolor", selfcolor, PY_THEME_ONEDARK_SELF);
	PY_COLOR_ATTR(c, "commentcolor", commentcolor, PY_THEME_ONEDARK_COMMENT);
	PY_COLOR_ATTR(c, "bracketcolor", bracketcolor, PY_THEME_ONEDARK_BRACKET);
	PY_COLOR_ATTR(c, "operatorcolor", operatorcolor, PY_THEME_ONEDARK_OPERATOR);
	PY_COLOR_ATTR(c, "matchcolor", matchcolor, PY_THEME_ONEDARK_MATCH);
	PY_COLOR_ATTR(c, "blinkcolor", blinkcolor, PY_THEME_ONEDARK_BLINK);
	PY_COLOR_ATTR(c, "barcolor", barcolor, PY_THEME_ONEDARK_BAR);
	PY_COLOR_ATTR(c, "barhighlight", barhighlight, PY_THEME_ONEDARK_BARHIGHLIGHT);

	class_register(CLASS_BOX, c);
	s_py_code_class = c;
	post("py_code %s", PY_CODE_VERSION);
}

static t_py_code *py_code_new(t_symbol *s, long argc, t_atom *argv) {
	t_dictionary *d = object_dictionaryarg(argc, argv);
	if (!d)
		return NULL;
	t_py_code *x = (t_py_code *)object_alloc(s_py_code_class);
	if (!x)
		return NULL;

	/* HILITE: takes focus on click (no focus, no keys). GROWBOTH: height resizes too. */
	const long flags = JBOX_DRAWFIRSTIN | JBOX_TRANSPARENT | JBOX_DRAWINLAST | JBOX_HILITE | JBOX_GROWBOTH;
	jbox_new(&x->j_box, flags, argc, argv);
	x->j_box.b_firstin = (t_object *)x;

	x->buf = new py_code::Buffer();
	x->syn = new py_code::Syntax();
	x->out_eval = outlet_new((t_object *)x, NULL);
	/* Values a setter may read before attr_dictionary_process applies them
	 * (`file` needs `autoload`). */
	x->linenumbers = 1;
	x->autoclose = 1;
	x->autoload = 1;
	x->snips = new PySnipState();
	x->completer = new py_code::Completer();
	x->compl_word = new std::string();
	x->completion = 1;
	x->autowrite_clock = clock_new(x, (method)py_code_autowrite_tick);
	x->titlebar = 1;
	x->matchbrackets = 1;
	x->wrap = 1;
	x->evalblink = 1;
	x->blinkduration = (long)py_code::Blink::kDefaultDuration;
	x->blink_clock = clock_new(x, (method)py_code_blink_tick);
	x->wraps.reset();
	x->blink_state.stop();
	x->blink_state.set_duration((double)x->blinkduration);
	x->buf->set_auto_close(true);
	x->tabwidth = 4;
	x->font_size = 12.;
	x->metrics_for = -1.;
	x->scroll = py_code::ScrollView();
	x->scrollspeed = 3.;

	attr_dictionary_process((t_object *)x, d);
	/* After the attributes: hand-set colors in `d` override the preset. */
	py_code_apply_theme(x, py_code::theme_find(x->theme ? x->theme->s_name : "onedark"), d);
	/* Text saved with the patch wins over @file. */
	py_code_jload(x, d);
	jbox_ready(&x->j_box);
	return x;
}

static void py_code_free(t_py_code *x) {
	if (x->blink_clock) {
		clock_unset(x->blink_clock);
		object_free(x->blink_clock);
	}
	if (x->autowrite_clock) {
		clock_unset(x->autowrite_clock);
		if (x->autowrite)
			py_code_write_file(x); /* a pending write is not lost */
		object_free(x->autowrite_clock);
	}
	jbox_free(&x->j_box);
	delete x->buf;
	delete x->syn;
	delete x->snips;
	delete x->completer;
	delete x->compl_word;
}

static void py_code_assist(t_py_code *x, void *b, long m, long a, char *s) {
	if (m == ASSIST_INLET)
		snprintf_zero(s, 256, "messages (theme, snippets, attributes)");
	else
		snprintf_zero(s, 256, "exec <code> / eval <expr>: connect to [py]");
}

/**********************************************************************/
/* paint                                                               */

static void py_code_update_metrics(t_py_code *x, t_jgraphics *g) {
	if (x->metrics_for == x->font_size && x->char_w > 0.)
		return;
	double w = 0., h = 0.;
	t_jgraphics_font_extents fe;
	jgraphics_text_measure(g, "M", &w, &h);
	jgraphics_font_extents(g, &fe);
	x->char_w = w > 0. ? w : x->font_size * 0.6;
	x->line_h = (fe.ascent + fe.descent) + 2.;
	x->ascent = fe.ascent;
	x->metrics_for = x->font_size;
}

/* Cells (code points) in a UTF-8 chunk. */
static int py_code_utf8_cells(const char *s, int len) {
	int cells = 0;
	for (int i = 0; i < len; ++i)
		if (((unsigned char)s[i] & 0xC0) != 0x80)
			++cells;
	return cells;
}

/* Bytes of the first `cells` cells of a UTF-8 chunk. */
static int py_code_utf8_bytes(const char *s, int len, int cells) {
	int i = 0, n = 0;
	while (i < len && n < cells) {
		++i;
		while (i < len && ((unsigned char)s[i] & 0xC0) == 0x80)
			++i;
		++n;
	}
	return i;
}

/* The scroll unit is the visual row (wrap), so `line_count` is the row count. */
static py_code::ScrollMetrics py_code_metrics(t_py_code *x) {
	x->wraps.ensure(*x->buf, x->text_cols > 0 ? x->text_cols : PY_NO_WRAP);
	py_code::ScrollMetrics m;
	m.line_count = x->wraps.row_count();
	m.line_h = x->line_h > 0. ? x->line_h : 16.;
	m.view_h = x->view_h - x->title_h - x->bar_h;
	if (m.view_h < 0.)
		m.view_h = 0.;
	m.pad_y = PAD_Y;
	return m;
}

static int py_code_digits(int n) {
	int d = 1;
	while (n >= 10) {
		n /= 10;
		++d;
	}
	return d;
}

static double py_code_gutter_width(t_py_code *x) {
	if (!x->linenumbers || x->char_w <= 0.)
		return 0.;
	return (double)py_code_digits(x->buf->line_count()) * x->char_w + GUTTER_GAP;
}

static double py_code_text_x(t_py_code *x) { return x->gutter_w + PAD_X; }

/* Room for the scroll bar is always reserved: otherwise its visibility would change
 * the width, the rows, and so its own visibility. */
static int py_code_cols_for_width(t_py_code *x, double width) {
	if (!x->wrap || x->char_w <= 0.)
		return PY_NO_WRAP;
	double avail = width - py_code_text_x(x) - (BAR_W + 2.);
	if (avail < x->char_w)
		avail = x->char_w;
	const int cols = (int)std::floor(avail / x->char_w);
	return cols < 1 ? 1 : cols;
}

static int py_code_row_of_y(t_py_code *x, double y) {
	if (x->line_h <= 0.)
		return 0;
	int row = (int)std::floor((y - x->title_h - PAD_Y + x->scroll.y) / x->line_h);
	if (row < 0)
		row = 0;
	if (row > x->wraps.row_count() - 1)
		row = x->wraps.row_count() - 1;
	return row;
}

static int py_code_row_of_offset(t_py_code *x, int offset) {
	const int line = x->buf->line_of_offset(offset);
	return x->wraps.row_of_line_cell(line, x->buf->display_col_of_offset(offset));
}

static const t_jrgba *py_code_token_color(t_py_code *x, py_code::TokenType type) {
	switch (type) {
	case py_code::Keyword: return &x->keywordcolor;
	case py_code::Builtin: return &x->builtincolor;
	case py_code::Function: return &x->functioncolor;
	case py_code::Class: return &x->classcolor;
	case py_code::Decorator: return &x->decoratorcolor;
	case py_code::String: return &x->stringcolor;
	case py_code::Escape: return &x->escapecolor;
	case py_code::Number: return &x->numbercolor;
	case py_code::Self: return &x->selfcolor;
	case py_code::Comment: return &x->commentcolor;
	case py_code::Bracket: return &x->bracketcolor;
	case py_code::Operator: return &x->operatorcolor;
	default: return &x->textcolor;
	}
}

/* Paint cannot move the caret into view (it only reads), so every edit or move does. */
static void py_code_caret_visible(t_py_code *x) {
	if (x->line_h <= 0. || x->view_h <= 0.)
		return;
	const py_code::ScrollMetrics m = py_code_metrics(x);
	x->scroll.ensure_line(py_code_row_of_offset(x, x->buf->caret()), m);
}

/* After any edit: caret visible and the debounced autowrite. */
static void py_code_after_edit(t_py_code *x) {
	py_code_caret_visible(x);
	py_code_autowrite_poke(x);
}

static bool py_code_thumb_rect(t_py_code *x, const py_code::ScrollMetrics &m, py_code::ScrollView::Bar *bar,
							   t_rect *out) {
	*bar = x->scroll.bar(m);
	if (!bar->visible)
		return false;
	out->x = x->view_w - BAR_W - 1.;
	out->y = x->title_h + PAD_Y + bar->thumb_y;
	out->width = BAR_W;
	out->height = bar->thumb_h;
	return true;
}

/* Background of the byte range [a, b), row by row (a range can span wrapped rows). */
static void py_code_paint_range(t_py_code *x, t_jgraphics *g, const py_code::ScrollMetrics &m, const t_jrgba &c,
								int a, int b, int first_row, int last_row) {
	if (b <= a || x->char_w <= 0.)
		return;
	const py_code::Buffer &buf = *x->buf;
	const double text_x = py_code_text_x(x);
	jgraphics_set_source_rgba(g, c.red, c.green, c.blue, c.alpha);
	for (int row = first_row; row <= last_row; ++row) {
		const int line = x->wraps.line_of_row(row);
		const py_code::LineWrap &lw = x->wraps.wrap(line);
		const int ril = row - x->wraps.first_row(line);
		const int cs = lw.cell_start(ril);
		const int ra = buf.offset_of_display_col(line, cs);
		const int rb = buf.offset_of_display_col(line, lw.cell_end(ril));
		const int le = buf.line_end(line);
		const double y = x->title_h + x->scroll.line_screen_y(row, m);
		const int from = a > ra ? a : ra;
		const int to = b < rb ? b : rb;
		if (to > from) {
			const double x1 = text_x + (double)(buf.display_col_of_offset(from) - cs) * x->char_w;
			const double x2 = text_x + (double)(buf.display_col_of_offset(to) - cs) * x->char_w;
			if (x2 > x1) {
				jgraphics_rectangle(g, x1, y, x2 - x1, x->line_h);
				jgraphics_fill(g);
			}
		}
		/* The '\n' in the range: a sliver after the last row of the line. */
		if (lw.cell_end(ril) == lw.cells() && a <= le && b > le) {
			jgraphics_rectangle(g, text_x + (double)(lw.cells() - cs) * x->char_w, y, x->char_w * 0.6, x->line_h);
			jgraphics_fill(g);
		}
	}
}

/* --- completion: automatic, display-only; Tab inserts, Shift-Tab cycles ---- */

static void py_code_compl_close(t_py_code *x) {
	x->compl_open = 0;
	x->compl_cycle = 0;
	x->compl_word->clear();
	x->compl_alternatives = 0;
}

static bool py_code_compl_refresh(t_py_code *x, int cycle, int min_prefix) {
	if (!x->completion)
		return false;
	const std::string &text = x->buf->text();
	x->completer->sync(text, x->buf->revision());
	py_code::Completion c;
	if (!x->completer->suggest(text, x->buf->caret(), cycle, min_prefix, c)) {
		py_code_compl_close(x);
		return false;
	}
	x->compl_open = 1;
	x->compl_cycle = c.index;
	x->compl_start = c.start;
	x->compl_end = c.end;
	*x->compl_word = c.word;
	x->compl_alternatives = c.alternatives;
	return true;
}

static void py_code_compl_accept(t_py_code *x) {
	const std::string word = *x->compl_word;
	const int len = x->buf->length();
	const int a = x->compl_start < 0 ? 0 : x->compl_start;
	const int b = x->compl_end > len ? len : (x->compl_end < a ? a : x->compl_end);
	py_code_compl_close(x);
	if (x->readonly)
		return;
	x->buf->set_selection(a, b);
	if (x->buf->insert_text(word))
		py_code_after_edit(x);
}

/* Reserved, never intermittent: toggling it would reflow the wrap and move the caret. */
static double py_code_bar_height(t_py_code *x) {
	return (x->completion && x->line_h > 0.) ? std::round(x->line_h) + 4. : 0.;
}

static void py_code_paint_bar(t_py_code *x, t_jgraphics *g, const t_rect &rect) {
	if (x->bar_h <= 0.)
		return;
	const double top = rect.height - x->bar_h;
	jgraphics_rectangle(g, 0., top, rect.width, x->bar_h);
	jgraphics_set_source_rgba(g, x->barcolor.red, x->barcolor.green, x->barcolor.blue, x->barcolor.alpha);
	jgraphics_fill(g);
	jgraphics_set_source_rgba(g, x->textcolor.red, x->textcolor.green, x->textcolor.blue, 0.12);
	jgraphics_rectangle(g, 0., top, rect.width, 1.);
	jgraphics_fill(g);
	if (!x->compl_open || x->compl_word->empty())
		return;

	const double text_x = py_code_text_x(x);
	const double right = rect.width - (BAR_W + 2.);
	const int cells = (x->char_w > 0. && right > text_x) ? (int)std::floor((right - text_x) / x->char_w) : 0;
	const double y = top + (x->bar_h - x->line_h) * 0.5 + x->ascent;
	const std::string counter = std::to_string(x->compl_cycle + 1) + "/" + std::to_string(x->compl_alternatives);
	jgraphics_set_source_rgba(g, x->barhighlight.red, x->barhighlight.green, x->barhighlight.blue,
							  x->barhighlight.alpha);
	jgraphics_move_to(g, text_x, y);
	jgraphics_show_text(g, x->compl_word->c_str());
	if (cells > (int)counter.size() + 1) { /* too narrow: the counter goes */
		jgraphics_set_source_rgba(g, x->textcolor.red, x->textcolor.green, x->textcolor.blue, 0.70);
		jgraphics_move_to(g, text_x + (double)(cells - (int)counter.size()) * x->char_w, y);
		jgraphics_show_text(g, counter.c_str());
	}
}

static void py_code_paint(t_py_code *x, t_object *view) {
	t_jgraphics *g = (t_jgraphics *)patcherview_get_jgraphics(view);
	t_rect rect;
	jbox_get_rect_for_view((t_object *)x, view, &rect);
	x->view_w = rect.width;
	x->view_h = rect.height;

	jgraphics_rectangle(g, 0., 0., rect.width, rect.height);
	jgraphics_set_source_rgba(g, x->bgcolor.red, x->bgcolor.green, x->bgcolor.blue, x->bgcolor.alpha);
	jgraphics_fill(g);

	jgraphics_select_font_face(g, "Menlo", JGRAPHICS_FONT_SLANT_NORMAL, JGRAPHICS_FONT_WEIGHT_NORMAL);
	jgraphics_set_font_size(g, x->font_size); /* every paint: the context belongs to the patcher */
	py_code_update_metrics(x, g);
	x->gutter_w = py_code_gutter_width(x);

	/* Rewrap at the width painted now: a resize reflows the text. */
	const int cols = py_code_cols_for_width(x, rect.width);
	const bool width_changed = x->text_cols > 0 && cols != x->text_cols;
	x->text_cols = cols;
	x->wraps.ensure(*x->buf, cols);

	x->title_h = (x->titlebar && x->line_h > 0.) ? x->line_h : 0.;
	x->bar_h = py_code_bar_height(x);
	if (x->title_h > 0.) {
		const double r = std::fmin(x->bgcolor.red + 0.07, 1.), gg = std::fmin(x->bgcolor.green + 0.07, 1.),
					 b = std::fmin(x->bgcolor.blue + 0.07, 1.);
		jgraphics_rectangle(g, 0., 0., rect.width, x->title_h);
		jgraphics_set_source_rgba(g, r, gg, b, 1.);
		jgraphics_fill(g);
		double tw = 0., th = 0.;
		jgraphics_text_measure(g, PY_TITLE_TEXT, &tw, &th);
		const double tx = std::fmax(rect.width - PAD_X - 2. - tw, PAD_X + 2.);
		jgraphics_set_source_rgba(g, x->textcolor.red, x->textcolor.green, x->textcolor.blue, 0.55);
		jgraphics_move_to(g, tx, x->ascent + 1.);
		jgraphics_show_text(g, PY_TITLE_TEXT);
		jgraphics_set_source_rgba(g, x->textcolor.red, x->textcolor.green, x->textcolor.blue, 0.12);
		jgraphics_rectangle(g, 0., x->title_h - 1., rect.width, 1.);
		jgraphics_fill(g);
	}

	py_code::ScrollMetrics m = py_code_metrics(x);
	x->scroll.clamp(m); /* an edit or resize can leave the offset past the end */
	if (width_changed)
		x->scroll.ensure_line(py_code_row_of_offset(x, x->buf->caret()), m);

	py_code::ScrollView::Bar bar;
	t_rect thumb;
	const bool has_bar = py_code_thumb_rect(x, m, &bar, &thumb);

	/* Current-line band, under the gutter numbers and the text. Alpha 0 disables it. */
	if (x->curlinecolor.alpha > 0.) {
		const int caret_row = py_code_row_of_offset(x, x->buf->caret());
		if (caret_row >= x->scroll.first_line(m) && caret_row <= x->scroll.last_line(m)) {
			jgraphics_rectangle(g, 0., x->title_h + x->scroll.line_screen_y(caret_row, m), rect.width, x->line_h);
			jgraphics_set_source_rgba(g, x->curlinecolor.red, x->curlinecolor.green, x->curlinecolor.blue,
									  x->curlinecolor.alpha);
			jgraphics_fill(g);
		}
	}

	const double text_x = py_code_text_x(x);
	const double clip_w = std::fmax(rect.width - text_x - (BAR_W + 2.), 1.);
	const double clip_h = std::fmax(rect.height - x->title_h - x->bar_h, 1.);
	jgraphics_save(g);
	jgraphics_clip(g, text_x, x->title_h, clip_w, clip_h);

	const py_code::Buffer &buf = *x->buf;
	const int first = x->scroll.first_line(m);
	const int last = x->scroll.last_line(m);
	py_code_sync(x);

	py_code_paint_range(x, g, m, x->selcolor, buf.selection_start(), buf.selection_end(), first, last);

	if (x->blink_state.active()) {
		const double a = x->blink_state.alpha(py_code_now_ms());
		const int b0 = x->blink_state.start_off();
		const int b1 = x->blink_state.end_off() < buf.length() ? x->blink_state.end_off() : buf.length();
		if (a > 0. && b1 > b0) {
			t_jrgba c = x->blinkcolor;
			c.alpha *= a;
			py_code_paint_range(x, g, m, c, b0, b1, first, last);
		}
	}

	if (x->matchbrackets) {
		const py_code::BracketMatch bm = x->syn->match_bracket(buf.caret());
		if (bm.valid()) {
			const int offs[2] = {bm.first, bm.second};
			jgraphics_set_source_rgba(g, x->matchcolor.red, x->matchcolor.green, x->matchcolor.blue,
									  x->matchcolor.alpha);
			for (int k = 0; k < 2; ++k) {
				const int line = buf.line_of_offset(offs[k]);
				const int row = py_code_row_of_offset(x, offs[k]);
				if (row < first || row > last)
					continue;
				const int cs = x->wraps.wrap(line).cell_start(x->wraps.row_in_line(row));
				const int col = buf.display_col_of_offset(offs[k]) - cs;
				jgraphics_rectangle(g, text_x + (double)col * x->char_w, x->title_h + x->scroll.line_screen_y(row, m),
									x->char_w, x->line_h);
				jgraphics_fill(g);
			}
		}
	}

	/* Text: visible rows only; each token is trimmed to the cells of its row. */
	std::string run;
	for (int row = first; row <= last; ++row) {
		const int line = x->wraps.line_of_row(row);
		const py_code::LineWrap &lw = x->wraps.wrap(line);
		const int ril = row - x->wraps.first_row(line);
		const int cs = lw.cell_start(ril);
		const int ce = lw.cell_end(ril);
		if (ce <= cs)
			continue;
		const std::string text = buf.line_text(line);
		const double y = x->title_h + x->scroll.line_screen_y(row, m) + x->ascent;
		int cell = 0;
		for (const py_code::Token &tk : x->syn->tokens(line)) {
			if (tk.col + tk.length > (int)text.size())
				break; /* out of sync: draw nothing rather than garbage */
			const char *p = text.c_str() + tk.col;
			const int n = py_code_utf8_cells(p, tk.length);
			if (cell + n > cs && cell < ce) {
				const int ca = cell > cs ? cell : cs;
				const int cb = (cell + n) < ce ? cell + n : ce;
				const int ba = py_code_utf8_bytes(p, tk.length, ca - cell);
				const int bb = py_code_utf8_bytes(p, tk.length, cb - cell);
				const t_jrgba *c = py_code_token_color(x, tk.type);
				run.assign(text, (size_t)(tk.col + ba), (size_t)(bb - ba));
				jgraphics_set_source_rgba(g, c->red, c->green, c->blue, c->alpha);
				jgraphics_move_to(g, text_x + (double)(ca - cs) * x->char_w, y);
				jgraphics_show_text(g, run.c_str());
			}
			cell += n;
			if (cell >= ce)
				break;
		}
	}

	const int caret = buf.caret();
	const int caret_line = buf.line_of_offset(caret);
	const int caret_cell = buf.display_col_of_offset(caret);
	const int caret_row = x->wraps.row_of_line_cell(caret_line, caret_cell);
	const int caret_cs = x->wraps.wrap(caret_line).cell_start(x->wraps.row_in_line(caret_row));
	if (x->focused) {
		jgraphics_rectangle(g, text_x + (double)(caret_cell - caret_cs) * x->char_w,
							x->title_h + x->scroll.line_screen_y(caret_row, m), 1.6, x->line_h);
		jgraphics_set_source_rgba(g, x->caretcolor.red, x->caretcolor.green, x->caretcolor.blue, x->caretcolor.alpha);
		jgraphics_fill(g);
	}

	if (buf.length() == 0) {
		jgraphics_set_source_rgba(g, x->textcolor.red, x->textcolor.green, x->textcolor.blue, 0.45);
		jgraphics_move_to(g, text_x, x->title_h + PAD_Y + x->ascent);
		jgraphics_show_text(g, "shift-ret line | cmd-ret block | cmd-shift-ret all");
	}

	jgraphics_restore(g);

	/* Gutter: the number goes on the first row of each line. */
	if (x->gutter_w > 0.) {
		jgraphics_set_source_rgba(g, x->textcolor.red, x->textcolor.green, x->textcolor.blue, 0.10);
		jgraphics_rectangle(g, x->gutter_w - 1., x->title_h, 1., rect.height - x->title_h - x->bar_h);
		jgraphics_fill(g);
		char num[16];
		for (int row = first; row <= last; ++row) {
			const int line = x->wraps.line_of_row(row);
			if (row != x->wraps.first_row(line))
				continue;
			snprintf(num, sizeof(num), "%d", line + 1);
			jgraphics_set_source_rgba(g, x->textcolor.red, x->textcolor.green, x->textcolor.blue,
									  line == caret_line ? 0.70 : 0.30);
			jgraphics_move_to(g, x->gutter_w - GUTTER_GAP * 0.5 - (double)py_code_digits(line + 1) * x->char_w,
							  x->title_h + x->scroll.line_screen_y(row, m) + x->ascent);
			jgraphics_show_text(g, num);
		}
	}

	if (has_bar) {
		jgraphics_set_source_rgba(g, COL_BAR_R, COL_BAR_G, COL_BAR_B, 0.10);
		jgraphics_rectangle(g, rect.width - BAR_W - 1., x->title_h + PAD_Y, BAR_W, bar.track_h);
		jgraphics_fill(g);
		jgraphics_set_source_rgba(g, COL_BAR_R, COL_BAR_G, COL_BAR_B, x->sb_drag ? 0.55 : 0.30);
		jgraphics_rectangle(g, thumb.x, thumb.y, thumb.width, thumb.height);
		jgraphics_fill(g);
	}

	py_code_paint_bar(x, g, rect);
}

static void py_code_getdrawparams(t_py_code *x, t_object *patcherview, t_jboxdrawparams *params) {
	if (x->focused) {
		params->d_bordercolor = x->caretcolor;
		params->d_bordercolor.alpha = 1.0;
		params->d_borderthickness = 2.;
	}
}

/**********************************************************************/
/* keyboard                                                            */
/* Everything arrives through `key` (a jbox without a textfield never gets
 * `keyfilter`). Decisions use keycode + modifiers: Cmd zeroes textcharacter and
 * Backspace/Delete share textcharacter 127. Return 1 = consumed. */

#define K_RETURN (-4)
#define K_TAB (-5)
#define K_ESC (-3)
#define K_DELETE (-6)
#define K_BACKSPACE (-7)
#define K_UPARROW (-9)
#define K_DOWNARROW (-10)
#define K_LEFTARROW (-11)
#define K_RIGHTARROW (-12)
#define K_PAGEUP (-13)
#define K_PAGEDOWN (-14)
#define K_HOME (-15)
#define K_END (-16)

static bool py_is_printable(long tc) {
	return tc > 0 && tc != 13 && tc != 9 && tc != 8 && tc != 27 && tc != 127;
}

static void py_code_copy(t_py_code *x) {
	if (x->buf->has_selection())
		py_code::clipboard_write_text(x->buf->selected_text());
}

static void py_code_cut(t_py_code *x) {
	if (!x->buf->has_selection())
		return;
	py_code::clipboard_write_text(x->buf->selected_text());
	x->buf->delete_selection();
}

static void py_code_paste(t_py_code *x) {
	const std::string text = py_code::clipboard_read_text();
	if (!text.empty())
		x->buf->insert_text(text);
}

/* Cmd shortcuts arrive with keycode = the ASCII letter. */
static bool py_code_command_key(t_py_code *x, long keycode, bool shift) {
	switch (keycode) {
	case 'a':
		x->buf->select_all();
		return true;
	case 'c':
		py_code_copy(x);
		return true;
	case 'v':
		if (!x->readonly)
			py_code_paste(x);
		return true;
	case 'x':
		if (!x->readonly)
			py_code_cut(x);
		return true;
	case 'z':
		if (!x->readonly)
			shift ? x->buf->redo() : x->buf->undo();
		return true;
	}
	return false;
}

static long py_code_key(t_py_code *x, t_object *patcherview, long keycode, long modifiers, long textcharacter) {
	const bool cmd = (modifiers & eCommandKey) != 0;
	const bool shift = (modifiers & eShiftKey) != 0;
	const bool alt = (modifiers & eAltKey) != 0;
	const bool ctrl = (modifiers & eControlKey) != 0;
	py_code::Buffer &buf = *x->buf;
	const int rev_before = buf.revision();

	if (cmd && py_code_command_key(x, keycode, shift)) {
		py_code_compl_close(x);
		if (buf.revision() != rev_before)
			py_code_after_edit(x);
		else
			py_code_caret_visible(x);
		jbox_redraw(&x->j_box);
		return 1;
	}

	/* Tab and Esc belong to an open completion; printable text refines it. */
	if (x->compl_open && keycode != K_TAB && keycode != K_ESC && (cmd || ctrl || !py_is_printable(textcharacter)))
		py_code_compl_close(x);

	switch (keycode) {
	case K_RETURN:
		if (cmd && shift) {
			py_code_eval_with_mode(x, py_code::EvalModeDocument);
			return 1;
		}
		if (cmd) {
			py_code_eval_with_mode(x, py_code::EvalModeBlock);
			return 1;
		}
		if (shift) {
			py_code_eval_with_mode(x, py_code::EvalModeLine);
			return 1;
		}
		if (ctrl || alt)
			return 0;
		if (x->readonly)
			return 1;
		py_code_sync(x);
		buf.insert_newline(std::string((size_t)x->syn->indent_at(buf.caret(), (int)x->tabwidth), ' '));
		break;

	case K_BACKSPACE:
		if (cmd)
			return 0;
		if (x->readonly)
			return 1;
		if (alt)
			buf.move_word_left(true);
		buf.backspace();
		break;

	case K_DELETE:
		if (cmd)
			return 0;
		if (x->readonly)
			return 1;
		if (alt)
			buf.move_word_right(true);
		buf.delete_forward();
		break;

	case K_LEFTARROW:
		cmd ? buf.move_line_start(shift) : alt ? buf.move_word_left(shift) : buf.move_left(shift);
		break;
	case K_RIGHTARROW:
		cmd ? buf.move_line_end(shift) : alt ? buf.move_word_right(shift) : buf.move_right(shift);
		break;
	case K_UPARROW:
		(cmd || alt) ? buf.move_document_start(shift) : buf.move_up(shift);
		break;
	case K_DOWNARROW:
		(cmd || alt) ? buf.move_document_end(shift) : buf.move_down(shift);
		break;

	case K_PAGEUP:
	case K_PAGEDOWN: {
		/* One page = visible rows minus one; the column inside the row is kept. */
		const py_code::ScrollMetrics m = py_code_metrics(x);
		const int page = std::max(1, x->scroll.visible_lines(m) - 1);
		const int caret_line = buf.line_of_offset(buf.caret());
		const int caret_cell = buf.display_col_of_offset(buf.caret());
		const int row = x->wraps.row_of_line_cell(caret_line, caret_cell);
		const int col_in_row = caret_cell - x->wraps.wrap(caret_line).cell_start(x->wraps.row_in_line(row));
		const int target = std::max(0, std::min(row + (keycode == K_PAGEUP ? -page : page), x->wraps.row_count() - 1));
		const int line = x->wraps.line_of_row(target);
		const py_code::LineWrap &lw = x->wraps.wrap(line);
		const int ril = x->wraps.row_in_line(target);
		const int cell = std::min(lw.cell_start(ril) + col_in_row, lw.cell_end(ril));
		buf.move_caret(buf.offset_of_display_col(line, cell), shift);
		break;
	}

	case K_HOME:
		buf.move_line_start(shift);
		break;
	case K_END:
		buf.move_line_end(shift);
		break;

	case K_TAB:
		if (x->compl_open) {
			if (shift)
				py_code_compl_refresh(x, x->compl_cycle - 1, PY_COMPL_MIN_PREFIX);
			else
				py_code_compl_accept(x);
			jbox_redraw(&x->j_box);
			return 1;
		}
		if (x->readonly)
			return 1;
		shift ? buf.outdent((int)x->tabwidth) : buf.insert_tab((int)x->tabwidth);
		break;

	case K_ESC:
		if (x->compl_open) {
			py_code_compl_close(x);
			jbox_redraw(&x->j_box);
			return 1;
		}
		return 0;

	default:
		if (cmd || ctrl)
			return 0; /* Max's own shortcuts (Cmd-E, Cmd-S, ...) */
		if (x->readonly)
			return 1;
		if (!py_is_printable(textcharacter))
			return 0; /* IME / dead keys are not handled */
		buf.insert_char((unsigned int)textcharacter);
		/* A closer or `else:`-style colon may move the line to its block's level;
		 * coalesced with the typing burst (no undo step of its own). */
		if (textcharacter == ')' || textcharacter == ']' || textcharacter == '}' || textcharacter == ':') {
			py_code_sync(x);
			const int indent = x->syn->reindent_at(buf.caret(), (int)x->tabwidth);
			if (indent >= 0)
				buf.reindent_line(buf.line_of_offset(buf.caret()), std::string((size_t)indent, ' '));
		}
		break;
	}

	if (!x->readonly && !cmd && !ctrl && py_is_printable(textcharacter))
		py_code_compl_refresh(x, 0, PY_COMPL_MIN_PREFIX);

	if (buf.revision() != rev_before)
		py_code_after_edit(x);
	else
		py_code_caret_visible(x);
	jbox_redraw(&x->j_box);
	return 1;
}

/**********************************************************************/
/* focus / mouse                                                       */

static void py_code_focusgained(t_py_code *x, t_object *patcherview) {
	x->focused = 1;
	jbox_redraw(&x->j_box);
}

static void py_code_focuslost(t_py_code *x, t_object *patcherview) {
	x->focused = 0;
	jbox_redraw(&x->j_box);
}

/* Box-local point -> buffer offset, with the metrics of the last paint. */
static int py_code_offset_at(t_py_code *x, t_pt pt) {
	if (x->char_w <= 0. || x->line_h <= 0.)
		return x->buf->caret();
	(void)py_code_metrics(x);
	const int row = py_code_row_of_y(x, pt.y);
	const int line = x->wraps.line_of_row(row);
	const py_code::LineWrap &lw = x->wraps.wrap(line);
	const int ril = x->wraps.row_in_line(row);
	const int cs = lw.cell_start(ril), ce = lw.cell_end(ril);
	int col = cs + (int)std::floor((pt.x - py_code_text_x(x)) / x->char_w + 0.5);
	col = std::max(cs, std::min(col, ce));
	return x->buf->offset_of_display_col(line, col);
}

/* Dragging past the text area scrolls, faster the further away. */
static void py_code_autoscroll_drag(t_py_code *x, t_pt *pt) {
	if (x->line_h <= 0. || x->view_h <= 0.)
		return;
	const double top = x->title_h + PAD_Y;
	const double bottom = x->view_h - x->bar_h - PAD_Y;
	if (pt->y >= top && pt->y <= bottom)
		return;
	const py_code::ScrollMetrics m = py_code_metrics(x);
	x->scroll.scroll_by(pt->y < top ? pt->y - top : pt->y - bottom, m);
	pt->y = pt->y < top ? top : bottom;
}

/* --- context menu ------------------------------------------------------- */

enum {
	PY_MENU_UNDO = 1,
	PY_MENU_REDO,
	PY_MENU_CUT,
	PY_MENU_COPY,
	PY_MENU_PASTE,
	PY_MENU_DELETE,
	PY_MENU_SELECTALL,
	PY_MENU_SAVE,
	PY_MENU_LOAD
};

static std::string py_code_default_name(t_py_code *x) {
	if (py_code_has_file(x)) {
		const char *slash = std::strrchr(x->filename->s_name, '/');
		const char *base = slash ? slash + 1 : x->filename->s_name;
		if (base[0])
			return std::string(base);
	}
	return std::string("py_code.py");
}

/* Sets @file without the autoload that would overwrite the document. */
static void py_code_set_file_quiet(t_py_code *x, const char *path) {
	const long autoload = x->autoload;
	x->autoload = 0;
	object_attr_setsym((t_object *)x, gensym("file"), gensym(path));
	x->autoload = autoload;
}

static void py_code_menu_save(t_py_code *x) {
	if (py_code_has_file(x)) {
		if (py_code_write_file(x))
			object_post((t_object *)x, "saved: %s", x->filename->s_name);
		return;
	}
	const std::string path = py_code::dialog_save_code(py_code_default_name(x));
	if (!path.empty() && py_code_save_to_path(x, path.c_str())) {
		py_code_set_file_quiet(x, path.c_str());
		object_post((t_object *)x, "saved: %s (kept as @file)", path.c_str());
	}
}

static void py_code_menu_load(t_py_code *x) {
	const std::string path = py_code::dialog_open_code();
	if (!path.empty() && py_code_load_from_path(x, path.c_str())) {
		py_code_set_file_quiet(x, path.c_str());
		object_post((t_object *)x, "loaded: %s (kept as @file)", path.c_str());
	}
}

static void py_code_menu_action(t_py_code *x, int item) {
	py_code::Buffer &buf = *x->buf;
	const bool ro = x->readonly != 0;
	const int rev_before = buf.revision();
	switch (item) {
	case PY_MENU_UNDO:
		if (!ro)
			buf.undo();
		break;
	case PY_MENU_REDO:
		if (!ro)
			buf.redo();
		break;
	case PY_MENU_CUT:
		if (!ro)
			py_code_cut(x);
		break;
	case PY_MENU_COPY:
		py_code_copy(x);
		break;
	case PY_MENU_PASTE:
		if (!ro)
			py_code_paste(x);
		break;
	case PY_MENU_DELETE:
		if (!ro)
			buf.has_selection() ? (void)buf.delete_selection() : (void)buf.delete_forward();
		break;
	case PY_MENU_SELECTALL:
		buf.select_all();
		break;
	case PY_MENU_SAVE:
		py_code_menu_save(x);
		break;
	case PY_MENU_LOAD:
		py_code_menu_load(x);
		break;
	default:
		return;
	}
	if (buf.revision() != rev_before && item != PY_MENU_LOAD)
		py_code_after_edit(x);
	else
		py_code_caret_visible(x);
	jbox_redraw(&x->j_box);
}

static void py_code_popup_menu(t_py_code *x, t_object *patcherview, t_pt pt) {
	t_jpopupmenu *menu = jpopupmenu_create();
	if (!menu)
		return;
	const bool ro = x->readonly != 0;
	const bool sel = x->buf->has_selection();
	jpopupmenu_additem(menu, PY_MENU_UNDO, "Undo", NULL, 0, ro || !x->buf->can_undo(), NULL);
	jpopupmenu_additem(menu, PY_MENU_REDO, "Redo", NULL, 0, ro || !x->buf->can_redo(), NULL);
	jpopupmenu_addseparator(menu);
	jpopupmenu_additem(menu, PY_MENU_CUT, "Cut", NULL, 0, ro || !sel, NULL);
	jpopupmenu_additem(menu, PY_MENU_COPY, "Copy", NULL, 0, !sel, NULL);
	jpopupmenu_additem(menu, PY_MENU_PASTE, "Paste", NULL, 0, ro, NULL);
	jpopupmenu_additem(menu, PY_MENU_DELETE, "Delete", NULL, 0, ro, NULL);
	jpopupmenu_additem(menu, PY_MENU_SELECTALL, "Select All", NULL, 0, x->buf->length() == 0, NULL);
	jpopupmenu_addseparator(menu);
	jpopupmenu_additem(menu, PY_MENU_SAVE, py_code_has_file(x) ? "Save to .py" : "Save to .py...", NULL, 0, 0, NULL);
	jpopupmenu_additem(menu, PY_MENU_LOAD, "Load from .py...", NULL, 0, ro, NULL);

	/* `pt` is box-local; the menu wants screen coordinates. Mapping two canvas
	 * corners gives the zoom, so a zoomed patcher still opens it under the cursor. */
	t_rect rect;
	long ox = 0, oy = 0, fx = 0, fy = 0;
	if (jbox_get_rect_for_view((t_object *)x, patcherview, &rect) != MAX_ERR_NONE)
		rect.x = rect.y = 0.;
	patcherview_canvas_to_screen(patcherview, rect.x, rect.y, &ox, &oy);
	patcherview_canvas_to_screen(patcherview, rect.x + rect.width, rect.y + rect.height, &fx, &fy);
	const double zx = rect.width > 0. ? (double)(fx - ox) / rect.width : 1.;
	const double zy = rect.height > 0. ? (double)(fy - oy) / rect.height : zx;
	t_pt where;
	where.x = (double)ox + pt.x * zx;
	where.y = (double)oy + pt.y * zy;
	const int item = jpopupmenu_popup(menu, where, 0);
	jpopupmenu_destroy(menu);
	if (item > 0)
		py_code_menu_action(x, item);
}

static void py_code_mouseenter(t_py_code *x, t_object *patcherview, t_pt pt, long modifiers) {
	jmouse_setcursor(patcherview, (t_object *)x, JMOUSE_CURSOR_IBEAM);
}

static long py_code_mousedown(t_py_code *x, t_object *patcherview, t_pt pt, long modifiers) {
	py_code::Buffer &buf = *x->buf;
	py_code_compl_close(x);

	if (modifiers & eRightButton) {
		py_code_popup_menu(x, patcherview, pt);
		return 1;
	}
	if (x->bar_h > 0. && pt.y >= x->view_h - x->bar_h) {
		x->bar_grab = 1; /* the bottom bar is not text */
		return 0;
	}
	if (pt.y < x->title_h)
		return 0;

	{ /* scroll bar: grab the thumb or jump to the click */
		const py_code::ScrollMetrics m = py_code_metrics(x);
		py_code::ScrollView::Bar bar;
		t_rect thumb;
		if (py_code_thumb_rect(x, m, &bar, &thumb) && pt.x >= x->view_w - BAR_W - 2.) {
			const double thumb_top = x->title_h + PAD_Y + bar.thumb_y;
			if (pt.y >= thumb_top && pt.y <= thumb_top + bar.thumb_h) {
				x->sb_grab = pt.y - thumb_top;
			} else {
				x->sb_grab = bar.thumb_h * 0.5;
				x->scroll.drag_thumb_to(pt.y - x->title_h - PAD_Y - x->sb_grab, m);
			}
			x->sb_drag = 1;
			x->dragging = 0;
			jbox_redraw(&x->j_box);
			return 0;
		}
	}

	/* Click count cycles 1 -> 2 -> 3 within 400 ms and 4 px. */
	const double now = py_code_now_ms();
	const bool near =
		std::fabs(pt.x - (double)x->last_click_x) <= 4. && std::fabs(pt.y - (double)x->last_click_y) <= 4.;
	x->click_count = (x->click_count > 0 && near && (now - x->last_click_ms) < 400.) ? (x->click_count % 3) + 1 : 1;
	x->last_click_ms = now;
	x->last_click_x = (long)pt.x;
	x->last_click_y = (long)pt.y;

	const int offset = py_code_offset_at(x, pt);
	int a = 0, b = 0;
	if (modifiers & eShiftKey) {
		buf.move_caret(offset, true);
	} else if (x->click_count == 2) {
		buf.word_range_at(offset, a, b);
		buf.set_selection(a, b);
	} else if (x->click_count == 3) {
		buf.line_range_at(offset, a, b);
		buf.set_selection(a, b);
	} else {
		buf.set_caret(offset);
	}
	x->dragging = 1;
	jbox_redraw(&x->j_box);
	return 0;
}

static void py_code_mousedrag(t_py_code *x, t_object *patcherview, t_pt pt, long modifiers) {
	if (x->bar_grab)
		return;
	if (x->sb_drag) {
		x->scroll.drag_thumb_to(pt.y - x->title_h - PAD_Y - x->sb_grab, py_code_metrics(x));
		jbox_redraw(&x->j_box);
		return;
	}
	x->dragging = 1;
	py_code_autoscroll_drag(x, &pt);
	x->buf->move_caret(py_code_offset_at(x, pt), true);
	jbox_redraw(&x->j_box);
}

static void py_code_mouseup(t_py_code *x, t_object *patcherview, t_pt pt, long modifiers) {
	x->dragging = 0;
	x->sb_drag = 0;
	x->bar_grab = 0;
	jbox_redraw(&x->j_box);
}

/* delta_y > 0 = wheel up = content moves down. Scrolls in pixels. */
static void py_code_mousewheel(t_py_code *x, t_object *patcherview, t_pt pt, long modifiers, double dx, double dy) {
	if (x->line_h <= 0. || x->view_h <= 0.)
		return;
	const py_code::ScrollMetrics m = py_code_metrics(x);
	if (!x->scroll.scrollable(m))
		return;
	x->scroll.scroll_by(-dy * x->line_h * x->scrollspeed, m);
	jbox_redraw(&x->j_box);
}
