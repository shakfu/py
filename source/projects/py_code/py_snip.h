// SPDX-License-Identifier: MIT
/* py_snip.h -- snippets file: evaluating exactly `snip.KEY` inserts KEY's body.
 *
 * Format (UTF-8, no escapes): a line `### key` (key = [A-Za-z0-9_]+) starts an
 * entry; its body runs verbatim to the next header. Text before the first
 * header is ignored. Leading and trailing blank lines of a body are trimmed.
 * The substitution is text only: nothing is sent to [py]. */

#ifndef PY_SNIP_H
#define PY_SNIP_H

#include <cstddef>
#include <string>
#include <vector>

namespace py_code {

struct SnipEntry {
	std::string key;
	std::string body;
};

/* Result of parsing a file. A file with no headers gives an empty dict (it is
 * not an error: the format is so lax that there is no "invalid file"). */
struct Snippets {
	std::vector<SnipEntry> entries;
	/* Declared keys that do NOT make it into the dict, to warn once at load
	 * time: duplicates (the first one wins) and empty bodies (they are
	 * ignored). */
	std::vector<std::string> duplicates;
	std::vector<std::string> empty;

	size_t size() const { return entries.size(); }
	/* Body of `key`, or nullptr if it is missing. An empty body never makes it into
	 * the dict. */
	const std::string *find(const std::string &key) const;
	/* Up to `max` space-separated keys ("kick pad"), for the error message. */
	std::string keys_list(size_t max = 12) const;
};

/* Valid key: non-empty `[A-Za-z0-9_]+` (same rule in the header and in `snip.<key>`). */
bool valid_key(const std::string &key);

/* true if `line` is a header; `key` comes out with the key (without the
 * `### `). */
bool is_header(const std::string &line, std::string &key);

/* Archivo completo => dict. CRLF/CR => LF y BOM UTF-8 fuera. */
Snippets parse_snippets(const std::string &text);

/* true if `code` (the range being sent to evaluate) is EXACTLY `snip.<key>`:
 * spaces and newlines on the edges are ignored, plus an optional trailing `;`. */
bool snip_matches(const std::string &code, std::string &key);

} // namespace py_code

#endif /* PY_SNIP_H */
