// SPDX-License-Identifier: MIT
/* py_repl.cpp -- the py_repl Max class: attributes, lifecycle, link to [py],
 * runs, completion, history and persistence. Painting and input live in
 * py_repl_paint.cpp and py_repl_input.cpp. Behaviour: docs/dev/py_repl-spec.md 3. */

#include "py_repl.h"

#include "ext_systime.h"
#include "jpatcher_syms.h"
#include "platform/clipboard.h"
#include "py_repl_abi.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdint>

using namespace pyc;

static t_class *s_class = nullptr;
static long s_next_id = 1;

static const char *const kColorAttr[RoleCount] = {
	"bgcolor",		"textcolor",	  "selcolor",	  "caretcolor",	 "curlinecolor",  "keywordcolor",
	"builtincolor", "functioncolor",  "classcolor",	  "decoratorcolor", "stringcolor", "escapecolor",
	"numbercolor",	"selfcolor",	  "commentcolor", "bracketcolor", "operatorcolor", "matchcolor",
	"blinkcolor",	"barcolor",		  "barhighlight", "errorcolor",	 "warningcolor"};

static std::unordered_map<long, t_py_repl *> &boxes() {
	static auto *m = new std::unordered_map<long, t_py_repl *>;
	return *m;
}

t_py_repl *pyc::find_box(long id) {
	auto it = boxes().find(id);
	return it == boxes().end() ? nullptr : it->second;
}

/* ---- small helpers ---- */

static std::string sym_str(t_symbol *s) { return s && s->s_name ? s->s_name : ""; }

/* Message arguments as code: one symbol verbatim, else atoms joined with spaces (as py's eval). */
static std::string code_text(long argc, t_atom *argv) {
	if (argc == 1 && atom_gettype(argv) == A_SYM)
		return sym_str(atom_getsym(argv));
	long size = 0;
	char *text = nullptr;
	const long flags = OBEX_UTIL_ATOM_GETTEXT_SYM_NO_QUOTE | OBEX_UTIL_ATOM_GETTEXT_NOESCAPE |
					   OBEX_UTIL_ATOM_GETTEXT_NUM_HI_RES;
	std::string out;
	if (atom_gettext(argc, argv, &size, &text, flags) == MAX_ERR_NONE && text)
		out = text;
	if (text)
		sysmem_freeptr(text);
	return out;
}

static std::string normalize_newlines(const std::string &s) {
	std::string o;
	o.reserve(s.size());
	for (size_t i = 0; i < s.size(); ++i) {
		if (s[i] == '\r') {
			o += '\n';
			if (i + 1 < s.size() && s[i + 1] == '\n')
				++i;
		} else
			o += s[i];
	}
	return o;
}

static std::string rtrim(const std::string &s) {
	const size_t k = s.find_last_not_of(" \t\r\n");
	return k == std::string::npos ? "" : s.substr(0, k + 1);
}

int pyc::cells_between(const std::string &s, int a, int b) {
	int n = 0;
	for (int p = a; p < b && p < (int)s.size(); p = utf8_next(s, p))
		++n;
	return n;
}

void pyc::redraw(t_py_repl *x) { jbox_redraw((t_jbox *)x); }

t_jrgba pyc::color(t_py_repl *x, int role, double alpha_mul) {
	t_jrgba c = x->colors[role];
	c.alpha *= alpha_mul;
	return c;
}

Editor &pyc::part_editor(t_py_repl *x, int part) { return part == PartInput ? x->st->in_ed : x->st->tr_ed; }
Layout &pyc::part_layout(t_py_repl *x, int part) { return part == PartInput ? x->st->in_layout : x->st->tr_layout; }

/* ---- metrics and geometry ---- */

