// SPDX-License-Identifier: MIT
#include "lsp/framing.h"

#include <cctype>

namespace pyc::lsp {

namespace {
const size_t kMaxBody = size_t(64) << 20;
const size_t kMaxJunk = size_t(1) << 20; /* garbage kept while no header ends */
} // namespace

void Framer::feed(const char *bytes, size_t len) { buf_.append(bytes, len); }

bool Framer::next(std::string *out) {
	for (;;) {
		const size_t crlf = buf_.find("\r\n\r\n");
		const size_t lf = buf_.find("\n\n");
		if (crlf == std::string::npos && lf == std::string::npos) {
			if (buf_.size() > kMaxJunk)
				buf_.erase(0, buf_.size() - 64);
			return false;
		}
		size_t term, skip;
		if (crlf != std::string::npos && (lf == std::string::npos || crlf < lf)) {
			term = crlf;
			skip = 4;
		} else {
			term = lf;
			skip = 2;
		}
		std::string head = buf_.substr(0, term);
		for (char &c : head)
			c = char(std::tolower((unsigned char)c));
		const size_t at = head.rfind("content-length:");
		size_t n = 0;
		bool ok = false;
		if (at != std::string::npos) {
			size_t i = at + 15;
			while (i < head.size() && (head[i] == ' ' || head[i] == '\t'))
				++i;
			while (i < head.size() && std::isdigit((unsigned char)head[i]) && n <= kMaxBody) {
				n = n * 10 + size_t(head[i] - '0');
				ok = true;
				++i;
			}
			ok = ok && n <= kMaxBody;
		}
		if (!ok) { /* no usable header in this block: drop it */
			buf_.erase(0, term + skip);
			continue;
		}
		const size_t body = term + skip;
		if (buf_.size() < body + n)
			return false;
		out->assign(buf_, body, n);
		buf_.erase(0, body + n);
		return true;
	}
}

std::string encode(const std::string &json_text) {
	return "Content-Length: " + std::to_string(json_text.size()) + "\r\n\r\n" + json_text;
}

} // namespace pyc::lsp
