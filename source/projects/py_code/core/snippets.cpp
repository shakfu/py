// SPDX-License-Identifier: MIT
/* snippets.cpp -- see snippets.h. */

#include "core/snippets.h"

#include <algorithm>

namespace pyc {

static bool is_ws(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'; }

static std::string trim(const std::string &s) {
	size_t a = 0, b = s.size();
	while (a < b && is_ws(s[a]))
		++a;
	while (b > a && is_ws(s[b - 1]))
		--b;
	return s.substr(a, b - a);
}

static bool blank(const std::string &line) { return trim(line).empty(); }

const std::string *Snippets::find(const std::string &key) const {
	for (const auto &e : entries)
		if (e.first == key)
			return &e.second;
	return nullptr;
}

std::vector<std::string> Snippets::keys() const {
	std::vector<std::string> k;
	for (const auto &e : entries)
		k.push_back(e.first);
	return k;
}

namespace snippets {

bool valid_key(const std::string &key) {
	if (key.empty())
		return false;
	for (char c : key)
		if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'))
			return false;
	return true;
}

/* Key of a header line, or "" when `line` is not a header. */
static std::string header_key(const std::string &line) {
	if (line.compare(0, 4, "### ") != 0)
		return "";
	const std::string k = trim(line.substr(4));
	return valid_key(k) ? k : "";
}

static void add_once(std::vector<std::string> &v, const std::string &k) {
	if (std::find(v.begin(), v.end(), k) == v.end())
		v.push_back(k);
}

Snippets parse(const std::string &input) {
	std::string text;
	size_t i = input.compare(0, 3, "\xEF\xBB\xBF") == 0 ? 3 : 0;
	for (; i < input.size(); ++i) {
		if (input[i] == '\r') {
			text += '\n';
			if (i + 1 < input.size() && input[i + 1] == '\n')
				++i;
		} else {
			text += input[i];
		}
	}

	Snippets out;
	std::vector<std::string> seen;
	std::string key;
	std::vector<std::string> body;
	auto flush = [&] {
		if (key.empty())
			return;
		size_t a = 0, b = body.size();
		while (a < b && blank(body[a]))
			++a;
		while (b > a && blank(body[b - 1]))
			--b;
		std::string s;
		for (size_t j = a; j < b; ++j)
			s += (j > a ? "\n" : "") + body[j];
		if (std::find(seen.begin(), seen.end(), key) != seen.end())
			add_once(out.duplicates, key);
		else if (s.empty())
			add_once(out.empty, key);
		else
			out.entries.emplace_back(key, s);
		seen.push_back(key);
	};

	size_t pos = 0;
	while (pos <= text.size()) {
		size_t nl = text.find('\n', pos);
		if (nl == std::string::npos)
			nl = text.size();
		const std::string line = text.substr(pos, nl - pos);
		const std::string k = header_key(line);
		if (!k.empty()) {
			flush();
			key = k;
			body.clear();
		} else if (!key.empty()) {
			body.push_back(line);
		}
		pos = nl + 1;
	}
	flush();
	return out;
}

bool match(const std::string &code, std::string *key) {
	const std::string t = trim(code);
	if (t.compare(0, 5, "snip.") != 0 || !valid_key(t.substr(5)))
		return false;
	if (key)
		*key = t.substr(5);
	return true;
}

} // namespace snippets
} // namespace pyc
