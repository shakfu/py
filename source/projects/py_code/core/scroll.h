// SPDX-License-Identifier: MIT
/* scroll.h -- vertical scroll and scroll bar over a number of equal-height rows.
 *
 * Pixels. Used by Layout, and by py_repl to scroll two Layouts as one.
 */

#pragma once

namespace pyc {

struct Thumb {
	bool visible = false;
	double y = 0; /* top, relative to the track */
	double h = 0;
};

class Scroll {
  public:
	static constexpr double kThumbMin = 24;

	void set_rows(int rows) { mRows = rows > 0 ? rows : 0; }
	int rows() const { return mRows; }

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

	Thumb thumb(double track_h) const;
	void drag_thumb(double y, double track_h);

  private:
	int mRows = 0;
	double mViewH = 0, mLineH = 0, mPad = 0, mScrollY = 0;
};

} // namespace pyc
