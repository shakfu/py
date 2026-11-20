// SPDX-License-Identifier: MIT
/* layout.h -- soft wrap, vertical scroll and hit testing for an Editor.
 *
 * Cells horizontally, pixels vertically. Rows are numbered globally across
 * lines. `update()` keeps a pointer to the Editor: it must outlive the
 * Layout, or `update()` must be called again with another one.
 */

#pragma once

#include <vector>

namespace pyc {

class Editor;

struct Row {
	int line;
	int cell_begin; /* line-relative cells */
	int cell_end;
};

struct Thumb {
	bool visible = false;
	double y = 0; /* top, relative to the track */
	double h = 0;
};

class Layout {
  public:
	static constexpr double kThumbMin = 24;

	/* Rebuilds the rows when the editor, its revision or `cols` changed.
	 * `cols <= 0`: no wrap. Does not clamp the scroll; call clamp(). */
	void update(const Editor &ed, int cols);

	int row_count() const { return (int)mRows.size(); }
	Row row(int r) const;
	int row_of(int offset) const;
	int offset_at(int row, int cell) const; /* `cell` is row-relative, clamped */
	int cell_in_row(int offset) const;      /* row-relative cell of `offset` */

	/* Scroll. */
	void set_view(double view_h, double line_h, double pad);
	double scroll_y() const { return mScrollY; }
	void set_scroll(double y) { mScrollY = y; } /* unclamped (restores) */
	double content_h() const;
	double max_scroll() const;
	void clamp();
	void scroll_by(double dy);
	void ensure_row_visible(int r); /* keeps `pad` around the row */
	int first_visible_row() const;
	int last_visible_row() const;
	double row_y(int r) const;
	int row_at_y(double y) const;
	int page_rows() const;

	/* Scroll bar. */
	Thumb thumb(double track_h) const;
	void drag_thumb(double y, double track_h);

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

	double mViewH = 0, mLineH = 0, mPad = 0, mScrollY = 0;
};

} // namespace pyc
