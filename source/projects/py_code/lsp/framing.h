// SPDX-License-Identifier: MIT
/* framing.h -- LSP base protocol: `Content-Length` framed JSON bodies. */

#pragma once

#include <cstddef>
#include <string>

namespace pyc::lsp {

/* Splits a byte stream into message bodies. Bytes before a header are skipped,
 * header names are case-insensitive, `\r\n\r\n` and `\n\n` both end a header. */
class Framer {
  public:
	void feed(const char *bytes, size_t len);
	bool next(std::string *json_text); /* true with one body when complete */
	size_t buffered() const { return buf_.size(); }

  private:
	std::string buf_;
};

std::string encode(const std::string &json_text);

} // namespace pyc::lsp