void pyc::ensure_metrics(t_py_repl *x) {
	Metrics &m = x->st->m;
	const double size = std::max(6.0, std::min(48.0, x->fontsize));
	if (m.font && m.size == size)
		return;
	if (m.font)
		jfont_destroy(m.font);
	m.font = jfont_create("Menlo", JGRAPHICS_FONT_SLANT_NORMAL, JGRAPHICS_FONT_WEIGHT_NORMAL, size);
	m.size = size;
	double ascent = size * 0.8, descent = size * 0.2;
	m.cw = size * 0.6;
	if (m.font) {
		t_jgraphics_font_extents e;
		jfont_extents(m.font, &e);
		double w = 0, h = 0;
		jfont_text_measure(m.font, "0000000000", &w, &h);
		if (w > 0)
			m.cw = w / 10;
		if (e.ascent > 0 && e.descent >= 0) {
			ascent = e.ascent;
			descent = e.descent;
		}
	}
	m.lh = std::ceil(ascent + descent) + 3;
	m.base = (m.lh - ascent - descent) / 2 + ascent;
}

Geom pyc::geometry(t_py_repl *x, double w, double h) {
	ensure_metrics(x);
	const Metrics &m = x->st->m;
	Geom g;
	g.w = w;
	g.h = h;
	g.title_h = x->titlebar ? m.lh : 0;
	g.prompt_x = kPadX;
	g.text_x = kPadX + kPromptCells * m.cw;
	g.sb_x = w - kScrollW;
	g.text_r = g.sb_x - 2; /* always reserves the scroll bar */
	g.view_y = g.title_h;
	g.view_h = std::max(0.0, h - g.title_h);
	g.cols = x->wrap ? std::max(1, (int)std::floor((g.text_r - g.text_x) / m.cw)) : 0;
	return g;
}

Geom pyc::sync_layout(t_py_repl *x) {
	State &st = *x->st;
	const Geom g = geometry(x, st.box_w, st.box_h);
	if (st.tr_rev != st.tr.revision()) {
		st.tr_ed.reset(st.tr.text());
		st.tr_rev = st.tr.revision();
	}
	st.tr_layout.update(st.tr_ed, g.cols);
	st.in_layout.update(st.in_ed, g.cols);
	st.stack.scroll().set_view(g.view_h, st.m.lh, kPadY);
	st.stack.update(content_rows(st.tr_ed, st.tr_layout), st.in_layout.row_count());
	return g;
}

void pyc::view_size(t_py_repl *x, t_object *patcherview) {
	t_rect r;
	if (patcherview && jbox_get_rect_for_view((t_object *)x, patcherview, &r) == MAX_ERR_NONE) {
		x->st->box_w = r.width;
		x->st->box_h = r.height;
	}
}

void pyc::reveal_caret(t_py_repl *x) {
	State &st = *x->st;
	sync_layout(x);
	const int part = st.focus;
	const Editor &ed = part_editor(x, part);
	const Layout &L = part_layout(x, part);
	st.stack.scroll().ensure_row_visible(st.stack.row_of(part, L.row_of(ed.caret())));
}

const std::vector<std::vector<py_code::Token>> &pyc::span_tokens(t_py_repl *x, const std::string &code) {
	State &st = *x->st;
	auto it = st.tokens.find(code);
	if (it != st.tokens.end())
		return it->second;
	if (st.tokens.size() > 4096)
		st.tokens.clear(); /* bound the cache: entries are rebuilt on demand */
	st.scratch.update(code, ++st.scratch_rev);
	std::vector<std::vector<py_code::Token>> lines;
	for (int l = 0; l < st.scratch.line_count(); ++l)
		lines.push_back(st.scratch.tokens(l));
	return st.tokens.emplace(code, std::move(lines)).first->second;
}

static void set_dirty(t_py_repl *x) {
	if (t_object *p = jbox_get_patcher((t_object *)x))
		jpatcher_set_dirty(p, 1);
}

/* ---- link to [py] ---- */

using AbiFn = void *(*)(t_object *);
using RunFn = void *(*)(t_object *, const char *, py_repl_sink *);
using CompleteFn = void *(*)(t_object *, const char *);
using NamesFn = void *(*)(t_object *, const char *, py_repl_sink *);
using InterruptFn = void *(*)(t_object *);

template <class F> static F py_method(t_object *o, const char *name) {
	return (F)object_getmethod(o, gensym(name));
}

