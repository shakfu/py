// SPDX-License-Identifier: MIT
/* position.h -- byte offset <-> LSP (line, character) conversions.
 * `character` counts UTF-16 code units (`utf16`) or UTF-8 bytes. Invalid UTF-8
 * bytes count as one unit each (the client sends them as `?`). */

#pragma once

#include <string>
#include <vector>

namespace pyc::lsp {

struct Position {
	int line = 0;
	int character = 0;
};

/* Bytes of the strictly valid UTF-8 sequence at s[i]; 1 for an invalid byte. */
int utf8_len(const std::string &s, size_t i);
/* Copy with every invalid UTF-8 byte replaced by `?` (byte offsets kept). */
std::string utf8_safe(const std::string &s);
/* Byte offset after `units` code units from `from`, not stopping at newlines. */
int advance_units(const std::string &s, int from, int units, bool utf16);

/* Line starts of one text, for many conversions. Keeps a reference to text. */
class LineIndex {
  public:
	explicit LineIndex(const std::string &text);
	Position position(int offset, bool utf16) const;          /* clamped */
	int offset(int line, int character, bool utf16) const; /* clamped */

  private:
	const std::string &text_;
	std::vector<int> starts_;
};

Position to_position(const std::string &text, int offset, bool utf16);
int to_offset(const std::string &text, int line, int character, bool utf16);

} // namespace pyc::lsp
