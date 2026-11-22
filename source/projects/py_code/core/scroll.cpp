// SPDX-License-Identifier: MIT
/* scroll.cpp -- see scroll.h. */

#include "core/scroll.h"

#include <algorithm>
#include <cmath>

namespace pyc {

void Scroll::set_view(double view_h, double line_h, double pad) {
	mViewH = std::max(0.0, view_h);
	mLineH = std::max(0.0, line_h);
	mPad = std::max(0.0, pad);
}

double Scroll::content_h() const { return mRows * mLineH + 2 * mPad; }

double Scroll::max_scroll() const { return std::max(0.0, content_h() - mViewH); }

void Scroll::clamp() { mScrollY = std::max(0.0, std::min(mScrollY, max_scroll())); }

void Scroll::scroll_by(double dy) {
	mScrollY += dy;
	clamp();
}

void Scroll::ensure_row_visible(int r) {
	r = std::max(0, std::min(r, mRows - 1));
	const double top = r * mLineH; /* row top minus pad, content coordinates */
	const double bottom = top + mLineH + 2 * mPad;
	if (bottom > mScrollY + mViewH)
		mScrollY = bottom - mViewH;
	if (top < mScrollY)
		mScrollY = top;
	clamp();
}

int Scroll::row_at_y(double y) const {
	if (mLineH <= 0 || mRows == 0)
		return 0;
	const int r = (int)std::floor((y + mScrollY - mPad) / mLineH);
	return std::max(0, std::min(r, mRows - 1));
}

int Scroll::first_visible_row() const { return row_at_y(0); }

int Scroll::last_visible_row() const {
	if (mLineH <= 0 || mRows == 0)
		return 0;
	const int r = (int)std::ceil((mScrollY + mViewH - mPad) / mLineH) - 1;
	return std::max(0, std::min(r, mRows - 1));
}

double Scroll::row_y(int r) const { return mPad + r * mLineH - mScrollY; }

int Scroll::page_rows() const {
	if (mLineH <= 0)
		return 1;
	return std::max(1, (int)std::floor(mViewH / mLineH) - 1);
}

Thumb Scroll::thumb(double track_h) const {
	Thumb t;
	const double content = content_h();
	if (content <= mViewH || track_h <= 0)
		return t;
	t.visible = true;
	t.h = std::min(track_h, std::max(kThumbMin, track_h * mViewH / content));
	const double range = max_scroll();
	t.y = range > 0 ? (track_h - t.h) * mScrollY / range : 0;
	return t;
}

void Scroll::drag_thumb(double y, double track_h) {
	const Thumb t = thumb(track_h);
	const double room = track_h - t.h;
	if (!t.visible || room <= 0)
		return;
	mScrollY = y / room * max_scroll();
	clamp();
}

} // namespace pyc
