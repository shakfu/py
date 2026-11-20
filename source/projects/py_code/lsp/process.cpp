// SPDX-License-Identifier: MIT
#include "lsp/process.h"

#include "lsp/framing.h"

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <thread>

#include <fcntl.h>
#include <poll.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

namespace pyc::lsp {

namespace {

std::atomic<int> g_live{0};
std::atomic<int> g_running{0};

const size_t kTailLines = 20;
const size_t kMaxLine = 1000;
const int kDrainMs = 100; /* after exit, read until this long without data */

/* POSIX sh, run by the login shell's exec so any login shell works. No single quotes. */
const char *kAutoScript =
	"for c in pylsp basedpyright-langserver pyright-langserver jedi-language-server ty; do "
	"command -v $c >/dev/null 2>&1 || continue; "
	"case $c in pylsp|jedi-language-server) exec $c;; ty) exec ty server;; *) exec $c --stdio;; esac; "
	"done; "
	"echo \"py_code: no Python language server found "
	"(pylsp, basedpyright, pyright, jedi-language-server, ty)\" >&2; exit 127";

void close_fd(int &fd) {
	if (fd >= 0)
		::close(fd);
	fd = -1;
}

bool write_all(int fd, const std::string &s) {
	size_t done = 0;
	while (done < s.size()) {
		const ssize_t n = ::write(fd, s.data() + done, s.size() - done);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			return false; /* EPIPE with F_SETNOSIGPIPE: child gone */
		}
		done += size_t(n);
	}
	return true;
}

} // namespace

std::vector<std::string> shell_command(const std::string &command) {
	const char *sh = std::getenv("SHELL");
	std::string shell = sh && sh[0] == '/' ? sh : "/bin/zsh";
	std::string inner = command == "auto" ? std::string("exec /bin/sh -c '") + kAutoScript + "'" : "exec " + command;
	return {shell, "-l", "-c", inner};
}

int Process::live() { return g_live.load(); }
int Process::running() { return g_running.load(); }

std::shared_ptr<Process> Process::start(std::vector<std::string> argv, MessageFn on_message, ExitFn on_exit) {
	std::shared_ptr<Process> p(new Process);
	g_live.fetch_add(1);
	p->argv_ = std::move(argv);
	p->on_message_ = std::move(on_message);
	p->on_exit_ = std::move(on_exit);
	g_running.fetch_add(1);
	try {
		std::thread([p] { p->run(); }).detach();
	} catch (const std::exception &e) {
		{
			std::lock_guard<std::mutex> l(p->m_);
			p->exited_ = true;
		}
		p->on_exit_(std::string("cannot start thread: ") + e.what());
		g_running.fetch_sub(1);
	}
	return p;
}

Process::~Process() {
	close_fd(wake_[0]);
	close_fd(wake_[1]);
	g_live.fetch_sub(1);
}

void Process::send(const std::string &json_text) {
	{
		std::lock_guard<std::mutex> l(m_);
		if (closing_)
			return;
		out_.push_back(encode(json_text));
	}
	cv_.notify_all();
}

void Process::stop(const std::vector<std::string> &last) {
	{
		std::lock_guard<std::mutex> l(m_);
		if (stopping_)
			return;
		stopping_ = true;
		if (!closing_)
			for (const std::string &m : last)
				out_.push_back(encode(m));
		closing_ = true;
	}
	cv_.notify_all();
	auto self = shared_from_this();
	try {
		std::thread([self] { self->kill_later(); }).detach();
	} catch (const std::exception &) {
		std::lock_guard<std::mutex> l(m_);
		signal_locked(SIGKILL);
	}
}

std::string Process::stderr_tail() const {
	std::lock_guard<std::mutex> l(m_);
	std::string s;
	for (const std::string &line : tail_)
		s += line + "\n";
	s += partial_;
	while (!s.empty() && s.back() == '\n')
		s.pop_back();
	return s;
}

void Process::signal_locked(int sig) {
	if (exited_)
		return;
	if (pid_ <= 0) { /* not spawned yet */
		doomed_ = true;
		return;
	}
	if (::kill(-pid_, sig) != 0) /* the child leads its own process group */
		::kill(pid_, sig);
}

void Process::kill_later() {
	std::unique_lock<std::mutex> l(m_);
	auto gone = [this] { return exited_.load(); };
	if (cv_.wait_for(l, std::chrono::seconds(2), gone))
		return;
	signal_locked(SIGTERM);
	if (cv_.wait_for(l, std::chrono::seconds(2), gone))
		return;
	signal_locked(SIGKILL);
}

