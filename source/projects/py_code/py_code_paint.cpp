// SPDX-License-Identifier: MIT
/* py_code_paint.cpp -- drawing. Reads document state; may update only the
 * layout, metrics, syntax tree and the popup window position. */

#include "py_code.h"

#include "ext_systime.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace pyc;

namespace {

struct Painter {
	t_py_code *x;
	State &st;
	t_jgraphics *g;
	Geom gm;
	const std::string &text;

	Painter(t_py_code *x_, t_jgraphics *g_, const Geom &gm_)
		: x(x_), st(*x_->st), g(g_), gm(gm_), text(x_->st->ed.text()) {}

	double lh() const { return st.m.lh; }
	double cw() const { return st.m.cw; }
	double row_top(int r) const { return gm.view_y + st.layout.row_y(r); }

	void fill(const t_jrgba &c, double x0, double y0, double w, double h) {
		if (w <= 0 || h <= 0 || c.alpha <= 0)
			return;
		t_jrgba k = c;
		jgraphics_set_source_jrgba(g, &k);
		jgraphics_rectangle(g, x0, y0, w, h);
		jgraphics_fill(g);
	}

	void hline(const t_jrgba &c, double x0, double x1, double y, double width = 1) {
		t_jrgba k = c;
		jgraphics_set_source_jrgba(g, &k);
		jgraphics_set_line_width(g, width);
		jgraphics_move_to(g, x0, y);
		jgraphics_line_to(g, x1, y);
		jgraphics_stroke(g);
	}

	void vline(const t_jrgba &c, double x0, double y0, double y1) {
		t_jrgba k = c;
		jgraphics_set_source_jrgba(g, &k);
		jgraphics_set_line_width(g, 1);
		jgraphics_move_to(g, x0, y0);
		jgraphics_line_to(g, x0, y1);
		jgraphics_stroke(g);
	}

	/* Control characters and stray bytes are drawn as placeholders. */
	static std::string printable(const std::string &s) {
		std::string o;
		o.reserve(s.size());
		for (int p = 0; p < (int)s.size();) {
			const int q = utf8_next(s, p);
			const unsigned char b = (unsigned char)s[(size_t)p];
			if (q - p == 1 && b >= 0x80)
				o += '?';
			else if (b < 0x20 || b == 0x7f)
				o += ' ';
			else
				o.append(s, (size_t)p, (size_t)(q - p));
			p = q;
		}
		return o;
	}

	void show(const t_jrgba &c, double x0, double baseline, const std::string &s) {
		if (s.empty() || c.alpha <= 0)
			return;
		t_jrgba k = c;
		jgraphics_set_source_jrgba(g, &k);
		jgraphics_move_to(g, x0, baseline);
		jgraphics_show_text(g, printable(s).c_str());
	}

	int cells(const std::string &s) const { return cells_between(s, 0, (int)s.size()); }

	/* Byte range [b, e) of row r. */
	void row_range(int r, int *b, int *e) const {
		const Layout &L = st.layout;
		*b = L.offset_at(r, 0);
		if (r + 1 < L.row_count() && L.row(r + 1).line == L.row(r).line)
			*e = L.offset_at(r + 1, 0);
		else
			*e = st.ed.line_end(L.row(r).line);
	}

	double x_at(int row_begin, int off) const {
		return gm.text_x - st.hscroll + cells_between(text, row_begin, off) * cw();
	}

	/* Fills (or underlines) [a, b) row by row; an empty range underlines one cell. */
	void range(int a, int b, const t_jrgba &c, bool underline) {
		const Layout &L = st.layout;
		if (L.row_count() == 0)
			return;
		const int r0 = std::max(L.row_of(a), L.first_visible_row());
		const int r1 = std::min(L.row_of(b), L.last_visible_row());
		for (int r = r0; r <= r1; ++r) {
			int rb, re;
			row_range(r, &rb, &re);
			const int s = std::max(a, rb), e = std::min(b, re);
			if (s > e || (s == e && !(underline && a == b) && !(b > re && s == re)))
				continue;
			double x0 = x_at(rb, s), x1 = x_at(rb, e);
			const bool last = re == st.ed.line_end(L.row(r).line);
			if ((b > re && last && !underline) || (underline && a == b))
				x1 += cw(); /* selected newline, or a zero-length diagnostic */
			if (x1 <= x0)
				continue;
			if (underline)
				hline(c, x0, x1, row_top(r) + st.m.base + 2.5);
			else
				fill(c, x0, row_top(r), x1 - x0, lh());
		}
	}

