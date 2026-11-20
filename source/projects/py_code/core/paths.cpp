// SPDX-License-Identifier: MIT
/* paths.cpp -- see paths.h. */

#include "core/paths.h"

namespace pyc {

std::string posix_path(const std::string &s) {
	if (s.empty() || s[0] == '/')
		return s;
	const size_t p = s.find(":/");
	return p == std::string::npos ? s : s.substr(p + 1);
}

std::string expand_home(const std::string &home, const std::string &s) {
	if (home.empty() || s.empty() || s[0] != '~' || (s.size() > 1 && s[1] != '/'))
		return s;
	std::string h = home;
	while (!h.empty() && h.back() == '/')
		h.pop_back();
	if (s.size() == 1)
		return h.empty() ? "/" : h;
	return h + s.substr(1);
}

std::string unquote(const std::string &s) {
	size_t a = 0, b = s.size();
	while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\n' || s[a] == '\r'))
		++a;
	while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\n' || s[b - 1] == '\r'))
		--b;
	if (b - a >= 2 && (s[a] == '"' || s[a] == '\'') && s[b - 1] == s[a])
		++a, --b;
	return s.substr(a, b - a);
}

std::string python_string(const std::string &s) {
	std::string o = "\"";
	for (char c : s) {
		if (c == '\\' || c == '"')
			o += '\\', o += c;
		else if (c == '\n')
			o += "\\n";
		else if (c == '\r')
			o += "\\r";
		else
			o += c;
	}
	return o + "\"";
}

std::string drop_text(const std::string &home, const std::vector<std::string> &names) {
	std::string o;
	for (const std::string &n : names) {
		if (!o.empty())
			o += ' ';
		o += python_string(expand_home(home, posix_path(n)));
	}
	return o;
}

} // namespace pyc