void Process::run() {
	int in[2] = {-1, -1}, out[2] = {-1, -1}, err[2] = {-1, -1};
	std::string why;
	pid_t pid = 0;
	if (::pipe(in) != 0 || ::pipe(out) != 0 || ::pipe(err) != 0 || ::pipe(wake_) != 0) {
		why = std::string("pipe: ") + std::strerror(errno);
	} else {
		/* Our ends must not leak into children other threads spawn. */
		for (int fd : {in[0], in[1], out[0], out[1], err[0], err[1], wake_[0], wake_[1]})
			::fcntl(fd, F_SETFD, FD_CLOEXEC);
		posix_spawn_file_actions_t fa;
		posix_spawnattr_t attr;
		posix_spawn_file_actions_init(&fa);
		posix_spawnattr_init(&attr);
		posix_spawn_file_actions_adddup2(&fa, in[0], 0);
		posix_spawn_file_actions_adddup2(&fa, out[1], 1);
		posix_spawn_file_actions_adddup2(&fa, err[1], 2);
		sigset_t none, all;
		sigemptyset(&none);
		sigfillset(&all);
		sigdelset(&all, SIGKILL);
		sigdelset(&all, SIGSTOP);
		posix_spawnattr_setsigmask(&attr, &none);
		posix_spawnattr_setsigdefault(&attr, &all);
		posix_spawnattr_setpgroup(&attr, 0);
		posix_spawnattr_setflags(&attr, POSIX_SPAWN_CLOEXEC_DEFAULT | POSIX_SPAWN_SETSIGMASK |
											POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETPGROUP);
		std::vector<char *> cargv;
		for (std::string &a : argv_)
			cargv.push_back(&a[0]);
		cargv.push_back(nullptr);
		const int rc = argv_.empty() ? EINVAL : posix_spawn(&pid, cargv[0], &fa, &attr, cargv.data(), environ);
		if (rc != 0)
			why = "cannot start " + (argv_.empty() ? std::string("server") : argv_[0]) + ": " + std::strerror(rc);
		posix_spawn_file_actions_destroy(&fa);
		posix_spawnattr_destroy(&attr);
	}
	close_fd(in[0]);
	close_fd(out[1]);
	close_fd(err[1]);
	if (!why.empty()) {
		close_fd(in[1]);
		close_fd(out[0]);
		close_fd(err[0]);
		{
			std::lock_guard<std::mutex> l(m_);
			exited_ = true;
		}
		cv_.notify_all();
		on_exit_(why);
		g_running.fetch_sub(1);
		return;
	}
	::fcntl(in[1], F_SETNOSIGPIPE, 1);
	{
		std::lock_guard<std::mutex> l(m_);
		pid_ = pid;
		if (doomed_)
			signal_locked(SIGKILL);
	}
	std::thread writer(&Process::write_loop, this, in[1]);
	std::thread drain(&Process::drain_loop, this, err[0]);
	std::thread reader(&Process::read_loop, this, out[0]);

	/* Wait without reaping: the pid stays reserved while kill_later may signal it. */
	siginfo_t info;
	int rc;
	while ((rc = ::waitid(P_PID, id_t(pid), &info, WEXITED | WNOWAIT)) != 0 && errno == EINTR) {
	}
	{
		std::lock_guard<std::mutex> l(m_);
		exited_ = true;
	}
	cv_.notify_all();
	int status = 0;
	bool reaped = false;
	if (rc == 0)
		while (!(reaped = ::waitpid(pid, &status, 0) == pid) && errno == EINTR) {
		}
	close_fd(wake_[1]); /* reader and drain stop once their pipes go quiet */
	reader.join();
	drain.join();
	writer.join();
	std::string reason = "exited";
	if (reaped && WIFEXITED(status))
		reason = "exited with status " + std::to_string(WEXITSTATUS(status));
	else if (reaped && WIFSIGNALED(status))
		reason = "killed by signal " + std::to_string(WTERMSIG(status));
	on_exit_(reason);
	g_running.fetch_sub(1);
}

/* Blocks until fd is readable. After the child exits, gives up after kDrainMs of quiet. */
bool Process::wait_readable(int fd, bool *done) {
	for (;;) {
		pollfd p[2] = {{fd, POLLIN, 0}, {wake_[0], POLLIN, 0}};
		const int rc = ::poll(p, *done ? 1 : 2, *done ? kDrainMs : -1);
		if (rc < 0) {
			if (errno == EINTR)
				continue;
			return false;
		}
		if (rc == 0)
			return false; /* quiet after exit */
		if (p[0].revents)
			return true;
		if (!*done && p[1].revents)
			*done = true;
	}
}

void Process::read_loop(int fd) {
	Framer f;
	std::string msg;
	char buf[65536];
	bool done = false;
	while (wait_readable(fd, &done)) {
		const ssize_t n = ::read(fd, buf, sizeof buf);
		if (n < 0 && (errno == EINTR || errno == EAGAIN))
			continue;
		if (n <= 0)
			break;
		f.feed(buf, size_t(n));
		while (f.next(&msg))
			on_message_(*this, std::move(msg));
	}
	::close(fd);
}

void Process::drain_loop(int fd) {
	char buf[4096];
	bool done = false;
	while (wait_readable(fd, &done)) {
		const ssize_t n = ::read(fd, buf, sizeof buf);
		if (n < 0 && (errno == EINTR || errno == EAGAIN))
			continue;
		if (n <= 0)
			break;
		std::lock_guard<std::mutex> l(m_);
		for (ssize_t i = 0; i < n; ++i) {
			if (buf[i] == '\n') {
				tail_.push_back(std::move(partial_));
				partial_.clear();
				if (tail_.size() > kTailLines)
					tail_.pop_front();
			} else if (partial_.size() < kMaxLine && buf[i] != '\r') {
				partial_ += buf[i];
			}
		}
	}
	::close(fd);
}

void Process::write_loop(int fd) {
	for (;;) {
		std::string msg;
		{
			std::unique_lock<std::mutex> l(m_);
			cv_.wait(l, [this] { return !out_.empty() || closing_ || exited_; });
			if (exited_ || out_.empty())
				break;
			msg = std::move(out_.front());
			out_.pop_front();
		}
		if (!write_all(fd, msg))
			break;
	}
	::close(fd); /* EOF tells the server to exit */
}

} // namespace pyc::lsp
