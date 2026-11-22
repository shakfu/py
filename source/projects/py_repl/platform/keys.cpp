// SPDX-License-Identifier: MIT
/* keys.cpp -- see keys.h. CoreGraphics only: the main thread is busy, so no AppKit. */

#include "platform/keys.h"

#include <CoreGraphics/CoreGraphics.h>
#include <unistd.h>

#include <chrono>

namespace pyc {

namespace {

constexpr CGKeyCode kKeyC = 8; /* kVK_ANSI_C, without pulling in Carbon */
constexpr auto kPoll = std::chrono::milliseconds(20);

bool ctrl_c_down() {
	const CGEventSourceStateID s = kCGEventSourceStateCombinedSessionState;
	return CGEventSourceKeyState(s, kKeyC) && (CGEventSourceFlagsState(s) & kCGEventFlagMaskControl);
}

int number(CFDictionaryRef d, CFStringRef key) {
	int v = -1;
	CFNumberRef n = (CFNumberRef)CFDictionaryGetValue(d, key);
	if (!n || !CFNumberGetValue(n, kCFNumberIntType, &v))
		return -1;
	return v;
}

/* Owner of the frontmost normal window: the window server answers while our
 * main thread, and so AppKit's view of the active app, is blocked. -1: unknown. */
pid_t front_pid() {
	CFArrayRef wins = CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
												 kCGNullWindowID);
	if (!wins)
		return -1;
	pid_t pid = -1;
	for (CFIndex i = 0; i < CFArrayGetCount(wins); ++i) {
		CFDictionaryRef w = (CFDictionaryRef)CFArrayGetValueAtIndex(wins, i);
		if (number(w, kCGWindowLayer) == 0) {
			pid = (pid_t)number(w, kCGWindowOwnerPID);
			break;
		}
	}
	CFRelease(wins);
	return pid;
}

} // namespace

void InterruptMonitor::start(std::function<void()> on_press) {
	stop();
	mRun = true;
	mThread = std::thread([this, on_press = std::move(on_press)] {
		bool was = ctrl_c_down(); /* a key held at start is not a press */
		while (mRun) {
			const bool now = ctrl_c_down();
			if (now && !was) {
				const pid_t front = front_pid();
				if (front < 0 || front == getpid()) /* unknown: interrupt anyway */
					on_press();
			}
			was = now;
			std::this_thread::sleep_for(kPoll);
		}
	});
}

void InterruptMonitor::stop() {
	mRun = false;
	if (mThread.joinable())
		mThread.join();
}

} // namespace pyc
