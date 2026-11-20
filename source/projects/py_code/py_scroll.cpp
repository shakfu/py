// SPDX-License-Identifier: MIT
/* py_scroll.cpp -- see py_scroll.h. */

#include "py_scroll.h"

#include <algorithm>
#include <cmath>

namespace py_code {

double ScrollView::content_height(const ScrollMetrics &m) const {
	const int lines = m.line_count > 0 ? m.line_count : 1;
	return (double)lines * m.line_h + 2. * m.pad_y;
}

double ScrollView::max_y(const ScrollMetrics &m) const {
	const double over = content_height(m) - m.view_h;
	return over > 0. ? over : 0.;
}

int ScrollView::visible_lines(const ScrollMetrics &m) const {
	if (m.line_h <= 0.)
		return 1;
	const double usable = m.view_h - 2. * m.pad_y;
	const int n = (int)std::floor(usable / m.line_h);
	return n > 0 ? n : 1;
}

void ScrollView::clamp(const ScrollMetrics &m) {
	const double top = max_y(m);
	if (y > top)
		y = top;
	if (y < 0.)
		y = 0.;
}

double ScrollView::line_screen_y(int line, const ScrollMetrics &m) const {
	return m.pad_y + (double)line * m.line_h - y;
}

int ScrollView::first_line(const ScrollMetrics &m) const {
	if (m.line_h <= 0.)
		return 0;
	/* first line whose bottom edge still falls inside the box */
	int line = (int)std::floor((y - m.pad_y) / m.line_h);
	if (line < 0)
		line = 0;
	return line;
}

int ScrollView::last_line(const ScrollMetrics &m) const {
	if (m.line_h <= 0.)
		return 0;
	int line = (int)std::floor((y + m.view_h - m.pad_y) / m.line_h);
	const int last = (m.line_count > 0 ? m.line_count : 1) - 1;
	if (line > last)
		line = last;
	if (line < 0)
		line = 0;
	return line;
}

void ScrollView::scroll_by(double dy, const ScrollMetrics &m) {
	y += dy;
	clamp(m);
}

void ScrollView::scroll_to_line(int line, const ScrollMetrics &m) {
	if (line < 0)
		line = 0;
	y = (double)line * m.line_h;
	clamp(m);
}

void ScrollView::ensure_line(int line, const ScrollMetrics &m) {
	if (line < 0)
		line = 0;
	/* Top: the top of the line cannot end up above the margin. */
	const double top = (double)line * m.line_h;
	/* Bottom: the bottom of the line cannot go past the bottom
	 * margin. */
	const double bottom = (double)(line + 1) * m.line_h + 2. * m.pad_y - m.view_h;

	if (y > top)
		y = top;
	else if (y < bottom)
		y = bottom;
	clamp(m);
}

ScrollView::Bar ScrollView::bar(const ScrollMetrics &m) const {
	Bar b;
	if (!scrollable(m))
		return b;
	const double track_h = m.view_h - 2. * m.pad_y;
	if (track_h <= 0.)
		return b;
	const double total = (double)(m.line_count > 0 ? m.line_count : 1) * m.line_h;
	const double usable = m.view_h - 2. * m.pad_y;
	double thumb_h = track_h * (usable / total);
	if (thumb_h < kMinThumb)
		thumb_h = kMinThumb;
	if (thumb_h > track_h)
		thumb_h = track_h;

	const double travel = track_h - thumb_h;
	const double top = max_y(m);
	b.visible = true;
	b.track_h = track_h;
	b.thumb_h = thumb_h;
	b.thumb_y = travel > 0. ? (y / top) * travel : 0.;
	if (b.thumb_y < 0.)
		b.thumb_y = 0.;
	if (b.thumb_y > travel)
		b.thumb_y = travel;
	return b;
}

void ScrollView::drag_thumb_to(double thumb_y, const ScrollMetrics &m) {
	const Bar b = bar(m);
	if (!b.visible)
		return;
	const double travel = b.track_h - b.thumb_h;
	if (travel <= 0.) {
		y = 0.;
		return;
	}
	if (thumb_y < 0.)
		thumb_y = 0.;
	if (thumb_y > travel)
		thumb_y = travel;
	y = (thumb_y / travel) * max_y(m);
	clamp(m);
}

} // namespace py_code
