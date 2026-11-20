// SPDX-License-Identifier: MIT
/* py_code_input.cpp -- keyboard, mouse, context menu and Finder drops. */

#include "py_code.h"

#include "core/paths.h"
#include "ext_drag.h"
#include "ext_systime.h"
#include "platform/clipboard.h"

#include <sys/stat.h>

#include <algorithm>
#include <cctype>
#include <cmath>

using namespace pyc;

namespace {

enum class Popup { Close, Keep, Refilter };

bool is_word(char c) { return std::isalnum((unsigned char)c) || c == '_'; }

/* Visual-row motion keeping the goal cell; past the first/last row goes to the document start/end. */
void vertical(t_py_code *x, int drows, bool extend, bool page) {
	State &st = *x->st;
	Layout &L = st.layout;
	Editor &ed = st.ed;
	const int r = L.row_of(ed.caret());
	if (st.goal < 0)
		st.goal = L.cell_in_row(ed.caret());
	int t = r + drows;
	if (page && t < 0 && r > 0)
		t = 0;
	if (page && t >= L.row_count() && r < L.row_count() - 1)
		t = L.row_count() - 1;
	if (t < 0)
		ed.set_caret(0, extend);
	else if (t >= L.row_count())
		ed.set_caret(ed.length(), extend);
	else
		ed.set_caret(L.offset_at(t, st.goal), extend);
	if (page)
		L.scroll_by(drows * st.m.lh);
}

/* Text offset under a box-local point; `floor_cell` picks the cell under the point. */
int hit(t_py_code *x, const Geom &g, double px, double py, bool floor_cell) {
	State &st = *x->st;
	const int r = st.layout.row_at_y(py - g.view_y);
	const double f = (px - g.text_x + st.hscroll) / st.m.cw;
	const int cell = std::max(0, (int)std::floor(floor_cell ? f : f + 0.5));
	return st.layout.offset_at(r, cell);
}

/* Shared tail of every input path: notifications, popup, scroll, repaint. */
void finish(t_py_code *x, int rev0, int caret0, int anchor0, Popup popup, bool keep_goal) {
	State &st = *x->st;
	Editor &ed = st.ed;
	if (!keep_goal)
		st.goal = -1;
	switch (popup) {
	case Popup::Close:
		st.comp.close();
		break;
	case Popup::Refilter:
		if (st.comp.is_open())
			st.comp.update(ed.text(), ed.caret());
		break;
	case Popup::Keep:
		break;
	}
	if (ed.revision() != rev0)
		after_edit(x);
	else if (ed.caret() != caret0 || ed.anchor() != anchor0)
		after_move(x);
	reveal_caret(x);
	redraw(x);
}

bool reindent_char(uint32_t c) { return c == ')' || c == ']' || c == '}' || c == ':'; }

void type(t_py_code *x, uint32_t cp, Popup *popup) {
	State &st = *x->st;
	Editor &ed = st.ed;
	ed.type_char(cp, x->autoclose != 0);
	if (reindent_char(cp)) {
		st.syntax.update(ed.text(), ed.revision());
		const int n = st.syntax.reindent_at(ed.caret(), (int)x->tabwidth);
		if (n >= 0)
			ed.set_line_indent(ed.line_of(ed.caret()), n);
	}
	if (cp < 128 && is_word((char)cp)) {
		*popup = Popup::Refilter;
	} else if (cp == '.' && x->completion) {
		st.comp.close();
		if (complete_open(x, true))
			*popup = Popup::Keep;
	}
}

} // namespace