/* Looked up on every use, never stored: a py object can be freed at any time. */
t_object *pyc::target(t_py_repl *x, Link *link) {
	State &st = *x->st;
	t_object *o = (t_object *)object_findregistered(CLASS_BOX, x->py);
	if (!o || object_classname(o) != gensym("py")) {
		if (link)
			*link = Link::NotFound;
		return nullptr;
	}
	if (st.abi_for != x->py) {
		st.abi_for = x->py;
		AbiFn abi = py_method<AbiFn>(o, "repl_abi");
		const long v = abi ? (long)(intptr_t)abi(o) : 0;
		st.abi_ok = v == PY_REPL_ABI;
		if (!st.abi_ok)
			object_error((t_object *)x, "py '%s': repl abi %ld, expected %d (rebuild py)", sym_str(x->py).c_str(), v,
						 PY_REPL_ABI);
	}
	if (!st.abi_ok) {
		if (link)
			*link = Link::AbiMismatch;
		return nullptr;
	}
	if (link)
		*link = Link::Ok;
	return o;
}

Completeness pyc::completeness(t_py_repl *x, const std::string &code) {
	t_object *o = target(x, nullptr);
	CompleteFn fn = o ? py_method<CompleteFn>(o, "repl_complete") : nullptr;
	if (!fn)
		return Completeness::Unknown;
	switch ((long)(intptr_t)fn(o, code.c_str())) {
	case PY_REPL_COMPLETE:
		return Completeness::Complete;
	case PY_REPL_INCOMPLETE:
		return Completeness::Incomplete;
	default:
		return Completeness::Invalid;
	}
}

/* ---- runs ---- */

static void run_emit(void *ctx, long kind, const char *text, long len) {
	Kind k = Kind::Traceback;
	switch (kind) {
	case PY_REPL_OUT: k = Kind::Out; break;
	case PY_REPL_ERR: k = Kind::Err; break;
	case PY_REPL_RESULT: k = Kind::Result; break;
	default: break;
	}
	static_cast<Transcript *>(ctx)->append(k, std::string(text, (size_t)len));
}

void pyc::submit_code(t_py_repl *x, const std::string &raw) {
	State &st = *x->st;
	const std::string code = rtrim(normalize_newlines(raw));
	if (code.empty())
		return;
	if (st.running) {
		st.tr.info("busy: a run is already active");
		redraw(x);
		return;
	}
	Link link = Link::Ok;
	t_object *o = target(x, &link);
	RunFn run = o ? py_method<RunFn>(o, "repl_run") : nullptr;
	if (!run) {
		st.tr.info(link == Link::AbiMismatch ? "py '" + sym_str(x->py) + "': repl interface mismatch"
											 : "py '" + sym_str(x->py) + "' not found");
		redraw(x);
		return;
	}
	st.tr.append_input(code);
	st.hist.add(code);
	if (x->savehistory)
		set_dirty(x); /* the saved history changed */

	InterruptFn interrupt = py_method<InterruptFn>(o, "repl_interrupt");
	if (interrupt)
		st.monitor.start([o, interrupt] { interrupt(o); }); /* o lives: frees happen on this busy thread */
	py_repl_sink sink = {(long)sizeof(py_repl_sink), &st.tr, run_emit};
	st.running = true;
	const long status = (long)(intptr_t)run(o, code.c_str(), &sink);
	st.running = false;
	st.monitor.stop();
	st.run_end = systime_ms();

	if (status == PY_REPL_BUSY)
		st.tr.info("busy: a run is already active");
	st.tr.end_run();
	st.focus = PartInput;
	sync_layout(x);
	st.stack.scroll().set_scroll(st.stack.scroll().max_scroll());
	redraw(x);
}

void pyc::submit_input(t_py_repl *x) {
	State &st = *x->st;
	const std::string code = st.in_ed.text();
	st.comp.close();
	st.hist.reset();
	if (rtrim(code).empty())
		return;
	if (!target(x, nullptr)) { /* keep the input: the user can fix @py and retry */
		submit_code(x, code);
		return;
	}
	st.in_ed.reset("");
	submit_code(x, code);
}

void pyc::interrupt_input(t_py_repl *x) {
	State &st = *x->st;
	if (systime_ms() - st.run_end < 300)
		return; /* the Ctrl+C that stopped a run, delivered after it */
	const std::string code = rtrim(st.in_ed.text());
	if (!code.empty())
		st.tr.append_input(code);
	st.tr.info("KeyboardInterrupt");
	st.in_ed.reset("");
	st.hist.reset();
	st.comp.close();
	redraw(x);
}