	void text_rows() {
		const Layout &L = st.layout;
		static const int kRole[py_code::TokenTypeCount] = {
			RoleText,	RoleKeyword, RoleBuiltin, RoleFunction, RoleClass,	 RoleDecorator, RoleString,
			RoleEscape, RoleNumber,	 RoleSelf,	  RoleComment,	RoleBracket, RoleOperator};
		for (int r = L.first_visible_row(); r <= L.last_visible_row() && r < L.row_count(); ++r) {
			const int line = L.row(r).line;
			if (line >= st.syntax.line_count())
				continue;
			int rb, re;
			row_range(r, &rb, &re);
			const int ls = st.ed.line_start(line);
			const double base = row_top(r) + st.m.base;
			for (const py_code::Token &t : st.syntax.tokens(line)) {
				const int a = std::max(ls + t.col, rb), b = std::min(ls + t.col + t.length, re);
				if (a >= b)
					continue;
				const std::string piece = text.substr((size_t)a, (size_t)(b - a));
				if (piece.find_first_not_of(" \t") == std::string::npos)
					continue;
				const int tt = (int)t.type;
				const int role = tt >= 0 && tt < py_code::TokenTypeCount ? kRole[tt] : RoleText;
				show(color(x, role), x_at(rb, a), base, piece);
			}
		}
	}

	void title() {
		if (gm.title_h <= 0)
			return;
		t_jrgba bg = color(x, RoleBg);
		bg.red += (1 - bg.red) * 0.07;
		bg.green += (1 - bg.green) * 0.07;
		bg.blue += (1 - bg.blue) * 0.07;
		bg.alpha = 1;
		fill(bg, 0, 0, gm.w, gm.title_h);
		hline(color(x, RoleText, 0.12), 0, gm.w, gm.title_h - 0.5);

		std::string right = "python";
		if (st.doc) {
			switch (st.doc->state()) {
			case lsp::State::Ready:
				right += " | " + st.doc->server_name();
				break;
			case lsp::State::Starting:
				right += " | lsp starting";
				break;
			case lsp::State::Failed:
				right += " | lsp failed";
				break;
			default:
				break;
			}
		}
		const double base = st.m.base;
		const t_jrgba dim = color(x, RoleText, 0.55);
		const double rx = gm.w - kPadX - cells(right) * cw();
		show(dim, rx, base, right);
		std::string name = x->file ? x->file->s_name : "";
		const size_t k = name.rfind('/');
		if (k != std::string::npos)
			name = name.substr(k + 1);
		if (!name.empty() && kPadX + (cells(name) + 2) * cw() < rx)
			show(dim, kPadX, base, name);
	}

	void gutter() {
		if (gm.gutter_w <= 0)
			return;
		const Layout &L = st.layout;
		const int caret_line = st.ed.line_of(st.ed.caret());
		jgraphics_save(g);
		jgraphics_clip(g, 0, gm.view_y, gm.gutter_w, gm.view_h);
		for (int r = L.first_visible_row(); r <= L.last_visible_row() && r < L.row_count(); ++r) {
			const Row row = L.row(r);
			if (row.cell_begin != 0)
				continue;
			char num[16];
			snprintf(num, sizeof num, "%d", row.line + 1);
			const double nx = gm.gutter_w - kPadX - cells(num) * cw();
			show(color(x, RoleText, row.line == caret_line ? 0.70 : 0.30), nx, row_top(r) + st.m.base, num);
		}
		jgraphics_restore(g);
		vline(color(x, RoleText, 0.10), gm.gutter_w - 0.5, gm.view_y, gm.view_y + gm.view_h);
	}

	void scrollbar() {
		const Thumb t = st.layout.thumb(gm.view_h);
		if (!t.visible)
			return;
		t_jrgba k = {0x6c / 255.0, 0x70 / 255.0, 0x86 / 255.0, 0.10};
		fill(k, gm.sb_x, gm.view_y, kScrollW, gm.view_h);
		k.alpha = st.thumb_drag ? 0.55 : 0.30;
		jgraphics_set_source_jrgba(g, &k);
		jgraphics_rectangle_rounded(g, gm.sb_x + 1, gm.view_y + t.y, kScrollW - 2, t.h, 4, 4);
		jgraphics_fill(g);
	}

