// SPDX-License-Identifier: GPL-3.0-only
/* py_blink.cpp -- see py_blink.h. */

#include "py_blink.h"

namespace py_code {

void Blink::start(int start, int end, double now_ms) {
	if (end <= start) {
		stop();
		return;
	}
	mStart = start;
	mEnd = end;
	mStartMs = now_ms;
	mActive = true;
}

void Blink::stop() {
	mActive = false;
	mStart = -1;
	mEnd = -1;
	mStartMs = 0.;
}

void Blink::set_duration(double ms) {
	/* `!(ms >= kMin)` instead of `ms < kMin` so that a NaN also falls into the min. */
	if (!(ms >= kMinDuration))
		ms = kMinDuration;
	if (ms > kMaxDuration)
		ms = kMaxDuration;
	mDuration = ms;
}

double Blink::alpha(double now_ms) const {
	if (!mActive)
		return 0.;
	const double t = elapsed(now_ms) / duration();
	if (t <= 0.)
		return 1.;
	if (t >= 1.)
		return 0.;
	/* InCubic over opacity 1.0 -> 0.0: stays high, then drops fast at the end. */
	return 1. - t * t * t;
}

} // namespace py_code
