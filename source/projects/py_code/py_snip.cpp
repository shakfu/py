// SPDX-License-Identifier: GPL-3.0-only
/* py_snip.cpp -- see py_snip.h. */

#include "py_snip.h"

namespace py_code {

namespace {

const char *const kHeader = "### ";
const size_t kHeaderLen = 4;
const char *const kPrefix = "snip.";
const size_t kPrefixLen = 5;

bool is_ident_char(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

bool is_space(char c) {
	return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

void ltrim_in_place(std::string &s) {
	size_t b = 0;
	while (b < s.size() && is_space(s[b]))
		++b;
	if (b)
		s.erase(0, b);
}

void rtrim_in_place(std::string &s) {
	size_t e = s.size();
	while (e > 0 && is_space(s[e - 1]))
		--e;
	s.resize(e);
}

std::string rtrim(const std::string &s) {
	std::string out = s;
	rtrim_in_place(out);
	return out;
}

std::string trim(const std::string &s) {
	std::string out = s;
	ltrim_in_place(out);
	rtrim_in_place(out);
	return out;
}

bool is_blank(const std::string &s) {
	for (char c : s)
		if (c != ' ' && c != '\t')
			return false;
	return true;
}

/* CRLF/CR => LF y BOM UTF-8 fuera. */
std::string normalize(const std::string &in) {
	std::string out;
	out.reserve(in.size() + 1);
	size_t i = 0;
	if (in.size() >= 3 && (unsigned char)in[0] == 0xEF && (unsigned char)in[1] == 0xBB &&
		(unsigned char)in[2] == 0xBF)
		i = 3;
	for (; i < in.size(); ++i) {
		if (in[i] == '\r') {
			if (i + 1 < in.size() && in[i + 1] == '\n')
				++i;
			out += '\n';
		} else {
			out += in[i];
		}
	}
	return out;
}

/* Closes a block: trims the blank lines at the edges (the inside goes verbatim)
 * and adds it to the dict, or to the list of "declared but ignored". */
void add_block(Snippets &out, const std::string &key, const std::string &raw) {
	/* Split into lines so the blank ones at the start and at the end can be skipped
	 * without touching the content (nor its indentation). */
	std::vector<std::string> lines;
	size_t pos = 0;
	while (pos < raw.size()) {
		size_t nl = raw.find('\n', pos);
		if (nl == std::string::npos)
			nl = raw.size();
		lines.push_back(raw.substr(pos, nl - pos));
		pos = nl + 1;
	}
	size_t b = 0, e = lines.size();
	while (b < e && is_blank(lines[b]))
		++b;
	while (e > b && is_blank(lines[e - 1]))
		--e;
	std::string body;
	for (size_t i = b; i < e; ++i) {
		if (i != b)
			body += '\n';
		body += lines[i];
	}
	rtrim_in_place(body); /* it can only touch the end of the last line */

	if (body.empty()) {
		out.empty.push_back(key);
		return;
	}
	if (out.find(key)) { /* duplicada: gana la primera */
		out.duplicates.push_back(key);
		return;
	}
	SnipEntry entry;
	entry.key = key;
	entry.body = body;
	out.entries.push_back(entry);
}

} // namespace

const std::string *Snippets::find(const std::string &key) const {
	for (const SnipEntry &e : entries)
		if (e.key == key)
			return &e.body;
	return nullptr;
}

std::string Snippets::keys_list(size_t max) const {
	std::string out;
	for (size_t i = 0; i < entries.size() && i < max; ++i) {
		if (!out.empty())
			out += ' ';
		out += entries[i].key;
	}
	if (entries.size() > max)
		out += " \xe2\x80\xa6"; /* ... */
	return out;
}

bool valid_key(const std::string &key) {
	if (key.empty())
		return false;
	for (char c : key)
		if (!is_ident_char(c))
			return false;
	return true;
}

bool is_header(const std::string &line, std::string &key) {
	if (line.size() <= kHeaderLen)
		return false;
	if (line.compare(0, kHeaderLen, kHeader) != 0)
		return false;
	const std::string k = rtrim(line.substr(kHeaderLen));
	if (!valid_key(k))
		return false;
	key = k;
	return true;
}

Snippets parse_snippets(const std::string &text) {
	const std::string src = normalize(text);
	Snippets out;
	std::string key, body;
	bool in_block = false;
	size_t pos = 0;
	while (pos <= src.size()) {
		const size_t nl = src.find('\n', pos);
		const size_t end = (nl == std::string::npos) ? src.size() : nl;
		const std::string line = src.substr(pos, end - pos);
		std::string header;
		if (is_header(line, header)) {
			if (in_block)
				add_block(out, key, body);
			key = header;
			body.clear();
			in_block = true;
		} else if (in_block) {
			body += line;
			body += '\n';
		}
		if (nl == std::string::npos)
			break;
		pos = nl + 1;
	}
	if (in_block)
		add_block(out, key, body);
	return out;
}

bool snip_matches(const std::string &code, std::string &key) {
	const std::string s = trim(code);
	std::string body = s;
	if (!body.empty() && body[body.size() - 1] == ';') {
		body.resize(body.size() - 1);
		body = trim(body);
	}
	if (body.size() <= kPrefixLen)
		return false;
	if (body.compare(0, kPrefixLen, kPrefix) != 0)
		return false;
	const std::string k = body.substr(kPrefixLen);
	if (!valid_key(k))
		return false;
	key = k;
	return true;
}

} // namespace py_code