	/* First diagnostic touching the caret line, or null. */
	const lsp::Diagnostic *caret_diagnostic() const {
		if (!st.doc)
			return nullptr;
		const int line = st.ed.line_of(st.ed.caret());
		for (const lsp::Diagnostic &d : st.doc->diagnostics()) {
			const int a = st.ed.line_of(std::max(0, std::min(d.start, st.ed.length())));
			const int b = st.ed.line_of(std::max(0, std::min(d.end, st.ed.length())));
			if (a <= line && line <= b)
				return &d;
		}
		return nullptr;
	}

	void status() {
		if (gm.status_h <= 0)
			return;
		const double y0 = gm.h - gm.status_h;
		fill(color(x, RoleBar), 0, y0, gm.w, gm.status_h);
		hline(color(x, RoleText, 0.12), 0, gm.w, y0 + 0.5);
		jgraphics_save(g);
		jgraphics_clip(g, 0, y0, gm.w, gm.status_h);
		const double base = y0 + 2 + st.m.base;
		const lsp::Signature &sig = st.sig;
		if (!sig.label.empty() && st.doc && st.doc->state() == lsp::State::Ready) {
			int ps = -1, pe = -1;
			if (sig.active >= 0 && sig.active < (int)sig.params.size()) {
				ps = sig.params[(size_t)sig.active].first;
				pe = sig.params[(size_t)sig.active].second;
			}
			const int n = (int)sig.label.size();
			if (ps < 0 || pe < ps || pe > n)
				ps = pe = n;
			const t_jrgba dim = color(x, RoleText, 0.55);
			double cx = kPadX;
			const std::string parts[3] = {sig.label.substr(0, (size_t)ps),
										  sig.label.substr((size_t)ps, (size_t)(pe - ps)), sig.label.substr((size_t)pe)};
			for (int i = 0; i < 3; ++i) {
				show(i == 1 ? color(x, RoleBarHighlight) : dim, cx, base, parts[i]);
				cx += cells(parts[i]) * cw();
			}
		} else if (const lsp::Diagnostic *d = caret_diagnostic()) {
			std::string msg = d->message.substr(0, d->message.find('\n'));
			show(color(x, d->severity == 1 ? RoleError : RoleWarning), kPadX, base, msg);
		}
		jgraphics_restore(g);
	}

	void popup() {
		CompletionSession &cs = st.comp;
		if (!cs.is_open() || cs.count() == 0)
			return;
		const CompletionWindow win = cs.visible(kPopupRows);
		int maxc = 1;
		for (int i = 0; i < cs.count(); ++i) {
			const CompletionItem &it = cs.item(i);
			maxc = std::max(maxc, cells(it.label) + (it.detail.empty() ? 0 : 2 + cells(it.detail)));
		}
		const double pw = std::min(maxc * cw() + 2 * kPadX, gm.w - 4);
		const double ph = win.count * lh() + 2;
		const Layout &L = st.layout;
		const int ws = cs.word_start();
		const int wr = L.row_of(ws);
		int wb, we;
		row_range(wr, &wb, &we);
		const double px = std::max(2.0, std::min(x_at(wb, ws) - kPadX, gm.w - pw - 2));
		const double ry = row_top(L.row_of(st.ed.caret()));
		double py = ry + lh();
		if (py + ph > gm.h - gm.status_h && ry - ph >= 0)
			py = ry - ph;

		fill(color(x, RoleBar, 1), px, py, pw, ph);
		jgraphics_save(g);
		jgraphics_clip(g, px, py, pw, ph);
		for (int k = 0; k < win.count; ++k) {
			const int i = win.first + k;
			const CompletionItem &it = cs.item(i);
			const double y = py + 1 + k * lh();
			if (i == cs.selected())
				fill(color(x, RoleSel), px + 1, y, pw - 2, lh());
			show(color(x, RoleText), px + kPadX, y + st.m.base, it.label);
			if (!it.detail.empty())
				show(color(x, RoleText, 0.5), px + kPadX + (cells(it.label) + 2) * cw(), y + st.m.base,
					 it.detail.substr(0, it.detail.find('\n')));
		}
		jgraphics_restore(g);
		t_jrgba b = color(x, RoleText, 0.15);
		jgraphics_set_source_jrgba(g, &b);
		jgraphics_set_line_width(g, 1);
		jgraphics_rectangle(g, px + 0.5, py + 0.5, pw - 1, ph - 1);
		jgraphics_stroke(g);
	}

