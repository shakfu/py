// SPDX-License-Identifier: MIT
/* py_scroll.h -- vertical scroll geometry, no Max dependency.
 *
 * `y` is the content offset in pixels (0 = top). Content height includes the
 * padding: line_count * line_h + 2 * pad_y. `ensure_line` scrolls the minimum
 * needed to show a line. */

#pragma once

namespace py_code {

/* Text window geometry, in pixels (the object builds it from its cached
 * metrics; the tests set it by hand). */
struct ScrollMetrics {
	int line_count = 1;
	double line_h = 16.;
	double view_h = 0.;
	double pad_y = 4.;
};

class ScrollView {
  public:
	/* Minimum height of the bar thumb: with 1000 lines a proportional thumb
	 * would be 2 px (impossible to grab). */
	static constexpr double kMinThumb = 24.;

	double y = 0.;

	/* --- geometry --------------------------------------------------------- */

	double content_height(const ScrollMetrics &m) const;
	double max_y(const ScrollMetrics &m) const; /* 0 if the text fits whole */
	bool scrollable(const ScrollMetrics &m) const { return max_y(m) > 0.; }
	int visible_lines(const ScrollMetrics &m) const; /* WHOLE lines that fit */

	/* --- visible lines ---------------------------------------------------- */

	double line_screen_y(int line, const ScrollMetrics &m) const; /* and in the box */
	int first_line(const ScrollMetrics &m) const; /* first line to paint */
	int last_line(const ScrollMetrics &m) const;  /* last line to paint */

	/* --- movimiento ------------------------------------------------------ */

	/* `dy` in pixels: positive = the content goes up (scrolling down). */
	void scroll_by(double dy, const ScrollMetrics &m);
	void scroll_to_line(int line, const ScrollMetrics &m); /* line at the top */
	void ensure_line(int line, const ScrollMetrics &m);    /* caret siempre visible */
	void clamp(const ScrollMetrics &m);

	/* --- barra vertical -------------------------------------------------- */

	/* Thumb geometry in TRACK coordinates (0..track_h). The track starts at
	 * `pad_y` and measures `view_h - 2*pad_y`. */
	struct Bar {
		bool visible = false;
		double track_h = 0.;
		double thumb_y = 0.;
		double thumb_h = 0.;
	};
	Bar bar(const ScrollMetrics &m) const;
	/* Thumb dragging: `thumb_y` in track coordinates. */
	void drag_thumb_to(double thumb_y, const ScrollMetrics &m);
};

} // namespace py_code
