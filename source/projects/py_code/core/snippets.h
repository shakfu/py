// SPDX-License-Identifier: MIT
/* snippets.h -- snippet files: `### key` headers, verbatim bodies. */

#pragma once

#include <string>
#include <utility>
#include <vector>

namespace pyc {

struct Snippets {
	std::vector<std::pair<std::string, std::string>> entries; /* key, body; file order */
	std::vector<std::string> duplicates; /* keys seen again (first wins), each once */
	std::vector<std::string> empty;      /* keys with no body (not stored), each once */

	const std::string *find(const std::string &key) const; /* null if absent */
	std::vector<std::string> keys() const;
	size_t size() const { return entries.size(); }
};

namespace snippets {

bool valid_key(const std::string &key); /* [A-Za-z0-9_]+ */
Snippets parse(const std::string &text);

/* True when `code`, trimmed, is exactly `snip.<key>`; sets `*key` if non-null. */
bool match(const std::string &code, std::string *key);

} // namespace snippets
} // namespace pyc
