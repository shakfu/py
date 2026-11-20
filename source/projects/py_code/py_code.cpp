// SPDX-License-Identifier: MIT
/* py_code.cpp -- the py_code Max class: attributes, lifecycle, LSP, files,
 * snippets and evaluation. Painting and input live in py_code_paint.cpp and
 * py_code_input.cpp. Behaviour: docs/dev/py_code-spec.md section 3. */

#include "py_code.h"

#include "core/paths.h"
#include "core/python_env.h"
#include "ext_systime.h"
#include "jpatcher_syms.h"
#include "platform/clipboard.h"
#include "platform/filedialog.h"

#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <unordered_map>

using namespace pyc;

static t_class *s_class = nullptr;
static t_qelem *s_qelem = nullptr; /* LSP wake-up, class level */
static long s_next_id = 1;

static const char *const kColorAttr[RoleCount] = {
	"bgcolor",		"textcolor",	  "selcolor",	  "caretcolor",	 "curlinecolor",  "keywordcolor",
	"builtincolor", "functioncolor",  "classcolor",	  "decoratorcolor", "stringcolor", "escapecolor",
	"numbercolor",	"selfcolor",	  "commentcolor", "bracketcolor", "operatorcolor", "matchcolor",
	"blinkcolor",	"barcolor",		  "barhighlight", "errorcolor",	 "warningcolor"};

/* Live boxes by id: callbacks and the qelem look boxes up here, never by raw pointer. */
static std::unordered_map<long, t_py_code *> &boxes() {
	static auto *m = new std::unordered_map<long, t_py_code *>;
	return *m;
}

t_py_code *pyc::find_box(long id) {
	auto it = boxes().find(id);
	return it == boxes().end() ? nullptr : it->second;
}

/* ---- small helpers ---- */

static std::string sym_str(t_symbol *s) { return s && s->s_name ? s->s_name : ""; }

std::string pyc::home_dir() {
	const char *h = getenv("HOME");
	return h ? h : "";
}

/* Attribute path -> filesystem path. */
static std::string resolve(t_symbol *s) { return expand_home(home_dir(), posix_path(unquote(sym_str(s)))); }

