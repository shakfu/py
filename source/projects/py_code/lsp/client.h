// SPDX-License-Identifier: MIT
/* client.h -- LSP client for py_code.
 *
 * One server process per (command, root, Python env), shared by its documents and stopped
 * when the last one closes. Everything here is called from one thread (Max's
 * main thread); callbacks run only inside poll() on that thread. The wake-up
 * function is the exception: it may be called from any thread. */

#pragma once

#include "core/completion_item.h"
#include "core/python_env.h"
#include "lsp/types.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace pyc::lsp {

namespace detail {
class Server;
}

class Document {
	struct Key {
		explicit Key() = default;
	};

  public:
	/* `revision` is that of the text the server answered for: compare it to the
	 * caller's current revision to drop stale results. */
	using CompletionFn = std::function<void(int revision, std::vector<CompletionItem> items)>;
	using SignatureFn = std::function<void(int revision, Signature sig)>;

	/* Opens (or joins) a server. Never blocks. `path`: absolute .py path (may not exist).
	 * `env` is sent as server settings (see python_settings, python_init_options). */
	static std::shared_ptr<Document> open(const std::string &command, const std::string &root_dir,
										  const std::string &path, const PythonEnv &env = {});
	Document(Key, const std::string &path);
	~Document(); /* didClose; releases the server */
	Document(const Document &) = delete;
	Document &operator=(const Document &) = delete;

	/* didOpen the first time (deferred until Ready), then one full didChange.
	 * The same revision and text again is a no-op. */
	void set_text(const std::string &text, int revision);

	/* Offsets are into the text of the last set_text. False when not Ready, no text
	 * was sent, or the server lacks the feature. A new request of the same kind
	 * cancels the previous one, whose callback then never runs. An error reply
	 * calls back with an empty result. */
	bool complete(int offset, CompletionFn cb);
	bool signature(int offset, SignatureFn cb); /* empty label = none */

	const std::vector<Diagnostic> &diagnostics() const { return diags_; }
	int diagnostics_serial() const { return diag_serial_; } /* +1 per publish */

	State state() const;
	std::string server_name() const; /* serverInfo.name, else the command */
	std::string error() const;       /* failure reason + stderr tail, or last server error message */
	const std::string &path() const { return path_; }

  private:
	friend class detail::Server;
	template <class Fn> struct Request {
		int id = 0; /* 0 = none pending */
		int revision = 0;
		std::shared_ptr<const std::string> text;
		Fn cb;
	};
	bool can_request() const;
	void flush();
	template <class Fn> void cancel(Request<Fn> &r);

	std::shared_ptr<detail::Server> server_;
	std::string path_, uri_, error_;
	std::shared_ptr<const std::string> sent_; /* text last sent */
	int sent_rev_ = 0;
	bool opened_ = false;
	bool has_pending_ = false;
	std::string pending_;
	int pending_rev_ = 0;
	Request<CompletionFn> completion_;
	Request<SignatureFn> signature_;
	std::vector<Diagnostic> diags_;
	int diag_serial_ = 0;
};

void set_wakeup(std::function<void()> fn); /* fn may run on any thread; {} clears */
void poll();         /* dispatch inbound messages and callbacks */
void shutdown_all(); /* stop every server; returns at once */

} // namespace pyc::lsp
