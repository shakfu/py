// SPDX-License-Identifier: MIT
/* Client against the real pylsp through the login shell. Own main():
 * exit 77 when pylsp cannot be started (ctest skip), else the failure count. */
#include "lsp/client.h"
#include "lsp/process.h"

#include "test.h"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <thread>

#include <sys/stat.h>

using namespace pyc::lsp;

namespace {

template <class Pred> bool wait_for(Pred pred, double seconds) {
	const auto end = std::chrono::steady_clock::now() + std::chrono::duration<double>(seconds);
	for (;;) {
		poll();
		if (pred())
			return true;
		if (std::chrono::steady_clock::now() > end)
			return false;
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

} // namespace

int main() {
	const std::string root = "/tmp/py_code_lsp_integration";
	auto doc = Document::open("pylsp", root, root + "/probe.py");
	wait_for([&] { return doc->state() != State::Starting; }, 60);
	if (doc->state() == State::Failed && doc->error().find("status 127") != std::string::npos) {
		std::printf("skip: pylsp not found through the login shell\n%s\n", doc->error().c_str());
		return 77;
	}
	CHECK(doc->state() == State::Ready);
	if (doc->state() != State::Ready) {
		std::printf("pylsp did not start: %s\n", doc->error().c_str());
		return 1;
	}
	CHECK_EQ(doc->server_name(), std::string("pylsp"));

	/* didOpen publishes diagnostics. Their content depends on the installed linter
	 * plugins (none in a bare pylsp), so ranges are checked, and the unused-import
	 * range only when a linter reports it. */
	const std::string text = "import os\nimport sys\nos.pa";
	doc->set_text(text, 1);
	CHECK(wait_for([&] { return doc->diagnostics_serial() > 0; }, 60));
	for (const Diagnostic &d : doc->diagnostics()) {
		CHECK(d.start >= 0 && d.start <= d.end && d.end <= int(text.size()));
		CHECK(d.severity >= 1 && d.severity <= 4);
		if (d.message.find("'sys'") != std::string::npos)
			CHECK_EQ(text.substr(size_t(d.start), size_t(d.end - d.start)), std::string("import sys"));
	}
	std::printf("pylsp published %zu diagnostics\n", doc->diagnostics().size());

	/* Completion after `os.pa`. */
	bool done = false;
	int rev = -1;
	std::vector<pyc::CompletionItem> items;
	CHECK(doc->complete(int(text.size()), [&](int r, std::vector<pyc::CompletionItem> v) {
		rev = r;
		items = std::move(v);
		done = true;
	}));
	CHECK(wait_for([&] { return done; }, 60));
	CHECK_EQ(rev, 1);
	bool path = false;
	for (const pyc::CompletionItem &c : items)
		path = path || c.insert == "path";
	CHECK(path);
	CHECK(items.size() <= 200);

	/* Signature help inside `len(`. */
	const std::string call = "len(";
	doc->set_text(call, 2);
	Signature sig;
	done = false;
	CHECK(doc->signature(int(call.size()), [&](int r, Signature s) {
		rev = r;
		sig = std::move(s);
		done = true;
	}));
	CHECK(wait_for([&] { return done; }, 60));
	CHECK_EQ(rev, 2);
	CHECK(sig.label.compare(0, 4, "len(") == 0);
	CHECK(!sig.params.empty());
	if (!sig.params.empty() && sig.params[0].first >= 0)
		CHECK_EQ(sig.label.substr(size_t(sig.params[0].first), size_t(sig.params[0].second - sig.params[0].first)),
				 std::string("obj"));
	CHECK_EQ(sig.active, 0);

	/* The api stub, passed as an extra path, completes the built-in `api` module. */
	const std::string stubs = root + "/stubs";
	::mkdir(root.c_str(), 0755);
	::mkdir(stubs.c_str(), 0755);
	std::ofstream(stubs + "/api.pyi") << std::ifstream(PY_API_STUB).rdbuf();
	pyc::PythonEnv env;
	env.paths = {stubs};
	auto api = Document::open("pylsp", root, root + "/api_probe.py", env);
	CHECK(wait_for([&] { return api->state() != State::Starting; }, 60));
	const std::string api_text = "import api\napi.Dic";
	api->set_text(api_text, 1);
	done = false;
	items.clear();
	CHECK(api->complete(int(api_text.size()), [&](int, std::vector<pyc::CompletionItem> v) {
		items = std::move(v);
		done = true;
	}));
	CHECK(wait_for([&] { return done; }, 60));
	bool dictionary = false;
	for (const pyc::CompletionItem &c : items)
		dictionary = dictionary || c.insert == "Dictionary";
	CHECK(dictionary);
	api.reset();

	/* `auto` finds a server too, and shutdown_all reaps everything. */
	auto any = Document::open("auto", root, root + "/auto.py");
	CHECK(wait_for([&] { return any->state() != State::Starting; }, 60));
	CHECK(any->state() == State::Ready);
	shutdown_all();
	CHECK(doc->state() == State::Stopped);
	CHECK(wait_for([] { return Process::running() == 0; }, 10));
	doc.reset();
	any.reset();
	CHECK_EQ(Process::live(), 0);
	std::printf("%s pylsp integration\n", test::failures() ? "FAIL" : "ok  ");
	return test::failures();
}