long pyc::key(t_py_code *x, t_object *patcherview, long keycode, long modifiers, long textcharacter) {
	if (!x->st)
		return 0;
	State &st = *x->st;
	Editor &ed = st.ed;
	CompletionSession &cs = st.comp;
	const bool cmd = modifiers & eCommandKey, shift = modifiers & eShiftKey, alt = modifiers & eAltKey;
	if (modifiers & eControlKey)
		return 0; /* Max shortcuts */
	view_size(x, patcherview);
	sync_layout(x);
	if (!x->completion)
		cs.close();
	const bool popup = cs.is_open();
	const bool ro = x->readonly != 0;
	const int tw = (int)x->tabwidth;
	const int rev0 = ed.revision(), caret0 = ed.caret(), anchor0 = ed.anchor();
	Popup pmode = Popup::Close;
	bool keep_goal = false;

	switch (keycode) {
	case JKEY_RETURN:
		if (cmd && shift)
			evaluate(x, py_code::EvalModeDocument);
		else if (cmd)
			evaluate(x, py_code::EvalModeBlock);
		else if (shift)
			evaluate(x, py_code::EvalModeLine);
		else if (popup)
			complete_accept(x);
		else if (!ro) {
			st.syntax.update(ed.text(), ed.revision());
			ed.newline(st.syntax.indent_at(ed.sel_start(), tw));
		}
		break;
	case JKEY_TAB:
		if (cmd)
			return 0;
		if (popup) {
			if (shift) {
				cs.select_prev();
				pmode = Popup::Keep;
			} else
				complete_accept(x);
		} else if (ro) {
		} else if (shift) {
			ed.outdent(tw);
		} else {
			const int c = ed.caret();
			const char prev = c > 0 ? ed.text()[(size_t)c - 1] : ' ';
			if (!ed.has_selection() && x->completion && (is_word(prev) || prev == '.') && complete_open(x, false))
				pmode = Popup::Keep;
			else
				ed.indent(tw);
		}
		break;
	case JKEY_ESC:
		if (!popup)
			return 0;
		break;
	case JKEY_UPARROW:
	case JKEY_DOWNARROW: {
		const bool up = keycode == JKEY_UPARROW;
		if (popup && !shift && !cmd && !alt) {
			if (up)
				cs.select_prev();
			else
				cs.select_next();
			pmode = Popup::Keep;
		} else if (cmd || alt)
			ed.move(up ? Move::DocStart : Move::DocEnd, shift);
		else {
			vertical(x, up ? -1 : 1, shift, false);
			keep_goal = true;
		}
		break;
	}
	case JKEY_PAGEUP:
	case JKEY_PAGEDOWN: {
		const int n = st.layout.page_rows();
		vertical(x, keycode == JKEY_PAGEUP ? -n : n, shift, true);
		keep_goal = true;
		break;
	}
	case JKEY_LEFTARROW:
		ed.move(cmd ? Move::LineStart : alt ? Move::WordLeft : Move::Left, shift);
		break;
	case JKEY_RIGHTARROW:
		ed.move(cmd ? Move::LineEnd : alt ? Move::WordRight : Move::Right, shift);
		break;
	case JKEY_HOME:
		ed.move(Move::LineStart, shift);
		break;
	case JKEY_END:
		ed.move(Move::LineEnd, shift);
		break;
	case JKEY_BACKSPACE:
		if (cmd)
			return 0;
		if (!ro) {
			ed.backspace(x->autoclose != 0, alt, tw);
			pmode = Popup::Refilter;
		}
		break;
	case JKEY_DELETE:
		if (cmd)
			return 0;
		if (!ro)
			ed.delete_forward(alt);
		break;
	default:
		if (cmd) {
			switch (std::tolower((int)keycode)) {
			case 'a':
				ed.select_all();
				break;
			case 'c':
				copy(x, false);
				break;
			case 'x':
				copy(x, !ro);
				break;
			case 'v':
				paste(x); /* no-op when readonly */
				break;
			case 'z':
				if (ro)
					break;
				if (shift)
					ed.redo();
				else
					ed.undo();
				break;
			default:
				return 0;
			}
		} else {
			const uint32_t cp = keycode == JKEY_SPACEBAR ? ' ' : (uint32_t)textcharacter;
			if (cp < 32 || cp == 127 || (cp >= 0xD800 && cp <= 0xDFFF) || (cp >= 0xF700 && cp <= 0xF8FF) || cp > 0x10FFFF)
				return 0; /* control, surrogate or AppKit function-key code */
			if (!ro)
				type(x, cp, &pmode);
		}
		break;
	}
	finish(x, rev0, caret0, anchor0, pmode, keep_goal);
	return 1;
}

/* ---- context menu ---- */

