// SPDX-License-Identifier: MIT
/* py_repl_paint.cpp -- drawing (py_repl-spec 3.3). Reads state; may update only
 * the layouts, the stack, metrics, syntax trees and the token cache. */

#include "py_repl.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace pyc;

namespace {

const int kRole[py_code::TokenTypeCount] = {RoleText,  RoleKeyword, RoleBuiltin, RoleFunction, RoleClass,
											RoleDecorator, RoleString, RoleEscape, RoleNumber,	 RoleSelf,
											RoleComment, RoleBracket, RoleOperator};

int kind_role(Kind k) {
	switch (k) {
	case Kind::Err:
	case Kind::Traceback: return RoleError;
	case Kind::Info: return RoleComment;
	default: return RoleText;
	}
}

struct Painter {
	t_py_repl *x;
	State &st;
	t_jgraphics *g;
	Geom gm;

	Painter(t_py_repl *x_, t_jgraphics *g_, const Geom &gm_) : x(x_), st(*x_->st), g(g_), gm(gm_) {}

	double lh() const { return st.m.lh; }
	double cw() const { return st.m.cw; }
	double row_top(int stack_row) const { return gm.view_y + st.stack.scroll().row_y(stack_row); }

	void fill(const t_jrgba &c, double x0, double y0, double w, double h) {
		if (w <= 0 || h <= 0 || c.alpha <= 0)
			return;
		t_jrgba k = c;
		jgraphics_set_source_jrgba(g, &k);
		jgraphics_rectangle(g, x0, y0, w, h);
		jgraphics_fill(g);
	}