	void run() {
		const Layout &L = st.layout;
		Editor &ed = st.ed;
		fill(color(x, RoleBg), 0, 0, gm.w, gm.h);
		title();

		/* Current line band, under the gutter too. */
		jgraphics_save(g);
		jgraphics_clip(g, 0, gm.view_y, gm.w, gm.view_h);
		fill(color(x, RoleCurline), 0, row_top(L.row_of(ed.caret())), gm.w, lh());
		jgraphics_restore(g);

		jgraphics_save(g);
		jgraphics_clip(g, gm.gutter_w, gm.view_y, std::max(0.0, gm.sb_x - gm.gutter_w), gm.view_h);
		if (ed.has_selection())
			range(ed.sel_start(), ed.sel_end(), color(x, RoleSel), false);
		if (st.blinking && x->blinkduration > 0) {
			double t = (double)(systime_ms() - st.blink_t0) / (double)x->blinkduration;
			t = std::max(0.0, std::min(1.0, t));
			const int a = std::min(st.blink_start, ed.length()), b = std::min(st.blink_end, ed.length());
			range(a, b, color(x, RoleBlink, 1 - t * t * t), false);
		}
		if (x->matchbrackets) {
			const py_code::BracketMatch bm = st.syntax.match_bracket(ed.caret());
			if (bm.valid()) {
				range(bm.first, utf8_next(ed.text(), bm.first), color(x, RoleMatch), false);
				range(bm.second, utf8_next(ed.text(), bm.second), color(x, RoleMatch), false);
			}
		}
		text_rows();
		if (st.doc) {
			for (const lsp::Diagnostic &d : st.doc->diagnostics()) {
				const int a = std::max(0, std::min(d.start, ed.length()));
				const int b = std::max(a, std::min(d.end, ed.length()));
				range(a, b, color(x, d.severity == 1 ? RoleError : RoleWarning), true);
			}
		}
		if (ed.length() == 0)
			show(color(x, RoleText, 0.45), gm.text_x, row_top(0) + st.m.base,
				 "shift-ret line | cmd-ret block | cmd-shift-ret all");
		if (st.focused) {
			const int r = L.row_of(ed.caret());
			int rb, re;
			row_range(r, &rb, &re);
			fill(color(x, RoleCaret), x_at(rb, ed.caret()) - 0.8, row_top(r) + 1, 1.6, lh() - 2);
		}
		jgraphics_restore(g);

		gutter();
		scrollbar();
		status();
		popup();
	}
};

} // namespace

void pyc::paint(t_py_code *x, t_object *patcherview) {
	t_jgraphics *g = (t_jgraphics *)patcherview_get_jgraphics(patcherview);
	if (!g || !x->st)
		return;
	State &st = *x->st;
	view_size(x, patcherview);
	const Geom gm = sync_layout(x);
	st.layout.clamp();
	st.syntax.update(st.ed.text(), st.ed.revision());
	if (st.m.font)
		jgraphics_select_jfont(g, st.m.font);
	else {
		jgraphics_select_font_face(g, "Menlo", JGRAPHICS_FONT_SLANT_NORMAL, JGRAPHICS_FONT_WEIGHT_NORMAL);
		jgraphics_set_font_size(g, st.m.size);
	}
	Painter(x, g, gm).run();
}

void pyc::getdrawparams(t_py_code *x, t_object *patcherview, t_jboxdrawparams *params) {
	if (!x->st)
		return;
	const bool f = x->st->focused;
	params->d_borderthickness = f ? 2 : 1;
	params->d_cornersize = 0;
	params->d_bordercolor = f ? color(x, RoleCaret) : color(x, RoleText, 0.15);
	params->d_boxfillcolor = color(x, RoleBg);
}
