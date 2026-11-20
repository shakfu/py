// SPDX-License-Identifier: GPL-3.0-only
/* py_blink.h -- fade of the evaluated block: state and opacity curve (InCubic,
 * 1 -> 0 over `@blinkduration` ms). The object repaints every kStep ms. */

#pragma once

namespace py_code {

class Blink {
  public:
	static constexpr double kDefaultDuration = 500.; /* ms */
	static constexpr double kMinDuration = 50.;      /* a 0 ms blink cannot be seen */
	static constexpr double kMaxDuration = 5000.;
	static constexpr double kStep = 25.; /* ms between repaints (~20 frames in 500) */

	/* Starts the blink of the range [start, end) (byte offsets of the buffer). */
	void start(int start, int end, double now_ms);

	/* Lo cancela (nuevo eval, documento recargado, objeto liberado). */
	void stop();

	/* Duration in ms, clamped to [kMinDuration, kMaxDuration]. */
	void set_duration(double ms);
	double duration() const { return mDuration > 0. ? mDuration : kDefaultDuration; }

	bool active() const { return mActive; }
	int start_off() const { return mStart; }
	int end_off() const { return mEnd; }
	double started_ms() const { return mStartMs; }

	double elapsed(double now_ms) const { return mActive ? now_ms - mStartMs : duration(); }
	/* 1 at the start, 0 at the end: paint multiplies it by the color alpha. */
	double alpha(double now_ms) const;
	bool expired(double now_ms) const { return !mActive || elapsed(now_ms) >= duration(); }

  private:
	bool mActive = false;
	int mStart = -1;
	int mEnd = -1;
	double mStartMs = 0.;
	double mDuration = kDefaultDuration;
};

} // namespace py_code
