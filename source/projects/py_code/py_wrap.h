// SPDX-License-Identifier: GPL-3.0-only
/* py_wrap.h -- soft wrap at the box width, no Max dependency.
 *
 * One cell = one code point (monospace font). Rows are numbered globally, so
 * the scroll works in rows. A cut prefers a space or separator (`( [ { , ;`) in
 * the second half of the row, else cuts at `cols`. `cols` <= 0 acts as 1; a huge
 * `cols` gives one row per line (`@wrap 0`). */

#pragma once

#include <vector>

namespace py_code {

class Buffer;

/* Cuts of ONE line into visual rows. `starts` is the cell where each row BEGINS;
 * `starts[0]` is always 0 and `rows() == starts.size()`. */
class LineWrap {
  public:
	LineWrap() { starts.push_back(0); }

	/* Recomputes the cuts of a line (UTF-8 text WITHOUT the '\n', `len` bytes).
	 * `mCells` is recomputed here (it is not passed in: the caller has no reason to
	 * count). */
	void layout(const char *text, int len, int cols);

	int rows() const { return (int)starts.size(); }
	int cells() const { return mCells; }

	int cell_start(int row) const; /* first cell of the row (clamp) */
	int cell_end(int row) const;   /* EXCLUSIVE last cell (== cells() on the last one) */

	/* Row (within THIS line) that contains the caret cell. On a cut exactly at the
	 * cut cell the NEXT row wins (the caret is drawn at the start of the row below,
	 * like any editor with wrap). */
	int row_of_cell(int cell) const;

  private:
	std::vector<int> starts;
	std::vector<int> cands; /* cells where a cut is allowed (reused per line) */
	int mCells = 0;
};

/* Wrap cache of the WHOLE document: the scroll needs the total number of rows, so
 * resolving the visible rows is not enough. It is redone when the width or the
 * document changes (`Buffer::revision()`) -- like the highlight cache, but without
 * incrementality: walking the text is O(n) and a reflow is rare (resize, or an
 * edit: 1000 lines ~ tens of us). */
class WrapCache {
  public:
	void reset();

	/* Brings the cache up to date for `cols` cell columns. Does nothing if it already
	 * is. */
	void ensure(const Buffer &buf, int cols);

	bool valid() const { return mValid; }
	int cols() const { return mCols; }
	int revision() const { return mRevision; }

	int line_count() const { return (int)mLines.size(); }
	int row_count() const { return mRowCount > 0 ? mRowCount : 1; }

	const LineWrap &wrap(int line) const; /* with clamp and fallback to an empty row */
	int first_row(int line) const;        /* global row of the first row of `line` */
	int line_of_row(int row) const;       /* line the global row belongs to */
	int row_in_line(int row) const { return row - first_row(line_of_row(row)); }
	/* (line, cell) -> GLOBAL row. */
	int row_of_line_cell(int line, int cell) const;

  private:
	std::vector<LineWrap> mLines;
	std::vector<int> mFirstRow; /* line_count + 1: start row of each line */
	int mRowCount = 0;
	int mCols = 0;
	int mRevision = -1;
	bool mValid = false;
};

} // namespace py_code
