// SPDX-License-Identifier: MIT
#include "lsp/position.h"

#include <algorithm>

namespace pyc::lsp {

namespace {
inline bool cont(unsigned char c, unsigned char lo = 0x80, unsigned char hi = 0xBF) { return c >= lo && c <= hi; }
inline int units_of(int bytes, bool utf16) { return utf16 ? (bytes == 4 ? 2 : 1) : bytes; }
} // namespace

int utf8_len(const std::string &s, size_t i) {
	const size_t n = s.size();
	const unsigned char c = (unsigned char)s[i];
	if (c < 0x80)
		return 1;
	auto at = [&](size_t k) { return i + k < n ? (unsigned char)s[i + k] : 0; };
	if (c >= 0xC2 && c <= 0xDF)
		return cont(at(1)) ? 2 : 1;
	if (c >= 0xE0 && c <= 0xEF) {
		const unsigned char lo = c == 0xE0 ? 0xA0 : 0x80, hi = c == 0xED ? 0x9F : 0xBF;
		return cont(at(1), lo, hi) && cont(at(2)) ? 3 : 1;
	}
	if (c >= 0xF0 && c <= 0xF4) {
		const unsigned char lo = c == 0xF0 ? 0x90 : 0x80, hi = c == 0xF4 ? 0x8F : 0xBF;
		return cont(at(1), lo, hi) && cont(at(2)) && cont(at(3)) ? 4 : 1;
	}
	return 1;
}

std::string utf8_safe(const std::string &s) {
	std::string out = s;
	for (size_t i = 0; i < out.size();) {
		const int n = utf8_len(out, i);
		if (n == 1 && (unsigned char)out[i] >= 0x80)
			out[i] = '?';
		i += size_t(n);
	}
	return out;
}

int advance_units(const std::string &s, int from, int units, bool utf16) {
	size_t i = size_t(std::clamp(from, 0, int(s.size())));
	int count = 0;
	while (i < s.size()) {
		const int n = utf8_len(s, i);
		const int u = units_of(n, utf16);
		if (count + u > units)
			break;
		count += u;
		i += size_t(n);
	}
	return int(i);
}

LineIndex::LineIndex(const std::string &text) : text_(text) {
	starts_.push_back(0);
	for (size_t i = 0; i < text.size(); ++i)
		if (text[i] == '\n')
			starts_.push_back(int(i + 1));
}

Position LineIndex::position(int offset, bool utf16) const {
	offset = std::clamp(offset, 0, int(text_.size()));
	const auto it = std::upper_bound(starts_.begin(), starts_.end(), offset);
	Position p;
	p.line = int(it - starts_.begin()) - 1;
	size_t i = size_t(starts_[size_t(p.line)]);
	while (i < size_t(offset)) {
		const int n = utf8_len(text_, i);
		if (i + size_t(n) > size_t(offset))
			break; /* offset inside a code point: stop before it */
		p.character += units_of(n, utf16);
		i += size_t(n);
	}
	return p;
}

int LineIndex::offset(int line, int character, bool utf16) const {
	if (line < 0)
		return 0;
	if (line >= int(starts_.size()))
		return int(text_.size());
	const size_t start = size_t(starts_[size_t(line)]);
	size_t end = line + 1 < int(starts_.size()) ? size_t(starts_[size_t(line) + 1]) - 1 : text_.size();
	size_t i = start;
	int count = 0;
	while (i < end) {
		const int n = utf8_len(text_, i);
		const int u = units_of(n, utf16);
		if (count + u > character)
			break;
		count += u;
		i += size_t(n);
	}
	return int(i);
}

Position to_position(const std::string &text, int offset, bool utf16) {
	return LineIndex(text).position(offset, utf16);
}

int to_offset(const std::string &text, int line, int character, bool utf16) {
	return LineIndex(text).offset(line, character, utf16);
}

} // namespace pyc::lsp