void pyc::clear_transcript(t_py_repl *x) {
	x->st->tr.clear();
	x->st->tokens.clear();
	redraw(x);
}

/* ---- history ---- */

void pyc::history_step(t_py_repl *x, bool up) {
	State &st = *x->st;
	std::string s;
	const bool moved = up ? st.hist.prev(st.in_ed.text(), &s) : st.hist.next(&s);
	if (!moved)
		return;
	st.in_ed.reset(s);
	st.in_ed.set_caret(st.in_ed.length());
	st.comp.close();
}

/* ---- completion ---- */

static bool is_word(char c) { return std::isalnum((unsigned char)c) || c == '_'; }

static void names_emit(void *ctx, long kind, const char *text, long len) {
	static_cast<std::vector<std::string> *>(ctx)->emplace_back(text, (size_t)len);
}

bool pyc::complete_open(t_py_repl *x) {
	State &st = *x->st;
	st.comp.close();
	const std::string &t = st.in_ed.text();
	const int caret = st.in_ed.caret();
	int ds = caret; /* start of the dotted name before the caret */
	while (ds > 0 && (is_word(t[(size_t)ds - 1]) || t[(size_t)ds - 1] == '.'))
		--ds;
	if (ds == caret)
		return false;
	const std::string text = t.substr((size_t)ds, (size_t)(caret - ds));

	std::vector<CompletionItem> items;
	std::vector<std::string> names;
	t_object *o = target(x, nullptr);
	if (NamesFn fn = o ? py_method<NamesFn>(o, "repl_names") : nullptr) {
		py_repl_sink sink = {(long)sizeof(py_repl_sink), &names, names_emit};
		fn(o, text.c_str(), &sink);
	}
	for (const std::string &n : names) {
		CompletionItem it;
		const size_t dot = n.rfind('.');
		it.label = dot == std::string::npos ? n : n.substr(dot + 1);
		it.insert = n;
		it.replace_start = ds; /* the whole dotted name is replaced */
		it.replace_end = caret;
		items.push_back(std::move(it));
	}
	if (text.find('.') == std::string::npos) { /* names typed before, which may not exist yet */
		st.words.sync(st.in_ed.text() + "\n" + st.tr.text(), ++st.words_rev);
		for (CompletionItem &w : st.words.query(text, 200)) {
			const bool dup = std::any_of(items.begin(), items.end(),
										 [&](const CompletionItem &i) { return i.label == w.label; });
			if (!dup)
				items.push_back(std::move(w));
		}
	}
	return st.comp.open(t, caret, std::move(items));
}

void pyc::complete_accept(t_py_repl *x) {
	State &st = *x->st;
	const CompletionEdit e = st.comp.accept();
	if (e.start < 0)
		return;
	st.in_ed.select(e.start, e.end);
	st.in_ed.insert(e.text);
}

/* ---- clipboard ---- */

void pyc::paste(t_py_repl *x) {
	const std::string s = normalize_newlines(clipboard_text());
	if (!s.empty())
		x->st->in_ed.insert(s);
}

void pyc::copy(t_py_repl *x, bool cut) {
	State &st = *x->st;
	Editor &ed = part_editor(x, st.focus);
	if (!ed.has_selection())
		return;
	set_clipboard_text(ed.selected_text());
	if (cut && st.focus == PartInput)
		ed.insert("");
}

/* ---- attribute setters ---- */

static void apply_theme(t_py_repl *x, const Theme &t, t_dictionary *keep) {
	for (int i = 0; i < RoleCount; ++i) {
		if (keep && dictionary_hasentry(keep, gensym(kColorAttr[i])))
			continue;
		x->colors[i].red = t[i].r;
		x->colors[i].green = t[i].g;
		x->colors[i].blue = t[i].b;
		x->colors[i].alpha = t[i].a;
	}
	if (!(keep && dictionary_hasentry(keep, gensym("promptcolor")))) {
		const Color &k = t[RoleComment];
		x->promptcolor = t_jrgba{k.r, k.g, k.b, k.a};
	}
}

