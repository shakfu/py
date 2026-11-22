// SPDX-License-Identifier: MIT
/* stack.h -- one vertical scroll over two Layouts: the transcript above the input.
 *
 * Spec: py_repl-spec.md, 2.4. Rows are numbered across both parts.
 */

#pragma once

#include "core/scroll.h"

namespace pyc {

class Editor;
class Layout;

/* Rows of `layout` to draw for `ed`: none for empty text, and not the empty
 * row after a final newline (the transcript always ends with one). */
int content_rows(const Editor &ed, const Layout &layout);

struct Place {
	int part; /* 0 top (transcript), 1 bottom (input) */
	int row;  /* row within the part */
	bool operator==(const Place &o) const { return part == o.part && row == o.row; }
};

class Stack {
  public:
	/* Sets the row counts. When the view showed the last row, it still does. */
	void update(int top_rows, int bottom_rows);

	int rows() const { return mTop + mBottom; }
	int top_rows() const { return mTop; }
	int bottom_rows() const { return mBottom; }
	Place place(int row) const;          /* clamped to the rows */
	int row_of(int part, int row) const; /* inverse of place() */

	/* True while the view shows the last row. */
	bool at_bottom() const;

	Scroll &scroll() { return mScroll; }
	const Scroll &scroll() const { return mScroll; }

  private:
	int mTop = 0, mBottom = 0;
	Scroll mScroll;
};

} // namespace pyc
