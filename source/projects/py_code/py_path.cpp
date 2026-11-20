// SPDX-License-Identifier: GPL-3.0-only
/* py_path.cpp -- see py_path.h. */

#include "py_path.h"

namespace py_code {

static std::string strip_trailing_slashes(std::string s) {
	while (s.size() > 1 && s[s.size() - 1] == '/')
		s.erase(s.size() - 1);
	return s;
}

static bool is_space(char c) {
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

std::string path_to_posix(const std::string &in) {
	if (in.empty())
		return in;
	std::string s = in;
	/* Max volume dialect: `<volume name>:/rest`. A volume name never starts with a
	 * `/` (that would already be a POSIX path), and the first `:/` is the boundary
	 * (volume names do contain spaces). */
	if (s[0] != '/') {
		const std::string::size_type p = s.find(":/");
		if (p != std::string::npos && p > 0)
			s = s.substr(p + 1);
	}
	return s;
}

std::string path_to_absolute(const std::string &home_in, const std::string &in) {
	if (in.empty())
		return in;

	/* Without the volume prefix the name may be relative to a search path: there is
	 * nothing to join it to here, so it is left as is (it works in Max). */
	std::string s = path_to_posix(in);
	if (s.empty() || s[0] != '~' || (s.size() > 1 && s[1] != '/'))
		return s;

	const std::string home = strip_trailing_slashes(home_in);
	if (home.empty() || home == "/")
		return s;
	return home + s.substr(1);
}

std::string unquote_path(const std::string &in) {
	std::string::size_type b = 0;
	std::string::size_type e = in.size();
	while (b < e && is_space(in[b]))
		++b;
	while (e > b && is_space(in[e - 1]))
		--e;
	std::string s = in.substr(b, e - b);

	if (s.size() < 2)
		return s;
	const char q = s[0];
	if ((q == '\'' || q == '"') && s[s.size() - 1] == q)
		return s.substr(1, s.size() - 2);
	return s;
}

std::string quote_py_string(const std::string &path) {
	std::string out;
	out.reserve(path.size() + 2);
	out += '"';
	for (std::string::size_type i = 0; i < path.size(); ++i) {
		const char c = path[i];
		if (c == '\\' || c == '"')
			out += '\\';
		out += c;
	}
	out += '"';
	return out;
}

std::string dropped_text(const std::string &home, const std::vector<std::string> &names) {
	std::string out;
	for (std::vector<std::string>::size_type i = 0; i < names.size(); ++i) {
		if (i)
			out += ' ';
		out += quote_py_string(path_to_absolute(home, names[i]));
	}
	return out;
}

} // namespace py_code
