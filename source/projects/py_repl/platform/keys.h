// SPDX-License-Identifier: MIT
/* keys.h -- polls Ctrl+C on a thread while the main thread runs code (py_repl-spec 3.6). */

#pragma once

#include <atomic>
#include <functional>
#include <thread>

namespace pyc {

class InterruptMonitor {
  public:
	~InterruptMonitor() { stop(); }

	/* Calls `on_press` on the monitor thread once per Ctrl+C press while Max is
	 * the frontmost app. `on_press` must be safe to call from any thread. */
	void start(std::function<void()> on_press);
	void stop(); /* joins the thread */

  private:
	std::thread mThread;
	std::atomic<bool> mRun{false};
};

} // namespace pyc