static std::string join_atoms(long argc, t_atom *argv) {
	std::string out;
	for (long i = 0; i < argc; ++i) {
		if (i)
			out += ' ';
		char buf[64];
		switch (atom_gettype(argv + i)) {
		case A_SYM:
			out += sym_str(atom_getsym(argv + i));
			break;
		case A_LONG:
			snprintf(buf, sizeof buf, "%lld", (long long)atom_getlong(argv + i));
			out += buf;
			break;
		case A_FLOAT:
			snprintf(buf, sizeof buf, "%g", atom_getfloat(argv + i));
			out += buf;
			break;
		default:
			break;
		}
	}
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

static bool read_file(const std::string &path, std::string *out) {
	std::ifstream f(path, std::ios::binary);
	if (!f)
		return false;
	std::ostringstream ss;
	ss << f.rdbuf();
	if (f.bad())
		return false;
	*out = ss.str();
	return true;
}

static bool write_file(const std::string &path, const std::string &text) {
	std::ofstream f(path, std::ios::binary | std::ios::trunc);
	if (!f)
		return false;
	f.write(text.data(), (std::streamsize)text.size());
	f.close();
	return !f.fail();
}

static bool stat_file(const std::string &path, long long *mtime, long long *size) {
	struct stat sb;
	if (stat(path.c_str(), &sb) != 0)
		return false;
	*mtime = (long long)sb.st_mtimespec.tv_sec * 1000000000LL + sb.st_mtimespec.tv_nsec;
	*size = (long long)sb.st_size;
	return true;
}

static std::string dirname_of(const std::string &path) {
	const size_t k = path.rfind('/');
	if (k == std::string::npos)
		return "";
	return k == 0 ? "/" : path.substr(0, k);
}

static void post_lines(t_py_code *x, const std::string &head, const std::string &body) {
	std::istringstream ss(body);
	std::string line;
	bool first = true;
	while (std::getline(ss, line)) {
		if (first)
			object_error((t_object *)x, "%s%s", head.c_str(), line.c_str());
		else
			object_error((t_object *)x, "  %s", line.c_str());
		first = false;
	}
	if (first)
		object_error((t_object *)x, "%s", head.c_str());
}

int pyc::cells_between(const std::string &s, int a, int b) {
	int n = 0;
	for (int p = a; p < b && p < (int)s.size(); p = utf8_next(s, p))
		++n;
	return n;
}

void pyc::redraw(t_py_code *x) { jbox_redraw((t_jbox *)x); }

t_jrgba pyc::color(t_py_code *x, int role, double alpha_mul) {
	t_jrgba c = x->colors[role];
	c.alpha *= alpha_mul;
	return c;
}

/* ---- metrics and geometry ---- */

void pyc::ensure_metrics(t_py_code *x) {
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

Geom pyc::geometry(t_py_code *x, double w, double h) {
	ensure_metrics(x);
	const Metrics &m = x->st->m;
	Geom g;
	g.w = w;
	g.h = h;
	g.title_h = x->titlebar ? m.lh : 0;
	g.status_h = x->statusbar ? m.lh + 4 : 0;
	if (x->linenumbers) {
		int n = x->st->ed.line_count(), d = 1;
		while (n >= 10) {
			n /= 10;
			++d;
		}
		g.gutter_w = std::max(d, 2) * m.cw + 2 * kPadX;
	}
	g.text_x = g.gutter_w + kPadX;
	g.sb_x = w - kScrollW;
	g.text_r = g.sb_x - 2; /* always reserves the scroll bar */
	g.view_y = g.title_h;
	g.view_h = std::max(0.0, h - g.title_h - g.status_h);
	g.cols = x->wrap ? std::max(1, (int)std::floor((g.text_r - g.text_x) / m.cw)) : 0;
	return g;
}

Geom pyc::sync_layout(t_py_code *x) {
	State &st = *x->st;
	const Geom g = geometry(x, st.box_w, st.box_h);
	st.layout.update(st.ed, g.cols);
	st.layout.set_view(g.view_h, st.m.lh, kPadY);
	if (x->wrap)
		st.hscroll = 0;
	return g;
}

void pyc::view_size(t_py_code *x, t_object *patcherview) {
	t_rect r;
	if (patcherview && jbox_get_rect_for_view((t_object *)x, patcherview, &r) == MAX_ERR_NONE) {
		x->st->box_w = r.width;
		x->st->box_h = r.height;
	}
}

void pyc::reveal_caret(t_py_code *x) {
	State &st = *x->st;
	const Geom g = sync_layout(x);
	st.layout.ensure_row_visible(st.layout.row_of(st.ed.caret()));
	if (!x->wrap) {
		const double cx = st.ed.cells_before(st.ed.caret()) * st.m.cw;
		const double area = std::max(st.m.cw, g.text_r - g.text_x);
		if (cx - st.hscroll > area - st.m.cw)
			st.hscroll = cx - area + st.m.cw;
		if (cx < st.hscroll)
			st.hscroll = std::max(0.0, cx - area / 3);
	}
}

/* ---- change notifications ---- */

static bool lsp_ready(t_py_code *x) { return x->st->doc && x->st->doc->state() == lsp::State::Ready; }

void pyc::after_move(t_py_code *x) {
	State &st = *x->st;
	if (x->statusbar && lsp_ready(x))
		clock_fdelay(st.sig_clock, 150);
}

static void set_dirty(t_py_code *x) {
	if (t_object *p = jbox_get_patcher((t_object *)x))
		jpatcher_set_dirty(p, 1);
}

void pyc::after_edit(t_py_code *x) {
	State &st = *x->st;
	st.goal = -1;
	set_dirty(x);
	if (st.doc)
		clock_fdelay(st.sync_clock, 300);
	if (x->autowrite && !resolve(x->file).empty()) {
		st.write_pending = true;
		clock_fdelay(st.write_clock, 400);
	}
	after_move(x);
}

/* ---- clocks ---- */

static void sync_tick(t_py_code *x) {
	State &st = *x->st;
	if (st.doc)
		st.doc->set_text(st.ed.text(), st.ed.revision());
}

static void write_now(t_py_code *x) {
	State &st = *x->st;
	st.write_pending = false;
	const std::string path = resolve(x->file);
	if (path.empty())
		return;
	if (!write_file(path, st.ed.text()))
		object_error((t_object *)x, "cannot write %s", path.c_str());
}

static void write_tick(t_py_code *x) {
	if (x->st->write_pending)
		write_now(x);
}

static void sig_tick(t_py_code *x) {
	State &st = *x->st;
	if (!x->statusbar || !lsp_ready(x))
		return;
	st.doc->set_text(st.ed.text(), st.ed.revision());
	const long id = st.id;
	const bool sent = st.doc->signature(st.ed.caret(), [id](int rev, lsp::Signature sig) {
		t_py_code *b = find_box(id);
		if (!b || rev != b->st->ed.revision())
			return;
		b->st->sig = std::move(sig);
		redraw(b);
	});
	if (!sent && !st.sig.label.empty()) {
		st.sig = lsp::Signature();
		redraw(x);
	}
}

static void blink_tick(t_py_code *x) {
	State &st = *x->st;
	if (!st.blinking)
		return;
	if ((double)(systime_ms() - st.blink_t0) < (double)x->blinkduration)
		clock_fdelay(st.blink_clock, 25);
	else
		st.blinking = false;
	redraw(x);
}

/* ---- LSP ---- */

static void lsp_check(t_py_code *x) {
	State &st = *x->st;
	if (!st.doc)
		return;
	bool dirty = false;
	const lsp::State s = st.doc->state();
	if (s != st.shown_state) {
		st.shown_state = s;
		dirty = true;
	}
	if (s == lsp::State::Failed && !st.error_posted) {
		st.error_posted = true;
		post_lines(x, "lsp failed: ", st.doc->error());
	}
	const int serial = st.doc->diagnostics_serial();
	if (serial != st.shown_serial) {
		st.shown_serial = serial;
		dirty = true;
	}
	if (dirty)
		redraw(x);
}

/* py.mxo: where the loaded py class came from, else beside py_code.mxo. Same
 * lookup as py's py_get_path_to_external, which py_code cannot link to. */
static std::string py_bundle() {
	t_class *c = class_findbyname(CLASS_BOX, gensym("py"));
	char out[MAX_PATH_CHARS] = "";
	if (path_toabsolutesystempath(class_getpath(c ? c : s_class), "py.mxo", out) != MAX_ERR_NONE)
		return "";
	return out;
}

static void lsp_open(t_py_code *x) {
	State &st = *x->st;
	st.doc.reset();
	st.sig = lsp::Signature();
	st.req_rev = st.req_caret = -1;
	st.error_posted = false;
	st.shown_serial = -1;
	st.shown_state = lsp::State::Stopped;
	const std::string cmd = unquote(sym_str(x->lsp));
	if (!cmd.empty()) {
		const std::string file = resolve(x->file);
		const bool abs = !file.empty() && file[0] == '/';
		st.doc = lsp::Document::open(cmd, abs ? dirname_of(file) : "", abs ? file : st.tmp_path,
									 bundled_python(py_bundle()));
		st.doc->set_text(st.ed.text(), st.ed.revision());
		lsp_check(x);
	}
	redraw(x);
}

/* qelem handler: main thread. */
static void lsp_wake(void *) {
	lsp::poll();
	std::vector<long> ids;
	for (const auto &kv : boxes())
		ids.push_back(kv.first);
	for (long id : ids)
		if (t_py_code *x = find_box(id))
			lsp_check(x);
}

static void py_code_quit(void *) {
	lsp::set_wakeup({});
	lsp::shutdown_all();
	if (s_qelem) {
		qelem_free(s_qelem);
		s_qelem = nullptr;
	}
}

/* Editing helpers below leave notifications to the caller (finish() in py_code_input.cpp). */

/* ---- completion ---- */

static bool local_complete(t_py_code *x) {
	State &st = *x->st;
	const std::string &t = st.ed.text();
	const int caret = st.ed.caret();
	int ws = caret;
	auto word = [](char c) { return std::isalnum((unsigned char)c) || c == '_'; };
	while (ws > 0 && word(t[(size_t)ws - 1]))
		--ws;
	if (ws == caret)
		return false;
	st.words.sync(t, st.ed.revision());
	return st.comp.open(t, caret, st.words.query(t.substr((size_t)ws, (size_t)(caret - ws)), 200));
}

/* Returns true when a popup opened or an LSP request is pending. */
bool pyc::complete_open(t_py_code *x, bool lsp_only) {
	State &st = *x->st;
	st.comp.close();
	if (lsp_ready(x)) {
		st.doc->set_text(st.ed.text(), st.ed.revision());
		const long id = st.id;
		st.req_rev = st.ed.revision();
		st.req_caret = st.ed.caret();
		const bool sent = st.doc->complete(st.ed.caret(), [id](int rev, std::vector<CompletionItem> items) {
			t_py_code *b = find_box(id);
			if (!b)
				return;
			State &s = *b->st;
			if (rev != s.ed.revision() || rev != s.req_rev || s.ed.caret() != s.req_caret || s.ed.has_selection())
				return;
			s.req_rev = s.req_caret = -1;
			if (items.empty() || !s.comp.open(s.ed.text(), s.ed.caret(), std::move(items)))
				local_complete(b);
			redraw(b);
		});
		if (sent)
			return true;
		st.req_rev = st.req_caret = -1;
	}
	return !lsp_only && local_complete(x);
}

void pyc::complete_accept(t_py_code *x) {
	State &st = *x->st;
	const CompletionEdit e = st.comp.accept();
	if (e.start < 0 || x->readonly)
		return;
	st.ed.select(e.start, e.end);
	st.ed.insert(e.text);
}

/* ---- clipboard ---- */

void pyc::paste(t_py_code *x) {
	State &st = *x->st;
	const std::string s = normalize_newlines(clipboard_text());
	if (!s.empty() && !x->readonly)
		st.ed.insert(s);
}

void pyc::copy(t_py_code *x, bool cut) {
	State &st = *x->st;
	if (!st.ed.has_selection())
		return;
	set_clipboard_text(st.ed.selected_text());
	if (cut && !x->readonly)
		st.ed.insert("");
}

/* ---- snippets ---- */

static void snippets_load(t_py_code *x) {
	State &st = *x->st;
	st.snips = Snippets();
	st.snip_path = resolve(x->snippets);
	st.snip_mtime = 0;
	st.snip_size = -1;
	if (st.snip_path.empty())
		return;
	std::string text;
	if (!read_file(st.snip_path, &text)) {
		object_error((t_object *)x, "snippets: cannot read %s", st.snip_path.c_str());
		return;
	}
	stat_file(st.snip_path, &st.snip_mtime, &st.snip_size);
	st.snips = snippets::parse(text);
	object_post((t_object *)x, "snippets: %ld entries from %s", (long)st.snips.size(), st.snip_path.c_str());
	auto list = [](const std::vector<std::string> &v) {
		std::string s;
		for (const auto &k : v)
			s += " " + k;
		return s;
	};
	if (!st.snips.duplicates.empty())
		object_warn((t_object *)x, "snippets: duplicate keys (first wins):%s", list(st.snips.duplicates).c_str());
	if (!st.snips.empty.empty())
		object_warn((t_object *)x, "snippets: empty keys ignored:%s", list(st.snips.empty).c_str());
}

static void snippets_refresh(t_py_code *x) {
	State &st = *x->st;
	long long mtime = 0, size = -1;
	if (st.snip_path != resolve(x->snippets) || !stat_file(st.snip_path, &mtime, &size) || mtime != st.snip_mtime ||
		size != st.snip_size)
		snippets_load(x);
}

static void expand_snippet(t_py_code *x, py_code::EvalSpan sp, const std::string &key) {
	State &st = *x->st;
	snippets_refresh(x);
	const std::string *body = st.snips.find(key);
	if (!body) {
		std::string keys;
		for (const auto &k : st.snips.keys())
			keys += " " + k;
		object_error((t_object *)x, "snippets: no '%s'; available:%s", key.c_str(), keys.empty() ? " (none)" : keys.c_str());
		return;
	}
	if (x->readonly) {
		object_error((t_object *)x, "readonly: snippet not expanded");
		return;
	}
	/* Keep the line's indentation and give it to every following body line. */
	const std::string &t = st.ed.text();
	const int line = st.ed.line_of(sp.start);
	const int ls = st.ed.line_start(line);
	const std::string lt = st.ed.line_text(line);
	const int lw = py_code::leading_ws(lt);
	int start = sp.start, end = sp.end;
	if (start <= ls + lw)
		start = ls + lw;
	while (end > start && (t[(size_t)end - 1] == '\n' || t[(size_t)end - 1] == ' ' || t[(size_t)end - 1] == '\t'))
		--end;
	const std::string prefix = lt.substr(0, (size_t)lw);
	std::string out;
	for (size_t i = 0; i < body->size(); ++i) {
		out += (*body)[i];
		if ((*body)[i] == '\n' && i + 1 < body->size() && (*body)[i + 1] != '\n')
			out += prefix;
	}
	st.ed.select(start, end);
	st.ed.insert(out);
	post("-> %s", key.c_str());
}

/* ---- evaluation ---- */

void pyc::evaluate(t_py_code *x, py_code::EvalMode mode) {
	State &st = *x->st;
	st.comp.close();
	st.syntax.update(st.ed.text(), st.ed.revision());
	const py_code::EvalSpan sp = st.syntax.eval_span(st.ed.caret(), st.ed.sel_start(), st.ed.sel_end(), mode);
	std::string code;
	if (sp.has_value())
		code = py_code::dedent(st.ed.text().substr((size_t)sp.start, (size_t)(sp.end - sp.start)));
	if (code.find_first_not_of(" \t\n") == std::string::npos) {
		object_post((t_object *)x, "nothing to evaluate");
		return;
	}
	std::string key;
	if (!sym_str(x->snippets).empty() && snippets::match(code, &key)) {
		expand_snippet(x, sp, key);
		return;
	}
	if (x->blink) {
		st.blinking = true;
		st.blink_start = sp.start;
		st.blink_end = sp.end;
		st.blink_t0 = systime_ms();
		clock_fdelay(st.blink_clock, 25);
	}
	const bool expr = st.syntax.is_expression(code);
	t_atom a;
	atom_setsym(&a, gensym(code.c_str()));
	outlet_anything((t_outlet *)x->outlet, gensym(expr ? "eval" : "exec"), 1, &a);
}

/* ---- files ---- */

static void reset_text(t_py_code *x, const std::string &text) {
	State &st = *x->st;
	st.ed.reset(normalize_newlines(text));
	st.comp.close();
	st.sig = lsp::Signature();
	st.layout.set_scroll(0);
	st.hscroll = 0;
	st.goal = -1;
	st.blinking = false;
}

/* Reads @file into the editor; on failure the document is unchanged. */
static bool load_file(t_py_code *x) {
	const std::string path = resolve(x->file);
	std::string text;
	if (!read_file(path, &text)) {
		object_error((t_object *)x, "cannot read %s", path.c_str());
		return false;
	}
	reset_text(x, text);
	return true;
}

static void set_file_quiet(t_py_code *x, const std::string &path) {
	x->st->no_reload = true;
	object_attr_setsym(x, gensym("file"), gensym(path.c_str()));
	x->st->no_reload = false;
}

void pyc::menu_save(t_py_code *x) {
	State &st = *x->st;
	std::string path = resolve(x->file);
	const bool ask = path.empty();
	if (ask) {
		path = save_dialog("untitled.py");
		if (path.empty())
			return;
	}
	if (!write_file(path, st.ed.text())) {
		object_error((t_object *)x, "cannot write %s", path.c_str());
		return;
	}
	if (!ask)
		st.write_pending = false;
	object_post((t_object *)x, "wrote %s", path.c_str());
	if (ask)
		set_file_quiet(x, path);
}

void pyc::menu_load(t_py_code *x) {
	if (x->readonly)
		return;
	const std::string path = open_dialog();
	if (path.empty())
		return;
	std::string text;
	if (!read_file(path, &text)) {
		object_error((t_object *)x, "cannot read %s", path.c_str());
		return;
	}
	reset_text(x, text);
	x->st->write_pending = false;
	set_dirty(x);
	set_file_quiet(x, path); /* reopens the LSP document with the new text */
	redraw(x);
}

/* ---- attribute setters ---- */

static void apply_theme(t_py_code *x, const Theme &t, t_dictionary *keep) {
	for (int i = 0; i < RoleCount; ++i) {
		if (keep && dictionary_hasentry(keep, gensym(kColorAttr[i])))
			continue;
		x->colors[i].red = t[i].r;
		x->colors[i].green = t[i].g;
		x->colors[i].blue = t[i].b;
		x->colors[i].alpha = t[i].a;
	}
}

static std::string theme_names() {
	std::string s;
	for (int i = 0; i < theme_count(); ++i)
		s += (i ? " " : "") + theme_at(i).name;
	return s;
}

static t_max_err theme_set(t_py_code *x, void *attr, long argc, t_atom *argv) {
	const std::string name = join_atoms(argc, argv);
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
		redraw(x);
	}
	return MAX_ERR_NONE;
}

static t_max_err file_set(t_py_code *x, void *attr, long argc, t_atom *argv) {
	x->file = gensym(join_atoms(argc, argv).c_str());
	if (!x->st || !x->st->ready)
		return MAX_ERR_NONE; /* new() loads after the saved text is known */
	if (!x->st->no_reload && x->autoload && !resolve(x->file).empty())
		load_file(x);
	lsp_open(x);
	return MAX_ERR_NONE;
}

static t_max_err snippets_set(t_py_code *x, void *attr, long argc, t_atom *argv) {
	x->snippets = gensym(join_atoms(argc, argv).c_str());
	if (x->st)
		snippets_load(x);
	return MAX_ERR_NONE;
}

static t_max_err lsp_set(t_py_code *x, void *attr, long argc, t_atom *argv) {
	x->lsp = gensym(join_atoms(argc, argv).c_str());
	if (x->st && x->st->ready)
		lsp_open(x);
	return MAX_ERR_NONE;
}

/* ---- messages ---- */

static void py_code_themes(t_py_code *x) {
	object_post((t_object *)x, "themes: %s (in use: %s)", theme_names().c_str(), sym_str(x->theme).c_str());
}

static void py_code_assist(t_py_code *x, void *b, long m, long a, char *s) {
	if (m == ASSIST_INLET)
		snprintf(s, 256, "messages and attributes");
	else
		snprintf(s, 256, "exec / eval messages for [py]");
}

static t_max_err py_code_notify(t_py_code *x, t_symbol *s, t_symbol *msg, void *sender, void *data) {
	return jbox_notify((t_jbox *)x, s, msg, sender, data);
}

static void py_code_focusgained(t_py_code *x, t_object *patcherview) {
	x->st->focused = true;
	redraw(x);
}

static void py_code_focuslost(t_py_code *x, t_object *patcherview) {
	x->st->focused = false;
	x->st->comp.close();
	redraw(x);
}

/* ---- persistence ---- */

static void py_code_jsave(t_py_code *x, t_dictionary *d) {
	State &st = *x->st;
	dictionary_appendstring(d, gensym("py_code_text"), st.ed.text().c_str());
	dictionary_appendlong(d, gensym("py_code_caret"), st.ed.caret());
	dictionary_appendfloat(d, gensym("py_code_scroll"), st.layout.scroll_y());
}

/* Restores saved text, caret and scroll; true when there was saved text. */
static bool jload(t_py_code *x, t_dictionary *d) {
	State &st = *x->st;
	t_symbol *k = gensym("py_code_text");
	const char *s = nullptr;
	t_atom a;
	if (dictionary_getstring(d, k, &s) != MAX_ERR_NONE || !s) {
		s = nullptr;
		if (dictionary_getatom(d, k, &a) == MAX_ERR_NONE && atom_gettype(&a) == A_SYM)
			s = atom_getsym(&a)->s_name;
	}
	if (!s || !*s)
		return false;
	st.ed.reset(normalize_newlines(s));
	if (dictionary_getatom(d, gensym("py_code_caret"), &a) == MAX_ERR_NONE)
		st.ed.set_caret((int)atom_getlong(&a));
	if (dictionary_getatom(d, gensym("py_code_scroll"), &a) == MAX_ERR_NONE)
		st.layout.set_scroll(std::max(0.0, (double)atom_getfloat(&a)));
	return true;
}

/* ---- new / free ---- */

static void set_defaults(t_py_code *x) {
	x->theme = gensym(theme_at(0).name.c_str());
	x->fontsize = 12;
	x->tabwidth = 4;
	x->linenumbers = x->titlebar = x->matchbrackets = x->wrap = x->blink = 1;
	x->completion = x->statusbar = x->autoclose = x->autoload = 1;
	x->readonly = x->autowrite = 0;
	x->blinkduration = 500;
	x->scrollspeed = 3;
	x->file = gensym("");
	x->snippets = gensym("");
	x->lsp = gensym("auto");
	apply_theme(x, theme_at(0), nullptr);
}

static void *py_code_new(t_symbol *s, long argc, t_atom *argv) {
	t_dictionary *d = object_dictionaryarg(argc, argv);
	if (!d)
		return nullptr;
	t_py_code *x = (t_py_code *)object_alloc(s_class);
	if (!x)
		return nullptr;
	x->st = new State; /* before jbox_new: setters may run from there on */
	set_defaults(x);
	const long flags = JBOX_DRAWFIRSTIN | JBOX_TRANSPARENT | JBOX_DRAWINLAST | JBOX_HILITE | JBOX_GROWBOTH;
	jbox_new(&x->box, flags, argc, argv);
	x->box.b_firstin = (t_object *)x;
	x->outlet = outlet_new(x, nullptr);

	State &st = *x->st;
	st.id = s_next_id++;
	boxes()[st.id] = x;
	st.sync_clock = (t_clock *)clock_new(x, (method)sync_tick);
	st.write_clock = (t_clock *)clock_new(x, (method)write_tick);
	st.sig_clock = (t_clock *)clock_new(x, (method)sig_tick);
	st.blink_clock = (t_clock *)clock_new(x, (method)blink_tick);
	st.scroll_clock = (t_clock *)clock_new(x, (method)scroll_tick);
	const char *tmp = getenv("TMPDIR");
	std::string dir = tmp && *tmp ? tmp : "/tmp/";
	if (dir.back() != '/')
		dir += '/';
	st.tmp_path = dir + "py_code/" + std::to_string((long)getpid()) + "-" + std::to_string(st.id) + ".py";

	attr_dictionary_process(x, d);
	if (const Theme *t = theme_find(sym_str(x->theme)))
		apply_theme(x, *t, d); /* colors in the dictionary keep their values */
	if (!jload(x, d) && x->autoload && !resolve(x->file).empty())
		load_file(x);
	st.ready = true;
	lsp_open(x);
	jbox_ready(&x->box);
	return x;
}

static void py_code_free(t_py_code *x) {
	State *st = x->st;
	if (st) {
		boxes().erase(st->id);
		for (t_clock *c : {st->sync_clock, st->write_clock, st->sig_clock, st->blink_clock, st->scroll_clock})
			if (c) {
				clock_unset(c);
				object_free(c);
			}
		if (st->write_pending)
			write_now(x);
		st->doc.reset();
		if (st->m.font)
			jfont_destroy(st->m.font);
		st->m.font = nullptr;
	}
	jbox_free(&x->box);
	delete st;
	x->st = nullptr;
}

/* ---- class ---- */

#define PYC_FLAG(name, member, def)                                                                                    \
	CLASS_ATTR_LONG(c, name, 0, t_py_code, member);                                                                    \
	CLASS_ATTR_STYLE(c, name, 0, "onoff");                                                                             \
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, name, 0, def)

