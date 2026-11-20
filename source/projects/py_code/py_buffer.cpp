// SPDX-License-Identifier: MIT
/* py_buffer.cpp -- see py_buffer.h. */

#include "py_buffer.h"

#include <algorithm>
#include <cstring>

namespace py_code {

Buffer::Buffer(const std::string &text) {
	mAutoClose = false;
	mRevision = 0;
	set_text(text);
}

/**********************************************************************/
/* UTF-8                                                               */

int Buffer::next_codepoint(const std::string &s, int offset) {
	const int n = (int)s.size();
	if (offset >= n)
		return n;
	if (offset < 0)
		return 0;

	const unsigned char c = (unsigned char)s[(size_t)offset];
	int len;
	if ((c & 0x80) == 0x00)
		len = 1;
	else if ((c & 0xE0) == 0xC0)
		len = 2;
	else if ((c & 0xF0) == 0xE0)
		len = 3;
	else if ((c & 0xF8) == 0xF0)
		len = 4;
	else
		len = 1; /* stray continuation byte or 0xF8+ (invalid UTF-8) */

	const int o = offset + len;
	return o > n ? n : o;
}

int Buffer::prev_codepoint(const std::string &s, int offset) {
	if (offset <= 0)
		return 0;
	int o = offset - 1;
	while (o > 0 && ((unsigned char)s[(size_t)o] & 0xC0) == 0x80)
		--o;
	return o;
}

std::string Buffer::encode_utf8(unsigned int cp) {
	std::string out;
	if (cp < 0x80) {
		out += (char)cp;
	} else if (cp < 0x800) {
		out += (char)(0xC0 | (cp >> 6));
		out += (char)(0x80 | (cp & 0x3F));
	} else if (cp < 0x10000) {
		out += (char)(0xE0 | (cp >> 12));
		out += (char)(0x80 | ((cp >> 6) & 0x3F));
		out += (char)(0x80 | (cp & 0x3F));
	} else if (cp <= 0x10FFFF) {
		out += (char)(0xF0 | (cp >> 18));
		out += (char)(0x80 | ((cp >> 12) & 0x3F));
		out += (char)(0x80 | ((cp >> 6) & 0x3F));
		out += (char)(0x80 | (cp & 0x3F));
	} else {
		out += '?'; /* outside the Unicode range: we do not invent anything */
	}
	return out;
}

bool Buffer::is_word_byte(unsigned char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

/**********************************************************************/
/* document and line index                                             */

void Buffer::set_text(const std::string &text) {
	mText = text;
	mCaret = 0;
	mAnchor = 0;
	mGoalCol = -1;
	mRun = RunNone;
	mUndo.clear();
	mRedo.clear();
	invalidate_lines();
}

void Buffer::invalidate_lines() {
	mLinesDirty = true;
	++mRevision; /* single place where "the text changed" is marked (see revision()) */
}

void Buffer::ensure_lines() const {
	if (!mLinesDirty)
		return;
	mLineStarts.clear();
	mLineStarts.push_back(0);
	for (size_t i = 0; i < mText.size(); ++i)
		if (mText[i] == '\n')
			mLineStarts.push_back((int)i + 1);
	mLinesDirty = false;
}

int Buffer::line_count() const {
	ensure_lines();
	return (int)mLineStarts.size();
}

int Buffer::line_start(int line) const {
	ensure_lines();
	if (line < 0)
		line = 0;
	if (line >= (int)mLineStarts.size())
		line = (int)mLineStarts.size() - 1;
	return mLineStarts[(size_t)line];
}

int Buffer::line_length(int line) const {
	ensure_lines();
	if (line < 0)
		line = 0;
	if (line >= (int)mLineStarts.size())
		line = (int)mLineStarts.size() - 1;
	const int start = mLineStarts[(size_t)line];
	const int end = (line + 1 < (int)mLineStarts.size()) ? mLineStarts[(size_t)line + 1] - 1 : length();
	return end - start;
}

std::string Buffer::line_text(int line) const {
	return mText.substr((size_t)line_start(line), (size_t)line_length(line));
}

int Buffer::line_of_offset(int offset) const {
	ensure_lines();
	const int o = std::max(0, std::min(offset, length()));
	int lo = 0, hi = (int)mLineStarts.size() - 1;
	while (lo < hi) {
		const int mid = (lo + hi + 1) / 2;
		if (mLineStarts[(size_t)mid] <= o)
			lo = mid;
		else
			hi = mid - 1;
	}
	return lo;
}

int Buffer::col_of_offset(int offset) const {
	const int o = std::max(0, std::min(offset, length()));
	return o - line_start(line_of_offset(o));
}

int Buffer::display_col_of_offset(int offset) const {
	const int o = std::max(0, std::min(offset, length()));
	const int ls = line_start(line_of_offset(o));
	int col = 0;
	for (int i = ls; i < o; ++i) {
		/* UTF-8 continuation bytes start with 10xxxxxx: a new code point is any
		 * other byte (ASCII or leading byte). */
		if (((unsigned char)mText[(size_t)i] & 0xC0) != 0x80)
			++col;
	}
	return col;
}

int Buffer::offset_of_display_col(int line, int col) const {
	if (line < 0)
		line = 0;
	if (line > line_count() - 1)
		line = line_count() - 1;
	if (col < 0)
		col = 0;

	const int ls = line_start(line);
	const int le = line_end(line);
	int remaining = col;
	int i = ls;
	while (i < le && remaining > 0) {
		i = next_codepoint(mText, i);
		--remaining;
	}
	return clamp_offset(i);
}

int Buffer::offset_of(int line, int col) const {
	const int start = line_start(line);
	const int len = line_length(line);
	if (col < 0)
		col = 0;
	if (col > len)
		col = len;
	return clamp_offset(start + col);
}

int Buffer::clamp_offset(int offset) const {
	if (offset <= 0)
		return 0;
	if (offset >= length())
		return length();
	int o = offset;
	while (o > 0 && ((unsigned char)mText[(size_t)o] & 0xC0) == 0x80)
		--o; /* never split a code point */
	return o;
}

/**********************************************************************/
/* semantic ranges (double/triple click)                                */

void Buffer::word_range_at(int offset, int &start, int &end) const {
	const int o = clamp_offset(offset);
	const int n = length();
	start = end = o;
	if (n == 0)
		return;

	/* 4 classes: 0 = '\n'/edge, 1 = word [A-Za-z0-9_], 2 = space/tab, 3 =
	 * other punctuation. The expansion advances by CODE POINT (it never
	 * splits a UTF-8 character: "e" stays whole). */
	auto class_of = [this, n](int pos) {
		if (pos < 0 || pos >= n)
			return 0;
		const unsigned char c = (unsigned char)mText[(size_t)pos];
		if (c == '\n')
			return 0;
		if (is_word_byte(c))
			return 1;
		if (c == ' ' || c == '\t')
			return 2;
		return 3;
	};

	int cls = class_of(o);
	/* On the end of line with a word before it: that word is selected (it is the
	 * "double click at the end of the line" case). On an empty line there is nothing. */
	if (cls == 0 && o > 0 && class_of(prev_codepoint(mText, o)) == 1)
		cls = 1;
	if (cls == 0)
		return;

	int a = o;
	while (a > 0) {
		const int p = prev_codepoint(mText, a);
		if (class_of(p) != cls)
			break;
		a = p;
	}
	int b = o;
	while (b < n) {
		if (class_of(b) != cls)
			break;
		b = next_codepoint(mText, b);
	}

	start = a;
	end = b;
}

void Buffer::line_range_at(int offset, int &start, int &end) const {
	const int line = line_of_offset(offset);
	start = line_start(line);
	end = line_end(line); /* without the '\n' */
}

/**********************************************************************/
/* caret and selection                                                 */

void Buffer::set_caret_internal(int offset, bool extend) {
	const int o = clamp_offset(offset);
	mCaret = o;
	if (!extend)
		mAnchor = o;
}

void Buffer::set_caret(int offset) {
	set_caret_internal(offset, false);
	reset_goal();
	mRun = RunNone;
}

void Buffer::move_caret(int offset, bool extend) {
	set_caret_internal(offset, extend);
	reset_goal();
	mRun = RunNone;
}

void Buffer::set_selection(int start, int end) {
	const int a = clamp_offset(start);
	const int b = clamp_offset(end);
	mAnchor = a;
	mCaret = b;
	reset_goal();
	mRun = RunNone;
}

void Buffer::select_all() { set_selection(0, length()); }

std::string Buffer::selected_text() const {
	return mText.substr((size_t)selection_start(), (size_t)(selection_end() - selection_start()));
}

/**********************************************************************/
/* undo: snapshots with per-burst coalescing                            */

void Buffer::push_state(const State &s) {
	mUndo.push_back(s);
	while ((int)mUndo.size() > kUndoLimit)
		mUndo.erase(mUndo.begin());
}

void Buffer::note_edit(RunTag tag) {
	/* Same burst => NO other snapshot is stacked: the undo step is the one
	 * already recorded when the burst started (continuous typing, held Backspace). */
	const bool coalesce = (tag != RunNone && mRun == tag);
	if (!coalesce) {
		State st;
		st.text = mText;
		st.caret = mCaret;
		st.anchor = mAnchor;
		push_state(st);
	}
	mRedo.clear();
	mRun = tag;
}

void Buffer::finish_edit() { invalidate_lines(); }

void Buffer::restore(const State &s) {
	mText = s.text;
	mCaret = s.caret;
	mAnchor = s.anchor;
	mGoalCol = -1;
	invalidate_lines();
}

bool Buffer::undo() {
	if (mUndo.empty())
		return false;
	State cur;
	cur.text = mText;
	cur.caret = mCaret;
	cur.anchor = mAnchor;
	mRedo.push_back(cur);
	restore(mUndo.back());
	mUndo.pop_back();
	mRun = RunNone;
	return true;
}

bool Buffer::redo() {
	if (mRedo.empty())
		return false;
	State cur;
	cur.text = mText;
	cur.caret = mCaret;
	cur.anchor = mAnchor;
	push_state(cur);
	restore(mRedo.back());
	mRedo.pop_back();
	mRun = RunNone;
	return true;
}

/**********************************************************************/
/* editing                                                             */

bool Buffer::delete_selection_impl() {
	if (!has_selection())
		return false;
	const int s = selection_start();
	const int e = selection_end();
	mText.erase((size_t)s, (size_t)(e - s));
	mCaret = s;
	mAnchor = s;
	return true;
}

bool Buffer::delete_selection() {
	if (!has_selection())
		return false;
	note_edit(RunNone);
	delete_selection_impl();
	finish_edit();
	return true;
}

bool Buffer::insert_text(const std::string &text) {
	if (text.empty() && !has_selection())
		return false;

	const bool multiline = text.find('\n') != std::string::npos;
	note_edit(multiline ? RunNone : RunInsert); /* a newline cuts the burst */
	delete_selection_impl();
	if (!text.empty()) {
		mText.insert((size_t)mCaret, text);
		mCaret += (int)text.size();
	}
	mAnchor = mCaret;
	finish_edit();
	return true;
}

/* Quote pairing is skipped after a word (`don't`) unless that word is a string
 * prefix (`f"`, `rb'`). */
static bool quote_may_open(const std::string &text, int caret) {
	int s = caret;
	while (s > 0 && Buffer::is_word_byte((unsigned char)text[(size_t)s - 1]))
		--s;
	const int n = caret - s;
	if (n == 0)
		return true;
	if (n > 2)
		return false;
	for (int i = s; i < caret; ++i)
		if (!std::strchr("rRbBuUfFtT", text[(size_t)i]))
			return false;
	return true;
}

static char closer_for(char c) {
	switch (c) {
	case '(': return ')';
	case '[': return ']';
	case '{': return '}';
	case '\'':
	case '"': return c;
	}
	return 0;
}

bool Buffer::insert_char(unsigned int codepoint) {
	if (codepoint == 0)
		return false;
	const char c = codepoint < 128 ? (char)codepoint : 0;
	if (!mAutoClose || !c || (!closer_for(c) && c != ')' && c != ']' && c != '}'))
		return insert_text(encode_utf8(codepoint));

	const bool quote = c == '\'' || c == '"';
	const char close = closer_for(c);
	const char next = mCaret < length() ? mText[(size_t)mCaret] : 0;

	if (has_selection()) {
		if (!close)
			return insert_text(std::string(1, c));
		/* Wrap the selection and keep it selected inside the pair. */
		const int s = selection_start(), e = selection_end();
		note_edit(RunNone);
		mText.insert((size_t)e, 1, close);
		mText.insert((size_t)s, 1, c);
		mAnchor = s + 1;
		mCaret = e + 1;
		finish_edit();
		return true;
	}

	/* Typing a closer (or a quote) over the same character only moves past it. */
	if ((!close || quote) && next == c) {
		move_caret(mCaret + 1);
		return true;
	}
	if (!close)
		return insert_text(std::string(1, c));

	std::string pair(1, c);
	if (quote) {
		const bool third = mCaret >= 2 && mText[(size_t)mCaret - 1] == c && mText[(size_t)mCaret - 2] == c;
		if (third)
			pair += std::string(3, c); /* `""` + `"` -> `"""|"""` */
		else if (quote_may_open(mText, mCaret))
			pair += c;
	} else if (!(next && is_word_byte((unsigned char)next))) {
		pair += close; /* no pair right before a word: `(|foo` */
	}
	if (pair.size() == 1)
		return insert_text(pair);
	note_edit(RunNone); /* the pair is its own undo step */
	mText.insert((size_t)mCaret, pair);
	mCaret += 1;
	mAnchor = mCaret;
	finish_edit();
	return true;
}

bool Buffer::backspace() {
	if (has_selection())
		return delete_selection();
	if (mCaret <= 0)
		return false;

	/* Backspace inside an empty pair removes both halves. */
	if (mAutoClose && mCaret < length()) {
		const char prev = mText[(size_t)mCaret - 1];
		const char close = closer_for(prev);
		if (close && mText[(size_t)mCaret] == close) {
			note_edit(RunNone);
			mText.erase((size_t)mCaret - 1, 2);
			mCaret -= 1;
			mAnchor = mCaret;
			finish_edit();
			return true;
		}
	}

	note_edit(RunBackspace);
	const int from = prev_codepoint(mText, mCaret);
	mText.erase((size_t)from, (size_t)(mCaret - from));
	mCaret = from;
	mAnchor = from;
	finish_edit();
	return true;
}

bool Buffer::delete_forward() {
	if (has_selection())
		return delete_selection();
	if (mCaret >= length())
		return false;
	note_edit(RunDelete);
	const int to = next_codepoint(mText, mCaret);
	mText.erase((size_t)mCaret, (size_t)(to - mCaret));
	mAnchor = mCaret;
	finish_edit();
	return true;
}

bool Buffer::insert_tab(int tabwidth) {
	if (tabwidth < 1)
		tabwidth = 1;
	note_edit(RunNone);
	delete_selection_impl();
	mText.insert((size_t)mCaret, std::string((size_t)tabwidth, ' '));
	mCaret += tabwidth;
	mAnchor = mCaret;
	finish_edit();
	return true;
}

/* Return with autoindent. The indentation arrives computed (Syntax::indent_at);
 * the buffer only decides where the closer goes when the pair is empty. */
bool Buffer::insert_newline(const std::string &indent) {
	note_edit(RunNone);      /* every Return is its own undo step */
	delete_selection_impl(); /* if there was a selection, it is replaced by the newline */

	const int line = line_of_offset(mCaret);
	const int ls = line_start(line);
	const int le = line_end(line);

	/* Base indentation of the current line: the leading spaces/tabs, as is. */
	int contentStart = ls;
	while (contentStart < le && (mText[(size_t)contentStart] == ' ' || mText[(size_t)contentStart] == '\t'))
		++contentStart;
	const std::string base = mText.substr((size_t)ls, (size_t)(contentStart - ls));

	/* Does the text before the caret (without trailing spaces) end in an opener? */
	int before = mCaret;
	while (before > ls && (mText[(size_t)before - 1] == ' ' || mText[(size_t)before - 1] == '\t'))
		--before;
	const bool opensBlock =
		before > ls && (mText[(size_t)before - 1] == '{' || mText[(size_t)before - 1] == '(' ||
						mText[(size_t)before - 1] == '[');

	/* Is there a closer to the right of the caret, with only spaces in
	 * between? */
	int after = mCaret;
	while (after < le && (mText[(size_t)after] == ' ' || mText[(size_t)after] == '\t'))
		++after;
	const bool closerAhead =
		after < le && (mText[(size_t)after] == '}' || mText[(size_t)after] == ')' || mText[(size_t)after] == ']');

	if (opensBlock && closerAhead) {
		/* `{ | }` => `{` + salto + indent + salto + base + `}` (el cierre baja). */
		mText = mText.substr(0, (size_t)mCaret) + "\n" + indent + "\n" + base + mText.substr((size_t)after);
	} else {
		mText.insert((size_t)mCaret, "\n" + indent);
	}
	mCaret += 1 + (int)indent.size();
	mAnchor = mCaret;
	finish_edit();
	return true;
}

bool Buffer::reindent_line(int line, const std::string &indent) {
	const int ls = line_start(line);
	const int le = line_end(line);

	int ws = ls;
	while (ws < le && (mText[(size_t)ws] == ' ' || mText[(size_t)ws] == '\t'))
		++ws;
	const std::string current = mText.substr((size_t)ls, (size_t)(ws - ls));
	if (current == indent)
		return false;

	/* Coalesced with the typing burst: typing a closer + its dedent = one step. */
	note_edit(RunInsert);

	const int delta = (int)indent.size() - (int)current.size();
	mText.replace((size_t)ls, (size_t)(ws - ls), indent);

	auto adjust = [&](int pos) {
		if (pos <= ls)
			return pos;
		if (pos < ws) {
			const int p = pos + delta;
			return p < ls ? ls : p;
		}
		return pos + delta;
	};
	mCaret = adjust(mCaret);
	mAnchor = adjust(mAnchor);
	finish_edit();
	return true;
}

bool Buffer::outdent(int tabwidth) {
	if (tabwidth < 1)
		tabwidth = 1;
	const int ls = line_start(line_of_offset(mCaret));
	int n = 0;
	while (n < tabwidth && ls + n < length() && mText[(size_t)(ls + n)] == ' ')
		++n;
	if (n == 0)
		return false;
	note_edit(RunNone);
	mText.erase((size_t)ls, (size_t)n);
	const int delta = mCaret - ls;
	mCaret = ls + (delta > n ? delta - n : 0);
	mAnchor = mCaret;
	finish_edit();
	return true;
}

/**********************************************************************/
/* movimiento                                                          */

void Buffer::move_left(bool extend) {
	move_caret(prev_codepoint(mText, mCaret), extend);
}

void Buffer::move_right(bool extend) {
	move_caret(next_codepoint(mText, mCaret), extend);
}

void Buffer::move_word_left(bool extend) {
	int o = mCaret;
	while (o > 0 && !is_word_byte((unsigned char)mText[(size_t)prev_codepoint(mText, o)]))
		o = prev_codepoint(mText, o);
	while (o > 0 && is_word_byte((unsigned char)mText[(size_t)prev_codepoint(mText, o)]))
		o = prev_codepoint(mText, o);
	move_caret(o, extend);
}

void Buffer::move_word_right(bool extend) {
	const int n = length();
	int o = mCaret;
	while (o < n && !is_word_byte((unsigned char)mText[(size_t)o]))
		o = next_codepoint(mText, o);
	while (o < n && is_word_byte((unsigned char)mText[(size_t)o]))
		o = next_codepoint(mText, o);
	move_caret(o, extend);
}

void Buffer::move_vertical(int delta, bool extend) {
	/* The desired column survives the Up/Down run (otherwise, going down through
	 * short lines "drags" the caret to the end of the line and it does not come
	 * back). */
	if (mGoalCol < 0)
		mGoalCol = col_of_offset(mCaret);

	int line = line_of_offset(mCaret) + delta;
	if (line < 0)
		line = 0;
	if (line >= line_count())
		line = line_count() - 1;

	int col = mGoalCol;
	const int len = line_length(line);
	if (col > len)
		col = len;

	set_caret_internal(line_start(line) + col, extend);
	mRun = RunNone; /* Up/Down cuts the typing burst, but NOT the desired column */
}

void Buffer::move_up(bool extend) { move_vertical(-1, extend); }

void Buffer::move_down(bool extend) { move_vertical(1, extend); }

void Buffer::move_line_start(bool extend) {
	move_caret(line_start(line_of_offset(mCaret)), extend);
}

void Buffer::move_line_end(bool extend) {
	move_caret(line_end(line_of_offset(mCaret)), extend);
}

void Buffer::move_document_start(bool extend) { move_caret(0, extend); }

void Buffer::move_document_end(bool extend) { move_caret(length(), extend); }

} // namespace py_code