namespace {

enum MenuId { MUndo = 1, MRedo, MCut, MCopy, MPaste, MDelete, MSelectAll, MSave, MLoad };

t_pt screen_point(t_py_code *x, t_object *pv, t_pt pt) {
	t_rect r;
	jbox_get_rect_for_view((t_object *)x, pv, &r);
	long sx0 = 0, sy0 = 0, sx1 = 0, sy1 = 0;
	patcherview_canvas_to_screen(pv, r.x, r.y, &sx0, &sy0);
	patcherview_canvas_to_screen(pv, r.x + r.width, r.y + r.height, &sx1, &sy1);
	const double kx = r.width > 0 ? (sx1 - sx0) / r.width : 1;
	const double ky = r.height > 0 ? (sy1 - sy0) / r.height : 1;
	t_pt s;
	s.x = sx0 + pt.x * kx;
	s.y = sy0 + pt.y * ky;
	return s;
}

void context_menu(t_py_code *x, t_object *pv, t_pt pt) {
	State &st = *x->st;
	Editor &ed = st.ed;
	const bool ro = x->readonly != 0, sel = ed.has_selection();
	const bool has_file = x->file && x->file->s_name[0];
	t_jpopupmenu *m = jpopupmenu_create();
	jpopupmenu_additem(m, MUndo, "Undo", nullptr, 0, ro || !ed.can_undo(), nullptr);
	jpopupmenu_additem(m, MRedo, "Redo", nullptr, 0, ro || !ed.can_redo(), nullptr);
	jpopupmenu_addseparator(m);
	jpopupmenu_additem(m, MCut, "Cut", nullptr, 0, ro || !sel, nullptr);
	jpopupmenu_additem(m, MCopy, "Copy", nullptr, 0, !sel, nullptr);
	jpopupmenu_additem(m, MPaste, "Paste", nullptr, 0, ro || clipboard_text().empty(), nullptr);
	jpopupmenu_additem(m, MDelete, "Delete", nullptr, 0, ro || !sel, nullptr);
	jpopupmenu_additem(m, MSelectAll, "Select All", nullptr, 0, ed.length() == 0, nullptr);
	jpopupmenu_addseparator(m);
	jpopupmenu_additem(m, MSave, has_file ? "Save to .py" : "Save to .py...", nullptr, 0, 0, nullptr);
	jpopupmenu_additem(m, MLoad, "Load from .py...", nullptr, 0, ro, nullptr);
	const long id = st.id;
	const int choice = jpopupmenu_popup(m, screen_point(x, pv, pt), 0);
	jpopupmenu_destroy(m);
	if (find_box(id) != x)
		return; /* freed while the menu was up */

	const int rev0 = ed.revision(), caret0 = ed.caret(), anchor0 = ed.anchor();
	switch (choice) {
	case MUndo:
		ed.undo();
		break;
	case MRedo:
		ed.redo();
		break;
	case MCut:
		copy(x, true);
		break;
	case MCopy:
		copy(x, false);
		break;
	case MPaste:
		paste(x);
		break;
	case MDelete:
		if (!ro)
			ed.delete_forward(false);
		break;
	case MSelectAll:
		ed.select_all();
		break;
	case MSave:
		menu_save(x);
		return;
	case MLoad:
		menu_load(x);
		return;
	default:
		return;
	}
	finish(x, rev0, caret0, anchor0, Popup::Close, false);
}

void drag_select(t_py_code *x, const Geom &g) {
	State &st = *x->st;
	const double y = std::max(g.view_y, std::min(st.mouse_y, g.view_y + g.view_h - 1));
	const int caret0 = st.ed.caret();
	st.ed.set_caret(hit(x, g, st.mouse_x, y, false), true);
	if (st.ed.caret() != caret0)
		after_move(x);
}

bool in_text(const Geom &g, t_pt pt) { return pt.y >= g.title_h && pt.y < g.h - g.status_h && pt.x < g.sb_x; }

} // namespace

/* ---- mouse ---- */

void pyc::mousedown(t_py_code *x, t_object *patcherview, t_pt pt, long modifiers) {
	if (!x->st)
		return;
	State &st = *x->st;
	Editor &ed = st.ed;
	view_size(x, patcherview);
	const Geom g = sync_layout(x);
	st.comp.close();
	if (modifiers & (eRightButton | ePopupMenu)) {
		redraw(x);
		context_menu(x, patcherview, pt);
		return;
	}
	if (pt.y < g.title_h || pt.y >= g.h - g.status_h) {
		redraw(x);
		return;
	}
	const Thumb t = st.layout.thumb(g.view_h);
	if (t.visible && pt.x >= g.sb_x) {
		const double ty = pt.y - g.view_y;
		if (ty >= t.y && ty < t.y + t.h)
			st.thumb_grab = ty - t.y;
		else {
			st.thumb_grab = t.h / 2;
			st.layout.drag_thumb(ty - t.h / 2, g.view_h);
		}
		st.thumb_drag = true;
		redraw(x);
		return;
	}
	const unsigned now = systime_ms();
	if (st.clicks > 0 && now - st.click_t <= 400 && std::fabs(pt.x - st.click_x) <= 4 &&
		std::fabs(pt.y - st.click_y) <= 4)
		st.clicks = st.clicks % 3 + 1;
	else
		st.clicks = 1;
	st.click_t = now;
	st.click_x = pt.x;
	st.click_y = pt.y;

	const int rev0 = ed.revision(), caret0 = ed.caret(), anchor0 = ed.anchor();
	if (st.clicks == 2)
		ed.select_word_at(hit(x, g, pt.x, pt.y, true));
	else if (st.clicks == 3)
		ed.select_line_at(hit(x, g, pt.x, pt.y, true));
	else
		ed.set_caret(hit(x, g, pt.x, pt.y, false), (modifiers & eShiftKey) != 0);
	st.selecting = true;
	st.mouse_x = pt.x;
	st.mouse_y = pt.y;
	st.goal = -1;
	if (ed.revision() == rev0 && (ed.caret() != caret0 || ed.anchor() != anchor0))
		after_move(x);
	redraw(x);
}