static std::string theme_names() {
	std::string s;
	for (int i = 0; i < theme_count(); ++i)
		s += (i ? " " : "") + theme_at(i).name;
	return s;
}

static t_max_err theme_set(t_py_repl *x, void *attr, long argc, t_atom *argv) {
	const std::string name = code_text(argc, argv);
	const Theme *t = theme_find(name);
	if (!t) {
		object_error((t_object *)x, "unknown theme '%s'; available: %s", name.c_str(), theme_names().c_str());
		return MAX_ERR_GENERIC;
	}
	x->theme = gensym(t->name.c_str());
	if (x->st && x->st->ready) {
		apply_theme(x, *t, nullptr);
		for (int i = 0; i < RoleCount; ++i)
			object_attr_touch((t_object *)x, gensym(kColorAttr[i]));
		object_attr_touch((t_object *)x, gensym("promptcolor"));
		redraw(x);
	}
	return MAX_ERR_NONE;
}

static t_max_err py_set(t_py_repl *x, void *attr, long argc, t_atom *argv) {
	x->py = gensym(code_text(argc, argv).c_str());
	if (x->st) {
		x->st->abi_for = nullptr; /* check the new target */
		redraw(x);
	}
	return MAX_ERR_NONE;
}

static t_max_err maxlines_set(t_py_repl *x, void *attr, long argc, t_atom *argv) {
	x->maxlines = std::max(100L, std::min(1000000L, argc ? (long)atom_getlong(argv) : 10000L));
	if (x->st) {
		x->st->tr.set_max_lines((int)x->maxlines);
		redraw(x);
	}
	return MAX_ERR_NONE;
}

/* ---- messages ---- */

static void py_repl_run(t_py_repl *x, t_symbol *s, long argc, t_atom *argv) {
	submit_code(x, code_text(argc, argv));
}

static void py_repl_clear(t_py_repl *x) { clear_transcript(x); }

static void py_repl_history(t_py_repl *x, t_symbol *s, long argc, t_atom *argv) {
	if (argc == 1 && atom_getsym(argv) == gensym("clear")) {
		x->st->hist.clear();
		set_dirty(x);
		redraw(x);
	} else
		object_post((t_object *)x, "history: %d entries ('history clear' clears them)", x->st->hist.size());
}

static void py_repl_themes(t_py_repl *x) {
	object_post((t_object *)x, "themes: %s (in use: %s)", theme_names().c_str(), sym_str(x->theme).c_str());
}

static void py_repl_assist(t_py_repl *x, void *b, long m, long a, char *s) {
	snprintf(s, 256, "run <code>, clear, history clear, attributes");
}

static t_max_err py_repl_notify(t_py_repl *x, t_symbol *s, t_symbol *msg, void *sender, void *data) {
	return jbox_notify((t_jbox *)x, s, msg, sender, data);
}

static void py_repl_focusgained(t_py_repl *x, t_object *patcherview) {
	x->st->focused = true;
	redraw(x);
}

static void py_repl_focuslost(t_py_repl *x, t_object *patcherview) {
	x->st->focused = false;
	x->st->comp.close();
	redraw(x);
}

/* ---- persistence ---- */

static void py_repl_jsave(t_py_repl *x, t_dictionary *d) {
	State &st = *x->st;
	dictionary_appendstring(d, gensym("py_repl_input"), st.in_ed.text().c_str());
	if (!x->savehistory)
		return;
	const std::vector<std::string> &e = st.hist.entries();
	const size_t from = e.size() > (size_t)kSavedHistory ? e.size() - kSavedHistory : 0;
	std::vector<t_atom> atoms(e.size() - from);
	for (size_t i = from; i < e.size(); ++i)
		atom_setsym(&atoms[i - from], gensym(e[i].c_str()));
	if (!atoms.empty())
		dictionary_appendatoms(d, gensym("py_repl_history"), (long)atoms.size(), atoms.data());
}