void ext_main(void *r) {
	jpatcher_syms_init();

	t_class *c = class_new("py_code", (method)py_code_new, (method)py_code_free, sizeof(t_py_code), 0L, A_GIMME, 0);
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
	class_addmethod(c, (method)pyc::acceptsdrag, "acceptsdrag_locked", A_CANT, 0);
	class_addmethod(c, (method)pyc::acceptsdrag, "acceptsdrag_unlocked", A_CANT, 0);
	class_addmethod(c, (method)pyc::dropfile, "dropfile", A_CANT, 0);
	class_addmethod(c, (method)py_code_focusgained, "focusgained", A_CANT, 0);
	class_addmethod(c, (method)py_code_focuslost, "focuslost", A_CANT, 0);
	class_addmethod(c, (method)py_code_jsave, "jsave", A_CANT, 0);
	class_addmethod(c, (method)py_code_notify, "notify", A_CANT, 0);
	class_addmethod(c, (method)py_code_assist, "assist", A_CANT, 0);
	class_addmethod(c, (method)py_code_themes, "themes", 0);

	CLASS_ATTR_SYM(c, "theme", 0, t_py_code, theme);
	CLASS_ATTR_ACCESSORS(c, "theme", NULL, theme_set);
	CLASS_ATTR_ENUM(c, "theme", 0, theme_names().c_str());
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "theme", 0, theme_at(0).name.c_str());

	CLASS_ATTR_DOUBLE(c, "fontsize", 0, t_py_code, fontsize);
	CLASS_ATTR_FILTER_CLIP(c, "fontsize", 6, 48);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "fontsize", 0, "12");

	CLASS_ATTR_LONG(c, "tabwidth", 0, t_py_code, tabwidth);
	CLASS_ATTR_FILTER_CLIP(c, "tabwidth", 1, 16);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "tabwidth", 0, "4");

	PYC_FLAG("linenumbers", linenumbers, "1");
	PYC_FLAG("titlebar", titlebar, "1");
	PYC_FLAG("matchbrackets", matchbrackets, "1");
	PYC_FLAG("wrap", wrap, "1");
	PYC_FLAG("blink", blink, "1");
	PYC_FLAG("completion", completion, "1");
	PYC_FLAG("statusbar", statusbar, "1");
	PYC_FLAG("autoclose", autoclose, "1");
	PYC_FLAG("autoload", autoload, "1");
	PYC_FLAG("readonly", readonly, "0");
	PYC_FLAG("autowrite", autowrite, "0");

	CLASS_ATTR_LONG(c, "blinkduration", 0, t_py_code, blinkduration);
	CLASS_ATTR_FILTER_CLIP(c, "blinkduration", 50, 5000);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "blinkduration", 0, "500");

	CLASS_ATTR_DOUBLE(c, "scrollspeed", 0, t_py_code, scrollspeed);
	CLASS_ATTR_FILTER_CLIP(c, "scrollspeed", 0.05, 50);
	CLASS_ATTR_DEFAULT_SAVE_PAINT(c, "scrollspeed", 0, "3");

	CLASS_ATTR_SYM(c, "file", 0, t_py_code, file);
	CLASS_ATTR_ACCESSORS(c, "file", NULL, file_set);
	CLASS_ATTR_SAVE(c, "file", 0);

	CLASS_ATTR_SYM(c, "snippets", 0, t_py_code, snippets);
	CLASS_ATTR_ACCESSORS(c, "snippets", NULL, snippets_set);
	CLASS_ATTR_SAVE(c, "snippets", 0);

	CLASS_ATTR_SYM(c, "lsp", 0, t_py_code, lsp);
	CLASS_ATTR_ACCESSORS(c, "lsp", NULL, lsp_set);
	CLASS_ATTR_DEFAULT_SAVE(c, "lsp", 0, "auto");

	for (int i = 0; i < RoleCount; ++i) {
		const char *n = kColorAttr[i];
		class_addattr(c, attr_offset_array_new(n, gensym("float64"), 4, 0, (method)0L, (method)0L, 0,
											   (long)(calcoffset(t_py_code, colors) + i * sizeof(t_jrgba))));
		CLASS_ATTR_ACCESSORS(c, n, NULL, jgraphics_attr_setrgba);
		CLASS_ATTR_STYLE(c, n, 0, "rgba");
		const Color &k = theme_at(0)[i];
		char def[128];
		snprintf(def, sizeof def, "%.4f %.4f %.4f %.4f", k.r, k.g, k.b, k.a);
		CLASS_ATTR_DEFAULT_SAVE_PAINT(c, n, 0, def);
	}

	CLASS_ATTR_DEFAULT(c, "patching_rect", 0, "0. 0. 420. 140.");

	class_register(CLASS_BOX, c);
	s_class = c;

	s_qelem = (t_qelem *)qelem_new(nullptr, (method)lsp_wake);
	lsp::set_wakeup([] {
		if (s_qelem)
			qelem_set(s_qelem);
	});
	quittask_install((method)py_code_quit, nullptr);
}
