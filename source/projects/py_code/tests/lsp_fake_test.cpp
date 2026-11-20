// SPDX-License-Identifier: MIT
/* Client against tests/fake_lsp.py, started through the login shell. */
#include "lsp/client.h"
#include "lsp/process.h"

#include "test.h"

#include <atomic>
#include <chrono>
#include <thread>

using namespace pyc::lsp;

namespace {

std::string cmd(const std::string &mode) { return "python3 '" PY_CODE_TEST_DIR "/fake_lsp.py' " + mode; }

template <class Pred> bool wait_for(Pred pred, double seconds = 20) {
	const auto end = std::chrono::steady_clock::now() + std::chrono::duration<double>(seconds);
	for (;;) {
		poll();
		if (pred())
			return true;
		if (std::chrono::steady_clock::now() > end)
			return false;
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
}

/* Polls for `ms` regardless. */
void settle(int ms) {
	wait_for([] { return false; }, ms / 1000.0);
}

bool contains(const std::string &s, const std::string &part) { return s.find(part) != std::string::npos; }

bool ready(const std::shared_ptr<Document> &d) { return wait_for([&] { return d->state() == State::Ready; }); }

/* Every child reaped; with `objects`, every Process freed too (no Document holds one). */
bool no_processes(bool objects = true) {
	return wait_for([&] { return Process::running() == 0 && (!objects || Process::live() == 0); }, 10);
}

const std::string kRoot = "/tmp/py code \xc3\xa9";

} // namespace

TEST(lsp_fake_session) {
	std::atomic<int> wakes{0};
	set_wakeup([&] { ++wakes; });
	auto doc = Document::open(cmd("normal"), kRoot, kRoot + "/a.py");
	CHECK(doc->state() == State::Starting);
	CHECK(!doc->complete(0, [](int, std::vector<pyc::CompletionItem>) {}));
	doc->set_text("x = 1\n", 1); /* held until Ready, then didOpen */
	CHECK(ready(doc));
	CHECK(wakes.load() > 0);
	CHECK_EQ(doc->server_name(), std::string("fake-lsp")); /* also: initialize params as specified */

	/* Server->client requests got null replies; info messages are ignored. */
	CHECK(wait_for([&] { return contains(doc->error(), "replies:"); }));
	CHECK_EQ(doc->error(), std::string("replies: cfg=[null,null] reg=null prog=null"));

	/* didOpen (version 1) publishes an empty list. */
	CHECK(wait_for([&] { return doc->diagnostics_serial() == 1; }));
	CHECK(doc->diagnostics().empty());

	/* didChange: UTF-16 ranges come back as bytes. */
	doc->set_text("\xc3\xa9\xf0\x9f\x98\x80 ERR\nWARN\n", 2);
	CHECK(wait_for([&] { return doc->diagnostics_serial() == 2; }));
	const auto &d = doc->diagnostics();
	CHECK_EQ(d.size(), size_t(2));
	if (d.size() == 2) {
		CHECK_EQ(d[0].start, 7);
		CHECK_EQ(d[0].end, 10);
		CHECK_EQ(d[0].severity, 1);
		CHECK_EQ(d[0].message, std::string("ERR v2"));
		CHECK_EQ(d[1].start, 11);
		CHECK_EQ(d[1].end, 15);
		CHECK_EQ(d[1].severity, 2);
		CHECK_EQ(d[1].message, std::string("WARN v2"));
	}
	doc->set_text("\xc3\xa9\xf0\x9f\x98\x80 ERR\nWARN\n", 2); /* same revision and text: not sent */
	settle(200);
	CHECK_EQ(doc->diagnostics_serial(), 2);

	/* Completion. */
	const std::string text = "\xc3\xa9\xf0\x9f\x98\x80.ab";
	doc->set_text(text, 3);
	int got_rev = -1;
	std::vector<pyc::CompletionItem> items;
	CHECK(doc->complete(int(text.size()), [&](int rev, std::vector<pyc::CompletionItem> v) {
		got_rev = rev;
		items = std::move(v);
	}));
	CHECK(wait_for([&] { return got_rev != -1; }));
	CHECK_EQ(got_rev, 3);
	CHECK_EQ(items.size(), size_t(4));
	if (items.size() == 4) {
		CHECK_EQ(items[0].label, std::string("alpha"));
		CHECK_EQ(items[0].kind, 6);
		CHECK_EQ(items[0].detail, std::string("var"));
		CHECK_EQ(items[0].sort, std::string("2"));
		CHECK_EQ(items[1].insert, std::string("beta()"));
		CHECK_EQ(items[2].insert, std::string("gamma_x"));
		CHECK_EQ(items[2].replace_start, 7);
		CHECK_EQ(items[2].replace_end, 9);
		CHECK_EQ(items[3].label, std::string("pos:0:6")); /* UTF-16 position sent */
	}

	/* Signature help: label offsets, string labels, none. */
	Signature sig;
	int sig_rev = -1;
	auto on_sig = [&](int rev, Signature s) {
		sig_rev = rev;
		sig = std::move(s);
	};
	doc->set_text("sig_offsets(", 4);
	CHECK(doc->signature(12, on_sig));
	CHECK(wait_for([&] { return sig_rev == 4; }));
	CHECK_EQ(sig.label, std::string("f(\xc3\xa4: int, b)"));
	CHECK_EQ(sig.params.size(), size_t(2));
	if (sig.params.size() == 2) {
		CHECK_EQ(sig.params[0].first, 2);
		CHECK_EQ(sig.params[0].second, 9);
		CHECK_EQ(sig.params[1].first, 11);
		CHECK_EQ(sig.params[1].second, 12);
	}
	CHECK_EQ(sig.active, 1);
	doc->set_text("g(", 5);
	CHECK(doc->signature(2, on_sig));
	CHECK(wait_for([&] { return sig_rev == 5; }));
	CHECK_EQ(sig.label, std::string("g(x, y=1)"));
	if (sig.params.size() == 2) {
		CHECK_EQ(sig.params[1].first, 5);
		CHECK_EQ(sig.params[1].second, 8);
	}
	CHECK_EQ(sig.active, 1);
	doc->set_text("nosig(", 6);
	CHECK(doc->signature(6, on_sig));
	CHECK(wait_for([&] { return sig_rev == 6; }));
	CHECK(sig.label.empty());

	/* A second request cancels the first before its reply arrives. */
	int first = 0, second = 0;
	CHECK(doc->signature(2, [&](int, Signature) { ++first; }));
	CHECK(doc->signature(2, [&](int, Signature) { ++second; }));
	CHECK(wait_for([&] { return second == 1; }));
	settle(100);
	CHECK_EQ(first, 0);

	/* Slow reply: the server honours $/cancelRequest. */
	first = second = 0;
	doc->set_text("SLOW", 7);
	CHECK(doc->complete(4, [&](int, std::vector<pyc::CompletionItem>) { ++first; }));
	CHECK(doc->complete(4, [&](int, std::vector<pyc::CompletionItem>) { ++second; }));
	CHECK(wait_for([&] { return contains(doc->error(), "cancelled"); }));
	CHECK(wait_for([&] { return second == 1; }));
	CHECK_EQ(first, 0);

	/* The server ignores the cancel and answers both: the first is still dropped. */
	first = second = 0;
	doc->set_text("STUBBORN", 8);
	CHECK(doc->complete(4, [&](int, std::vector<pyc::CompletionItem>) { ++first; }));
	CHECK(doc->complete(4, [&](int, std::vector<pyc::CompletionItem>) { ++second; }));
	CHECK(wait_for([&] { return second == 1; }));
	CHECK(contains(doc->error(), "late")); /* the first reply was processed before the second */
	CHECK_EQ(first, 0);

	/* Stale revision: the callback reports the revision (and text) it answered for. */
	got_rev = -1;
	doc->set_text("ab", 9);
	CHECK(doc->complete(2, [&](int rev, std::vector<pyc::CompletionItem> v) {
		got_rev = rev;
		items = std::move(v);
	}));
	doc->set_text("xyz ab", 10);
	CHECK(wait_for([&] { return got_rev != -1; }));
	CHECK_EQ(got_rev, 9);
	if (items.size() == 4) {
		CHECK_EQ(items[2].replace_start, 0);
		CHECK_EQ(items[2].replace_end, 2);
	}

	/* Destroying the document with a request pending: callback never runs. */
	bool late = false;
	CHECK(doc->complete(1, [&](int, std::vector<pyc::CompletionItem>) { late = true; }));
	doc.reset();
	CHECK(no_processes());
	settle(100);
	CHECK(!late);
	set_wakeup({});
}

TEST(lsp_fake_utf8_positions) {
	auto doc = Document::open(cmd("utf8"), "", "/tmp/py_code_fake/u.py");
	doc->set_text("\xc3\xa9\xf0\x9f\x98\x80.ab ERR", 1);
	CHECK(ready(doc));
	CHECK(wait_for([&] { return doc->diagnostics_serial() == 1; }));
	if (doc->diagnostics().size() == 1) {
		CHECK_EQ(doc->diagnostics()[0].start, 10);
		CHECK_EQ(doc->diagnostics()[0].end, 13);
	}
	std::vector<pyc::CompletionItem> items;
	bool done = false;
	CHECK(doc->complete(9, [&](int, std::vector<pyc::CompletionItem> v) {
		items = std::move(v);
		done = true;
	}));
	CHECK(wait_for([&] { return done; }));
	if (items.size() == 4) {
		CHECK_EQ(items[3].label, std::string("pos:0:9")); /* byte position sent */
		CHECK_EQ(items[2].replace_start, 7);
		CHECK_EQ(items[2].replace_end, 9);
	}
	doc->set_text("x = 1", 2);
	CHECK(wait_for([&] { return doc->diagnostics_serial() == 2; }));
	CHECK(doc->diagnostics().empty());
	doc.reset();
	CHECK(no_processes());
}

TEST(lsp_fake_sharing) {
	auto a = Document::open(cmd("normal"), kRoot, kRoot + "/a.py");
	auto b = Document::open(cmd("normal"), kRoot, kRoot + "/b.py");
	CHECK(ready(a));
	CHECK(ready(b));
	CHECK_EQ(Process::live(), 1); /* same (command, root): one server */
	auto other_root = Document::open(cmd("normal"), "/tmp", "/tmp/c.py");
	auto same_path = Document::open(cmd("normal"), kRoot, kRoot + "/a.py");
	CHECK(ready(other_root));
	CHECK(ready(same_path));
	CHECK_EQ(Process::live(), 3); /* a path already open gets its own server */

	/* Diagnostics go to the right document. */
	a->set_text("ERR", 1);
	b->set_text("ok", 1);
	same_path->set_text("WARN WARN", 1);
	CHECK(wait_for([&] {
		return a->diagnostics_serial() == 1 && b->diagnostics_serial() == 1 && same_path->diagnostics_serial() == 1;
	}));
	CHECK_EQ(a->diagnostics().size(), size_t(1));
	CHECK(b->diagnostics().empty());
	CHECK_EQ(same_path->diagnostics().size(), size_t(2));

	/* Releasing one document keeps the shared server for the other. */
	a.reset();
	settle(300);
	CHECK(b->state() == State::Ready);
	bool done = false;
	CHECK(b->complete(1, [&](int, std::vector<pyc::CompletionItem>) { done = true; }));
	CHECK(wait_for([&] { return done; }));
	CHECK(wait_for([] { return Process::live() == 3; }));

	/* A new document joins the running server. */
	auto c = Document::open(cmd("normal"), kRoot, kRoot + "/d.py");
	CHECK(c->state() == State::Ready);
	b.reset();
	CHECK(c->state() == State::Ready);
	c.reset();
	other_root.reset();
	same_path.reset();
	CHECK(no_processes());
}

TEST(lsp_fake_python_env) {
	pyc::PythonEnv env;
	env.executable = "/b/python3";
	env.paths = {"/stubs"};
	auto doc = Document::open(cmd("env"), kRoot, kRoot + "/a.py", env);
	CHECK(ready(doc));
	CHECK(wait_for([&] { return contains(doc->error(), "replies:"); }));
	CHECK_EQ(doc->error(),
			 std::string("replies: cfg=[{\"analysis\":{\"extraPaths\":[\"/stubs\"]},\"pythonPath\":\"/b/python3\"},"
						 "{\"extraPaths\":[\"/stubs\"]},null,"
						 "{\"basedpyright\":{\"analysis\":{\"extraPaths\":[\"/stubs\"]}},"
						 "\"python\":{\"analysis\":{\"extraPaths\":[\"/stubs\"]},\"pythonPath\":\"/b/python3\"}}]"
						 " reg=null prog=null"
						 " init={\"pylsp\":{\"plugins\":{\"jedi\":{\"environment\":\"/b/python3\",\"extra_paths\":[\"/stubs\"]}}},"
						 "\"workspace\":{\"environmentPath\":\"/b/python3\",\"extraPaths\":[\"/stubs\"]}}"));

	/* Another env gets its own server; no env sends null initializationOptions. */
	auto plain = Document::open(cmd("env"), kRoot, kRoot + "/b.py");
	CHECK(ready(plain));
	CHECK_EQ(Process::live(), 2);
	CHECK(wait_for([&] { return contains(plain->error(), "replies:"); }));
	CHECK_EQ(plain->error(), std::string("replies: cfg=[null,null,null,null] reg=null prog=null init=null"));
	doc.reset();
	plain.reset();
	CHECK(no_processes());
}

TEST(lsp_fake_crash) {
	auto doc = Document::open(cmd("crash"), "", "/tmp/py_code_fake/crash.py");
	doc->set_text("x", 1);
	CHECK(wait_for([&] { return doc->state() == State::Failed; }));
	CHECK(contains(doc->error(), "status 3"));
	CHECK(contains(doc->error(), "fatal: boom")); /* stderr tail */
	CHECK(!doc->complete(0, [](int, std::vector<pyc::CompletionItem>) {}));
	doc->set_text("y", 2); /* harmless */

	/* No automatic restart; a new document retries with a new server. */
	auto again = Document::open(cmd("crash"), "", "/tmp/py_code_fake/crash.py");
	CHECK(again->state() == State::Starting);
	CHECK(wait_for([&] { return again->state() == State::Failed; }));
	doc.reset();
	again.reset();
	CHECK(no_processes());
}

TEST(lsp_fake_init_error) {
	auto doc = Document::open(cmd("init-error"), "", "/tmp/py_code_fake/e.py");
	CHECK(wait_for([&] { return doc->state() == State::Failed; }));
	CHECK(contains(doc->error(), "initialize failed: nope"));
	doc.reset();
	CHECK(no_processes());
}

TEST(lsp_bad_command) {
	auto doc = Document::open("py_code_no_such_server_xyz --stdio", "", "/tmp/py_code_fake/n.py");
	CHECK(wait_for([&] { return doc->state() == State::Failed; }));
	CHECK(contains(doc->error(), "status 127"));
	CHECK_EQ(doc->server_name(), std::string("py_code_no_such_server_xyz --stdio"));
	auto none = Document::open("", "", "/tmp/py_code_fake/n.py");
	CHECK(none->state() == State::Failed);
	CHECK(!none->error().empty());
	none->set_text("x", 1);
	CHECK(!none->signature(0, [](int, Signature) {}));
	doc.reset();
	CHECK(no_processes());
}

TEST(lsp_destroy_while_starting) {
	for (int i = 0; i < 8; ++i) {
		auto doc = Document::open(cmd("normal"), "", "/tmp/py_code_fake/s.py");
		doc->set_text("x = 1", 1);
		CHECK(!doc->complete(0, [](int, std::vector<pyc::CompletionItem>) {}));
		if (i % 2)
			poll();
	}
	CHECK(no_processes());
	settle(100);
}

TEST(lsp_shutdown_all) {
	auto ready_doc = Document::open(cmd("normal"), "", "/tmp/py_code_fake/r.py");
	ready_doc->set_text("x", 1);
	CHECK(ready(ready_doc));
	auto starting = Document::open(cmd("normal"), "/tmp", "/tmp/py_code_fake/s.py");
	shutdown_all();
	CHECK(ready_doc->state() == State::Stopped);
	CHECK(starting->state() == State::Stopped);
	CHECK(!ready_doc->complete(0, [](int, std::vector<pyc::CompletionItem>) {}));
	ready_doc->set_text("y", 2);
	CHECK(no_processes(false)); /* the documents still hold their stopped servers */
	poll();
	CHECK(ready_doc->state() == State::Stopped);
	ready_doc.reset();
	starting.reset();
	shutdown_all(); /* twice is fine */
	/* A new document after shutdown_all starts a fresh server. */
	auto fresh = Document::open(cmd("normal"), "", "/tmp/py_code_fake/r.py");
	CHECK(ready(fresh));
	fresh.reset();
	CHECK(no_processes());
}

TEST(lsp_stop_escalates_to_sigkill) {
	auto doc = Document::open(cmd("immortal"), "", "/tmp/py_code_fake/i.py");
	CHECK(ready(doc));
	const auto t0 = std::chrono::steady_clock::now();
	doc.reset(); /* shutdown + exit are ignored, so are EOF and SIGTERM */
	const auto took = std::chrono::steady_clock::now() - t0;
	CHECK(took < std::chrono::milliseconds(50)); /* the caller never waits */
	CHECK_EQ(Process::running(), 1);
	settle(1000);
	CHECK_EQ(Process::running(), 1); /* still alive before the 2 + 2 s deadlines */
	CHECK(no_processes());
	const auto total = std::chrono::steady_clock::now() - t0;
	CHECK(total > std::chrono::milliseconds(3900));
	CHECK(total < std::chrono::milliseconds(6000));
}
