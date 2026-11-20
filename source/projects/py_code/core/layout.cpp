// SPDX-License-Identifier: MIT
/* layout.cpp -- see layout.h. */

#include "core/layout.h"

#include "core/editor.h"

#include <algorithm>
#include <cmath>

namespace pyc {

static bool is_break(char c) { return c == ' ' || c == '(' || c == '[' || c == '{' || c == ',' || c == ';'; }

void Layout::update(const Editor &ed, int cols) {
	if (cols <= 0)
		cols = 0;
	if (&ed == mEditor && ed.revision() == mRevision && cols == mCols)
		return;
	mEditor = &ed;
	mRevision = ed.revision();
	mCols = cols;
	mRows.clear();
	for (int l = 0; l < ed.line_count(); ++l)
		wrap_line(l, cols);
}

void Layout::wrap_line(int line, int cols) {
	const std::string &t = mEditor->text();
	const int end = mEditor->line_end(line);
	std::vector<int> offs; /* byte offset of each cell, then the line end */
	for (int p = mEditor->line_start(line); p < end; p = utf8_next(t, p))
		offs.push_back(p);
	offs.push_back(end);
	const int n = (int)offs.size() - 1;
	int b = 0;
	for (;;) {
		if (cols == 0 || n - b <= cols) {
			mRows.push_back(RowData{Row{line, b, n}, offs[(size_t)b], end, true});
			return;
		}
		int brk = b + cols;
		for (int i = b + cols - 1; i >= b && i + 1 - b >= cols / 2; --i)
			if (is_break(t[(size_t)offs[(size_t)i]])) {
				brk = i + 1;
				break;
			}
		mRows.push_back(RowData{Row{line, b, brk}, offs[(size_t)b], offs[(size_t)brk], false});
		b = brk;
	}
}

Row Layout::row(int r) const {
	if (mRows.empty())
		return Row{0, 0, 0};
	return mRows[(size_t)std::max(0, std::min(r, row_count() - 1))].row;
}

int Layout::row_of(int offset) const {
	auto it = std::upper_bound(mRows.begin(), mRows.end(), offset,
							   [](int off, const RowData &rd) { return off < rd.begin; });
	return std::max(0, (int)(it - mRows.begin()) - 1);
}

int Layout::offset_at(int r, int cell) const {
	if (mRows.empty() || !mEditor)
		return 0;
	const RowData &rd = mRows[(size_t)std::max(0, std::min(r, row_count() - 1))];
	const int width = rd.row.cell_end - rd.row.cell_begin;
	const int c = std::max(0, std::min(cell, rd.last ? width : width - 1));
	return mEditor->offset_at_cell(rd.row.line, rd.row.cell_begin + c);
}

int Layout::cell_in_row(int offset) const {
	if (!mEditor)
		return 0;
	return mEditor->cells_before(offset) - row(row_of(offset)).cell_begin;
}

/* ---- scroll ---- */

void Layout::set_view(double view_h, double line_h, double pad) {
	mViewH = std::max(0.0, view_h);
	mLineH = std::max(0.0, line_h);
	mPad = std::max(0.0, pad);
}

double Layout::content_h() const { return row_count() * mLineH + 2 * mPad; }

double Layout::max_scroll() const { return std::max(0.0, content_h() - mViewH); }

void Layout::clamp() { mScrollY = std::max(0.0, std::min(mScrollY, max_scroll())); }

void Layout::scroll_by(double dy) {
	mScrollY += dy;
	clamp();
}

void Layout::ensure_row_visible(int r) {
	r = std::max(0, std::min(r, row_count() - 1));
	const double top = r * mLineH; /* row top minus pad, content coordinates */
	const double bottom = top + mLineH + 2 * mPad;
	if (bottom > mScrollY + mViewH)
		mScrollY = bottom - mViewH;
	if (top < mScrollY)
		mScrollY = top;
	clamp();
}

int Layout::row_at_y(double y) const {
	if (mLineH <= 0 || mRows.empty())
		return 0;
	const int r = (int)std::floor((y + mScrollY - mPad) / mLineH);
	return std::max(0, std::min(r, row_count() - 1));
}

int Layout::first_visible_row() const { return row_at_y(0); }

int Layout::last_visible_row() const {
	if (mLineH <= 0 || mRows.empty())
		return 0;
	const int r = (int)std::ceil((mScrollY + mViewH - mPad) / mLineH) - 1;
	return std::max(0, std::min(r, row_count() - 1));
}

double Layout::row_y(int r) const { return mPad + r * mLineH - mScrollY; }

int Layout::page_rows() const {
	if (mLineH <= 0)
		return 1;
	return std::max(1, (int)std::floor(mViewH / mLineH) - 1);
}

/* ---- scroll bar ---- */

Thumb Layout::thumb(double track_h) const {
	Thumb t;
	const double content = content_h();
	if (content <= mViewH || track_h <= 0)
		return t;
	t.visible = true;
	t.h = std::min(track_h, std::max(kThumbMin, track_h * mViewH / content));
	const double range = max_scroll();
	t.y = range > 0 ? (track_h - t.h) * mScrollY / range : 0;
	return t;
}

void Layout::drag_thumb(double y, double track_h) {
	const Thumb t = thumb(track_h);
	const double room = track_h - t.h;
	if (!t.visible || room <= 0)
		return;
	mScrollY = y / room * max_scroll();
	clamp();
}

} // namespace pyc
