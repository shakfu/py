// SPDX-License-Identifier: MIT
#include "lsp/client.h"

#include "lsp/convert.h"
#include "lsp/position.h"
#include "lsp/process.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <mutex>

#include <unistd.h>

namespace pyc::lsp {

using json = nlohmann::json;

namespace detail {

std::string dump(const json &j) { return j.dump(-1, ' ', false, json::error_handler_t::replace); }

/* Inbound queue: filled by process threads, drained by poll(). */
struct Event {
	std::weak_ptr<Server> server;
	bool exit = false;
	json msg;
	std::string reason;
};

struct Hub {
	std::mutex m;
	std::vector<Event> queue;
	std::mutex wake_m; /* held while calling wake: set_wakeup({}) waits out a call */
	std::function<void()> wake;

	void push(Event e) {
		bool was_empty;
		{
			std::lock_guard<std::mutex> l(m);
			was_empty = queue.empty();
			queue.push_back(std::move(e));
		}
		if (was_empty) {
			std::lock_guard<std::mutex> l(wake_m);
			if (wake)
				wake();
		}
	}
};

/* Never destroyed: process threads may still push during static destruction. */
Hub &hub() {
	static Hub *h = new Hub;
	return *h;
}

std::vector<std::shared_ptr<Server>> &servers() {
	static auto *v = new std::vector<std::shared_ptr<Server>>;
	return *v;
}

/* Reply to a server->client request (runs on the reader thread). */
json reply_to(const json &req, const json &settings) {
	json result = nullptr;
	if (req.value("method", "") == "workspace/configuration")
		result = configuration_reply(settings, req.value("params", json::object()));
	return {{"jsonrpc", "2.0"}, {"id", req["id"]}, {"result", result}};
}

bool provides(const json &caps, const char *key) {
	const auto it = caps.find(key);
	return it != caps.end() && !it->is_null() && !(it->is_boolean() && !it->get<bool>());
}

class Server : public std::enable_shared_from_this<Server> {
  public:
	Server(const std::string &command, const std::string &root, const PythonEnv &env)
		: command(command), root(root), env(env) {}

	const std::string command, root;
	const PythonEnv env;
	std::shared_ptr<Process> proc;
	State state = State::Starting;
	std::string name, error;
	bool utf16 = true, has_completion = false, has_signature = false;
	int next_id = 1, init_id = 0;
	std::vector<Document *> docs;

	bool has_path(const std::string &path) const {
		return std::any_of(docs.begin(), docs.end(), [&](const Document *d) { return d->path_ == path; });
	}

	void start() {
		std::weak_ptr<Server> weak = shared_from_this();
		auto on_message = [weak, settings = python_settings(env)](Process &p, std::string &&text) {
			json msg = json::parse(text, nullptr, false);
			if (msg.is_discarded() || !msg.is_object())
				return;
			if (msg.contains("method") && msg.contains("id")) {
				p.send(dump(reply_to(msg, settings)));
				return;
			}
			hub().push(Event{weak, false, std::move(msg), {}});
		};
		auto on_exit = [weak](const std::string &reason) { hub().push(Event{weak, true, nullptr, reason}); };
		proc = Process::start(shell_command(command), on_message, on_exit);
		json folders = nullptr, root_uri = nullptr;
		if (!root.empty()) {
			root_uri = path_to_uri(root);
			const size_t slash = root.find_last_of('/', root.size() > 1 ? root.size() - 2 : 0);
			std::string base = slash == std::string::npos ? root : root.substr(slash + 1);
			if (!base.empty() && base.back() == '/')
				base.pop_back();
			folders = json::array({{{"uri", root_uri}, {"name", base.empty() ? root : base}}});
		}
		json caps = {
			{"general", {{"positionEncodings", {"utf-8", "utf-16"}}}},
			{"textDocument",
			 {{"synchronization", {{"dynamicRegistration", false}, {"didSave", false}}},
			  {"completion",
			   {{"completionItem", {{"snippetSupport", false}, {"documentationFormat", {"plaintext"}}}}}},
			  {"signatureHelp",
			   {{"signatureInformation",
				 {{"documentationFormat", {"plaintext"}}, {"parameterInformation", {{"labelOffsetSupport", true}}}}}}},
			  {"publishDiagnostics", json::object()}}},
			{"workspace", {{"configuration", true}}},
		};
		init_id = request("initialize", {{"processId", int(getpid())},
										 {"clientInfo", {{"name", "py_code"}}},
										 {"rootUri", root_uri},
										 {"workspaceFolders", folders},
										 {"capabilities", caps},
										 {"initializationOptions", python_init_options(env)}});
	}

