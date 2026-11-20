// SPDX-License-Identifier: MIT
#include "lsp/convert.h"

#include "lsp/position.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>

namespace pyc::lsp {

using json = nlohmann::json;

namespace {

std::string str(const json &j, const char *key) {
	const auto it = j.find(key);
	return it != j.end() && it->is_string() ? it->get<std::string>() : std::string();
}

int num(const json &j, const char *key, int fallback) {
	const auto it = j.find(key);
	return it != j.end() && it->is_number_integer() ? it->get<int>() : fallback;
}

bool range_bytes(const json &range, const LineIndex &idx, bool utf16, int *start, int *end) {
	if (!range.is_object())
		return false;
	const auto s = range.find("start"), e = range.find("end");
	if (s == range.end() || e == range.end() || !s->is_object() || !e->is_object())
		return false;
	*start = idx.offset(num(*s, "line", 0), num(*s, "character", 0), utf16);
	*end = idx.offset(num(*e, "line", 0), num(*e, "character", 0), utf16);
	if (*end < *start)
		std::swap(*start, *end);
	return true;
}

bool ident(char c) { return std::isalnum((unsigned char)c) || c == '_' || (unsigned char)c >= 0x80; }

/* First occurrence of `p` at or after `from` not glued to identifier characters. */
size_t find_param(const std::string &label, const std::string &p, size_t from) {
	if (p.empty())
		return std::string::npos;
	for (size_t at = label.find(p, from); at != std::string::npos; at = label.find(p, at + 1)) {
		const bool left = at == 0 || !ident(p.front()) || !ident(label[at - 1]);
		const size_t after = at + p.size();
		const bool right = after >= label.size() || !ident(p.back()) || !ident(label[after]);
		if (left && right)
			return at;
	}
	const size_t at = label.find(p, from);
	return at != std::string::npos ? at : label.find(p);
}

} // namespace

std::vector<CompletionItem> completion_items(const json &result, const std::string &text, bool utf16,
											 size_t limit) {
	const json *list = nullptr;
	if (result.is_array())
		list = &result;
	else if (result.is_object() && result.contains("items") && result["items"].is_array())
		list = &result["items"];
	std::vector<CompletionItem> out;
	if (!list)
		return out;
	const LineIndex idx(text);
	for (const json &it : *list) {
		if (!it.is_object())
			continue;
		CompletionItem c;
		c.label = str(it, "label");
		c.detail = str(it, "detail");
		c.filter = str(it, "filterText");
		c.sort = str(it, "sortText");
		c.kind = num(it, "kind", 0);
		const auto edit = it.find("textEdit");
		if (edit != it.end() && edit->is_object() && edit->contains("newText")) {
			c.insert = str(*edit, "newText");
			const auto r = edit->contains("range") ? edit->find("range") : edit->find("insert");
			if (r != edit->end())
				range_bytes(*r, idx, utf16, &c.replace_start, &c.replace_end);
		} else if (it.contains("insertText") && it["insertText"].is_string()) {
			c.insert = str(it, "insertText");
		} else {
			c.insert = c.label;
		}
		out.push_back(std::move(c));
	}
	if (out.size() > limit) { /* keep the best by LSP order: sortText, else label */
		std::stable_sort(out.begin(), out.end(), [](const CompletionItem &a, const CompletionItem &b) {
			return (a.sort.empty() ? a.label : a.sort) < (b.sort.empty() ? b.label : b.sort);
		});
		out.resize(limit);
	}
	return out;
}

Signature signature_help(const json &result, bool utf16) {
	Signature s;
	if (!result.is_object() || !result.contains("signatures") || !result["signatures"].is_array())
		return s;
	const json &sigs = result["signatures"];
	if (sigs.empty())
		return s;
	int which = num(result, "activeSignature", 0);
	if (which < 0 || which >= int(sigs.size()))
		which = 0;
	const json &sig = sigs[size_t(which)];
	if (!sig.is_object())
		return s;
	s.label = str(sig, "label");
	size_t from = s.label.find('(');
	from = from == std::string::npos ? 0 : from + 1;
	if (sig.contains("parameters") && sig["parameters"].is_array()) {
		for (const json &p : sig["parameters"]) {
			std::pair<int, int> r{-1, -1};
			const json lab = p.is_object() && p.contains("label") ? p["label"] : json();
			if (lab.is_array() && lab.size() == 2 && lab[0].is_number_integer() && lab[1].is_number_integer()) {
				r.first = advance_units(s.label, 0, lab[0].get<int>(), utf16);
				r.second = std::max(r.first, advance_units(s.label, 0, lab[1].get<int>(), utf16));
			} else if (lab.is_string()) {
				const std::string name = lab.get<std::string>();
				const size_t at = find_param(s.label, name, from);
				if (at != std::string::npos) {
					r = {int(at), int(at + name.size())};
					from = at + name.size();
				}
			}
			s.params.push_back(r);
		}
	}
	/* The signature's activeParameter overrides the top-level one; null = none. */
	json active = 0;
	if (sig.contains("activeParameter"))
		active = sig["activeParameter"];
	else if (result.contains("activeParameter"))
		active = result["activeParameter"];
	if (active.is_number_integer() && !s.params.empty()) {
		const int a = active.get<int>();
		s.active = a >= 0 && a < int(s.params.size()) ? a : -1;
	}
	return s;
}

std::vector<Diagnostic> diagnostics(const json &list, const std::string &text, bool utf16) {
	std::vector<Diagnostic> out;
	if (!list.is_array())
		return out;
	const LineIndex idx(text);
	for (const json &d : list) {
		Diagnostic g;
		if (!d.is_object() || !range_bytes(d.value("range", json()), idx, utf16, &g.start, &g.end))
			continue;
		g.severity = std::clamp(num(d, "severity", 1), 1, 4);
		g.message = str(d, "message");
		out.push_back(std::move(g));
	}
	return out;
}

json python_settings(const PythonEnv &env) {
	json s = json::object();
	if (env.empty())
		return s;
	const json analysis = {{"extraPaths", env.paths}};
	json python = {{"analysis", analysis}};
	if (!env.executable.empty())
		python["pythonPath"] = env.executable;
	s["python"] = python;
	s["basedpyright"] = {{"analysis", analysis}};
	return s;
}

json python_init_options(const PythonEnv &env) {
	if (env.empty())
		return nullptr;
	json jedi = {{"extra_paths", env.paths}};
	json workspace = {{"extraPaths", env.paths}};
	if (!env.executable.empty()) {
		jedi["environment"] = env.executable;
		workspace["environmentPath"] = env.executable;
	}
	return {{"pylsp", {{"plugins", {{"jedi", jedi}}}}}, {"workspace", workspace}};
}

json configuration_reply(const json &settings, const json &params) {
	json out = json::array();
	if (!params.is_object() || !params.contains("items") || !params["items"].is_array())
		return out;
	for (const json &item : params["items"]) {
		if (!item.is_object()) {
			out.push_back(nullptr);
			continue;
		}
		const std::string section = str(item, "section");
		if (section.empty()) {
			out.push_back(settings.empty() ? json(nullptr) : settings);
			continue;
		}
		const json *at = &settings;
		for (size_t a = 0; at && a <= section.size();) {
			const size_t b = std::min(section.find('.', a), section.size());
			const auto it = at->is_object() ? at->find(section.substr(a, b - a)) : at->end();
			at = it == at->end() ? nullptr : &*it;
			a = b + 1;
		}
		out.push_back(at ? *at : json(nullptr));
	}
	return out;
}

std::string path_to_uri(const std::string &path) {
	static const char hex[] = "0123456789ABCDEF";
	std::string out = "file://";
	for (unsigned char c : path) {
		if (std::isalnum(c) || c == '/' || c == '-' || c == '.' || c == '_' || c == '~') {
			out += char(c);
		} else {
			out += '%';
			out += hex[c >> 4];
			out += hex[c & 15];
		}
	}
	return out;
}

std::string uri_to_path(const std::string &uri) {
	if (uri.compare(0, 7, "file://") != 0)
		return "";
	size_t i = 7;
	if (i < uri.size() && uri[i] != '/') /* skip an authority such as localhost */
		i = uri.find('/', i);
	if (i == std::string::npos)
		return "";
	auto val = [](char c) { return std::isdigit((unsigned char)c) ? c - '0' : (std::tolower((unsigned char)c) - 'a' + 10); };
	std::string out;
	for (; i < uri.size(); ++i) {
		if (uri[i] == '%' && i + 2 < uri.size() && std::isxdigit((unsigned char)uri[i + 1]) &&
			std::isxdigit((unsigned char)uri[i + 2])) {
			out += char(val(uri[i + 1]) * 16 + val(uri[i + 2]));
			i += 2;
		} else {
			out += uri[i];
		}
	}
	return out;
}

} // namespace pyc::lsp
