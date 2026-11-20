// SPDX-License-Identifier: MIT
/* py_wrap.cpp -- see py_wrap.h. */

#include "py_wrap.h"

#include <algorithm>

#include "py_buffer.h"

namespace py_code {

/* Can the row be cut AFTER this character? Space and the separators: they are
 * the places where the cut does not split an identifier in two. */
static bool is_break_char(char c) {
	return c == ' ' || c == '\t' || c == '(' || c == '[' || c == '{' || c == ',' || c == ';';
}

void LineWrap::layout(const char *text, int len, int cols) {
	if (cols < 1)
		cols = 1;
	if (len < 0)
		len = 0;

	/* One pass to (a) count cells and (b) record the natural cuts. UTF-8
	 * continuation bytes (10xxxxxx) do not open a new cell. */
	cands.clear();
	int cell = 0;
	for (int i = 0; i < len; ++i) {
		const unsigned char c = (unsigned char)text[i];
		if ((c & 0xC0) == 0x80)
			continue;
		++cell;
		if (is_break_char((char)c))
			cands.push_back(cell);
	}
	mCells = cell;

	starts.clear();
	starts.push_back(0);
	if (mCells == 0)
		return; /* empty line: a single empty row */

	int pos = 0;
	while (mCells - pos > cols) {
		const int limit = pos + cols; /* last cell that fits in the row */
		const int low = pos + cols / 2; /* we do not leave nearly empty rows */
		int cut = limit;                /* corte duro */
		/* Last natural boundary that fits; if it falls too far back, it is
		 * ignored. */
		std::vector<int>::const_iterator it = std::upper_bound(cands.begin(), cands.end(), limit);
		if (it != cands.begin()) {
			const int c = *(it - 1);
			if (c > low && c > pos)
				cut = c;
		}
		starts.push_back(cut);
		pos = cut;
	}
}

int LineWrap::cell_start(int row) const {
	if (row < 0)
		row = 0;
	if (row >= (int)starts.size())
		row = (int)starts.size() - 1;
	return starts[(size_t)row];
}

int LineWrap::cell_end(int row) const {
	if (row >= (int)starts.size() - 1)
		return mCells;
	if (row < 0)
		return starts.empty() ? 0 : starts[0];
	return starts[(size_t)row + 1];
}

int LineWrap::row_of_cell(int cell) const {
	if (cell <= 0)
		return 0;
	/* upper_bound: with cell == the start of a row THAT row wins (not the previous
	 * one). */
	std::vector<int>::const_iterator it = std::upper_bound(starts.begin(), starts.end(), cell);
	int row = (int)(it - starts.begin()) - 1;
	if (row < 0)
		row = 0;
	if (row > (int)starts.size() - 1)
		row = (int)starts.size() - 1;
	return row;
}

/**********************************************************************/

void WrapCache::reset() {
	mLines.clear();
	mFirstRow.clear();
	mRowCount = 0;
	mCols = 0;
	mRevision = -1;
	mValid = false;
}

void WrapCache::ensure(const Buffer &buf, int cols) {
	if (cols < 1)
		cols = 1;
	const int lines = buf.line_count();
	const int rev = buf.revision();
	if (mValid && cols == mCols && rev == mRevision && (int)mLines.size() == lines)
		return;

	mLines.resize((size_t)lines);
	mFirstRow.resize((size_t)lines + 1);
	const std::string &text = buf.text();
	int row = 0;
	for (int i = 0; i < lines; ++i) {
		const int ls = buf.line_start(i);
		mLines[(size_t)i].layout(text.data() + ls, buf.line_length(i), cols);
		mFirstRow[(size_t)i] = row;
		row += mLines[(size_t)i].rows();
	}
	mFirstRow[(size_t)lines] = row;
	mRowCount = row > 0 ? row : 1;
	mCols = cols;
	mRevision = rev;
	mValid = true;
}

const LineWrap &WrapCache::wrap(int line) const {
	static const LineWrap kEmpty;
	if (mLines.empty())
		return kEmpty;
	if (line < 0)
		line = 0;
	if (line > (int)mLines.size() - 1)
		line = (int)mLines.size() - 1;
	return mLines[(size_t)line];
}

int WrapCache::first_row(int line) const {
	if (mFirstRow.empty())
		return 0;
	if (line < 0)
		line = 0;
	if (line > (int)mFirstRow.size() - 1)
		line = (int)mFirstRow.size() - 1;
	return mFirstRow[(size_t)line];
}

int WrapCache::line_of_row(int row) const {
	if (mFirstRow.size() < 2)
		return 0;
	const int lines = (int)mFirstRow.size() - 1;
	if (row <= 0)
		return 0;
	if (row >= mRowCount)
		row = mRowCount - 1;
	/* first line whose start row is > row, minus one. */
	std::vector<int>::const_iterator it =
		std::upper_bound(mFirstRow.begin() + 1, mFirstRow.end(), row);
	int line = (int)(it - mFirstRow.begin()) - 1;
	if (line < 0)
		line = 0;
	if (line > lines - 1)
		line = lines - 1;
	return line;
}

int WrapCache::row_of_line_cell(int line, int cell) const {
	return first_row(line) + wrap(line).row_of_cell(cell);
}

} // namespace py_code