	void hline(const t_jrgba &c, double x0, double x1, double y) {
		t_jrgba k = c;
		jgraphics_set_source_jrgba(g, &k);
		jgraphics_set_line_width(g, 1);
		jgraphics_move_to(g, x0, y);
		jgraphics_line_to(g, x1, y);
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

	/* Byte range [b, e) of a part's row. */
	void row_range(int part, int r, int *b, int *e) {
		const Layout &L = part_layout(x, part);
		const Editor &ed = part_editor(x, part);
		*b = L.offset_at(r, 0);
		if (r + 1 < L.row_count() && L.row(r + 1).line == L.row(r).line)
			*e = L.offset_at(r + 1, 0);
		else
			*e = ed.line_end(L.row(r).line);
	}

	double x_at(int part, int row_begin, int off) {
		return gm.text_x + cells_between(part_editor(x, part).text(), row_begin, off) * cw();
	}

	/* Draws tokens of `line` (offset `ls`) clipped to [rb, re). */
	void tokens(int part, const std::vector<py_code::Token> &toks, int ls, int rb, int re, double base) {
		const std::string &text = part_editor(x, part).text();
		for (const py_code::Token &t : toks) {
			const int a = std::max(ls + t.col, rb), b = std::min(ls + t.col + t.length, re);
			if (a >= b)
				continue;
			const std::string piece = text.substr((size_t)a, (size_t)(b - a));
			if (piece.find_first_not_of(" \t") == std::string::npos)
				continue;
			const int tt = (int)t.type;
			show(color(x, tt >= 0 && tt < py_code::TokenTypeCount ? kRole[tt] : RoleText), x_at(part, rb, a), base, piece);
		}
	}

	void prompt(int stack_row, bool first) {
		show(x->promptcolor, gm.prompt_x, row_top(stack_row) + st.m.base, first ? ">>>" : "...");
	}

	void transcript_row(int r, int stack_row) {
		const Layout &L = st.tr_layout;
		const Editor &ed = st.tr_ed;
		const std::vector<Span> &spans = st.tr.spans();
		if (st.tr_rev != st.tr.revision())
			return; /* mirror out of date: the next sync fixes it */
		int rb, re;
		row_range(PartTranscript, r, &rb, &re);
		const Row row = L.row(r);
		const int ls = ed.line_start(row.line);
		const double base = row_top(stack_row) + st.m.base;
		auto it = std::upper_bound(spans.begin(), spans.end(), rb, [](int off, const Span &s) { return off < s.start; });
		if (it != spans.begin())
			--it;
		for (; it != spans.end() && it->start < std::max(re, rb + 1); ++it) {
			const int a = std::max(it->start, rb), b = std::min(it->end, re);
			if (it->kind == Kind::Input) {
				const int first_line = ed.line_of(it->start);
				if (row.cell_begin == 0)
					prompt(stack_row, row.line == first_line);
				const std::string code = ed.text().substr((size_t)it->start, (size_t)(it->end - it->start));
				const auto &lines = span_tokens(x, code);
				const int k = row.line - first_line;
				if (k >= 0 && k < (int)lines.size())
					tokens(PartTranscript, lines[(size_t)k], ls, a, b, base);
			} else if (a < b) {
				show(color(x, kind_role(it->kind)), x_at(PartTranscript, rb, a), base,
					 ed.text().substr((size_t)a, (size_t)(b - a)));
			}
		}
	}

	void input_row(int r, int stack_row) {
		const Layout &L = st.in_layout;
		const Row row = L.row(r);
		if (row.cell_begin == 0)
			prompt(stack_row, row.line == 0);
		if (row.line >= st.in_syntax.line_count())
			return;
		int rb, re;
		row_range(PartInput, r, &rb, &re);
		tokens(PartInput, st.in_syntax.tokens(row.line), st.in_ed.line_start(row.line), rb, re,
			   row_top(stack_row) + st.m.base);
	}

	/* Fills [a, b) of one part, row by row. */
	void range(int part, int a, int b, const t_jrgba &c) {
		const Layout &L = part_layout(x, part);
		const Editor &ed = part_editor(x, part);
		const int rows = part == PartInput ? st.stack.bottom_rows() : st.stack.top_rows();
		if (rows == 0)
			return;
		const Scroll &S = st.stack.scroll();
		for (int r = std::min(L.row_of(a), rows - 1); r <= std::min(L.row_of(b), rows - 1); ++r) {
			const int sr = st.stack.row_of(part, r);
			if (sr < S.first_visible_row() || sr > S.last_visible_row())
				continue;
			int rb, re;
			row_range(part, r, &rb, &re);
			const int s = std::max(a, rb), e = std::min(b, re);
			if (s > e || (s == e && !(b > re && s == re)))
				continue;
			double x0 = x_at(part, rb, s), x1 = x_at(part, rb, e);
			if (b > re && re == ed.line_end(L.row(r).line))
				x1 += cw(); /* selected newline */
			if (x1 > x0)
				fill(c, x0, row_top(sr), x1 - x0, lh());
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
		Link link = Link::Ok;
		target(x, &link);
		std::string right = "py: " + std::string(x->py ? x->py->s_name : "");
		if (link == Link::NotFound)
			right += " | not found";
		else if (link == Link::AbiMismatch)
			right += " | abi mismatch";
		const t_jrgba dim = color(x, RoleText, 0.55);
		const double rx = gm.w - kPadX - cells(right) * cw();
		show(dim, rx, st.m.base, right);
		std::string left = "py_repl";
		if (st.hist.browsing()) {
			char buf[48];
			snprintf(buf, sizeof buf, "  history %d/%d", st.hist.position() + 1, st.hist.size());
			left += buf;
		}
		if (kPadX + (cells(left) + 2) * cw() < rx)
			show(dim, kPadX, st.m.base, left);
	}

	void scrollbar() {
		const Thumb t = st.stack.scroll().thumb(gm.view_h);
		if (!t.visible)
			return;
		t_jrgba k = {0x6c / 255.0, 0x70 / 255.0, 0x86 / 255.0, 0.10};
		fill(k, gm.sb_x, gm.view_y, kScrollW, gm.view_h);
		k.alpha = st.thumb_drag ? 0.55 : 0.30;
		jgraphics_set_source_jrgba(g, &k);
		jgraphics_rectangle_rounded(g, gm.sb_x + 1, gm.view_y + t.y, kScrollW - 2, t.h, 4, 4);
		jgraphics_fill(g);
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
		const Layout &L = st.in_layout;
		const int ws = cs.word_start();
		const int wr = L.row_of(ws);
		int wb, we;
		row_range(PartInput, wr, &wb, &we);
		const double px = std::max(2.0, std::min(x_at(PartInput, wb, ws) - kPadX, gm.w - pw - 2));
		const double ry = row_top(st.stack.row_of(PartInput, L.row_of(st.in_ed.caret())));
		double py = ry + lh();
		if (py + ph > gm.h && ry - ph >= gm.view_y)
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
		fill(color(x, RoleBg), 0, 0, gm.w, gm.h);
		title();

		const Scroll &S = st.stack.scroll();
		jgraphics_save(g);
		jgraphics_clip(g, 0, gm.view_y, std::max(0.0, gm.sb_x), gm.view_h);
		for (int part : {PartTranscript, PartInput}) {
			const Editor &ed = part_editor(x, part);
			if (ed.has_selection())
				range(part, ed.sel_start(), ed.sel_end(), color(x, RoleSel));
		}
		const py_code::BracketMatch bm = st.in_syntax.match_bracket(st.in_ed.caret());
		if (st.focus == PartInput && bm.valid()) {
			range(PartInput, bm.first, utf8_next(st.in_ed.text(), bm.first), color(x, RoleMatch));
			range(PartInput, bm.second, utf8_next(st.in_ed.text(), bm.second), color(x, RoleMatch));
		}
		for (int sr = S.first_visible_row(); sr <= S.last_visible_row() && sr < st.stack.rows(); ++sr) {
			const Place p = st.stack.place(sr);
			if (p.part == PartTranscript)
				transcript_row(p.row, sr);
			else
				input_row(p.row, sr);
		}
		if (st.focused && st.focus == PartInput && !st.running) {
			const int r = st.in_layout.row_of(st.in_ed.caret());
			int rb, re;
			row_range(PartInput, r, &rb, &re);
			fill(color(x, RoleCaret), x_at(PartInput, rb, st.in_ed.caret()) - 0.8,
				 row_top(st.stack.row_of(PartInput, r)) + 1, 1.6, lh() - 2);
		}
		jgraphics_restore(g);

		scrollbar();
		popup();
	}
};

} // namespace

void pyc::paint(t_py_repl *x, t_object *patcherview) {
	t_jgraphics *g = (t_jgraphics *)patcherview_get_jgraphics(patcherview);
	if (!g || !x->st)
		return;
	State &st = *x->st;
	view_size(x, patcherview);
	const Geom gm = sync_layout(x);
	st.in_syntax.update(st.in_ed.text(), st.in_ed.revision());
	if (st.m.font)
		jgraphics_select_jfont(g, st.m.font);
	else {
		jgraphics_select_font_face(g, "Menlo", JGRAPHICS_FONT_SLANT_NORMAL, JGRAPHICS_FONT_WEIGHT_NORMAL);
		jgraphics_set_font_size(g, st.m.size);
	}
	Painter(x, g, gm).run();
}

void pyc::getdrawparams(t_py_repl *x, t_object *patcherview, t_jboxdrawparams *params) {
	if (!x->st)
		return;
	const bool f = x->st->focused;
	params->d_borderthickness = f ? 2 : 1;
	params->d_cornersize = 0;
	params->d_bordercolor = f ? color(x, RoleCaret) : color(x, RoleText, 0.15);
	params->d_boxfillcolor = color(x, RoleBg);
}
