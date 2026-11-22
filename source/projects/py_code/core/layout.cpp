// SPDX-License-Identifier: MIT
/* layout.cpp -- see layout.h. */

#include "core/layout.h"

#include "core/editor.h"

#include <algorithm>

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
	mScroll.set_rows(row_count());
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

} // namespace pyc