static void jload(t_py_repl *x, t_dictionary *d) {
	State &st = *x->st;
	const char *s = nullptr;
	if (dictionary_getstring(d, gensym("py_repl_input"), &s) == MAX_ERR_NONE && s) {
		st.in_ed.reset(normalize_newlines(s));
		st.in_ed.set_caret(st.in_ed.length());
	}
	long argc = 0;
	t_atom *argv = nullptr;
	if (x->savehistory && dictionary_getatoms(d, gensym("py_repl_history"), &argc, &argv) == MAX_ERR_NONE) {
		std::vector<std::string> entries;
		for (long i = 0; i < argc; ++i)
			if (atom_gettype(argv + i) == A_SYM)
				entries.push_back(sym_str(atom_getsym(argv + i)));
		st.hist.load(entries);
	}
}

/* ---- new / free ---- */

static void set_defaults(t_py_repl *x) {
	x->py = gensym("__main__");
	x->theme = gensym(theme_at(0).name.c_str());
	x->fontsize = 12;
	x->tabwidth = 4;
	x->autoclose = x->completion = x->wrap = x->titlebar = x->savehistory = 1;
	x->maxlines = 10000;
	x->scrollspeed = 3;
	apply_theme(x, theme_at(0), nullptr);
}

static void *py_repl_new(t_symbol *s, long argc, t_atom *argv) {
	t_dictionary *d = object_dictionaryarg(argc, argv);
	if (!d)
		return nullptr;
	t_py_repl *x = (t_py_repl *)object_alloc(s_class);
	if (!x)
		return nullptr;
	x->st = new State; /* before jbox_new: setters may run from there on */
	set_defaults(x);
	const long flags = JBOX_DRAWFIRSTIN | JBOX_TRANSPARENT | JBOX_DRAWINLAST | JBOX_HILITE | JBOX_GROWBOTH;
	jbox_new(&x->box, flags, argc, argv);
	x->box.b_firstin = (t_object *)x;

	State &st = *x->st;
	st.id = s_next_id++;
	boxes()[st.id] = x;
	st.scroll_clock = (t_clock *)clock_new(x, (method)scroll_tick);

	attr_dictionary_process(x, d);
	if (const Theme *t = theme_find(sym_str(x->theme)))
		apply_theme(x, *t, d); /* colors in the dictionary keep their values */
	st.tr.set_max_lines((int)x->maxlines);
	jload(x, d);
	st.ready = true;
	jbox_ready(&x->box);
	return x;
}

static void py_repl_free(t_py_repl *x) {
	State *st = x->st;
	if (st) {
		boxes().erase(st->id);
		st->monitor.stop();
		if (st->scroll_clock) {
			clock_unset(st->scroll_clock);
			object_free(st->scroll_clock);
		}
		if (st->m.font)
			jfont_destroy(st->m.font);
		st->m.font = nullptr;
	}
	jbox_free(&x->box);
	delete st;
	x->st = nullptr;
}

/* ---- class ---- */

#define PYR_FLAG(name, member, def)                                                                                    \
	CLASS_ATTR_LONG(c, name, 0, t_py_repl, member);                                                                    \
	CLASS_ATTR_STYLE(c, name, 0, "onoff");                                                                             \
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, name, 0, def)

