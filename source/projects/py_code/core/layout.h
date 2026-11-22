// SPDX-License-Identifier: MIT
/* layout.h -- soft wrap, vertical scroll and hit testing for an Editor.
 *
 * Cells horizontally, pixels vertically. Rows are numbered globally across
 * lines. `update()` keeps a pointer to the Editor: it must outlive the
 * Layout, or `update()` must be called again with another one.
 */

#pragma once

#include "core/scroll.h"

#include <vector>

namespace pyc {

class Editor;

struct Row {
	int line;
	int cell_begin; /* line-relative cells */
	int cell_end;
};

class Layout {
  public:
	static constexpr double kThumbMin = Scroll::kThumbMin;

	/* Rebuilds the rows when the editor, its revision or `cols` changed.
	 * `cols <= 0`: no wrap. Does not clamp the scroll; call clamp(). */
	void update(const Editor &ed, int cols);

	int row_count() const { return (int)mRows.size(); }
	Row row(int r) const;
	int row_of(int offset) const;
	int offset_at(int row, int cell) const; /* `cell` is row-relative, clamped */
	int cell_in_row(int offset) const;      /* row-relative cell of `offset` */

	/* Scroll and scroll bar over the rows: see scroll.h. */
	void set_view(double view_h, double line_h, double pad) { mScroll.set_view(view_h, line_h, pad); }
	double scroll_y() const { return mScroll.scroll_y(); }
	void set_scroll(double y) { mScroll.set_scroll(y); } /* unclamped (restores) */
	double content_h() const { return mScroll.content_h(); }
	double max_scroll() const { return mScroll.max_scroll(); }
	void clamp() { mScroll.clamp(); }
	void scroll_by(double dy) { mScroll.scroll_by(dy); }
	void ensure_row_visible(int r) { mScroll.ensure_row_visible(r); } /* keeps `pad` around the row */
	int first_visible_row() const { return mScroll.first_visible_row(); }
	int last_visible_row() const { return mScroll.last_visible_row(); }
	double row_y(int r) const { return mScroll.row_y(r); }
	int row_at_y(double y) const { return mScroll.row_at_y(y); }
	int page_rows() const { return mScroll.page_rows(); }
	Thumb thumb(double track_h) const { return mScroll.thumb(track_h); }
	void drag_thumb(double y, double track_h) { mScroll.drag_thumb(y, track_h); }

  private:
	struct RowData {
		Row row;
		int begin; /* byte offsets */
		int end;
		bool last; /* last row of its line */
	};
	void wrap_line(int line, int cols);

	const Editor *mEditor = nullptr;
	int mRevision = -1;
	int mCols = -1;
	std::vector<RowData> mRows;

	Scroll mScroll; /* its row count follows mRows */
};

} // namespace pyc