	int request(const char *method, json params) {
		const int id = next_id++;
		proc->send(dump({{"jsonrpc", "2.0"}, {"id", id}, {"method", method}, {"params", std::move(params)}}));
		return id;
	}

	void notify(const char *method, json params) {
		proc->send(dump({{"jsonrpc", "2.0"}, {"method", method}, {"params", std::move(params)}}));
	}

	void clear_requests() {
		for (Document *d : docs) {
			d->completion_ = {};
			d->signature_ = {};
		}
	}

	void unregister() {
		auto &v = servers();
		v.erase(std::remove(v.begin(), v.end(), shared_from_this()), v.end());
	}

	void fail(const std::string &why) {
		state = State::Failed;
		error = why;
		const std::string tail = proc->stderr_tail();
		if (!tail.empty())
			error += "\n" + tail;
		clear_requests();
		proc->stop({dump({{"jsonrpc", "2.0"}, {"method", "exit"}})});
		unregister();
	}

	/* Deliberate stop: shutdown request, then exit. */
	void stop() {
		if (state != State::Starting && state != State::Ready)
			return;
		state = State::Stopped;
		clear_requests();
		const std::string shutdown = dump({{"jsonrpc", "2.0"}, {"id", next_id++}, {"method", "shutdown"}});
		proc->stop({shutdown, dump({{"jsonrpc", "2.0"}, {"method", "exit"}})});
		unregister();
	}

	void exited(const std::string &reason) {
		if (state == State::Starting || state == State::Ready)
			fail((name.empty() ? command : name) + ": " + reason);
	}

	void handle(const json &msg) {
		if (state != State::Starting && state != State::Ready)
			return;
		if (msg.contains("method"))
			notification(msg.value("method", ""), msg.value("params", json::object()));
		else if (msg.contains("id") && msg["id"].is_number_integer())
			response(msg["id"].get<int>(), msg);
	}

	void initialized(const json &msg) {
		if (msg.contains("error")) {
			const json &e = msg["error"];
			fail("initialize failed: " + (e.is_object() ? e.value("message", std::string("error")) : dump(e)));
			return;
		}
		const json result = msg.value("result", json::object());
		const json caps = result.is_object() ? result.value("capabilities", json::object()) : json::object();
		if (caps.is_object()) {
			utf16 = caps.value("positionEncoding", json("utf-16")) != json("utf-8");
			has_completion = provides(caps, "completionProvider");
			has_signature = provides(caps, "signatureHelpProvider");
		}
		if (result.is_object() && result.contains("serverInfo") && result["serverInfo"].is_object())
			name = result["serverInfo"].value("name", std::string());
		notify("initialized", json::object());
		state = State::Ready;
		for (Document *d : docs)
			d->flush();
	}

	void response(int id, const json &msg) {
		if (id == init_id && state == State::Starting) {
			initialized(msg);
			return;
		}
		if (state != State::Ready)
			return;
		const bool ok = !msg.contains("error");
		const json result = ok ? msg.value("result", json()) : json();
		for (Document *d : docs) {
			if (d->completion_.id == id) {
				auto r = std::move(d->completion_);
				d->completion_ = {};
				/* d may be destroyed by the callback: only r from here on. */
				r.cb(r.revision, ok ? completion_items(result, *r.text, utf16) : std::vector<CompletionItem>());
				return;
			}
			if (d->signature_.id == id) {
				auto r = std::move(d->signature_);
				d->signature_ = {};
				r.cb(r.revision, ok ? signature_help(result, utf16) : Signature());
				return;
			}
		}
	}