void pyc::mousedrag(t_py_code *x, t_object *patcherview, t_pt pt, long modifiers) {
	if (!x->st)
		return;
	State &st = *x->st;
	view_size(x, patcherview);
	const Geom g = sync_layout(x);
	st.mouse_x = pt.x;
	st.mouse_y = pt.y;
	if (st.thumb_drag) {
		st.layout.drag_thumb(pt.y - g.view_y - st.thumb_grab, g.view_h);
		redraw(x);
		return;
	}
	if (!st.selecting)
		return;
	drag_select(x, g);
	const bool outside = pt.y < g.view_y || pt.y >= g.view_y + g.view_h;
	if (outside && !st.autoscroll) {
		st.autoscroll = true;
		clock_fdelay(st.scroll_clock, 30);
	}
	redraw(x);
}

void pyc::scroll_tick(t_py_code *x) {
	State &st = *x->st;
	st.autoscroll = false;
	if (!st.selecting)
		return;
	const Geom g = sync_layout(x);
	const double top = g.view_y, bottom = g.view_y + g.view_h;
	const double d = st.mouse_y < top ? st.mouse_y - top : st.mouse_y > bottom ? st.mouse_y - bottom : 0;
	if (d == 0)
		return;
	st.layout.scroll_by(d * 0.5); /* px per 30 ms tick, proportional to the distance */
	drag_select(x, g);
	st.autoscroll = true;
	clock_fdelay(st.scroll_clock, 30);
	redraw(x);
}

void pyc::mouseup(t_py_code *x, t_object *patcherview, t_pt pt, long modifiers) {
	if (!x->st)
		return;
	State &st = *x->st;
	st.selecting = false;
	st.thumb_drag = false;
	st.autoscroll = false;
	clock_unset(st.scroll_clock);
	redraw(x);
}

void pyc::mouseenter(t_py_code *x, t_object *patcherview, t_pt pt, long modifiers) {
	jmouse_setcursor(patcherview, (t_object *)x, JMOUSE_CURSOR_IBEAM);
}

void pyc::mousemove(t_py_code *x, t_object *patcherview, t_pt pt, long modifiers) {
	if (!x->st)
		return;
	view_size(x, patcherview);
	const Geom g = geometry(x, x->st->box_w, x->st->box_h);
	jmouse_setcursor(patcherview, (t_object *)x, in_text(g, pt) ? JMOUSE_CURSOR_IBEAM : JMOUSE_CURSOR_ARROW);
}

void pyc::mouseleave(t_py_code *x, t_object *patcherview, t_pt pt, long modifiers) {
	jmouse_setcursor(patcherview, (t_object *)x, JMOUSE_CURSOR_ARROW);
}

void pyc::mousewheel(t_py_code *x, t_object *patcherview, t_pt pt, long modifiers, double dx, double dy) {
	if (!x->st)
		return;
	State &st = *x->st;
	view_size(x, patcherview);
	sync_layout(x);
	st.comp.close();
	st.layout.scroll_by(-dy * st.m.lh * x->scrollspeed);
	if (!x->wrap)
		st.hscroll = std::max(0.0, st.hscroll - dx * st.m.cw * x->scrollspeed);
	redraw(x);
}

/* ---- Finder drops ---- */

long pyc::acceptsdrag(t_py_code *x, t_object *drag, t_object *view) {
	if (!x->st || x->readonly || jdrag_itemcount(drag) < 1)
		return 0;
	const char *s = jdrag_getitemstring(drag, 0, 0, 0);
	struct stat sb;
	if (!s || stat(posix_path(s).c_str(), &sb) != 0)
		return 0; /* also rejects object drags */
	jdrag_add(drag, (t_object *)x, gensym("dropfile"), "Insert path", nullptr);
	return 1;
}

void pyc::dropfile(t_py_code *x, t_object *drag, void *arg) {
	if (!x->st || x->readonly)
		return;
	State &st = *x->st;
	std::vector<std::string> names;
	const long n = jdrag_itemcount(drag);
	for (long i = 0; i < n; ++i)
		if (const char *s = jdrag_getitemstring(drag, i, 0, 0))
			names.push_back(s);
	if (names.empty())
		return;
	const int rev0 = st.ed.revision(), caret0 = st.ed.caret(), anchor0 = st.ed.anchor();
	st.ed.insert(drop_text(home_dir(), names));
	finish(x, rev0, caret0, anchor0, Popup::Close, false);
}