void ext_main(void *r) {
	jpatcher_syms_init();

	t_class *c = class_new("py_repl", (method)py_repl_new, (method)py_repl_free, sizeof(t_py_repl), 0L, A_GIMME, 0);
	c->c_flags |= CLASS_FLAG_NEWDICTIONARY;
	jbox_initclass(c, 0);

	class_addmethod(c, (method)pyc::paint, "paint", A_CANT, 0);
	class_addmethod(c, (method)pyc::getdrawparams, "getdrawparams", A_CANT, 0);
	class_addmethod(c, (method)pyc::key, "key", A_CANT, 0);
	class_addmethod(c, (method)pyc::mousedown, "mousedown", A_CANT, 0);
	class_addmethod(c, (method)pyc::mousedrag, "mousedrag", A_CANT, 0);
	class_addmethod(c, (method)pyc::mouseup, "mouseup", A_CANT, 0);
	class_addmethod(c, (method)pyc::mouseenter, "mouseenter", A_CANT, 0);
	class_addmethod(c, (method)pyc::mousemove, "mousemove", A_CANT, 0);
	class_addmethod(c, (method)pyc::mouseleave, "mouseleave", A_CANT, 0);
	class_addmethod(c, (method)pyc::mousewheel, "mousewheel", A_CANT, 0);
	class_addmethod(c, (method)py_repl_focusgained, "focusgained", A_CANT, 0);
	class_addmethod(c, (method)py_repl_focuslost, "focuslost", A_CANT, 0);
	class_addmethod(c, (method)py_repl_jsave, "jsave", A_CANT, 0);
	class_addmethod(c, (method)py_repl_notify, "notify", A_CANT, 0);
	class_addmethod(c, (method)py_repl_assist, "assist", A_CANT, 0);
	class_addmethod(c, (method)py_repl_run, "run", A_GIMME, 0);
	class_addmethod(c, (method)py_repl_clear, "clear", 0);
	class_addmethod(c, (method)py_repl_history, "history", A_GIMME, 0);
	class_addmethod(c, (method)py_repl_themes, "themes", 0);

	CLASS_ATTR_SYM(c, "py", 0, t_py_repl, py);
	CLASS_ATTR_ACCESSORS(c, "py", NULL, py_set);
	CLASS_ATTR_DEFAULT_SAVE(c, "py", 0, "__main__");
	CLASS_ATTR_BASIC(c, "py", 0);

	CLASS_ATTR_SYM(c, "theme", 0, t_py_repl, theme);
	CLASS_ATTR_ACCESSORS(c, "theme", NULL, theme_set);
	CLASS_ATTR_ENUM(c, "theme", 0, theme_names().c_str());
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "theme", 0, theme_at(0).name.c_str());

	CLASS_ATTR_DOUBLE(c, "fontsize", 0, t_py_repl, fontsize);
	CLASS_ATTR_FILTER_CLIP(c, "fontsize", 6, 48);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "fontsize", 0, "12");

	CLASS_ATTR_LONG(c, "tabwidth", 0, t_py_repl, tabwidth);
	CLASS_ATTR_FILTER_CLIP(c, "tabwidth", 1, 16);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "tabwidth", 0, "4");

	PYR_FLAG("autoclose", autoclose, "1");
	PYR_FLAG("completion", completion, "1");
	PYR_FLAG("wrap", wrap, "1");
	PYR_FLAG("titlebar", titlebar, "1");
	PYR_FLAG("savehistory", savehistory, "1");

	CLASS_ATTR_LONG(c, "maxlines", 0, t_py_repl, maxlines);
	CLASS_ATTR_ACCESSORS(c, "maxlines", NULL, maxlines_set);
	CLASS_ATTR_DEFAULT_SAVE(c, "maxlines", 0, "10000");

	CLASS_ATTR_DOUBLE(c, "scrollspeed", 0, t_py_repl, scrollspeed);
	CLASS_ATTR_FILTER_CLIP(c, "scrollspeed", 0.05, 50);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "scrollspeed", 0, "3");

	for (int i = 0; i < RoleCount; ++i) {
		const char *n = kColorAttr[i];
		class_addattr(c, attr_offset_array_new(n, gensym("float64"), 4, 0, (method)0L, (method)0L, 0,
											   (long)(calcoffset(t_py_repl, colors) + i * sizeof(t_jrgba))));
		CLASS_ATTR_ACCESSORS(c, n, NULL, jgraphics_attr_setrgba);
		CLASS_ATTR_STYLE(c, n, 0, "rgba");
		const Color &k = theme_at(0)[i];
		char def[128];
		snprintf(def, sizeof def, "%.4f %.4f %.4f %.4f", k.r, k.g, k.b, k.a);
		CLASS_ATTR_DEFAULT_SAVE_PAINT(c, n, 0, def);
	}

	CLASS_ATTR_RGBA(c, "promptcolor", 0, t_py_repl, promptcolor);
	CLASS_ATTR_STYLE_LABEL(c, "promptcolor", 0, "rgba", "Prompt Color");
	{
		const Color &k = theme_at(0)[RoleComment];
		char def[128];
		snprintf(def, sizeof def, "%.4f %.4f %.4f %.4f", k.r, k.g, k.b, k.a);
		CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "promptcolor", 0, def);
	}

	CLASS_ATTR_DEFAULT(c, "patching_rect", 0, "0. 0. 420. 220.");

	class_register(CLASS_BOX, c);
	s_class = c;
}
