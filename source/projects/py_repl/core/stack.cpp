// SPDX-License-Identifier: MIT
/* stack.cpp -- see stack.h. */

#include "core/stack.h"

#include "core/editor.h"
#include "core/layout.h"

#include <algorithm>

namespace pyc {

int content_rows(const Editor &ed, const Layout &layout) {
	if (ed.length() == 0)
		return 0;
	const int n = layout.row_count();
	return ed.text().back() == '\n' ? n - 1 : n;
}

void Stack::update(int top_rows, int bottom_rows) {
	const bool stick = at_bottom();
	mTop = std::max(0, top_rows);
	mBottom = std::max(0, bottom_rows);
	mScroll.set_rows(rows());
	if (stick)
		mScroll.set_scroll(mScroll.max_scroll());
	mScroll.clamp();
}

Place Stack::place(int row) const {
	row = std::max(0, std::min(row, rows() - 1));
	if (row < mTop)
		return Place{0, row};
	return Place{1, row - mTop};
}

int Stack::row_of(int part, int row) const { return part == 0 ? row : mTop + row; }

bool Stack::at_bottom() const { return mScroll.scroll_y() >= mScroll.max_scroll() - 0.5; }

} // namespace pyc
