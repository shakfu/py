// SPDX-License-Identifier: MIT
/* editor.cpp -- see editor.h. */

#include "core/editor.h"

#include <algorithm>

namespace pyc {

/* ---- UTF-8 ---- */

static bool is_cont(unsigned char b) { return (b & 0xC0) == 0x80; }

/* Length of the code point at `off`, 1 for any invalid sequence. */
static int cp_len(const std::string &s, int off) {
	const unsigned char b = (unsigned char)s[(size_t)off];
	int n = 1;
	if ((b & 0xE0) == 0xC0)
		n = 2;
	else if ((b & 0xF0) == 0xE0)
		n = 3;
	else if ((b & 0xF8) == 0xF0)
		n = 4;
	if (off + n > (int)s.size())
		return 1;
	for (int i = 1; i < n; ++i)
		if (!is_cont((unsigned char)s[(size_t)(off + i)]))
			return 1;
	return n;
}

int utf8_next(const std::string &s, int off) {
	if (off < 0)
		return 0;
	if (off >= (int)s.size())
		return (int)s.size();
	return off + cp_len(s, off);
}

int utf8_prev(const std::string &s, int off) {
	off = std::min(off, (int)s.size());
	if (off <= 0)
		return 0;
	for (int k = 1; k <= 3 && off - k >= 0; ++k) {
		const int c = off - k;
		if (!is_cont((unsigned char)s[(size_t)c]))
			return cp_len(s, c) == k ? c : off - 1;
	}
	return off - 1;
}

int utf8_snap(const std::string &s, int off) {
	const int n = (int)s.size();
	if (off <= 0)
		return 0;
	if (off >= n)
		return n;
	for (int k = 1; k <= 3 && off - k >= 0; ++k) {
		const int c = off - k;
		if (!is_cont((unsigned char)s[(size_t)c]))
			return c + cp_len(s, c) > off ? c : off;
	}
	return off;
}

std::string utf8_encode(uint32_t cp) {
	std::string o;
	if (cp == 0 || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
		return o;
	if (cp < 0x80) {
		o += (char)cp;
	} else if (cp < 0x800) {
		o += (char)(0xC0 | (cp >> 6));
		o += (char)(0x80 | (cp & 0x3F));
	} else if (cp < 0x10000) {
		o += (char)(0xE0 | (cp >> 12));
		o += (char)(0x80 | ((cp >> 6) & 0x3F));
		o += (char)(0x80 | (cp & 0x3F));
	} else {
		o += (char)(0xF0 | (cp >> 18));
		o += (char)(0x80 | ((cp >> 12) & 0x3F));
		o += (char)(0x80 | ((cp >> 6) & 0x3F));
		o += (char)(0x80 | (cp & 0x3F));
	}
	return o;
}

/* ---- character classes ---- */

static bool is_word(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}
static bool is_blank(char c) { return c == ' ' || c == '\t' || c == '\r'; }
static bool is_space(char c) { return is_blank(c) || c == '\n'; }

enum Class { Newline, Word, Blank, Other };
static Class class_of(char c) {
	if (c == '\n')
		return Newline;
	if (is_word(c))
		return Word;
	return is_blank(c) ? Blank : Other;
}

static char closer_of(char c) {
	switch (c) {
	case '(': return ')';
	case '[': return ']';
	case '{': return '}';
	case '\'': return '\'';
	case '"': return '"';
	default: return 0;
	}
}

static bool is_empty_pair(char a, char b) {
	return (a == '(' || a == '[' || a == '{' || a == '\'' || a == '"') && closer_of(a) == b;
}

/* ---- Editor ---- */

Editor::Editor(const std::string &text) : mText(text) { index_lines(); }

void Editor::index_lines() {
	mLineStarts.assign(1, 0);
	for (size_t i = 0; i < mText.size(); ++i)
		if (mText[i] == '\n')
			mLineStarts.push_back((int)i + 1);
}

int Editor::line_start(int line) const {
	line = std::max(0, std::min(line, line_count() - 1));
	return mLineStarts[(size_t)line];
}

int Editor::line_end(int line) const {
	line = std::max(0, std::min(line, line_count() - 1));
	return line + 1 < line_count() ? mLineStarts[(size_t)line + 1] - 1 : length();
}

int Editor::line_of(int offset) const {
	auto it = std::upper_bound(mLineStarts.begin(), mLineStarts.end(), offset);
	return std::max(0, (int)(it - mLineStarts.begin()) - 1);
}

std::string Editor::line_text(int line) const {
	const int s = line_start(line);
	return mText.substr((size_t)s, (size_t)(line_end(line) - s));
}

int Editor::cells_before(int offset) const {
	offset = clamp(offset);
	int n = 0;
	for (int p = line_start(line_of(offset)); p < offset; p = utf8_next(mText, p))
		++n;
	return n;
}

int Editor::offset_at_cell(int line, int cell) const {
	int p = line_start(line);
	const int e = line_end(line);
	for (; cell > 0 && p < e; --cell)
		p = utf8_next(mText, p);
	return p;
}

std::string Editor::selected_text() const {
	return mText.substr((size_t)sel_start(), (size_t)(sel_end() - sel_start()));
}

/* ---- selection and motion ---- */

void Editor::select(int anchor, int caret) {
	mAnchor = clamp(anchor);
	mCaret = clamp(caret);
	mGoal = -1;
	mCanJoin = false;
	end_group();
}

void Editor::collapse(int offset) { select(offset, offset); }

void Editor::set_caret(int offset, bool extend) { select(extend ? mAnchor : offset, offset); }

void Editor::select_all() { select(0, length()); }

void Editor::select_word_at(int offset) {
	offset = clamp(offset);
	if (offset >= length() || mText[(size_t)offset] == '\n') {
		collapse(offset);
		return;
	}
	const Class c = class_of(mText[(size_t)offset]);
	int s = offset, e = offset;
	while (s > 0 && class_of(mText[(size_t)utf8_prev(mText, s)]) == c)
		s = utf8_prev(mText, s);
	while (e < length() && class_of(mText[(size_t)e]) == c)
		e = utf8_next(mText, e);
	select(s, e);
}

void Editor::select_line_at(int offset) {
	const int l = line_of(clamp(offset));
	select(line_start(l), line_end(l));
}

int Editor::word_left(int p) const {
	while (p > 0 && is_space(mText[(size_t)p - 1]))
		--p;
	if (p == 0)
		return 0;
	const Class c = class_of(mText[(size_t)utf8_prev(mText, p)]);
	while (p > 0 && class_of(mText[(size_t)utf8_prev(mText, p)]) == c)
		p = utf8_prev(mText, p);
	return p;
}

int Editor::word_right(int p) const {
	while (p < length() && is_space(mText[(size_t)p]))
		++p;
	if (p == length())
		return p;
	const Class c = class_of(mText[(size_t)p]);
	while (p < length() && class_of(mText[(size_t)p]) == c)
		p = utf8_next(mText, p);
	return p;
}

int Editor::first_nonblank(int line) const {
	int p = line_start(line);
	const int e = line_end(line);
	while (p < e && (mText[(size_t)p] == ' ' || mText[(size_t)p] == '\t'))
		++p;
	return p;
}

void Editor::move(Move m, bool extend) {
	const bool collapse_sel = has_selection() && !extend;
	const int line = line_of(mCaret);
	int to = mCaret;
	switch (m) {
	case Move::Left: to = collapse_sel ? sel_start() : utf8_prev(mText, mCaret); break;
	case Move::Right: to = collapse_sel ? sel_end() : utf8_next(mText, mCaret); break;
	case Move::WordLeft: to = word_left(mCaret); break;
	case Move::WordRight: to = word_right(mCaret); break;
	case Move::Up:
	case Move::Down: {
		const int goal = mGoal >= 0 ? mGoal : cells_before(mCaret);
		if (m == Move::Up)
			to = line == 0 ? 0 : offset_at_cell(line - 1, goal);
		else
			to = line == line_count() - 1 ? length() : offset_at_cell(line + 1, goal);
		set_caret(to, extend);
		mGoal = goal;
		return;
	}
	case Move::LineStart: {
		const int fnb = first_nonblank(line);
		to = mCaret == fnb ? line_start(line) : fnb;
		break;
	}
	case Move::LineEnd: to = line_end(line); break;
	case Move::DocStart: to = 0; break;
	case Move::DocEnd: to = length(); break;
	}
	set_caret(to, extend);
}

/* ---- edits ---- */

void Editor::apply(int start, int end, const std::string &s) {
	mText.replace((size_t)start, (size_t)(end - start), s);
	index_lines();
	++mRevision;
}

void Editor::edit(int start, int end, const std::string &s, int anchor, int caret, Group kind, bool join) {
	Record r{start, mText.substr((size_t)start, (size_t)(end - start)), s, mAnchor, mCaret, anchor, caret};
	apply(start, end, s);
	mAnchor = anchor;
	mCaret = caret;
	mGoal = -1;
	if (join && !mUndo.empty())
		mUndo.back().push_back(std::move(r));
	else
		mUndo.push_back(Records{std::move(r)});
	++mRecords;
	mRedo.clear();
	mGroup = kind;
	mCanJoin = true;
	while (mRecords > kHistoryLimit && mUndo.size() > 1) {
		mRecords -= (int)mUndo.front().size();
		mUndo.erase(mUndo.begin());
	}
}

bool Editor::insert(const std::string &s) {
	if (s.empty() && !has_selection())
		return false;
	const int a = sel_start() + (int)s.size();
	edit(sel_start(), sel_end(), s, a, a, Group::None, false);
	return true;
}

bool Editor::type_char(uint32_t codepoint, bool autoclose) {
	const std::string ch = utf8_encode(codepoint);
	if (ch.empty())
		return false;
	const int s = sel_start(), e = sel_end();
	const bool sel = s != e;
	const char c = codepoint < 0x80 ? (char)codepoint : 0;
	const char close = closer_of(c);

	if (autoclose && sel && close) { /* wrap, inner text stays selected */
		const bool forward = mAnchor <= mCaret;
		const std::string wrapped = c + mText.substr((size_t)s, (size_t)(e - s)) + close;
		edit(s, e, wrapped, forward ? s + 1 : e + 1, forward ? e + 1 : s + 1, Group::None, false);
		return true;
	}
	if (autoclose && !sel) {
		const int at = mCaret;
		const char next = at < length() ? mText[(size_t)at] : 0;
		const bool is_quote = c == '\'' || c == '"';
		if ((c == ')' || c == ']' || c == '}' || is_quote) && next == c) { /* type over */
			mAnchor = mCaret = at + 1;
			mGoal = -1;
			return false;
		}
		if (is_quote) {
			if (at >= 2 && mText[(size_t)at - 1] == c && mText[(size_t)at - 2] == c) {
				edit(at, at, std::string(4, c), at + 1, at + 1, Group::None, false);
				return true;
			}
			int w = at;
			while (w > 0 && is_word(mText[(size_t)w - 1]))
				--w;
			bool pair = at - w <= 2 && !is_word(next);
			for (int i = w; i < at && pair; ++i)
				pair = std::string("rRbBuUfFtT").find(mText[(size_t)i]) != std::string::npos;
			if (pair) {
				edit(at, at, std::string(2, c), at + 1, at + 1, Group::None, false);
				return true;
			}
		} else if (close && !is_word(next)) {
			edit(at, at, std::string(1, c) + close, at + 1, at + 1, Group::None, false);
			return true;
		}
	}

	/* Plain typing; a whitespace after a non-whitespace closes the group. */
	const char before = s > 0 ? mText[(size_t)s - 1] : '\n';
	const int a = s + (int)ch.size();
	edit(s, e, ch, a, a, Group::Typing, !sel && mGroup == Group::Typing);
	if (is_space(c) && !is_space(before))
		end_group();
	return true;
}

bool Editor::backspace(bool autoclose, bool word, int tabwidth) {
	if (has_selection()) {
		edit(sel_start(), sel_end(), "", sel_start(), sel_start(), Group::None, false);
		return true;
	}
	const int at = mCaret;
	if (at == 0)
		return false;
	int from = utf8_prev(mText, at), to = at;
	if (word) {
		from = word_left(at);
	} else if (autoclose && at < length() && is_empty_pair(mText[(size_t)at - 1], mText[(size_t)at])) {
		to = at + 1;
	} else {
		const int ls = line_start(line_of(at));
		bool spaces = at > ls;
		for (int p = ls; p < at && spaces; ++p)
			spaces = mText[(size_t)p] == ' ';
		if (spaces) {
			const int tw = std::max(1, tabwidth);
			from = ls + (at - ls - 1) / tw * tw;
		}
	}
	edit(from, to, "", from, from, Group::Backspace, mGroup == Group::Backspace);
	return true;
}

bool Editor::delete_forward(bool word) {
	if (has_selection()) {
		edit(sel_start(), sel_end(), "", sel_start(), sel_start(), Group::None, false);
		return true;
	}
	const int at = mCaret;
	if (at == length())
		return false;
	const int to = word ? word_right(at) : utf8_next(mText, at);
	edit(at, to, "", at, at, Group::Delete, mGroup == Group::Delete);
	return true;
}

bool Editor::newline(int indent) {
	const std::string ind((size_t)std::max(0, indent), ' ');
	const int s = sel_start(), e = sel_end();
	int p = s, q = e;
	while (p > 0 && mText[(size_t)p - 1] == ' ')
		--p;
	while (q < length() && mText[(size_t)q] == ' ')
		++q;
	const char open = p > 0 ? mText[(size_t)p - 1] : 0;
	if (open && open != '\'' && open != '"' && q < length() && closer_of(open) == mText[(size_t)q]) {
		const int ol = line_of(p - 1);
		const std::string oind = mText.substr((size_t)line_start(ol), (size_t)(first_nonblank(ol) - line_start(ol)));
		const int a = p + 1 + (int)ind.size();
		edit(p, q, "\n" + ind + "\n" + oind, a, a, Group::None, false);
		return true;
	}
	const int a = s + 1 + (int)ind.size();
	edit(s, e, "\n" + ind, a, a, Group::None, false);
	return true;
}

bool Editor::indent(int tabwidth) {
	const int tw = std::max(1, tabwidth);
	const int s = sel_start(), e = sel_end();
	if (s == e || line_of(s) == line_of(e)) {
		const int n = tw - cells_before(s) % tw;
		edit(s, e, std::string((size_t)n, ' '), s + n, s + n, Group::None, false);
		return true;
	}
	return shift_lines(tw, true);
}

bool Editor::outdent(int tabwidth) { return shift_lines(std::max(1, tabwidth), false); }

/* Adds `tw` spaces to, or removes up to `tw` leading spaces from, the lines
 * the selection touches; one record for the block. */
bool Editor::shift_lines(int tw, bool add) {
	const int s = sel_start(), e = sel_end();
	const int l0 = line_of(s);
	int l1 = line_of(e);
	if (l1 > l0 && e == line_start(l1))
		--l1;
	std::vector<int> delta((size_t)(l1 - l0 + 1));
	std::string block;
	bool changed = false;
	for (int l = l0; l <= l1; ++l) {
		std::string t = line_text(l);
		int d = tw;
		if (add) {
			t.insert(0, (size_t)tw, ' ');
		} else {
			d = 0;
			while (d < tw && d < (int)t.size() && t[(size_t)d] == ' ')
				++d;
			t.erase(0, (size_t)d);
			d = -d;
		}
		changed |= d != 0;
		delta[(size_t)(l - l0)] = d;
		block += t;
		if (l < l1)
			block += '\n';
	}
	if (!changed)
		return false;
	auto map = [&](int pos) {
		const int l = line_of(pos);
		int cum = 0;
		for (int i = l0; i < l && i <= l1; ++i)
			cum += delta[(size_t)(i - l0)];
		if (l > l1)
			return pos + cum;
		const int ls = line_start(l), d = delta[(size_t)(l - l0)];
		if (add)
			return cum + (pos == ls ? ls : pos + d);
		return cum + ls + std::max(0, pos - ls + d);
	};
	edit(line_start(l0), line_end(l1), block, map(mAnchor), map(mCaret), Group::None, false);
	return true;
}

bool Editor::set_line_indent(int line, int spaces) {
	if (line < 0 || line >= line_count())
		return false;
	spaces = std::max(0, spaces);
	const int ls = line_start(line);
	const int ws = first_nonblank(line);
	const std::string ind((size_t)spaces, ' ');
	if (mText.compare((size_t)ls, (size_t)(ws - ls), ind) == 0)
		return false;
	const int delta = spaces - (ws - ls);
	auto map = [&](int pos) {
		if (pos >= ws)
			return pos + delta;
		return pos < ls ? pos : ls + std::min(pos - ls, spaces);
	};
	edit(ls, ws, ind, map(mAnchor), map(mCaret), mGroup, mCanJoin);
	return true;
}

void Editor::reset(const std::string &text) {
	mText = text;
	index_lines();
	++mRevision;
	mAnchor = mCaret = 0;
	mGoal = -1;
	mUndo.clear();
	mRedo.clear();
	mRecords = 0;
	mGroup = Group::None;
	mCanJoin = false;
}

/* ---- undo ---- */

bool Editor::undo() {
	if (mUndo.empty())
		return false;
	Records g = std::move(mUndo.back());
	mUndo.pop_back();
	mRecords -= (int)g.size();
	for (auto r = g.rbegin(); r != g.rend(); ++r)
		apply(r->start, r->start + (int)r->inserted.size(), r->removed);
	mAnchor = g.front().anchor_before;
	mCaret = g.front().caret_before;
	mRedo.push_back(std::move(g));
	mGoal = -1;
	mGroup = Group::None;
	mCanJoin = false;
	return true;
}

bool Editor::redo() {
	if (mRedo.empty())
		return false;
	Records g = std::move(mRedo.back());
	mRedo.pop_back();
	for (const Record &r : g)
		apply(r.start, r.start + (int)r.removed.size(), r.inserted);
	mAnchor = g.back().anchor_after;
	mCaret = g.back().caret_after;
	mRecords += (int)g.size();
	mUndo.push_back(std::move(g));
	mGoal = -1;
	mGroup = Group::None;
	mCanJoin = false;
	return true;
}

} // namespace pyc
