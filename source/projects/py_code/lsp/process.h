// SPDX-License-Identifier: MIT
/* process.h -- a language server child process speaking framed JSON on stdio.
 *
 * `start` returns at once; spawning, reading, writing, the stderr drain and
 * reaping all run on threads that keep the Process alive until the child is
 * reaped. Handlers run on those threads, never on the caller's. */

#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <sys/types.h>

namespace pyc::lsp {

class Process : public std::enable_shared_from_this<Process> {
  public:
	using MessageFn = std::function<void(Process &, std::string &&json_text)>; /* reader thread */
	using ExitFn = std::function<void(const std::string &reason)>; /* after the last message */

	/* argv[0] must be an absolute path. */
	static std::shared_ptr<Process> start(std::vector<std::string> argv, MessageFn on_message, ExitFn on_exit);
	~Process();
	Process(const Process &) = delete;
	Process &operator=(const Process &) = delete;

	void send(const std::string &json_text); /* any thread; dropped after stop() */
	/* Send `last`, close stdin; SIGTERM after 2 s, SIGKILL after 2 more. Returns at once. */
	void stop(const std::vector<std::string> &last = {});
	std::string stderr_tail() const; /* last 20 lines */

	static int live();    /* Process objects not yet destroyed (tests) */
	static int running(); /* children not yet reaped, threads not finished (tests) */

  private:
	Process() = default;
	void run();
	void read_loop(int fd);
	void drain_loop(int fd);
	void write_loop(int fd);
	void kill_later();
	void signal_locked(int sig);
	bool wait_readable(int fd, bool *done);

	std::vector<std::string> argv_;
	MessageFn on_message_;
	ExitFn on_exit_;

	mutable std::mutex m_;
	std::condition_variable cv_;
	std::deque<std::string> out_;
	bool closing_ = false; /* no more sends; writer closes stdin when drained */
	bool stopping_ = false;
	bool doomed_ = false; /* SIGKILL as soon as spawned */
	std::atomic<bool> exited_{false};
	pid_t pid_ = 0;
	int wake_[2] = {-1, -1}; /* write end closed when the child exits */
	std::deque<std::string> tail_;
	std::string partial_;
};

/* argv for `$SHELL -l -c 'exec <command>'`; "auto" picks the first server found. */
std::vector<std::string> shell_command(const std::string &command);

} // namespace pyc::lsp