	void notification(const std::string &method, const json &params) {
		if (!params.is_object())
			return;
		if (method == "textDocument/publishDiagnostics") {
			const std::string path = uri_to_path(params.value("uri", std::string()));
			for (Document *d : docs) {
				if (d->path_ != path || !d->sent_)
					continue;
				d->diags_ = diagnostics(params.value("diagnostics", json::array()), *d->sent_, utf16);
				++d->diag_serial_;
			}
		} else if (method == "window/showMessage" || method == "window/logMessage") {
			if (params.value("type", 0) == 1)
				error = params.value("message", std::string());
		}
	}
};

} // namespace detail

using detail::hub;
using detail::Server;
using detail::servers;

std::shared_ptr<Document> Document::open(const std::string &command, const std::string &root_dir,
										 const std::string &path, const PythonEnv &env) {
	auto d = std::make_shared<Document>(Key{}, path);
	if (command.empty()) {
		d->error_ = "no server command";
		return d;
	}
	std::shared_ptr<Server> s;
	for (const auto &c : servers())
		if (c->command == command && c->root == root_dir && c->env == env && !c->has_path(path)) {
			s = c;
			break;
		}
	if (!s) { /* a path already open on the shared server gets its own */
		s = std::make_shared<Server>(command, root_dir, env);
		servers().push_back(s);
		s->start();
	}
	d->server_ = s;
	s->docs.push_back(d.get());
	if (s->state == State::Ready)
		d->flush();
	return d;
}

Document::Document(Key, const std::string &path) : path_(path), uri_(path_to_uri(path)) {}

Document::~Document() {
	if (!server_)
		return;
	Server &s = *server_;
	if (s.state == State::Ready) {
		cancel(completion_);
		cancel(signature_);
		if (opened_)
			s.notify("textDocument/didClose", {{"textDocument", {{"uri", uri_}}}});
	}
	s.docs.erase(std::remove(s.docs.begin(), s.docs.end(), this), s.docs.end());
	if (s.docs.empty())
		s.stop();
}

void Document::set_text(const std::string &text, int revision) {
	if (!server_)
		return;
	std::string safe = utf8_safe(text);
	if (has_pending_ ? (revision == pending_rev_ && safe == pending_)
					 : (sent_ && revision == sent_rev_ && safe == *sent_))
		return;
	pending_ = std::move(safe);
	pending_rev_ = revision;
	has_pending_ = true;
	if (server_->state == State::Ready)
		flush();
}

void Document::flush() {
	if (!has_pending_)
		return;
	sent_ = std::make_shared<const std::string>(std::move(pending_));
	sent_rev_ = pending_rev_;
	pending_.clear();
	has_pending_ = false;
	if (!opened_) {
		opened_ = true;
		server_->notify("textDocument/didOpen",
						{{"textDocument",
						  {{"uri", uri_}, {"languageId", "python"}, {"version", sent_rev_}, {"text", *sent_}}}});
	} else {
		server_->notify("textDocument/didChange", {{"textDocument", {{"uri", uri_}, {"version", sent_rev_}}},
												   {"contentChanges", json::array({{{"text", *sent_}}})}});
	}
}

bool Document::can_request() const { return server_ && server_->state == State::Ready && opened_ && sent_; }

template <class Fn> void Document::cancel(Request<Fn> &r) {
	if (r.id != 0 && server_ && server_->state == State::Ready)
		server_->notify("$/cancelRequest", {{"id", r.id}});
	r = {};
}

bool Document::complete(int offset, CompletionFn cb) {
	if (!can_request() || !server_->has_completion)
		return false;
	cancel(completion_);
	const Position p = to_position(*sent_, offset, server_->utf16);
	completion_.id = server_->request("textDocument/completion",
									  {{"textDocument", {{"uri", uri_}}},
									   {"position", {{"line", p.line}, {"character", p.character}}}});
	completion_.revision = sent_rev_;
	completion_.text = sent_;
	completion_.cb = std::move(cb);
	return true;
}

bool Document::signature(int offset, SignatureFn cb) {
	if (!can_request() || !server_->has_signature)
		return false;
	cancel(signature_);
	const Position p = to_position(*sent_, offset, server_->utf16);
	signature_.id = server_->request("textDocument/signatureHelp",
									 {{"textDocument", {{"uri", uri_}}},
									  {"position", {{"line", p.line}, {"character", p.character}}}});
	signature_.revision = sent_rev_;
	signature_.text = sent_;
	signature_.cb = std::move(cb);
	return true;
}

State Document::state() const { return server_ ? server_->state : State::Failed; }

std::string Document::server_name() const {
	if (!server_)
		return "";
	return server_->name.empty() ? server_->command : server_->name;
}

std::string Document::error() const { return server_ ? server_->error : error_; }

void set_wakeup(std::function<void()> fn) {
	detail::Hub &h = hub();
	bool pending;
	{
		std::lock_guard<std::mutex> l(h.m);
		pending = !h.queue.empty();
	}
	std::lock_guard<std::mutex> l(h.wake_m);
	h.wake = std::move(fn);
	if (pending && h.wake) /* work queued before the wake-up existed */
		h.wake();
}

void poll() {
	static bool busy = false; /* a callback calling poll() is a no-op */
	if (busy)
		return;
	busy = true;
	struct Reset {
		~Reset() { busy = false; }
	} reset;
	std::vector<detail::Event> batch;
	{
		std::lock_guard<std::mutex> l(hub().m);
		batch.swap(hub().queue);
	}
	for (detail::Event &e : batch) {
		const std::shared_ptr<Server> s = e.server.lock();
		if (!s)
			continue;
		if (e.exit)
			s->exited(e.reason);
		else
			s->handle(e.msg);
	}
}

void shutdown_all() {
	const auto list = servers();
	for (const auto &s : list)
		s->stop();
	servers().clear();
}

} // namespace pyc::lsp
